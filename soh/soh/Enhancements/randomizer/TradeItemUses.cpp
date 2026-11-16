#include <libultraship/bridge/consolevariablebridge.h>
#include "assets/objects/object_link_boy/object_link_boy.h"
#include "assets/objects/object_link_child/object_link_child.h"
#include "assets/objects/object_custom_equip/object_custom_equip.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

#include "assets/objects/object_niw/object_niw.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "global.h"

extern PlayState* gPlayState;
extern SaveContext gSaveContext;

extern void Player_DestroyHookshot(Player*);
extern void Player_DetachHeldActor(PlayState*, Player*);
extern void Player_SetUpperActionFunc(Player*, UpperActionFunc);
extern s32 Player_UpperAction_CarryActor(Player*, PlayState*);
extern s32 Player_UpdateUpperBody(Player*, PlayState*);
extern void Player_Action_808464B0(Player*, PlayState*);
extern s32 Player_SetupAction(PlayState*, Player*, PlayerActionFunc, s32);
extern s32 Player_InitItemActionWithAnim(PlayState*, Player*, s32);
extern s32 Player_UseItem(PlayState*, Player*, s32);
}

typedef struct TradeItemActor {
    ActorID id;
    Actor* actor;
    int32_t item;
    Color_RGBA8 color;
    s8 delayedDestroy;
    s32 storage;
} TradeItemActor;

static TradeItemActor managedActor;


void ClearManagedData() {
        switch (managedActor.item) {
            case ITEM_COJIRO:
            case ITEM_POCKET_CUCCO:
            case ITEM_CHICKEN:
                managedActor.delayedDestroy = 0;
                managedActor.actor = nullptr;
                managedActor.color.r = 0xFF;
                managedActor.color.g = 0xFF;
                managedActor.color.b = 0xFF;
                managedActor.color.a = 0xFF;
                break;
            case ITEM_SAW:
                //disable custom model
            case ITEM_SWORD_BROKEN:
                gSaveContext.swordHealth = managedActor.storage;
                managedActor.storage = 0;
            default:
                break;
        }
        managedActor.id = ACTOR_PLAYER; //probably something better to do here.
        managedActor.item = ITEM_NONE;
}

void SpawnCarryActor(Player* player, PlayState* playState) {
    if (managedActor.actor != nullptr) {
        Actor_Kill(managedActor.actor);
    }

    player->nextModelGroup = Player_ActionToModelGroup(player, PLAYER_IA_NONE);
    Player_DestroyHookshot(player);
    Player_DetachHeldActor(playState, player);
    Player_InitItemActionWithAnim(playState, player, PLAYER_IA_NONE);

    managedActor.actor = Actor_SpawnAsChild(
        &playState->actorCtx, &player->actor, playState, managedActor.id,
        player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z,
        0, player->actor.shape.rot.y, 0, 0);

    player->interactRangeActor = managedActor.actor;
    player->heldActor = managedActor.actor;
    player->unk_3BC.y = managedActor.actor->shape.rot.y - player->actor.shape.rot.y;
    player->stateFlags1 |= PLAYER_STATE1_CARRYING_ACTOR;

    Player_SetUpperActionFunc(player, Player_UpperAction_CarryActor);
    LinkAnimation_PlayOnce(playState, &player->upperSkelAnime, (LinkAnimationHeader*)&gPlayerAnim_link_normal_free2bom);
    Player_UpdateUpperBody(player, playState);
}

void DestroyCarryActor(u8 delay) {
    if (delay > 0) {
        Player* player = GET_PLAYER(gPlayState);
        Player_SetupAction(gPlayState, player, Player_Action_808464B0, 1);
        //play bomb take-out anim in reverse
        LinkAnimationHeader* anim = (LinkAnimationHeader*)&gPlayerAnim_link_normal_free2bom;
        LinkAnimation_Change(gPlayState, &player->skelAnime, anim, -1.0f, Animation_GetLastFrame(anim)-1.0f, 0.0f, ANIMMODE_ONCE, -6.0f);
        managedActor.delayedDestroy = delay;
    } else if (!(managedActor.actor == nullptr) && --managedActor.delayedDestroy <= 0) {
        Actor_Kill(managedActor.actor);
        ClearManagedData();
    } 
    
}

void DestroyCarryActor() {
    DestroyCarryActor(0);
}

void EnvPatchSkele(int size, const char* skele[], int offset[]) {
    char patchName[32];

    for (int i = 0; i < size; i++) {
        snprintf(patchName, sizeof(patchName), "env%d", i);
        //patch skeleton to allow env shaders
        ResourceMgr_PatchGfxByName(
            skele[i], patchName, offset[i], 
            gsDPSetCombineLERP(TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, ENVIRONMENT, 0, COMBINED, 0, 0, 0, 0, COMBINED)
        );
    }
}

