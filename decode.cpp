#include <fstream>
#include <cassert>
#include "decode.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <memory.h>
}

#ifdef SILENT
#define printf(fmt, ...) (0)
#endif

void decode_key(const char* buf) {
    printf("[key] ");
    char type = buf[0];

    if (type == NUM) {
        decode_num(buf);
    } else if (type == STR) {
        decode_str(buf);
    } else {
        printf("Unknown type %d\n", type);
        assert(false);
    }
}

void decode_value(const char* buf) {
    printf("[value] ");
    char type = buf[0];

    if (type == TABLE) {
        decode_table(buf);
    } else if (type == NUM) {
        decode_num(buf);
    } else if (type == STR) {
        decode_str(buf);
    } else {
        printf("Unknown type %d\n", type);
        assert(false);
    }
}

void decode_num(const char* buf) {
    lua_Number num;
    read_value(&num, buf + 1);

    printf("decode %f\n", num);
}

void decode_str(const char* buf) {
    uint16_t size;
    // memcpy((void*) &size, (void*)(buf + 1), sizeof(uint16_t));
    const void* ptr = read_value(&size, buf + 1);

    /* char* str = new char[size](); */
    /* read_value(str, ptr, size); */

    printf("decode %s\n", ptr);

    /* delete[] str; */
}

void decode_table(const char* buf) {
    printf("decode table ");
    const void* ptr = buf + 1;

    uint32_t key_size, arr_size;
    ptr = read_value(&key_size, ptr);
    ptr = read_value(&arr_size, ptr);

    printf("key_size = %u, arr_size = %u\n", key_size, arr_size);

    uint32_t* key_addr = (uint32_t*) ptr;
    uint32_t* val_addr = (uint32_t*) addr_offset(key_addr, sizeof(uint32_t) * key_size);

    for (unsigned int i = 0; i < key_size; i++) {
        uint32_t key_offset = key_addr[i];
        uint32_t val_offset = val_addr[i];

        const char* cur_key = (const char*) addr_offset(buf, key_offset);
        const char* cur_val = (const char*) addr_offset(buf, val_offset);

        decode_key(cur_key);
        decode_value(cur_val);
    }
}

void decode_file(const char* path) {
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    std::streamsize size = inf.tellg();
    inf.seekg(0, std::ios::beg);

    printf("all size = %ld\n", size);

    char* buf = new char[size]();
    inf.read(buf, size);
    inf.close();

    decode_value(buf);

    delete[] buf;
}

const char* create_from_file(const char* path) {
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    std::streamsize size = inf.tellg();
    inf.seekg(0, std::ios::beg);

    printf("all size = %ld\n", size);

    char* buf = (char*) malloc(size);
    inf.read(buf, size);
    inf.close();

#ifdef DEBUG
    decode_value(buf);
#endif

    return buf;
}

void create_from_file_as_userdata(const char* path, lua_State* L) {
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    std::streamsize size = inf.tellg();
    inf.seekg(0, std::ios::beg);

    printf("all size = %ld\n", size);

    char* buf = (char*) lua_newuserdata(L, size);

    printf("create lt: %p\n", buf);

    inf.read(buf, size);
    inf.close();

#ifdef DEBUG
    decode_value(buf);
#endif
}

#ifdef RUN
int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("require path\n");
        return -1;
    }

    decode_file(argv[1]);

    return 0;
}
#endif
