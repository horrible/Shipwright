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

//i think this is everything a custom screen will need. was going to drop vanilla index but then i realized MM screens will likely need it too
typedef struct SubScreenOpts {
    std::function<void()> drawPageFunc; // vanilla draws use 1-2 params that ship has in globals, so those are wrapped below & output as noarg funcs
    void* name; // tex for name that other subscreens should call this one
    Vtx* bgVtx;
    void** contentTex;
    u16 vanillaIndex = (u16) -1; // i'm underflowing this and you can't stop me (so that it defaults non-0)
    u16 textureFilter = false;
    bool runPostDraw = false;
    std::function<void()> postDrawFunc;
} SubScreenOpts;

static SubScreenOpts activeSubScreens[4];
static std::vector<std::function<SubScreenOpts(u16)>> screens; //this code assumes that screens.size() will be 4 at a minimum, so if you change it to be less, keep that in mind.
static s32 activeIndex = 3; //index = screen index, vanilla starts on PAUSE_EQUIP (page 3)
static u16 previousPage; //page = 0-3 pause screen face.

void DrawSubScreenAtPage (u16 drawPage, SubScreenOpts opts) {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        OPEN_DISPS(gPlayState->state.gfxCtx);
        gDPPipeSync(OVERLAY_DISP++);
        if (opts.textureFilter) {
            // needed on PAUSE_QUEST, which sets G_TF_BILERP
            gDPSetTextureFilter(POLY_OPA_DISP++, opts.textureFilter);
        }
       //PAUSE_ITEM draws with OVERLAY_DISP++ instead when not the active index, but doesn't look like it needs to
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA, G_CC_MODULATEIA);
        //switch which face of the pause menu cube (yes, it's a cube) we're drawing on based on the index supplied
        switch (drawPage) {
            case PAUSE_ITEM:
                //translations all stolen verbatim from KaleidoScope_DrawPageSections, check there directly below the hook
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
        POLY_OPA_DISP = KaleidoScope_DrawPageSections(POLY_OPA_DISP, opts.bgVtx, opts.contentTex);
        //for vanilla screens, override pageIndex to have functions display correctly
        u16 displayedPage = pauseCtx->pageIndex;
        if (opts.vanillaIndex <= 3) {
            pauseCtx->pageIndex = (pauseCtx->pageIndex + 4 - drawPage + opts.vanillaIndex) % 4;
        }
        opts.drawPageFunc();
        //some vanilla functions need a second function call for cursor or compass functions
        if (opts.runPostDraw) {
            //todo - next two lines only occur in the draw map flow, but don't seem to cause problems for equip & quest cursors
            Gfx_SetupDL_42Opa(gPlayState->state.gfxCtx);
            gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
            opts.postDrawFunc();
        }
        if (opts.vanillaIndex <= 3) {
            pauseCtx->pageIndex = displayedPage;
        }
        activeSubScreens[drawPage] = opts;

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

SubScreenOpts DrawItemScreen(u16 index) { //PAUSE_ITEM 0 - Item Subscreen
    return SubScreenOpts{
        AssumeGlobal(KaleidoScope_DrawItemSelect),
        (void *) sScreenTransitionTexs[PAUSE_ITEM][gSaveContext.language],
        gPlayState->pauseCtx.itemPageVtx,
        (void **) sSelectItemTexs[gSaveContext.language],
        PAUSE_ITEM,
        //item screen draws its own cursor
    };
}
SubScreenOpts DrawMapScreen(u16 index) { // PAUSE_MAP 1 - Map Subscreen - can be both dungeon and world
    return SubScreenOpts{
        AssumeGlobal((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) ? KaleidoScope_DrawDungeonMap : KaleidoScope_DrawWorldMap),
        (void *) sScreenTransitionTexs[PAUSE_MAP][gSaveContext.language],
        gPlayState->pauseCtx.mapPageVtx,
        (void **) sMapTexs[gSaveContext.language],
        PAUSE_MAP,
        false,
        //CHECK_DUNGEON_ITEM should always be 0 when not in a dungeon, but it looked wonky and i didnt want to check it so :shrug:
        (bool) ((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, gSaveContext.mapIndex)), 
        AssumeGlobal(PauseMapMark_Draw)
    };
}
SubScreenOpts DrawQuestScreen(u16 index) { //PAUSE_QUEST 2 - Quest Item Subscreen
    return SubScreenOpts{
        AssumeGlobal(KaleidoScope_DrawQuestStatus),
        (void *) sScreenTransitionTexs[PAUSE_QUEST][gSaveContext.language],
        gPlayState->pauseCtx.questPageVtx,
        (void **) sQuestStatusTexs[gSaveContext.language],
        PAUSE_QUEST,
        G_TF_BILERP,
        //draw cursor when visible, & not on Z/L or R
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index), 
        //might only be needed because of KaleidoScope_DrawCursor's weird PAUSE_QUEST check (or might not be needed atp)
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_QUEST) % 4)) 

    };
}
SubScreenOpts DrawEquipScreen(u16 index) { //PAUSE_EQUIP 3 - Equipment Subscreen
    return SubScreenOpts{
        AssumeGlobal(KaleidoScope_DrawEquipment),
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4),
        (void *) sScreenTransitionTexs[PAUSE_EQUIP][gSaveContext.language],
        gPlayState->pauseCtx.equipPageVtx,
        (void **) sEquipmentTexs[gSaveContext.language],
        PAUSE_EQUIP,
        false,
        (gPlayState->pauseCtx.cursorSpecialPos == 0 && gPlayState->pauseCtx.pageIndex == index), 
        AssumeGlobal(KaleidoScope_DrawCursor, (u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4))
    };
}

