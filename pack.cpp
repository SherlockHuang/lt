#include <cstdio>
#include <iostream>
#include <vector>
#include <exception>
#include <cstring>
#include <algorithm>
#include <string>
#include <memory>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

enum key_type {
    NUM,
    STR,
};

struct tkey {
    tkey() : type(NUM), num(0) {}
    tkey(int num) : type(NUM), num(num) {}
    tkey(const char* str) : type(STR) {
        size_t len = strlen(str);
        this->str = new char[len + 1]();
        strcpy(this->str, str);
    }
    tkey(const char*str, int len) : type(STR) {
        this->str = new char[len + 1]();
        this->str[len] = '\0';
        strncpy(this->str, str, len);
    }
    tkey(tkey& v) : type(v.type) {
        if (v.type == NUM) {
            num = v.num;
        } else {
            size_t len = strlen(v.str);
            str = new char[len + 1]();
            strcpy(str, v.str);
        }
    }
    tkey(tkey&& v) : tkey(v) {}
    tkey& operator= (const tkey& v) {
        type = v.type;
        if (type == NUM) {
            num = v.num;
        } else {
            size_t len = strlen(v.str);
            str = new char[len + 1]();
            strcpy(str, v.str);
        }
    }
    ~tkey() {
        if (type == STR) {
            free(str);
        }
    }

    key_type type;
    union {
        lua_Number num;
        char* str;
    };
};

bool sort_tkey(tkey& k1, tkey& k2) {
    if (k1.type != k2.type) {
        return k1.type < k2.type;
    }

    if (k1.type == NUM) {
        return k1.num < k2.num;
    }

    if (k1.type == STR) {
        return strcmp(k1.str, k2.str) < 0;
    }

    return false;
}

void pack(lua_State* L, int t) {
    std::vector<tkey> key_vec;

    lua_pushnil(L);
    while (lua_next(L, t - 1) != 0)
    {
        if (lua_isnumber(L, -2)) {
            lua_Number num = lua_tonumber(L, -2);
            key_vec.emplace_back(num);
        } else if (lua_isstring(L, -2)) {
            size_t len;
            const char* str = luaL_tolstring(L, -2, &len);
            key_vec.emplace_back(str, len);

            lua_pop(L, 1);
        } else {
            char msg[64];
            const char* fmt = "unsupport key type %s";
            snprintf(msg, 64, "unsupport key type %s", lua_typename(L, lua_type(L, -2)));
            std::cout << msg << std::endl;
            throw msg;
        }

        lua_pop(L, 1);
    }

    std::sort(key_vec.begin(), key_vec.end(), sort_tkey);

    for (auto& key : key_vec) {
        if (key.type == NUM) {
            std::cout << key.num << std::endl;
        } else {
            std::cout << key.str << std::endl;
        }
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