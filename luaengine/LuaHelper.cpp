
#include <precomp.h>
#include "LuaHelper.h"


CLuaHelper::CLuaHelper(lua_State *l)
{
    L = l;
}

int lua_var(lua_State *L, const char *var)
{
    lua_getglobal(L, var);
    return 0;
}

int lua_var(lua_State *L, const char *table, const char *field1, const char *field2)
{
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) {
        return -1;
    }

    lua_getfield(L, -1, field1);
    if (field2 == NULL) return 0;

    if (!lua_istable(L, -1)) {
        return -1;
    }

    lua_getfield(L, -1, field2);
    return 0;
}

int lua_getinteger(lua_State *L, const char *var)
{
    if (lua_var(L, var) < 0) return INT_MIN;
    if (lua_type(L, -1) != LUA_TNUMBER) {
        return INT_MIN;
    }
    return (int)lua_tointeger(L, -1);
}


int lua_getinteger(lua_State *L, const char *table, const char *field1, const char *field2)
{
    if (table) {
        if (lua_var(L, table, field1, field2) < 0) return INT_MIN;
    } else {
        lua_getfield(L, -1, field1);
    }
    if (lua_type(L, -1) != LUA_TNUMBER) {
        return INT_MIN;
    }
    return (int)lua_tointeger(L, -1);
}

const char *lua_getstring(lua_State *L, const char *var)
{
    if (lua_var(L, var) < 0) return NULL;
    if (lua_type(L, -1) != LUA_TSTRING) {
        return NULL;
    }
    return lua_tostring(L, -1);
    return NULL;
}

const char *lua_getstring(lua_State *L, const char *table, const char *field1, const char *field2)
{
    if (table) {
        if (lua_var(L, table, field1, field2) < 0) return NULL;
    } else {
        lua_getfield(L, -1, field1);
    }
    if (lua_type(L, -1) != LUA_TSTRING) {
        return NULL;
    }
    return lua_tostring(L, -1);
}

int lua_gettable(lua_State *L, const char *var)
{
    if (lua_var(L, var) < 0) return 0;
    if (lua_istable(L, -1)) {
        return 1;
    }
    return 0;
}

int lua_gettable(lua_State *L, const char *table, const char *field)
{
    if (lua_var(L, table, field, NULL) < 0) return 0;
    if (lua_istable(L, -1)) {
        return 1;
    }
    return 0;
}

int CLuaHelper::GetInt(const char *var)
{
    return lua_getinteger(L, var);
}

int CLuaHelper::GetInt(const char *table, const char *field1, const char *field2)
{
    return lua_getinteger(L, table, field1, field2);
}

const char *CLuaHelper::GetString(const char *var)
{
    return lua_getstring(L, var);
}

const char *CLuaHelper::GetString(const char *table, const char *field1, const char *field2)
{
    return lua_getstring(L, table, field1, field2);
}

int CLuaHelper::GetTable(const char *var)
{
    return lua_gettable(L, var);
}

int CLuaHelper::GetTable(const char *table, const char *field)
{
    return lua_gettable(L, table, field);
}
