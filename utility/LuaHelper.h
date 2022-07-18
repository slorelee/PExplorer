#pragma once

#include<Windows.h>
#include "lua.hpp"

class CLuaHelper {
public:
	CLuaHelper() { L = NULL; };
	CLuaHelper(lua_State *l);
    ~CLuaHelper() {};
	int GetInt(const char *var);
	int GetInt(const char *table, const char *field1, const char *field2 = NULL);
	const char *GetString(const char *var);
	const char *GetString(const char *table, const char *field1, const char *field2 = NULL);

	int GetTable(const char *var);
	int GetTable(const char *table, const char *field);

	lua_State *L;
};


