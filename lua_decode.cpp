#include "decode.h"
#include <cstdint>
#include <cassert>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#define PLT_NAME "plt"

void set_plt_metatable(lua_State* L);

int compare_num(const char* buf, double num) {
    char t = buf[0];
    if (t == NUM) {
        double buf_value;
        read_value(&buf_value, buf + 1);

        if (buf_value == num) {
            return 0;
        } else if (buf_value > num) {
            return 1;
        }
    }

    return -1;
}

int compare_str(const char* buf, const char* str) {
    char t = buf[0];
    if (t == NUM) {
        return -1;
    } else if (t == STR) {
        uint16_t size;
        const char* buf_value = (const char*) read_value(&size, buf + 1);
        int ret = strncmp(str, buf_value, size);
        return ret;
    }

    printf("Error key type %d\n", t);
    assert(false);
}

bool avl_search_num(const char* plt, uint32_t* key_addr, uint32_t arr_size, uint32_t key_size, double key, uint32_t* index) {
    if (key_size < arr_size) {
        return false;
    }
    
    uint32_t cur = 1;
    while(arr_size + cur <= key_size) {
        *index = arr_size + cur - 1;
        uint32_t key_offset = key_addr[*index];
        const char* key_buf = (const char*) addr_offset(plt, key_offset);
        int ret = compare_num(key_buf, key);
        if (ret == 0) {
            return true;
        } else if (ret > 0) {
            cur = cur * 2;
        } else {
            cur = cur * 2 + 1;
        }
    }
    return false;
}

bool avl_search_str(const char* plt, uint32_t* key_addr, uint32_t arr_size, uint32_t key_size, const char* key, uint32_t* index) {
    if (key_size < arr_size) {
        return false;
    }
    
    uint32_t cur = 1;
    while(arr_size + cur <= key_size) {
        uint32_t key_offset = key_addr[arr_size + cur - 1];
        const char* key_buf = (const char*) addr_offset(plt, key_offset);
        int ret = compare_str(key_buf, key);
        if (ret == 0) {
            *index = arr_size + cur - 1;
            return true;
        } else if (ret > 0) {
            cur = cur * 2 + 1;
        } else {
            cur = cur * 2;
        }
    }
    return false;
}

void read_table_to_stack(const char* buf, lua_State* L) {
    lua_pushlightuserdata(L, (void*) buf);
    set_plt_metatable(L);
}

void read_num_to_stack(const char* buf, lua_State* L) {
    double num;
    read_value(&num, buf + 1);
    lua_pushnumber(L, num);
}

void read_str_to_stack(const char* buf, lua_State* L) {
    uint16_t size;
    const char* str = (const char*) read_value(&size, buf + 1);
    lua_pushstring(L, str);
}

void read_value_to_stack(const char* buf, lua_State* L) {
    char type = buf[0];

    if (type == TABLE) {
        read_table_to_stack(buf, L);
    } else if (type == NUM) {
        read_num_to_stack(buf, L);
    } else if (type == STR) {
        read_str_to_stack(buf, L);
    } else {
        printf("Unknown type %d\n", type);
        assert(false);
    }
}

int search_num(char* plt, double key, lua_State* L) {
    const void* ptr = plt + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    if (key_size == 0) {
        return 0;
    }

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    uint32_t key_int = (uint32_t) key;
    if (key_int == key) {
        key_int -= 1;
        if (key_int >= 0 && key_int < arr_size) {
            uint32_t val_offset = val_addr[key_int];
            const char* val_buf = (const char*) addr_offset(plt, val_offset);
            read_value_to_stack(val_buf, L);
            return 1;
        }
    }

    uint32_t index;
    bool ok = avl_search_num(plt, key_addr, arr_size, key_size, key, &index);
    if (!ok) {
        return 0;
    }

    uint32_t val_offset = val_addr[index];
    const char* val_buf = (const char*) addr_offset(plt, val_offset);
    read_value_to_stack(val_buf, L);
    return 1;
}

int search_str(char* plt, const char* key, lua_State* L) {
    const void* ptr = plt + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    if (key_size == 0) {
        return 0;
    }

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    uint32_t index;
    bool ok = avl_search_str(plt, key_addr, arr_size, key_size, key, &index);
    if (!ok) {
        return 0;
    }

    uint32_t val_offset = val_addr[index];
    const char* val_buf = (const char*) addr_offset(plt, val_offset);
    read_value_to_stack(val_buf, L);
    return 1;
}

int plt_index(lua_State* L) {
    char* plt = (char*) lua_touserdata(L, 1);

    int lt = lua_type(L, 2);

    if (lt == LUA_TNUMBER) {
        lua_Number num = lua_tonumber(L, 2);
        return search_num(plt, num, L);
    } else if (lt == LUA_TSTRING) {
        const char* str = lua_tostring(L, 2);
        return search_str(plt, str, L);
    } else {
        return 0;
    }
}