SubScreenOpts DrawTrackerScreen(u16 index) {
    return SubScreenOpts{
        DrawTrackerSubScreen,
        //todo - textures for name and tex
        (void *) sScreenTransitionTexs[PAUSE_ITEM][gSaveContext.language],
        gPlayState->pauseCtx.saveVtx,
        (void **) sSaveTexs[gSaveContext.language]
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
        static bool isIncrement; //want this to persist because most frames will be neither

        //these will only be different when a page was just swapped
        if (pageIndex == previousPage + 1 || (pageIndex == 0 && previousPage == 3)) {
            if (activeIndex < screens.size() - 1) {
                activeIndex++;
            } else {
                activeIndex = 0;
            }
            isIncrement = true;
        } else if (pageIndex == previousPage - 1 || (pageIndex == 3 && previousPage == 0)) {
            if (activeIndex > 0) {
                activeIndex--;
            } else {
                activeIndex = (s32) screens.size() - 1;
            }
            isIncrement = false;
        }

        //not sure why, but vanilla draws the active screen last and it sometimes bugs out if we don't do that too?
        u16 curr = pageIndex;
        u16 next = (pageIndex + 1) % 4;
        u16 prev = (pageIndex + 3) % 4;
        u16 off =  (pageIndex + 2) % 4;

        DrawSubScreenAtPage(prev, screens.at(activeIndex > 0 ? activeIndex - 1 : (s32) (screens.size()-1))(prev));
        DrawSubScreenAtPage(next, screens.at(activeIndex < screens.size() - 1 ? activeIndex + 1 : 0)(next));
        //real todo - actually test it
        //todo - attempt to fix display suttering to having off be -2 instead when going down
        if (isIncrement) {
            DrawSubScreenAtPage(off, screens.at(activeIndex < screens.size() - 2 ? activeIndex + 2 : 1)(off));
        } else {
            DrawSubScreenAtPage(off, screens.at(activeIndex > 1 ? activeIndex - 2 : (s32) (screens.size() - 2))(off));
        }
        DrawSubScreenAtPage(curr, screens.at(activeIndex)(curr));

        //most of the time this will just be setting it back to the same value. and that's good.
        previousPage = pageIndex;
    });

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN_NAME, shouldRegister, {
        Gfx** gfx = va_arg(args, Gfx**);
        u16 pageIndex = gPlayState->pauseCtx.pageIndex;
        s16 cursorPos = gPlayState->pauseCtx.cursorSpecialPos;
        //the adjacent names should always be accurate because we swap screens out when they're opposite to the active one
        //it'd probably be more accurate to do based on the actual screens vector instead, but that's probably much slower
        if (cursorPos == PAUSE_CURSOR_PAGE_LEFT) {
            *should = true;
            *gfx = KaleidoScope_QuadTextureIA8(*gfx, activeSubScreens[(pageIndex + 3) % 4].name, 128, 16, 0);
        } else if (cursorPos == PAUSE_CURSOR_PAGE_RIGHT) {
            *should = true;
            *gfx = KaleidoScope_QuadTextureIA8(*gfx, activeSubScreens[(pageIndex + 1) % 4].name, 128, 16, 0);
        }
    });

    COND_VB_SHOULD(VB_CHANGE_SUBSCREEN_NAME_PANEL, shouldRegister, {
        u16 displayedIndex = gPlayState->pauseCtx.pageIndex;

        if (displayedIndex != activeSubScreens[displayedIndex].vanillaIndex) {
            *should = true;
            //retain vanilla functionality for vanilla screens
            if (activeSubScreens[displayedIndex].vanillaIndex <= 3) {
                gPlayState->pauseCtx.pageIndex = activeSubScreens[displayedIndex].vanillaIndex;
                KaleidoScope_UpdateNamePanel(gPlayState);
                gPlayState->pauseCtx.pageIndex = displayedIndex;
            } else {
                //okay NOW do custom subscreen updates
            }

        }
    });

    COND_VB_SHOULD(VB_SHOULD_DIM_BUTTON, shouldRegister, {
        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        *should = false;
        //if subscreen vanilla, then just dim based on index
        if (activeSubScreens[pauseCtx->pageIndex].vanillaIndex < 4) {
            switch (activeSubScreens[pauseCtx->pageIndex].vanillaIndex) {
                case PAUSE_ITEM:
                case PAUSE_EQUIP:
                    //cbtn
                    gSaveContext.buttonStatus[1] = BTN_ENABLED;
                    gSaveContext.buttonStatus[2] = BTN_ENABLED;
                    gSaveContext.buttonStatus[3] = BTN_ENABLED;
                    //dpad
                    gSaveContext.buttonStatus[5] = BTN_ENABLED;
                    gSaveContext.buttonStatus[6] = BTN_ENABLED;
                    gSaveContext.buttonStatus[7] = BTN_ENABLED;
                    gSaveContext.buttonStatus[8] = BTN_ENABLED;
                    break;
                case PAUSE_QUEST:
                case PAUSE_MAP:
                    //cbtn
                    gSaveContext.buttonStatus[1] = BTN_DISABLED;
                    gSaveContext.buttonStatus[2] = BTN_DISABLED;
                    gSaveContext.buttonStatus[3] = BTN_DISABLED;
                    //dpad
                    gSaveContext.buttonStatus[5] = BTN_DISABLED;
                    gSaveContext.buttonStatus[6] = BTN_DISABLED;
                    gSaveContext.buttonStatus[7] = BTN_DISABLED;
                    gSaveContext.buttonStatus[8] = BTN_DISABLED;
                    break;
                default:
                    break;
            }
        } else {
            //do custom subscreen button dimming here
        }
    });
}

static RegisterShipInitFunc changeSubScreen(RegisterChangeSubScreen);
