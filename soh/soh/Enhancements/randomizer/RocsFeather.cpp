#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include <soh_assets.h>
#include <z64.h>
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

#define MAX_ROCS_USES 1

static uint8_t& rocsMaxUse = gSaveContext.ship.quest.data.randomizer.rocsFeathersAcquired;
static uint8_t& rocsRemaining = gSaveContext.ship.quest.data.randomizer.rocsFeathersRemaining; 
static uint8_t groundTimer = 0;
static f32 effectsScale = 1.0f;
static bool rocsConsumable = false;


void RegisterRocsFeather() {
    // Always register hooks - they only activate when ITEM_ROCS_FEATHER is actually used
    // Rando-specific logic (item pool, accessibility, etc.) is handled elsewhere
    bool shouldRegister = true;


    COND_HOOK(OnLoadFile, shouldRegister, [](int32_t fileNum) {
        rocsConsumable = Rando::Context::GetInstance()->GetOption(RSK_ROCS_FEATHER).Is(RO_ROCS_FEATHER_CONSUMABLE);
    });

    COND_HOOK(OnPlayerUpdate, shouldRegister, []() {
        Player* player = GET_PLAYER(gPlayState);

        // Reset Rocs count when touching the ground for 3+ frames
        if (player->actor.bgCheckFlags & 1) {
            if (groundTimer <= 3) {
                groundTimer++;
            }
        } else {
            groundTimer = 0;
        }

        if (!rocsConsumable && groundTimer >= 3) {
            rocsRemaining = rocsMaxUse;
        }
    });

    COND_VB_SHOULD(VB_CHANGE_HELD_ITEM_AND_USE_ITEM, shouldRegister, {
        int32_t usedItem = va_arg(args, int32_t);

        // Roc's Feather behaviour
        if (usedItem == ITEM_ROCS_FEATHER) {
            *should = false;

            if (rocsRemaining > 0) {
                rocsRemaining--;

                Player* player = GET_PLAYER(gPlayState);

                func_80838940(player, (LinkAnimationHeader*)&gPlayerAnim_link_rocs_feather_jump, 5.8f, gPlayState, 0);

                // Actionvar needed to prevent weird animation morph
                player->av2.actionVar2 = 1;

                // Move player forward on Roc's use
                player->linearVelocity = 5.0f;
                player->actor.world.rot.y = player->yaw = player->actor.shape.rot.y;

                if (gSaveContext.linkAge == LINK_AGE_CHILD) {
                    player->actor.velocity.y = 7.0f;
                    effectsScale = 1.0f;
                } else {
                    player->actor.velocity.y = 7.5f;
                    effectsScale = 1.5f;
                }

                Vec3f effectsPos = player->actor.home.pos;
                effectsPos.y += 3;

                EffectSsGRipple_Spawn(gPlayState, &effectsPos, static_cast<s16>(200 * effectsScale),
                                      static_cast<s16>(300 * effectsScale), 1);
                EffectSsGSplash_Spawn(gPlayState, &effectsPos, NULL, NULL, 0, static_cast<s16>(150 * effectsScale));

                // Remove hopping state when using Roc's after sidehop/backflip to allow grabbing ledges again
                player->stateFlags2 &= ~(PLAYER_STATE2_HOPPING);

                Player_PlaySfx(&player->actor, NA_SE_PL_SKIP);
            }
        }
    });

    COND_VB_SHOULD(VB_DRAW_CUSTOM_ITEM_NAME, shouldRegister, {
        u32 namedItem = va_arg(args, u32);
        if (namedItem == ITEM_ROCS_FEATHER) {
            *should = true;
            const char* textureName = gRocsFeatherItemNameENGTex;

            if (gSaveContext.language == LANGUAGE_GER) {
                textureName = gRocsFeatherItemNameGERTex;
            } else if (gSaveContext.language == LANGUAGE_FRA) {
                textureName = gRocsFeatherItemNameFRATex;
            }

            memcpy(gPlayState->pauseCtx.nameSegment, textureName, strlen(textureName) + 1);
        }
    });

    COND_VB_SHOULD(VB_ITEM_USES_AMMO, shouldRegister, {
        s16 item = *va_arg(args, s16*);
        if (item == ITEM_ROCS_FEATHER) {
            if (gSaveContext.ship.quest.data.randomizer.rocsFeathersAcquired > 1) {
                *should = true;
            } else {
                *should = false;
            }
        }
    });

    COND_VB_SHOULD(VB_DRAW_AMMO_COUNT, shouldRegister, {
        s16 item = *va_arg(args, s16*);
        if (item == ITEM_ROCS_FEATHER) {
            if (gSaveContext.ship.quest.data.randomizer.rocsFeathersAcquired > 1) {
                *should = true;
            } else {
                *should = false;
            }
        }
    });

    COND_VB_SHOULD(VB_COLOR_AMMO_GREEN, shouldRegister, {
        int16_t item = va_arg(args, int);
        u8 max = gSaveContext.ship.quest.data.randomizer.rocsFeathersAcquired;
        u8 current = gSaveContext.ship.quest.data.randomizer.rocsFeathersRemaining;
        if (item == ITEM_ROCS_FEATHER) {
            if (max == current) {
                *should = true;
            } else {
                *should = false;
            }
        }
    });
}

static RegisterShipInitFunc registerRocsFeather(RegisterRocsFeather);
