
#include <precomp.h>
#include "LuaHelper.h"

static int lua_errorfunc = MININT;

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

static int fetch_errorfunc(lua_State *L)
{
    if (lua_errorfunc != MININT) return lua_errorfunc;

    int top = lua_gettop(L);
    lua_getglobal(L, "__G__TRACKBACK__");

    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find function <__G__TRACKBACK__>err");
        lua_settop(L, top);
        return MININT;
    }

    int errfunc = lua_gettop(L);
    lua_errorfunc = errfunc;
    return errfunc;
}

CLuaHelper::CLuaHelper()
{
    L = NULL;
    _ownername = "LUAHELPER_OWNER";
    _errorfunc = MININT;
}

void CLuaHelper::Init(lua_State *l, char *owner)
{
    L = l;
    if (owner) _ownername = owner;
    _errorfunc = fetch_errorfunc(L);
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


#ifdef _DEBUG
int CLuaHelper::HasFunc(const char *funcname)
{
    int luatype = 0;
    char buff[1024] = { 0 };

    if (g_Globals._isDebug) {
        sprintf(buff, "[DEBUG] getfunc %s() function", funcname);
        LOGA(buff);
    }
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);

    lua_getglobal(L, funcname);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TFUNCTION) {
        LOGA(buff);
        return 0;
    }
    return 1;
}

int CLuaHelper::HasFunc(const char *tablename, const char *funcname)
{
    int luatype = 0;
    char buff[1024] = { 0 };

    if (g_Globals._isDebug) {
        sprintf(buff, "[DEBUG] getfunc %s() function", funcname);
        LOGA(buff);
    }
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);

    lua_getglobal(L, tablename);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TTABLE) {
        LOGA(buff);
        return 0;
    }
    lua_getfield(L, -1, funcname);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TFUNCTION) {
        LOGA(buff);
        return 0;
    }
    return 1;
}
#endif // _DEBUG

int CLuaHelper::GetFunc(const char *funcname)
{
    char func_buff[64] = { 0 };
    char msg_buff[1024] = { 0 };
    char *table = _ownername;
    char *method_colon = NULL;
    char *table_method = NULL;

    if (!funcname) return -1;
    if (lua_errorfunc == MININT) return -1;

    if (g_Globals._isDebug) {
        sprintf(msg_buff, "[DEBUG] CLuaHelper::GetFunc %s() function", funcname);
        LOGA(msg_buff);
    }
    sprintf(msg_buff, "[LUA ERROR] can't find %s() function", funcname);

    strcpy(func_buff, funcname);
    method_colon = strchr(func_buff, ':');
    table_method = method_colon;
    if (table_method == NULL) {
        table_method = strchr(func_buff, '.');
    }
    if (table_method != NULL) {
        if (funcname[0] != ':') {
            *table_method = '\0';
            table = func_buff;
        }
        lua_getglobal(L, table);
        if (!lua_istable(L, -1)) {
            LOGA(msg_buff);
            return -1;
        }
        lua_getfield(L, -1, table_method + 1);
    } else {
        lua_getglobal(L, funcname);
    }
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(msg_buff);
        return -1;
    }
    if (method_colon != NULL) {
        // push self
        lua_getglobal(L, table);
        return 1;
    }

    return 0;
}

int CLuaHelper::Call(const char *funcname, int nres)
{
    int self = GetFunc(funcname);
    if (self == -1) return -1;

    int rel = lua_pcall(L, self, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}

int CLuaHelper::Call(const char *funcname, int p1, int p2, int nres)
{
    int self = GetFunc(funcname);
    if (self == -1) return -1;

    lua_pushinteger(L, p1);
    lua_pushinteger(L, p2);
    int rel = lua_pcall(L, 2 + self, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}

int CLuaHelper::Call(const char *funcname, string_t& p1, string_t& p2, int nres)
{
    int self = GetFunc(funcname);
    if (self == -1) return -1;

    lua_pushstring(L, w2s(p1).c_str());
    lua_pushstring(L, w2s(p2).c_str());
    int rel = lua_pcall(L, 2 + self, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}
