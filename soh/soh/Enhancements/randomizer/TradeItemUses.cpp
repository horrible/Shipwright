#include <libultraship/bridge/consolevariablebridge.h>

#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "assets/objects/object_niw/object_niw.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include <z64.h>
#include "functions.h"
#include "macros.h"

extern PlayState* gPlayState;
extern SaveContext gSaveContext;

extern void Player_DetachHeldActor(PlayState*, Player*);
extern void Player_SetUpperActionFunc(Player*, UpperActionFunc);
extern s32 Player_UpperAction_CarryActor(Player*, PlayState*);
extern s32 Player_UpdateUpperBody(Player*, PlayState*);
extern void Player_AnimPlayOnce(PlayState*, Player*, LinkAnimationHeader*);
extern void Player_Action_808464B0(Player*, PlayState*);
extern s32 Player_SetupAction(PlayState*, Player*, PlayerActionFunc, s32);
extern s32 Player_GetItemOnButton(PlayState*, s32);


}

#define CUCCO_WHITE 255, 255, 255, 255
#define CUCCO_YELLOW 0x4E, 0x8E, 0x2C, 0xFF
#define CUCCO_BLUE 0x2C, 0x4E, 0x8E, 0xFF

typedef struct TradeItemActor {
    ActorID id;
    Actor* actor;
    int32_t item;
    Color_RGBA8 color;
    u8 delayedDestroy;
} TradeItemActor;

static TradeItemActor managedActor;

void SpawnCarryActor(PlayState* playState, Player* player, ActorID actorId) {
    if (managedActor.actor != NULL) {
        Actor_Kill(managedActor.actor); //kthxbai :^)
    }
    managedActor.actor = Actor_SpawnAsChild(
        &playState->actorCtx, &player->actor, playState, actorId,
        player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z,
        0, player->actor.shape.rot.y, 0, 0);

        player->interactRangeActor = managedActor.actor;
        player->heldActor = managedActor.actor;
        player->getItemId = GI_NONE; //is this needed? who knows! not me :)
        player->getItemEntry = GET_ITEM_NONE; //ditto
        player->unk_3BC.y = managedActor.actor->shape.rot.y - player->actor.shape.rot.y;
        player->stateFlags1 |= PLAYER_STATE1_CARRYING_ACTOR;

        //todo - figure out which of these are needed lol
        Player_SetUpperActionFunc(player, Player_UpperAction_CarryActor);
        Player_AnimPlayOnce(playState, player, (LinkAnimationHeader*)&gPlayerAnim_link_normal_free2bom);
        LinkAnimation_PlayOnce(playState, &player->upperSkelAnime, (LinkAnimationHeader*)&gPlayerAnim_link_normal_free2bom);
        Player_UpdateUpperBody(player, playState);
}

void DestroyCarryActor(){
    if (!(managedActor.actor == nullptr) && !managedActor.delayedDestroy) {
        Actor_Kill(managedActor.actor);
        managedActor.delayedDestroy = 0;
        managedActor.actor = nullptr;
        managedActor.color.r = 0xFF;
        managedActor.color.g = 0xFF;
        managedActor.color.b = 0xFF;
        managedActor.color.a = 0xFF;
        managedActor.item = ITEM_NONE;
    } else if (managedActor.actor != nullptr) {
        --managedActor.delayedDestroy;
    }
    
}

