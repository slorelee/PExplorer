#include "precomp.h"

#include "LuaEngine.h"

#ifdef _DEBUG
#   ifdef _WIN64
#       pragma comment(lib, "lua/lua53_d_x64.lib")
#       pragma comment(lib, "lua/lua-cjson_d_x64.lib")
#   else
#       pragma comment(lib, "lua/lua53_d.lib")
#       pragma comment(lib, "lua/lua-cjson_d.lib")
#   endif
#else
#   ifdef _WIN64
#       pragma comment(lib, "lua/lua53_x64.lib")
#       pragma comment(lib, "lua/lua-cjson_x64.lib")
#   else
#       pragma comment(lib, "lua/lua53.lib")
#       pragma comment(lib, "lua/lua-cjson.lib")
#   endif
#endif

