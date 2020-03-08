#include <cstdio>
#include <iostream>
#include <vector>
#include <exception>
#include <cstring>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

enum key_type {
    INT,
    STR,
};

struct tkey {
    key_type type;

    union {
        lua_Number num;
        const char* str;
    } value;
};

bool sort_tkey(tkey& k1, tkey& k2) {
    if (k1.type != k2.type) {
        return k1.type < k2.type;
    }

    if (k1.type == INT) {
        return k1.value.num < k2.value.num;
    }

    if (k1.type == STR) {
    }
}

void pack(lua_State* L, int t) {
    std::vector<tkey> key_vec;

    lua_pushnil(L);
    while (lua_next(L, t - 1) != 0)
    {
        tkey& key = key_vec.emplace_back();
        if (lua_isnumber(L, -2)) {
            key.type = INT;
            key.value.num = lua_tonumber(L, -2);
        } else if (lua_isstring(L, -2)) {
            size_t len;
            const char* str = luaL_tolstring(L, -2, &len);
            char* new_str = new char[len + 1]();
            strncpy(new_str, str, len);

            key.type = STR;
            key.value.str = new_str;

            lua_pop(L, 1);
        } else {
            char msg[64];
            const char* fmt = "unsupport key type %s";
            snprintf(msg, 64, "unsupport key type %s", lua_typename(L, lua_type(L, -2)));
            throw msg;
        }

        if (key.type == INT) {
            std::cout << key.value.num << std::endl;
        } 
        else if (key.type == STR) {
            std::cout << key.value.str << std::endl;
        }

        lua_pop(L, 1);
    }
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        std::cout << "require path" << std::endl;
        return -1;
    }

    lua_State* L = luaL_newstate();
    const char* path = argv[1];

    int ret = luaL_dofile(L, path);
    if (ret != LUA_OK) {
        std::cout << "load file failed" << std::endl;
        return -1;
    }

    if (!lua_istable(L, -1))
    {
        std::cout << "load file result is not a table" << std::endl;
        return -1;
    }

    std::cout << "load success" << std::endl;

    pack(L, -1);
    
    return 0;
}