//she did cosmetic surgery on a cucco...
void enableCuccoColoring() {
    const char* skele[] = { gCuccoHeadDL, gCuccoLeftFootDL, gCuccoLeftWingDL, gCuccoRightFootDL, gCuccoRightWingDL, gCuccoNeckDL, gCuccoTailAndUndersideDL };
    int offset[] = { 6, 6, 6, 6, 6, 6, 13 };
    char patchName[32];
    int i;
    for (i = 0; i < ARRAY_COUNT(skele); i++) {
        snprintf(patchName, sizeof(patchName), "tint %d", i);
        //patch to allow skeleton coloring to use the env shaders that most others do
        ResourceMgr_PatchGfxByName(
            skele[i], patchName, offset[i], 
            gsDPSetCombineLERP(TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0, ENVIRONMENT, 0, COMBINED, 0, 0, 0, 0, COMBINED)
        );
    }
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
        } else if (C_BTN_ITEM(1) == item) {
            equipped |= BTN_CDOWN;
        } else if (C_BTN_ITEM(2) == item) {
            equipped |= BTN_CRIGHT;
        } else if (DPAD_ITEM(0) == item) {
            equipped |= BTN_DUP;
        } else if (DPAD_ITEM(1) == item) {
            equipped |= BTN_DDOWN;
        } else if (DPAD_ITEM(2) == item) {
            equipped |= BTN_DLEFT;
        } else if (DPAD_ITEM(3) == item) {
            equipped |= BTN_DRIGHT;
        }
    }
    va_end(args);

    return equipped;
}
int BtnNotEquippedWith(int qty, ...) {
    va_list args;
    va_start(args, qty);

    u8 equipped = 0;
    int item;
    while (qty-- > 0) {
        item = va_arg(args, int);
        if (C_BTN_ITEM(0) == item) {
            equipped |= BTN_CLEFT;
        } else if (C_BTN_ITEM(1) == item) {
            equipped |= BTN_CDOWN;
        } else if (C_BTN_ITEM(2) == item) {
            equipped |= BTN_CRIGHT;
        } else if (DPAD_ITEM(0) == item) {
            equipped |= BTN_DUP;
        } else if (DPAD_ITEM(1) == item) {
            equipped |= BTN_DDOWN;
        } else if (DPAD_ITEM(2) == item) {
            equipped |= BTN_DLEFT;
        } else if (DPAD_ITEM(3) == item) {
            equipped |= BTN_DRIGHT;
        }
    }
    va_end(args);

    return (BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT | BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT) & ~equipped;
}



