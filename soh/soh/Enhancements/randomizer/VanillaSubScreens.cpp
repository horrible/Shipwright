
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/kaleido.h"
#include "ChangeSubScreen.h"
#include "soh/ShipInit.hpp"
#include "SubScreenKaleidoTexs.h"
#include "libultraship/bridge/consolevariablebridge.h"

extern "C" {
#include "src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
extern PlayState* gPlayState;
}

#define CVAR_SUBSCREEN_NAME CVAR_ENHANCEMENT("EnableReplaceSubscreen")
#define CVAR_SUBSCREEN_ACTIVE (bool) CVarGetInteger(CVAR_SUBSCREEN_NAME, 0)

SubScreenOpts ItemSubScreenOpts() { //PAUSE_ITEM 0 - Item Subscreen
    if (gPlayState ==  nullptr) {
        return SubScreenOpts{ "Item" };
    }
    return SubScreenOpts{
        "Item",
        AssumeGlobal(KaleidoScope_DrawItemSelect),
        (void *) sScreenTransitionTexs[PAUSE_ITEM][gSaveContext.language],
        gPlayState->pauseCtx.itemPageVtx,
        (void **) sSelectItemTexs[gSaveContext.language],
        false,
        PAUSE_ITEM,
        false
    };
        //item screen draws its own cursor
}

SubScreenOpts MapSubScreenOpts() { // PAUSE_MAP 1 - Map Subscreen - can be both dungeon and world
    if (gPlayState ==  nullptr) {
        return SubScreenOpts{ "Map" };
    }
    return SubScreenOpts{
        "Map",
        //todo - fix dungeon list
        AssumeGlobal((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) ? KaleidoScope_DrawDungeonMap : KaleidoScope_DrawWorldMap),
        (void *) sScreenTransitionTexs[PAUSE_MAP][gSaveContext.language],
        gPlayState->pauseCtx.mapPageVtx,
        (void **) sMapTexs[gSaveContext.language],
        true,
        PAUSE_MAP,
        false,
        (bool) ((gPlayState->sceneNum >= 0) && (gPlayState->sceneNum <= 24) && CHECK_DUNGEON_ITEM(DUNGEON_COMPASS, gSaveContext.mapIndex)),
        AssumeGlobal(PauseMapMark_Draw)
    };
}

// has an existing override see below.
SubScreenOpts QuestSubScreenOpts() { //PAUSE_QUEST 2 - Quest Item Subscreen
    if (gPlayState ==  nullptr) {
        return SubScreenOpts{ "Quest" };
    }
    return SubScreenOpts{
        "Quest",
        AssumeGlobal(KaleidoScope_DrawQuestStatus),
        (void *) sScreenTransitionTexs[PAUSE_QUEST][gSaveContext.language],
        gPlayState->pauseCtx.questPageVtx,
        (void **) sQuestStatusTexs[gSaveContext.language],
        true,
        PAUSE_QUEST,
        G_TF_BILERP
    };
}

//not technically vanilla but /shrug close enough
// when this reimpl is disabled, this screen shows in place of PAUSE_QUEST when randoQuestMode is set
SubScreenOpts MiscCollectiblesSubScreenOpts() { //PAUSE_QUEST 2 - Quest Item Subscreen
    if (gPlayState ==  nullptr) {
        return SubScreenOpts{ "Misc Collectibles" };
    }
    return SubScreenOpts{
        "Misc Collectibles",
        AssumeGlobal(RandoKaleido_DrawMiscCollectibles),
        (void *) sScreenTransitionTexs[PAUSE_QUEST][gSaveContext.language],
        gPlayState->pauseCtx.saveVtx,
        (void **) sGameOverTexs, //not language dependent?
        false,
        PAUSE_QUEST,
        G_TF_BILERP
    };
}

SubScreenOpts EquipSubScreenOpts() { //PAUSE_EQUIP 3 - Equipment Subscreen
    if (gPlayState ==  nullptr) {
        return SubScreenOpts{ "Equipment" };
    }
    return SubScreenOpts{
        "Equipment",
        AssumeGlobal(KaleidoScope_DrawEquipment),
        //(u16) ((gPlayState->pauseCtx.pageIndex + 4 - index + PAUSE_EQUIP) % 4),
        (void *) sScreenTransitionTexs[PAUSE_EQUIP][gSaveContext.language],
        gPlayState->pauseCtx.equipPageVtx,
        (void **) sEquipmentTexs[gSaveContext.language],
        true,
        PAUSE_EQUIP,
        false
    };
}

void EnableVanillaSubScreens() {
    if (CVAR_SUBSCREEN_ACTIVE) {
        AddSubScreen(MapSubScreenOpts);
        AddSubScreen(QuestSubScreenOpts);
        AddSubScreen(EquipSubScreenOpts);
        AddSubScreen(ItemSubScreenOpts);
        if (IS_RANDO) {
            AddSubScreen(MiscCollectiblesSubScreenOpts);
        }
    } else {
        RemoveSubScreen(MapSubScreenOpts);
        RemoveSubScreen(QuestSubScreenOpts);
        RemoveSubScreen(EquipSubScreenOpts);
        RemoveSubScreen(ItemSubScreenOpts);
        RemoveSubScreen(MiscCollectiblesSubScreenOpts);
    }
}

void RegisterVanillaSubScreens() {
    EnableVanillaSubScreens();

    COND_HOOK(OnLoadFile, CVAR_SUBSCREEN_ACTIVE, [](int32_t) {
        EnableVanillaSubScreens();
    });

}

static RegisterShipInitFunc registerVanillaSubScreens(RegisterVanillaSubScreens, { CVAR_SUBSCREEN_NAME });

