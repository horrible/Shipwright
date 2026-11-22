#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_nes_static/icon_item_nes_static.h"
#include "soh/frame_interpolation.h"

extern "C" {
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_nes_static/icon_item_nes_static.h"
#include "textures/icon_item_gameover_static/icon_item_gameover_static.h"
#include "soh/frame_interpolation.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <libultraship/bridge/consolevariablebridge.h>
extern PlayState* gPlayState;
extern "C" Gfx* KaleidoScope_DrawPageSections(Gfx* gfx, Vtx* vertices, void** textures);
extern "C" void KaleidoScope_UpdateNamePanel(PlayState* play);
}
//todo - move out
static const char* sEquipmentENGTexs[] = {
    gPauseEquipment00Tex,    gPauseEquipment01Tex, gPauseEquipment02Tex, gPauseEquipment03Tex, gPauseEquipment04Tex,
    gPauseEquipment10ENGTex, gPauseEquipment11Tex, gPauseEquipment12Tex, gPauseEquipment13Tex, gPauseEquipment14Tex,
    gPauseEquipment20Tex,    gPauseEquipment21Tex, gPauseEquipment22Tex, gPauseEquipment23Tex, gPauseEquipment24Tex,
};
static const char* sSelectItemENGTexs[] = {
    gPauseSelectItem00ENGTex, gPauseSelectItem01Tex,    gPauseSelectItem02Tex,    gPauseSelectItem03Tex,
    gPauseSelectItem04Tex,    gPauseSelectItem10ENGTex, gPauseSelectItem11Tex,    gPauseSelectItem12Tex,
    gPauseSelectItem13Tex,    gPauseSelectItem14Tex,    gPauseSelectItem20ENGTex, gPauseSelectItem21Tex,
    gPauseSelectItem22Tex,    gPauseSelectItem23Tex,    gPauseSelectItem24Tex,
};
static const char* sMapENGTexs[] = {
    gPauseMap00Tex,    gPauseMap01Tex, gPauseMap02Tex, gPauseMap03Tex, gPauseMap04Tex,
    gPauseMap10ENGTex, gPauseMap11Tex, gPauseMap12Tex, gPauseMap13Tex, gPauseMap14Tex,
    gPauseMap20Tex,    gPauseMap21Tex, gPauseMap22Tex, gPauseMap23Tex, gPauseMap24Tex,
};
static const char* sQuestStatusENGTexs[] = {
    gPauseQuestStatus00ENGTex, gPauseQuestStatus01Tex,    gPauseQuestStatus02Tex,    gPauseQuestStatus03Tex,
    gPauseQuestStatus04Tex,    gPauseQuestStatus10ENGTex, gPauseQuestStatus11Tex,    gPauseQuestStatus12Tex,
    gPauseQuestStatus13Tex,    gPauseQuestStatus14Tex,    gPauseQuestStatus20ENGTex, gPauseQuestStatus21Tex,
    gPauseQuestStatus22Tex,    gPauseQuestStatus23Tex,    gPauseQuestStatus24Tex,
};
static const char* sSaveENGTexs[] = {
    gPauseSave00Tex,    gPauseSave01Tex, gPauseSave02Tex, gPauseSave03Tex, gPauseSave04Tex,
    gPauseSave10ENGTex, gPauseSave11Tex, gPauseSave12Tex, gPauseSave13Tex, gPauseSave14Tex,
    gPauseSave20Tex,    gPauseSave21Tex, gPauseSave22Tex, gPauseSave23Tex, gPauseSave24Tex,
};

static const char* sGameOverTexs[] = {
    gPauseSave00Tex,     gPauseSave01Tex, gPauseSave02Tex, gPauseSave03Tex, gPauseSave04Tex,
    gPauseGameOver10Tex, gPauseSave11Tex, gPauseSave12Tex, gPauseSave13Tex, gPauseSave14Tex,
    gPauseSave20Tex,     gPauseSave21Tex, gPauseSave22Tex, gPauseSave23Tex, gPauseSave24Tex,
};

