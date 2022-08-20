#include "precomp.h"

#include "LuaEngine.h"
#include "../utility/Dialog.h"

EXTERN_C {
    int lua_dialog_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "dialog::openfile") {
            TCHAR titleBuff[MAX_PATH] = { 0 };
            TCHAR dirBuff[MAX_PATH] = { 0 };
            const TCHAR *title = NULL;
            const TCHAR *filters = NULL;
            const TCHAR *dir = NULL;
            if (lua_type(L, base + 2) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 2));
                lstrcpy(titleBuff, v.str.c_str());
                title = titleBuff;
            }
            if (lua_type(L, base + 4) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 4));
                varstr_expand(v.str);
                lstrcpy(dirBuff, v.str.c_str());
                dir = dirBuff;
            }
            if (lua_type(L, base + 3) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 3));
                filters = v.str.c_str();
            }
            v.iVal = Dialog->OpenFile(title, filters, dir);
            v.str = (TCHAR *)Dialog->SelectedFileName;
            PUSH_STR(v);
            PUSH_INT(v);
        } else if (func == "dialog::browsefolder") {
            const TCHAR *title = NULL;
            if (lua_type(L, base + 2) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 2));
                title = v.str.c_str();
            }
            if (lua_isinteger(L, base + 3)) {
                v.iVal = (int)lua_tointeger(L, base + 3);
            }
            v.iVal = Dialog->BrowseFolder(title, v.iVal);
            v.str = (TCHAR *)Dialog->SelectedFolderName;
            PUSH_STR(v);
            PUSH_INT(v);
        }
        return ret;
    }

}