void RegisterTradeItemUses() {
    bool shouldRegister = true;

    COND_HOOK(OnSceneInit, shouldRegister, [](int32_t) {
        enableCuccoColoring();
    });
    
    COND_HOOK(OnPlayerUpdate, shouldRegister, []() {
        //early exits
        Player* player = GET_PLAYER(gPlayState);
        if (managedActor.actor == nullptr) {
            return;
        } else if (managedActor.delayedDestroy || managedActor.id == ACTOR_EN_NIW && player->heldActor != managedActor.actor) {
            DestroyCarryActor();
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
        if (managedActor.item != ITEM_COJIRO) {
            //do nothing if we're not coloring this item.
            return;
        }

        Actor* actor = va_arg(args, Actor*);
        Color_RGBA8* color = va_arg(args, Color_RGBA8*);

        if (actor == managedActor.actor) { 
            if (managedActor.item == ITEM_COJIRO) {
                *should = true;
                *color = managedActor.color;
            }
        }
    });

    COND_VB_SHOULD(VB_DISABLE_CUCCO_FEATHERS, shouldRegister, {
        if (managedActor.id != ACTOR_EN_NIW) {
            //do nothing if we're not a chicken
            return;
        }

        Actor* actor = va_arg(args, Actor*);

        if (actor == managedActor.actor) { 
            if (managedActor.item == ITEM_COJIRO) {
                *should = true;
            }
        }
    });

    COND_VB_SHOULD(VB_IGNORE_ITEM_BLOCKING_STATE, shouldRegister, {
        Player* player = va_arg(args, Player*);
        if (managedActor.actor == nullptr || player->heldActor == nullptr || (player->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) || func_8008F128(player)) {
            //do nothing if we're blocked for a non-carry reason (or not blocked, etc).
            return;
        }

        //ignore carry item-use block for other items when holding a managed item so that the hook below handles it instead.
       LUSLOG_WARN("LIZDEBUG!!!!! VB_IGNORE_ITEM_BLOCKING_STATE passed early exit!!! held: %d managed: %d", player->heldActor->id, managedActor.id);
        if (player->heldActor == managedActor.actor && (player->stateFlags1 & (PLAYER_STATE1_CARRYING_ACTOR))) {
            *should = true;

        }

    });

    COND_VB_SHOULD(VB_CHANGE_HELD_ITEM_AND_USE_ITEM, shouldRegister, {
        int32_t usedItem = va_arg(args, int32_t);
        Player* player = GET_PLAYER(gPlayState);
        LUSLOG_WARN("LIZDEBUG!!!!! VB_CHANGE_HELD_ITEM_AND_USE_ITEM!!! %d", usedItem);

        switch (usedItem) {
            case ITEM_COJIRO:
                /* //Dark
                managedActor.color.r = 0x2C;
                managedActor.color.g = 0x4E;
                managedActor.color.b = 0x8E;
                managedActor.color.a = 0xFF;
                //*/
                /* //Slightly Less Dark
                managedActor.color.r = 0x35;
                managedActor.color.g = 0x5D;
                managedActor.color.b = 0xAB;
                managedActor.color.a = 0xFF;
                //*/
                /* //Middle?
                managedActor.color.r = 0x2C;
                managedActor.color.g = 0x4D;
                managedActor.color.b = 0x8E;
                managedActor.color.a = 0xFF;
                //*/
                /* //Light
                managedActor.color.r = 0x46;
                managedActor.color.g = 0x7A;
                managedActor.color.b = 0xDE;
                managedActor.color.a = 0xFF;
                //*/
                /* //Lighter
                managedActor.color.r = 0x3C;
                managedActor.color.g = 0x6A;
                managedActor.color.b = 0xC1;
                managedActor.color.a = 0xFF;
                //*/
            case ITEM_POCKET_CUCCO:
            case ITEM_CHICKEN:
                if (!(player->stateFlags1 & (PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS))) {
                    *should = false;                    
                    if (usedItem != managedActor.item) {
                        managedActor.id = ACTOR_EN_NIW;
                        managedActor.item = usedItem;
                        SpawnCarryActor(gPlayState, player, managedActor.id);
                        if (usedItem == ITEM_POCKET_CUCCO) {
                            Actor_SetScale(managedActor.actor, 0.005f);
                            managedActor.actor->shape.yOffset +=500.0f;
                        } else if (usedItem == ITEM_COJIRO){
                            managedActor.color.r = 0x34;
                            managedActor.color.g = 0x55;
                            managedActor.color.b = 0xA6;
                            managedActor.color.a = 0xFF;
                        }
                    }
                }
                break;
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

    COND_VB_SHOULD(VB_THROW_OR_PUT_DOWN_HELD_ITEM, shouldRegister, {
        Input* input = va_arg(args, Input*);
        Player* player = GET_PLAYER(gPlayState);

        //disregard all previous instructions and hold that bird :)
        if (managedActor.actor != nullptr && managedActor.id == ACTOR_EN_NIW) {
            //can't swap between cuccos or re-deploy a cucco that's already out
            if (CHECK_BTN_ANY(input->press.button, BtnEquippedWith(4, ITEM_CHICKEN, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_NONE))) {
                *should = false;
            } else if (CHECK_BTN_ANY(input->press.button, BTN_A)) {
                *should = false;
                Player_SetupAction(gPlayState, player, Player_Action_808464B0, 1);
                LinkAnimationHeader* anim = (LinkAnimationHeader*)&gPlayerAnim_link_normal_free2bom;
                LinkAnimation_Change(gPlayState, &player->skelAnime, anim, -1.0f, Animation_GetLastFrame(anim)-1.0f, 0.0f, ANIMMODE_ONCE, -6.0f);
                //Player_AnimPlayOnce(gPlayState, player, (LinkAnimationHeader*)&gPlayerAnim_link_normal_fighter2free);
                //set destroy on future player update to make the cucco appear more like it's being put away
                managedActor.delayedDestroy = 6;

            //destroy actor, but don't intercept default action
            } else if (CHECK_BTN_ANY(input->press.button, BTN_B | BtnNotEquippedWith(4, ITEM_CHICKEN, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_NONE))) {
                DestroyCarryActor();
            }
        }
    });
}

static RegisterShipInitFunc registerTradeItemUses(RegisterTradeItemUses);
