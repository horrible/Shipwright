#pragma once

#include <string>
#include <vector>

#include <ship/window/gui/GuiWindow.h>

#ifdef __cplusplus
extern std::vector<std::string> clientMods;

class ModMenuWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;


    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override{};
};
#endif