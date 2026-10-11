#pragma once

#ifdef __cplusplus

#include <filesystem>
#include "soh/Network/Network.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/window/gui/GuiWindow.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <list>

extern "C" {
#include "variables.h"
#include "z64.h"
}

void populateClientModelList(std::filesystem::directory_entry p);


#endif // __cplusplus
