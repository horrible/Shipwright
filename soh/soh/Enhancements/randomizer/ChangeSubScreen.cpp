#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "ChangeSubScreen.h"
#include "TrackerSubScreen.h"

extern "C" {
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "soh/frame_interpolation.h"
extern PlayState* gPlayState;
extern "C" Gfx* KaleidoScope_DrawPageSections(Gfx* gfx, Vtx* vertices, void** textures);
extern "C" void KaleidoScope_UpdateNamePanel(PlayState* play);
}

typedef struct DisplayOpts {
    std::function<void()> drawPageFunc;
    Vtx* bgVtx;
    void** displayText;
    u16 textureFilter;
    bool runPostDraw;
    std::function<void()> postDrawFunc;
} DisplayOpts;

static u16 activeSubScreens[4];
static std::vector<std::function<DisplayOpts(u16)>> screens;
static s32 activeIndex = 3;
static u16 previousPage = 4;

void DrawSubScreenAtFace (u16 drawIndex, DisplayOpts opts) {
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
                Matrix_Translate(0.0f, (f32)WREG(2) / 100.0f, -(f32)WREG(3) / 100.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateX(-pauseCtx->unk_1F4 / 100.0f, MTXMODE_APPLY);
                break;
            case PAUSE_MAP:
                Matrix_Translate((f32)WREG(3) / 100.0f, (f32)WREG(2) / 100.0f, 0.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateZ(-pauseCtx->unk_1FC / 100.0f, MTXMODE_APPLY);
                Matrix_RotateY(-1.57f, MTXMODE_APPLY);
                break;
            case PAUSE_QUEST:
                Matrix_Translate(0.0f, (f32)WREG(2) / 100.0f, (f32)WREG(3) / 100.0f, MTXMODE_NEW);
                Matrix_Scale(0.78f, 0.78f, 0.78f, MTXMODE_APPLY);
                Matrix_RotateX(pauseCtx->unk_200 / 100.0f, MTXMODE_APPLY);
                Matrix_RotateY(3.14f, MTXMODE_APPLY);
                break;
            case PAUSE_EQUIP:
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
        if(activeIndex >= 0 && activeIndex < 4) {
            u16 oldIndex = pauseCtx->pageIndex;
            switch (activeIndex) {
                case 0:
                    pauseCtx->pageIndex = PAUSE_MAP;
                    break;
                case 1:
                    pauseCtx->pageIndex = PAUSE_QUEST;
                    break;
                case 2:
                    pauseCtx->pageIndex = PAUSE_EQUIP;
                    break;
                case 3:
                    pauseCtx->pageIndex = PAUSE_ITEM;
                    break;
            }
            opts.drawPageFunc();
            //some vanilla functions need a second function call for cursor or compass functions
            if (opts.runPostDraw) {
                Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);
                gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                opts.postDrawFunc();
            }
            pauseCtx->pageIndex = oldIndex;
            activeSubScreens[drawIndex] = activeIndex;
        } else {
            opts.drawPageFunc();
            if (opts.runPostDraw) {
                Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);
                gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
                opts.postDrawFunc();
            }
            activeSubScreens[drawIndex] = activeIndex;
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
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_ITEM) % 4),
        gPlayState->pauseCtx.itemPageVtx,
        (void **) sSelectItemTexs[gSaveContext.language],
        false,
        false //item screen draws its own cursor
    };
}
DisplayOpts DrawMapScreen(u16 index) { // PAUSE_MAP 1 - Map Subscreen - can be both dungeon and world
    return DisplayOpts{
        AssumeGlobal((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) ? KaleidoScope_DrawDungeonMap : KaleidoScope_DrawWorldMap),
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_MAP) % 4),
        gPlayState->pauseCtx.mapPageVtx,
        (void **) sMapTexs[gSaveContext.language],
        false,
        (bool) ((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, gSaveContext.mapIndex)), 
        //should always be 0 when not in a dungeon, but might crash since i haven't tested it
        AssumeGlobal(PauseMapMark_Draw)
    };
}
DisplayOpts DrawQuestScreen(u16 index) { //PAUSE_QUEST 2 - Quest Item Subscreen
    return DisplayOpts{
        AssumeGlobal(KaleidoScope_DrawQuestStatus),
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_QUEST) % 4),
        gPlayState->pauseCtx.questPageVtx,
        (void **) sQuestStatusTexs[gSaveContext.language],
        G_TF_BILERP,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index), //draw cursor when visible, todo recheck
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_QUEST) % 4)) //todo check
    };
}
DisplayOpts DrawEquipScreen(u16 index) { //PAUSE_EQUIP 3 - Equipment Subscreen
    return DisplayOpts{
        AssumeGlobal(KaleidoScope_DrawEquipment),
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4),
        gPlayState->pauseCtx.equipPageVtx,
        (void **) sEquipmentTexs[gSaveContext.language],
        false,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index),  //draw cursor when visible, todo recheck
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4)) //todo check
    };
}

