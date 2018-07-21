#pragma once

#include <stdio.h>
#include "lua.hpp"
#include "../DUI/Helper.h"

class LuaAppEngine {

public:
    LuaAppEngine(string_t& file);
    ~LuaAppEngine();
    void *getFrame() { return _frame; };
    int hasfunc(const char *funcname);
    void call(const char *funcname);
    void call(const char * funcname, string_t & p1, string_t & p2);
    void onLoad();
    void onFirstRun();
    void onShell();
    int onClick(string_t& ctrl);
    void onTimer(int id);
private:
    void init(string_t& file);
    void *_frame;
    lua_State *L;
};

