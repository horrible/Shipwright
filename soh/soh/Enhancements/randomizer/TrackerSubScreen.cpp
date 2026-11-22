#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/randomizer_item_tracker.h"
#include "soh/frame_interpolation.h"

extern "C" {
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "soh/frame_interpolation.h"
extern PlayState* gPlayState;
}

//extremely rough example of a tracker, cannibalized from the kaleido item screen
void DrawTrackerSubScreen() {

    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    int i;
    int j;
    s32 canSpeak[] = {
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_DEKU),
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_GERUDO),
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_GORON),
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_HYLIAN),
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_KOKIRI),
        Flags_GetRandomizerInf(RAND_INF_CAN_SPEAK_ZORA)
    };

    for (i = j = 0; i < 6; i++, j += 4) {
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, gPlayState->pauseCtx.alpha);

        gSPVertex(POLY_OPA_DISP++, (uintptr_t) &gPlayState->pauseCtx.itemVtx[j + 0], 4, 0);
        bool not_acquired = !canSpeak[i];
        if (not_acquired) {
            gDPSetGrayscaleColor(POLY_OPA_DISP++, 109, 109, 109, 255);
            gSPGrayscale(POLY_OPA_DISP++, true);
        }
        KaleidoScope_DrawQuadTextureRGBA32(gPlayState->state.gfxCtx, gItemIcons[1], 32, 32, 0);
        gSPGrayscale(POLY_OPA_DISP++, false);
    }
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}
