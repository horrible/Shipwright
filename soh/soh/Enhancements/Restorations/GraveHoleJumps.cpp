#include <libultraship/bridge/consolevariablebridge.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "z64scene.h"

extern "C" {
#include "z64.h"
extern PlayState* gPlayState;
}

#define NORMALIZE_GRAVE_JUMPS true
#define CVAR_GRAVE_HOLE_NAME CVAR_ENHANCEMENT("GraveHoles")
#define GRAVE_HOLES_DEFAULT 0
#define CVAR_GRAVE_HOLE_VALUE CVarGetInteger(CVAR_GRAVE_HOLE_NAME, GRAVE_HOLES_DEFAULT)
#define GRAVE_HOLES { { {723, 43}, {806, 118} }, { {121, 297}, {205, 380} }, { {-387, 137}, {-305, 219} }, { {-622, -390}, {-530, -290} } }

bool InBounds(Vec3f pos, f32 bounds[][2] ) {
 return pos.x >= bounds[0][0] && pos.x <= bounds[1][0] &&
     pos.z >= bounds[0][1] && pos.z <= bounds[1][1];
}

void HandleFloorTypeUpdate(Player* player, s32* floorProp, bool* should) {

        float holes[4][2][2] = GRAVE_HOLES;
        if (InBounds(player->actor.world.pos, holes[0]) ||
            InBounds(player->actor.world.pos, holes[1]) ||
            InBounds(player->actor.world.pos, holes[2]) ||
            InBounds(player->actor.world.pos, holes[3])) {
            *should = false;
            *floorProp = CVAR_GRAVE_HOLE_VALUE ? 0 : 9;
        }
}


void RegisterGraveHoleJumps() {
    COND_VB_SHOULD(VB_SET_STATIC_PREV_FLOOR_TYPE, NORMALIZE_GRAVE_JUMPS, {
        if (gPlayState->sceneNum != SCENE_GRAVEYARD) {
            *should = true;
            return;
        }
        Player* player = va_arg(args, Player*);
        s32* floorProp = va_arg(args, s32*);
        HandleFloorTypeUpdate(player, floorProp, should);
    });
}

static RegisterShipInitFunc registerGraveHoleJumps(RegisterGraveHoleJumps);
