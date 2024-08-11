#pragma once

#include "LuaEngine.h"

class LuaAppEngine {

public:
    LuaAppEngine(string_t& file);
    ~LuaAppEngine();
    void *getFrame() { return _frame; };

    int call(const char *funcname, int nres = 0);
    int call(const char *funcname, int p1, int p2, int nres = 0);
    int call(const char * funcname, string_t & p1, string_t & p2, int nres = 0);
    int InitOutputStream();
    void RunCode(string_t & code);
    void LoadFile(string_t & file);
    void onLoad();
    void onFirstShellRun();
    void preShell();
    void onShell();
    void onDaemon();
    int onClick(string_t& ctrl);
    int onTimer(int id);

    lua_State *L;

    CLuaHelper LuaApp;
private:
    void init(string_t& file);
    void *_frame;
    char *_name;
};