void EnableCuccoColoring() {
    const char* skele[] = { gCuccoHeadDL, gCuccoLeftFootDL, gCuccoLeftWingDL, gCuccoRightFootDL, gCuccoRightWingDL, gCuccoNeckDL, gCuccoTailAndUndersideDL };
    int offset[] = { 6, 6, 6, 6, 6, 6, 13 };
    int size = ARRAY_COUNT(skele);
    EnvPatchSkele(size, skele, offset);
}


int BtnEquippedWith(int qty, ...) {
    va_list args;
    va_start(args, qty);

    u8 equipped = 0;
    int item;
    while (qty-- > 0) {
        item = va_arg(args, int);
        if (C_BTN_ITEM(0) == item) {
            equipped |= BTN_CLEFT;
        } 
        if (C_BTN_ITEM(1) == item) {
            equipped |= BTN_CDOWN;
        } 
        if (C_BTN_ITEM(2) == item) {
            equipped |= BTN_CRIGHT;
        } 

        if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0) != 0) {
            if (DPAD_ITEM(0) == item) {
                equipped |= BTN_DUP;
            }
            if (DPAD_ITEM(1) == item) {
                equipped |= BTN_DDOWN;
            }
            if (DPAD_ITEM(2) == item) {
                equipped |= BTN_DLEFT;
            }
            if (DPAD_ITEM(3) == item) {
                equipped |= BTN_DRIGHT;
            }
        }
    }
    va_end(args);

    return equipped;
}

