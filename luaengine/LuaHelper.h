#pragma once

#include<Windows.h>
#include "lua.hpp"

#ifndef string_t
#ifdef UNICODE
#define string_t std::wstring
#else
#define string_t string
#endif
#endif


class CLuaHelper {
public:
    CLuaHelper();
    ~CLuaHelper() {};

    void Init(lua_State *l, char *owner);
    int GetInt(const char *var);
    int GetInt(const char *table, const char *field1, const char *field2 = NULL);
    const char *GetString(const char *var);
    const char *GetString(const char *table, const char *field1, const char *field2 = NULL);

    int GetTable(const char *var);
    int GetTable(const char *table, const char *field);

#ifdef _DEBUG
    int HasFunc(const char *funcname);
    int HasFunc(const char *tablename, const char *funcname);
#endif // _DEBUG

    int GetFunc(const char *funcname);
    int Call(const char *funcname, int nres = 0);
    int Call(const char *funcname, int p1, int p2, int nres = 0);
    int Call(const char * funcname, string_t & p1, string_t & p2, int nres = 0);

    lua_State *L;
    char *_ownername;
    int _errorfunc;
};


