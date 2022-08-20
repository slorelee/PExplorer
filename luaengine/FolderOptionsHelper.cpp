#include "precomp.h"

#include "LuaEngine.h"
#include "../utility/FolderOptions.h"

EXTERN_C{
    int lua_folderoptions_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "folderoptions::get") {
            v.str = s2w(lua_tostring(L, base + 2));
            v.iVal = FolderOptions->Get(v.str.c_str());
            PUSH_INT(v);
        } else if (func == "folderoptions::set") {
            if (lua_isnumber(L, base + 2)) {
                FolderOptions->Set((DWORD)lua_tointeger(L, base + 2), lua_tointeger(L, base + 3));
            } else {
                v.str = s2w(lua_tostring(L, base + 2));
                FolderOptions->Set(v.str.c_str(), (DWORD)lua_tointeger(L, base + 3));
            }
        }
        return ret;
    }
}
