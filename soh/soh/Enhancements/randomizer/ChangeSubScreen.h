#pragma once
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <z64.h>

std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*, GraphicsContext *gfxCtx)) ;
std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*, u16 pageIndex), u16 pageIndex);
std::function<void()> AssumeGlobal(void (*wrappedFunc)(PlayState*));

//i think this is everything a custom screen will need? was going to drop vanilla index but then i realized MM screens will likely need it too
struct SubScreenOpts {
    std::string name;
    std::function<void()> drawPageFunc; // vanilla draws use 1-2 params that ship has in globals, so those are wrapped below & output as noarg funcs
    void* nameTex; // tex for name that other subscreens should display when adjacent and the appropriate Z/L/R button is under the cursor
    std::optional<Vtx*> bgVtx;
    std::optional<void**> contentTex;
    bool drawCursor = false;
    u16 vanillaIndex = (u16) -1;
    u16 textureFilter = false;
    bool runPostDraw = false;
    std::function<void()> postDrawFunc;
};

u16* GetDefaultIndex();
std::vector<std::add_pointer_t<SubScreenOpts()>>* GetSubScreens();
bool SwapSubScreen(int pos, std::add_pointer_t<SubScreenOpts()> screen);
bool AddSubScreen(std::add_pointer_t<SubScreenOpts()> screenFunc);
bool RemoveSubScreen(std::add_pointer_t<SubScreenOpts()> screenFunc);