typedef struct DisplayOpts {
    std::function<void()> drawPageFunc;
    u16 screenIndex;
    Vtx* bgVtx;
    void** displayText;
    u16 textureFilter;
    bool runPostDraw;
    std::function<void()> postDrawFunc;
} DisplayOpts;

static u16 customSubScreen[4];

void DrawSubScreenAtFace (u16 drawIndex, DisplayOpts opts) {
        LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! START");
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        OPEN_DISPS(gPlayState->state.gfxCtx);
        gDPPipeSync(OVERLAY_DISP++);
        if (opts.textureFilter) {
            gDPSetTextureFilter(POLY_OPA_DISP++, opts.textureFilter); // needed on PAUSE_QUEST
        } /* else if { PAUSE_ITEM uses this for some reason? but only when not in focus?
            gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA, G_CC_MODULATEIA);
        }*/
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA, G_CC_MODULATEIA);
        switch (drawIndex) {
            case PAUSE_ITEM:
                LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! PAUSE_ITEM");
                Matrix_Translate(0.0f, (f32)WREG(2) / 100.0f, -(f32)WREG(3) / 100.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateX(-pauseCtx->unk_1F4 / 100.0f, MTXMODE_APPLY);
                break;
            case PAUSE_MAP:
                LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! PAUSE_MAP");
                Matrix_Translate((f32)WREG(3) / 100.0f, (f32)WREG(2) / 100.0f, 0.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateZ(-pauseCtx->unk_1FC / 100.0f, MTXMODE_APPLY);
                Matrix_RotateY(-1.57f, MTXMODE_APPLY);
                break;
            case PAUSE_QUEST:
                LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! PAUSE_QUEST");
                Matrix_Translate(0.0f, (f32)WREG(2) / 100.0f, (f32)WREG(3) / 100.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateX(pauseCtx->unk_200 / 100.0f, MTXMODE_APPLY);
                Matrix_RotateY(3.14f, MTXMODE_APPLY);
                break;
            case PAUSE_EQUIP:
                LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! PAUSE_EQUIP");
                Matrix_Translate(-(f32)WREG(3) / 100.0f, (f32)WREG(2) / 100.0f, 0.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateZ(pauseCtx->unk_1F8 / 100.0f, MTXMODE_APPLY);
                Matrix_RotateY(1.57f, MTXMODE_APPLY);
                break;
        }
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gPlayState->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        POLY_OPA_DISP = KaleidoScope_DrawPageSections(POLY_OPA_DISP, opts.bgVtx, opts.displayText);
        //for vanilla screens
        //override pageIndex to let vanilla functions display correctly
        if(opts.screenIndex >= 0 && opts.screenIndex < 4) {
            LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! screenIndex %d", opts.screenIndex);
            u16 oldIndex = pauseCtx->pageIndex;
            pauseCtx->pageIndex = opts.screenIndex;
            opts.drawPageFunc();
            //some vanilla functions need a second function call for cursor or compass functions
            if (opts.runPostDraw) {
        LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! runPostDraw");
                Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);
                gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                opts.postDrawFunc();
            }
            pauseCtx->pageIndex = oldIndex;
            customSubScreen[drawIndex] = opts.screenIndex;
        } else {
            opts.drawPageFunc();
            if (opts.runPostDraw) {
        LUSLOG_ERROR("LIZDEBUG!!! in DrawSubScreenAtFace!!! runPostDraw");
                Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);
                gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                opts.postDrawFunc();
            }
            customSubScreen[drawIndex] = opts.screenIndex;
        }

        CLOSE_DISPS(gPlayState->state.gfxCtx);
}

//overload wrapper to assume globally available parameters to normalize expected signature
std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*, GraphicsContext *gfxCtx)) {
    return [wrappedFunc](void) {
        wrappedFunc(gPlayState, gPlayState->state.gfxCtx);
    };
}
//overload wrapper to assume globally available parameters to normalize expected signature
std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*, u16 pageIndex), u16 pageIndex) {
    return [wrappedFunc, pageIndex](void) {
        wrappedFunc(gPlayState, pageIndex);
    };
}

