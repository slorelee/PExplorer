#include "precomp.h"
#include "../utility/ProductPolicyEditor.h"

#include "LuaEngine.h"

EXTERN_C {
    int lua_productpolicy_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "productpolicy::load") {
            ProductPolicyLoad(lua_tostring(L, base + 2), lua_tostring(L, base + 3));
        } else if (func == "productpolicy::get") {
            v.str = s2w(lua_tostring(L, base + 2));
            ProductPolicyGet(v.str.c_str());
        } else if (func == "productpolicy::set") {
            v.str = s2w(lua_tostring(L, base + 2));
            v.iVal = lua_tointeger(L, base + 3);
            ProductPolicySet(v.str.c_str(), v.iVal);
        } else if (func == "productpolicy::save") {
            ProductPolicySave();
        }
        return ret;
    }
}
