#include "precomp.h"

#include "LuaEngine.h"
#include "../systemsettings/Volume.h"

EXTERN_C {
    int lua_volume_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "volume::mute") {
            v.iVal = SetVolumeMute((int)lua_tointeger(L, base + 2));
            PUSH_INT(v);
        } else if (func == "volume::ismuted") {
            v.iVal = GetVolumeMute();
            PUSH_INT(v);
        } else if (func == "volume::getlevel") {
            v.iVal = GetVolumeLevel();
            PUSH_INT(v);
        } else if (func == "volume::setlevel") {
            v.iVal = SetVolumeLevel((int)lua_tointeger(L, base + 2));
            PUSH_INT(v);
        } else if (func == "volume::getname") {
            GetEndpointVolume();
            v.str = GetVolumeDeviceName(NULL);
            PUSH_STR(v);
        }
        return ret;
    }

}