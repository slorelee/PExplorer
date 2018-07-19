#pragma once

#include <stdio.h>
#include "lua.hpp"
#include "../DUI/Helper.h"

class LuaAppEngine {

public:
    LuaAppEngine(string_t& file);
    ~LuaAppEngine();
    void *getFrame() { return _frame; };
    void onLoad();
    void onFirstRun();
    void onShell();
    void onClick(string_t& ctrl);
    void onTimer(int id);
private:
    void init(string_t& file);
    void callFunc(const char *funcname);
    void *_frame;
    lua_State *L;
};

