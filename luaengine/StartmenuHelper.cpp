#include "precomp.h"


#include "LuaEngine.h"

EXTERN_C {
    int lua_startmenu_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        char strPath[MAX_PATH + 1] = { 0 };

        std::string func = funcname;

        if (func == "startmenu::getprogramspath") {
            SHGetSpecialFolderPathA(0, strPath, CSIDL_PROGRAMS, FALSE);
            lua_pushstring(L, strPath);
            ret++;
        } else if (func == "startmenu::pin") {
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("startpin"));
        } else if (func == "startmenu::unpin") {
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("startunpin"));
        }
        return ret;
    }
}
