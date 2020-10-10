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
    int call(const char *funcname, int nres = 0);
    int call(const char *funcname, int p1, int p2, int nres = 0);
    int call(const char * funcname, string_t & p1, string_t & p2, int nres = 0);
    void RunCode(string_t & code);
    void LoadFile(string_t & file);
    void onLoad();
    void onFirstRun();
    void preShell();
    void onShell();
    void onDaemon();
    int onClick(string_t& ctrl);
    void onTimer(int id);
private:
    void init(string_t& file);
    void *_frame;
    lua_State *L;
};

