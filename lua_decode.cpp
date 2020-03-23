#include "decode.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
#include <lauxlib.h>
#ifdef __cplusplus
}
#endif

#define PLT_NAME "PLT"

int plt_index(lua_State* L) {
    void* plt = luaL_checkudata(L, 1, PLT_NAME);
    if (lua_isnumber(L, 2)) {
        lua_Number num = lua_tonumber(L, 2);
        printf("plt: %p index = %f\n", plt, num);
    } else if(lua_isstring(L, 2)) {
        const char* str = lua_tostring(L, 2);
        printf("plt: %p index = %s\n", plt, str);
    }
    return 0;
}

void set_plt_metatable(lua_State* L) {
    int ret = luaL_newmetatable(L, PLT_NAME);
    if (ret) {
        luaL_Reg l[] = {
            { "__index", plt_index },
            { NULL, NULL },
        };
        luaL_setfuncs(L, l, 0);
    }
    lua_pop(L, 1);
    luaL_setmetatable(L, PLT_NAME);
}

static int _create_plt(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    if (!path) {
        return 0;
    }

    create_from_file_as_userdata(path, L);
    set_plt_metatable(L);

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