DisplayOpts DrawTrackerScreen(u16 index) {
    return DisplayOpts{
        DrawTrackerSubScreen,
        gPlayState->pauseCtx.saveVtx,
        (void **) sSaveTexs[gSaveContext.language],
        false,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index), //use default cursor
        AssumeGlobal(KaleidoScope_DrawCursor, gPlayState->pauseCtx.pageIndex)
    };
}

void RegisterChangeSubScreen() {
    bool shouldRegister = true;

    screens.emplace_back(DrawMapScreen);
    screens.emplace_back(DrawQuestScreen);
    screens.emplace_back(DrawEquipScreen);
    screens.emplace_back(DrawItemScreen);
    screens.emplace_back(DrawTrackerScreen);

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN, shouldRegister, {
        u16 pageIndex = *va_arg(args, u16*);
        *should = true;
        PauseContext* pauseCtx = &gPlayState->pauseCtx;

        u16 prev;
        u16 curr;
        u16 next;
        u16 off;
        //increment
        if (pageIndex == previousPage+1 || (pageIndex == 0 && previousPage == 3)) {
            if (activeIndex < screens.size()-1) {
                activeIndex++;
            } else {
                activeIndex = 0;
            }
        //decrement
        } else if (pageIndex == previousPage-1 || (pageIndex == 3 && previousPage == 0)) {
            if (activeIndex > 0) {
                activeIndex--;
            } else {
                activeIndex = (s32) screens.size()-1;
            }
        }

        //not sure why, but vanilla draws the active screen last and it sometimes bugs out if we don't do that too?
        switch (pauseCtx->pageIndex) {
            case PAUSE_ITEM:
                next = PAUSE_MAP;
                prev = PAUSE_EQUIP;
                off =  PAUSE_QUEST;
                curr = PAUSE_ITEM;
                break;
            case PAUSE_MAP:
                next = PAUSE_QUEST;
                prev = PAUSE_ITEM;
                off =  PAUSE_EQUIP;
                curr = PAUSE_MAP;
                break;
            case PAUSE_QUEST:
                next = PAUSE_EQUIP;
                prev = PAUSE_MAP;
                off =  PAUSE_ITEM;
                curr = PAUSE_QUEST;
                break;
            case PAUSE_EQUIP:
                next = PAUSE_ITEM;
                prev = PAUSE_QUEST;
                off =  PAUSE_MAP;
                curr = PAUSE_EQUIP;
                break;
        }

        DrawSubScreenAtFace(prev, screens.at(activeIndex > 0 ? activeIndex-1 : (s32) (screens.size()-1))(prev));
        DrawSubScreenAtFace(next, screens.at(activeIndex < screens.size()-1 ? activeIndex+1 : 0)(next));
        DrawSubScreenAtFace(off, screens.at(activeIndex < screens.size()-2 ? activeIndex+2 : 0)(off));
        DrawSubScreenAtFace(curr, screens.at(activeIndex)(curr));
                
        previousPage = pageIndex;
    });

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN_NAME_PANEL, shouldRegister, {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        *should = true;
        //if subscreen at index is still vanilla, then just adjust the index
        if (activeSubScreens[pauseCtx->pageIndex] < 4) {
            u16 oldIndex = pauseCtx->pageIndex;
            switch (activeSubScreens[pauseCtx->pageIndex]) {
                case 0:
                    pauseCtx->pageIndex = PAUSE_MAP;
                    break;
                case 1:
                    pauseCtx->pageIndex = PAUSE_QUEST;
                    break;
                case 2:
                    pauseCtx->pageIndex = PAUSE_EQUIP;
                    break;
                case 3:
                    pauseCtx->pageIndex = PAUSE_ITEM;
                    break;
            }
            KaleidoScope_UpdateNamePanel(gPlayState);
            pauseCtx->pageIndex = oldIndex;
        } else {
            //do custom subscreen name panel updates here
        }
    });

    COND_VB_SHOULD(VB_SHOULD_DIM_BUTTON, shouldRegister, {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        *should = false;
        //if subscreen at index is still vanilla, then just adjust the index
        if (activeSubScreens[pauseCtx->pageIndex] < 4) {
            switch (activeSubScreens[pauseCtx->pageIndex]) {
                case 2:
                case 3:
                    gSaveContext.buttonStatus[1] = BTN_ENABLED;
                    gSaveContext.buttonStatus[2] = BTN_ENABLED;
                    gSaveContext.buttonStatus[3] = BTN_ENABLED;
                    // A-Button is already enabled
                    gSaveContext.buttonStatus[5] = BTN_ENABLED;
                    gSaveContext.buttonStatus[6] = BTN_ENABLED;
                    gSaveContext.buttonStatus[7] = BTN_ENABLED;
                    gSaveContext.buttonStatus[8] = BTN_ENABLED;
                    break;
                case 0:
                case 1:
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
    });
}

static RegisterShipInitFunc changeSubScreen(RegisterChangeSubScreen);