void RegisterTradeItemUses() {
    bool shouldRegister = true;

    COND_HOOK(OnSceneInit, shouldRegister, [](int32_t) {
        EnableCuccoColoring();
    });
    
    COND_HOOK(OnPlayerUpdate, shouldRegister, []() {
        //early exits
        Player* player = GET_PLAYER(gPlayState);
        if (managedActor.delayedDestroy > 0  || player->heldActor != managedActor.actor) {
            DestroyCarryActor();
            return;
        } else if (managedActor.actor == nullptr) {
            return;
        } 

        if (managedActor.item == ITEM_COJIRO) {
            // Cojiro is a quiet boy!!!!
            Audio_StopSfxByPosAndId(&managedActor.actor->projectedPos, NA_SE_EV_CHICKEN_CRY_M);
            Audio_StopSfxByPosAndId(&managedActor.actor->projectedPos, NA_SE_EV_CHICKEN_CRY_A);
            Audio_StopSfxByPosAndId(&managedActor.actor->projectedPos, NA_SE_EV_CHICKEN_CRY_N);
        }
    });

    

    COND_VB_SHOULD(VB_SET_ACTOR_COLOR, shouldRegister, {
        Actor* actor = va_arg(args, Actor*);

        //atm only cuccos colored
        if (actor->id != ACTOR_EN_NIW) {
            return;
        }

        *should = true;

        if (actor == managedActor.actor) { 
            if (managedActor.item == ITEM_COJIRO) {
                Color_RGBA8* color = va_arg(args, Color_RGBA8*);
                *color = managedActor.color;
            }
        }
    });

    COND_VB_SHOULD(VB_DISABLE_ACTOR_EFFECT, shouldRegister, {
        if (managedActor.id != ACTOR_EN_NIW) {
            //do nothing if we're not a chicken
            return;
        }

        Actor* actor = va_arg(args, Actor*);

        //feather spawns are white, and cojiro is blue, so they'd look out of place.
        if (actor == managedActor.actor) { 
            if (managedActor.item == ITEM_COJIRO) {
                *should = true;
            }
        }
    });

    COND_VB_SHOULD(VB_IGNORE_ITEM_BLOCKING_STATE, shouldRegister, {
        Player* player = va_arg(args, Player*);
        //state not (exclusively) caused by any trade item, so don't override
        if (managedActor.actor == nullptr || player->heldActor == nullptr || (player->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) || func_8008F128(player)) {
            return;
        }
        //ignore carry actor state when holding cucco
        if (player->heldActor == managedActor.actor && (player->stateFlags1 & (PLAYER_STATE1_CARRYING_ACTOR))) {
            *should = true;
        }

    });

    COND_VB_SHOULD(VB_PUTAWAY_BECAUSE_DISABLED_ITEM_BUTTONS, shouldRegister, {
        Player* player = GET_PLAYER(gPlayState);
        if (player->itemAction >= PLAYER_IA_SWORD_MASTER && player ->itemAction <= PLAYER_IA_SWORD_BIGGORON) {
            *should = BtnEquippedWith(2, ITEM_SWORD_BROKEN, ITEM_SAW) < 0;
        }
    });

    COND_VB_SHOULD(VB_CHANGE_HELD_ITEM_AND_USE_ITEM, shouldRegister, {
        int32_t usedItem = va_arg(args, int32_t);
        Player* player = GET_PLAYER(gPlayState);
        
        //retain their trade-quest usage
        if (player->stateFlags1 & (PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS)) {
            return;
        }

        switch (usedItem) {
            //trade items:
            // portable cucco
            case ITEM_COJIRO:
                managedActor.color.r = 0x34;
                managedActor.color.g = 0x55;
                managedActor.color.b = 0xA6;
                managedActor.color.a = 0xFF;
            case ITEM_POCKET_CUCCO:
            case ITEM_CHICKEN:
                *should = false;
                //allow z movement
                if (player->stateFlags1 & (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_PARALLEL)) {
                    return;
                }
                if (managedActor.item != usedItem) {
                    managedActor.id = ACTOR_EN_NIW;
                    managedActor.item = usedItem;
                    SpawnCarryActor(player, gPlayState);
                    if (usedItem == ITEM_POCKET_CUCCO) {
                        Actor_SetScale(managedActor.actor, 0.005f);
                        managedActor.actor->shape.yOffset +=500.0f;
                    }
                }
                break;
            // alt swords
            case ITEM_SAW:
                // load custom model
                // see customequipment.cpp
              /*const bool isAdult = gSaveContext.linkAge == LINK_AGE_ADULT;
                Gfx* dList = nullptr;
                Gfx* held = ResourceMgr_LoadGfxByName(gCustomBrokenLongswordDL);
                // to be stolen wholesale from customequipment.cpp to avoid updating it
                Gfx* hand = ResourceMgr_LoadGfxByName(isAdult ? gLinkAdultLeftHandClosedNearDL : gLinkChildLeftFistNearDL);
                BuildHandItemDL(gPlayState, &dList, hand, held, !isAdult); */
                //hide sheath
                Player_UseItem(gPlayState, player, ITEM_SWORD_KOKIRI);
                DestroyCarryActor();
                break;
            case ITEM_SWORD_BROKEN:
                *should = false;
                //todo - implement proper usage:
                //hide sheath since these come from pocket
                //add logic to return these to default when sword used
                managedActor.item = usedItem;
                managedActor.storage = gSaveContext.swordHealth;
                gSaveContext.swordHealth = 0;
                Player_UseItem(gPlayState, player, ITEM_SWORD_BGS);
                DestroyCarryActor();
                break;
            //other items:
            case ITEM_SWORD_BGS:
            case ITEM_SWORD_MASTER:
            case ITEM_SWORD_KOKIRI:
            case ITEM_SWORD_KNIFE:
                if (managedActor.storage > 0) {
                    gSaveContext.swordHealth = managedActor.storage;
                }
                //ensure reset to default model
                DestroyCarryActor();
                break;
            // soome items shouldn't impact casuse, exclude here
            case ITEM_LENS:
                break;
            //kill instead of drop
            default:
                DestroyCarryActor();
                break;
        }
    });


    COND_VB_SHOULD(VB_SET_ACTION_TEXTURE, shouldRegister, {
        u16* action = va_arg(args, u16*);
        va_arg(args, const char**);

        Player* player = GET_PLAYER(gPlayState);
        if (managedActor.actor != nullptr && 
            !((player->stateFlags1 &
            (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_PARALLEL)) ||
            (player->stateFlags1 & PLAYER_STATE1_IN_WATER) || (player->stateFlags2 & PLAYER_STATE2_CRAWLING) ||
            (gPlayState->pauseCtx.state > 0 && gPlayState->pauseCtx.state < 18)) && 
           (*action == DO_ACTION_THROW || *action == DO_ACTION_DROP)) {
            *action = DO_ACTION_PUTAWAY;
        } else if ((player->stateFlags1 & (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_PARALLEL)) && *action == DO_ACTION_THROW) {
            *action = DO_ACTION_ATTACK;
        }
    });

    //todo - id & fix bug where still can throw mid-air in sidehop?
    COND_VB_SHOULD(VB_THROW_OR_PUT_DOWN_HELD_ITEM, shouldRegister, {
        Input* input = va_arg(args, Input*);
        Player* player = GET_PLAYER(gPlayState);

        if (!*should || managedActor.actor == nullptr || player->heldActor != managedActor.actor) {
            return;
        }
        
        *should = false;

        if (managedActor.id == ACTOR_EN_NIW) {
            if (CHECK_BTN_ANY(input->press.button, BTN_A) && !(player->stateFlags1 & (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_PARALLEL))) {
                DestroyCarryActor((player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ? 6 : 0);
            }
        }
    
    });
}

static RegisterShipInitFunc registerTradeItemUses(RegisterTradeItemUses);
