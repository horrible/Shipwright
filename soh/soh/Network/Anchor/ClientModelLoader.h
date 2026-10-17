#pragma once

#ifdef __cplusplus
extern std::vector<std::tuple<std::string, std::string, std::string>> clientMods;

#include <filesystem>
#include "soh/Network/Network.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/window/gui/GuiWindow.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <list>

void PopulateClientModelList(std::filesystem::directory_entry p);

#endif // __cplusplus
