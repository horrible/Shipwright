#include <libultraship/bridge/consolevariablebridge.h>


#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include <soh_assets.h>
#include <z64.h>
#include "z64animation.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
extern s32 func_8083A4A8(Player*, PlayState*);
}

// basically just yoinked from Roc's Feather, and combined with values from z_player.c to try and match the auto-jump
// characteristics
void TriggerManualJump() {

    Player* player = GET_PLAYER(gPlayState);
    player->linearVelocity = 5.8f;
    func_8083A4A8(player, gPlayState);
}

void RegisterManualJump() {
    bool shouldRegister = true;

    COND_VB_SHOULD(VB_CHANGE_ACTION_AND_USE, shouldRegister, {
        if (CVarGetInteger(CVAR_ENHANCEMENT("ManualJumpOnA"), 0) != 1) {
            *should = true;
            return;
        }

        Player* player = va_arg(args, Player*);
        PlayState* playState = va_arg(args, PlayState*);
        Input* input = &playState->state.input[0];

        u16 action = playState->interfaceCtx.unk_1F0;

        // not usable from A during z-targeting, swimming, while paused, when grounded and any
        // action is displayed, while climbing ladders, when midair with something held,
        // while crawling, and when there are no rocs remaining to use
        if ((player->stateFlags1 &
             (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL)) ||
            (player->stateFlags1 & PLAYER_STATE1_IN_WATER) || (player->stateFlags2 & PLAYER_STATE2_CRAWLING) ||
            (playState->pauseCtx.state > 0 && playState->pauseCtx.state < 18) ||
            ((player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && action != DO_ACTION_NONE) ||
            (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
            *should = true;
        } else {

            if (CHECK_BTN_ANY(input->press.button, BTN_A)) {
                *should = false;
                TriggerManualJump();

            } else if (!(CHECK_BTN_ANY(input->press.button, BTN_A)) &&
                       player->actor.bgCheckFlags & (BGCHECKFLAG_GROUND_LEAVE | BGCHECKFLAG_GROUND_TOUCH)) {
                *should = false;
                playState->interfaceCtx.unk_1F0 = DO_ACTION_NONE;
            }
        }
    });

    COND_VB_SHOULD(VB_SET_ACTION_TEXTURE, shouldRegister, {
        if (CVarGetInteger(CVAR_ENHANCEMENT("ManualJumpOnA"), 0) != 1) {
            *should = false;
            return;
        }

        u16* action = va_arg(args, u16*);
        const char** alternateTexture = va_arg(args, const char**);
        Player* player = GET_PLAYER(gPlayState);

        // clear action manually on jumping/landing to avoid issues with value not refreshing on change when direction
        // input is maintained
        if (player->actor.bgCheckFlags & (BGCHECKFLAG_GROUND_LEAVE | BGCHECKFLAG_GROUND_TOUCH)) {
            *action = *action == DO_ACTION_NONE ? DO_ACTION_MAX : DO_ACTION_NONE;
            *should = false;

            // do not display on A during z-targeting, while paused, when grounded and any
            // action is displayed, while climbing ladders, when midair with something held,
            // while crawling, and when there are no rocs remaining to use
        } else if ((player->stateFlags1 &
                    (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL)) ||
                   (player->stateFlags1 & PLAYER_STATE1_IN_WATER) || (player->stateFlags2 & PLAYER_STATE2_CRAWLING) ||
                   (gPlayState->pauseCtx.state > 0 && gPlayState->pauseCtx.state < 18) ||
                   ((player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && *action != DO_ACTION_NONE) ||
                   (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND))) {
            *should = false;
        } else {
            *action = DO_ACTION_JUMP;
        }
    });
}

static RegisterShipInitFunc registerManualJump(RegisterManualJump);