int next_num(char* plt, lua_Number key, lua_State* L) {
    const void* ptr = plt + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    if (key_size == 0) {
        return 0;
    }

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    uint32_t key_int = (uint32_t) key;
    if (key_int == key) {
        bool found = false;
        uint32_t index = 0;

        if (key_int >= 0 && key_int < arr_size) {
            found = true;
            index = key_int + 1;
            lua_pushinteger(L, index);
        } else if (key_int == arr_size && key_int < key_size) {
            found = true;
            index = arr_size;
            uint32_t key_offset = key_addr[index];
            const char* key_buf = (const char*) addr_offset(plt, key_offset);
            read_value_to_stack(key_buf, L);
        }

        if (found) {
            uint32_t val_offset = val_addr[key_int];
            const char* val_buf = (const char*) addr_offset(plt, val_offset);
            read_value_to_stack(val_buf, L);
            return 2;
        }
    }

    uint32_t index;
    bool ok = avl_search_num(plt, key_addr, arr_size, key_size, key, &index);

    if (!ok) {
        return 0;
    }

    if (++index < key_size) {
        uint32_t key_offset = key_addr[index];
        const char* key_buf = (const char*) addr_offset(plt, key_offset);
        read_value_to_stack(key_buf, L);

        uint32_t val_offset = val_addr[index];
        const char* val_buf = (const char*) addr_offset(plt, val_offset);
        read_value_to_stack(val_buf, L);

        return 2;
    }

    return 0;
}

int next_str(char* plt, const char* key, lua_State* L) {
    const void* ptr = plt + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    if (key_size == 0) {
        return 0;
    }

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    uint32_t index;
    bool ok = avl_search_str(plt, key_addr, arr_size, key_size, key, &index);
    if (!ok) {
        return 0;
    }

    if (++index < key_size) {
        uint32_t key_offset = key_addr[index];
        const char* key_buf = (const char*) addr_offset(plt, key_offset);
        read_value_to_stack(key_buf, L);

        uint32_t val_offset = val_addr[index];
        const char* val_buf = (const char*) addr_offset(plt, val_offset);
        read_value_to_stack(val_buf, L);

        return 2;
    }

    return 0;
}

int plt_next(lua_State* L) {
    /* char* plt = (char*) luaL_checkudata(L, 1, PLT_NAME); */
    char* plt = (char*) lua_touserdata(L, 1);

    int lt = lua_type(L, 2);

    if (lt == LUA_TNIL) {
        return next_num(plt, 0, L);
    } else if (lt == LUA_TNUMBER) {
        lua_Number num = lua_tonumber(L, 2);
        return next_num(plt, num, L);
    } else if (lt == LUA_TSTRING) {
        const char* str = lua_tostring(L, 2);
        return next_str(plt, str, L);
    }

    return 0;
}

int iter_num(char* plt, lua_Number key, lua_State* L) {
    const void* ptr = plt + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    if (arr_size == 0) {
        return 0;
    }

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    uint32_t key_int = (uint32_t) key;

    if (key_int == key && key_int >= 0 and key_int < arr_size) {
        lua_pushinteger(L, key_int + 1);

        uint32_t val_offset = val_addr[key_int];
        const char* val_buf = (const char*) addr_offset(plt, val_offset);
        read_value_to_stack(val_buf, L);
        return 2;
    }

    return 0;
}


int plt_iter(lua_State* L) {
    char* plt = (char*) lua_touserdata(L, 1);

    int lt = lua_type(L, 2);

    if (lt == LUA_TNIL) {
        return iter_num(plt, 0, L);
    }
    else if (lt == LUA_TNUMBER) {
        lua_Number num = lua_tonumber(L, 2);
        return iter_num(plt, num, L);
    }

    return 0;
}

int plt_pairs(lua_State* L) {
    lua_pushcfunction(L, plt_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);

    return 3;
}

void set_plt_metatable(lua_State* L) {
    int ret = luaL_newmetatable(L, PLT_NAME);
    if (ret) {
        luaL_Reg l[] = {
            { "__index", plt_index },
            { "__pairs", plt_pairs },
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

static int _ipairs(lua_State* L) {
    lua_pushcfunction(L, plt_iter);
    lua_pushvalue(L, 1);
    lua_pushnil(L);

    return 3;
}

static int _pairs(lua_State* L) {
    return 0;
}

extern "C"
int luaopen_plt_c(lua_State* L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        { "create_plt", _create_plt },
        { "ipairs", _ipairs },
        { "pairs", _pairs },
        { NULL, NULL },
    };

    luaL_newlib(L, l);
    return 1;
}
