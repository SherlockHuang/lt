#include "decode.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
#include <lauxlib.h>
#ifdef __cplusplus
}
#endif

static int _create_plt(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    if (!path) {
        return 0;
    }

    create_from_file_as_userdata(path, L);
    return 1;
}

#ifdef __cplusplus
extern "C"
#endif
int luaopen_plt_c(lua_State* L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "create_plt", _create_plt },
        { NULL, NULL },
    };

    luaL_newlib(L, l);
    return 1;
}