//overload wrapper to assume globally available parameters to normalize expected signature
std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*)) {
    return [wrappedFunc](void) {
        wrappedFunc(gPlayState);
    };
}

DisplayOpts DrawItemScreen(u16 index) { //PAUSE_ITEM 0 - Item Subscreen
    return DisplayOpts{
        AssumeGlobal(KaleidoScope_DrawItemSelect),
        (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_ITEM) % 4),
        gPlayState->pauseCtx.itemPageVtx,
        (void **) sSelectItemENGTexs,
        false,
        false //item screen draws its own cursor
    };
}
DisplayOpts DrawMapScreen(u16 index) { // PAUSE_MAP 1 - Map Subscreen - can be both dungeon and world
    return DisplayOpts{
        AssumeGlobal((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) ? KaleidoScope_DrawDungeonMap : KaleidoScope_DrawWorldMap),
        (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_MAP) % 4),
        gPlayState->pauseCtx.mapPageVtx,
        (void **) sMapENGTexs,
        false,
        (bool) ((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, gSaveContext.mapIndex)), //should always be 0 when not in a dungeon, but might crash since i haven't tested it
        AssumeGlobal(PauseMapMark_Draw)
    };
}
DisplayOpts DrawQuestScreen(u16 index) { //PAUSE_QUEST 2 - Quest Item Subscreen
    return DisplayOpts{
        AssumeGlobal(KaleidoScope_DrawQuestStatus),
        (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_QUEST) % 4),
        gPlayState->pauseCtx.questPageVtx,
        (void **) sQuestStatusENGTexs,
        G_TF_BILERP,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index), //draw cursor when visible, todo recheck
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_QUEST) % 4)) //todo check
    };
}
DisplayOpts DrawEquipScreen(u16 index) { //PAUSE_EQUIP 3 - Equipment Subscreen
    return DisplayOpts{
        AssumeGlobal(KaleidoScope_DrawEquipment),
        (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4),
        gPlayState->pauseCtx.equipPageVtx,
        (void **) sEquipmentENGTexs,
        false,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index),  //draw cursor when visible, todo recheck
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4)) //todo check
    };
}

