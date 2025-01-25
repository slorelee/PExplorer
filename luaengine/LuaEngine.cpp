#include "precomp.h"

#include "LuaEngine.h"

#ifdef _DEBUG
#   ifdef _WIN64
#       pragma comment(lib, "lua/lib/lua_d_x64.lib")
#       pragma comment(lib, "lua/lib/lua-cjson_d_x64.lib")
#   else
#       pragma comment(lib, "lua/lib/lua_d.lib")
#       pragma comment(lib, "lua/lib/lua-cjson_d.lib")
#   endif
#else
#   ifdef _WIN64
#       pragma comment(lib, "lua/lib/lua_x64.lib")
#       pragma comment(lib, "lua/lib/lua-cjson_x64.lib")
#   else
#       pragma comment(lib, "lua/lib/lua.lib")
#       pragma comment(lib, "lua/lib/lua-cjson.lib")
#   endif
#endif