void RegisterChangeSubScreen() {
    bool shouldRegister = true;

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN, shouldRegister, {
        LUSLOG_ERROR("LIZDEBUG!!! in VB_CHANGE_SUBSCREEN!!! START");
        u16 pageIndex = *va_arg(args, u16*);
        *should = true;
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        //todo - add managing array to dynamically assign screens to faces as needed for screen counts > 4

        //not sure why, but vanilla draws the active screen last and it bugs out if we don't do that too
        switch (pauseCtx->pageIndex) {
            case PAUSE_ITEM:
                DrawSubScreenAtFace(PAUSE_MAP,   DrawMapScreen(PAUSE_MAP));
                DrawSubScreenAtFace(PAUSE_QUEST, DrawQuestScreen(PAUSE_QUEST));
                DrawSubScreenAtFace(PAUSE_EQUIP, DrawEquipScreen(PAUSE_EQUIP));
                DrawSubScreenAtFace(PAUSE_ITEM,  DrawItemScreen(PAUSE_ITEM));
                break;
            case PAUSE_MAP:
                DrawSubScreenAtFace(PAUSE_QUEST, DrawQuestScreen(PAUSE_QUEST));
                DrawSubScreenAtFace(PAUSE_EQUIP, DrawEquipScreen(PAUSE_EQUIP));
                DrawSubScreenAtFace(PAUSE_ITEM,  DrawItemScreen(PAUSE_ITEM));
                DrawSubScreenAtFace(PAUSE_MAP,   DrawMapScreen(PAUSE_MAP));
                break;
            case PAUSE_QUEST:
                DrawSubScreenAtFace(PAUSE_EQUIP, DrawEquipScreen(PAUSE_EQUIP));
                DrawSubScreenAtFace(PAUSE_ITEM,  DrawItemScreen(PAUSE_ITEM));
                DrawSubScreenAtFace(PAUSE_MAP,   DrawMapScreen(PAUSE_MAP));
                DrawSubScreenAtFace(PAUSE_QUEST, DrawQuestScreen(PAUSE_QUEST));
                break;
            case PAUSE_EQUIP:
                DrawSubScreenAtFace(PAUSE_ITEM,  DrawItemScreen(PAUSE_ITEM));
                DrawSubScreenAtFace(PAUSE_MAP,   DrawMapScreen(PAUSE_MAP));
                DrawSubScreenAtFace(PAUSE_QUEST, DrawQuestScreen(PAUSE_QUEST));
                DrawSubScreenAtFace(PAUSE_EQUIP, DrawEquipScreen(PAUSE_EQUIP));
                break;
        }
        LUSLOG_ERROR("LIZDEBUG!!! in VB_CHANGE_SUBSCREEN!!! END!! customSubScreen %d drawn on %d", customSubScreen[pauseCtx->pageIndex], pauseCtx->pageIndex);
    });

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN_NAME_PANEL, shouldRegister, {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        if (customSubScreen[pauseCtx->pageIndex] !=  pauseCtx->pageIndex) {
            *should = true;
            //if subscreen at index is still vanilla, then use vanila name panel updates and just adjust the index
            if (customSubScreen[pauseCtx->pageIndex] < 4) {
                u16 oldIndex = pauseCtx->pageIndex;
                pauseCtx->pageIndex = customSubScreen[pauseCtx->pageIndex];
                KaleidoScope_UpdateNamePanel(gPlayState);
                pauseCtx->pageIndex = oldIndex;
            } else {
                //do custom subscreen name panel updates here
            }
        }
    });

    COND_VB_SHOULD(VB_SHOULD_DIM_BUTTON, shouldRegister, {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        if (customSubScreen[pauseCtx->pageIndex] !=  pauseCtx->pageIndex) {
            *should = false;
            //if subscreen at index is still vanilla, then use vanila name panel updates and just adjust the index
            if (customSubScreen[pauseCtx->pageIndex] < 4) {
                switch (customSubScreen[pauseCtx->pageIndex]) {
                    case PAUSE_ITEM:
                    case PAUSE_EQUIP:
                        gSaveContext.buttonStatus[1] = BTN_ENABLED;
                        gSaveContext.buttonStatus[2] = BTN_ENABLED;
                        gSaveContext.buttonStatus[3] = BTN_ENABLED;
                        // A-Button is already enabled
                        gSaveContext.buttonStatus[5] = BTN_ENABLED;
                        gSaveContext.buttonStatus[6] = BTN_ENABLED;
                        gSaveContext.buttonStatus[7] = BTN_ENABLED;
                        gSaveContext.buttonStatus[8] = BTN_ENABLED;
                        break;
                    case PAUSE_MAP:
                    case PAUSE_QUEST:
                    default:
                        gSaveContext.buttonStatus[1] = BTN_DISABLED;
                        gSaveContext.buttonStatus[2] = BTN_DISABLED;
                        gSaveContext.buttonStatus[3] = BTN_DISABLED;
                        // A-Button is already enabled
                        gSaveContext.buttonStatus[5] = BTN_DISABLED;
                        gSaveContext.buttonStatus[6] = BTN_DISABLED;
                        gSaveContext.buttonStatus[7] = BTN_DISABLED;
                        gSaveContext.buttonStatus[8] = BTN_DISABLED;
                        break;
                }
            } else {
                //do custom subscreen button dimming here
            }
        }
    });
}

static RegisterShipInitFunc changeSubScreen(RegisterChangeSubScreen);
