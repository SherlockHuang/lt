#include <fstream>
#include <cassert>
#include "decode.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <memory.h>
}

enum key_type {
    NUM,
    STR,
    TABLE,
};

template<typename T>
const void* read_value(T* dst, const void* src) {
    size_t size = sizeof(T);
    memcpy(dst, src, size);
    return (char*)src + size;
}

template<typename T>
const void* read_value(T* dst, const void* src, size_t size) {
    size = sizeof(T) * size;
    memcpy(dst, src, size);
    return (char*)src + size;
}

inline
const void* addr_offset(const void* root, size_t offset) {
    return (char*)root + offset;
}

template<typename T>
void stream_read(const char*& buf, T& dst) {
    T* ptr = (T*) buf;
    dst = *ptr;

    for (int i = 0; i < sizeof(T); i++) {
        char c = buf[i];
        printf("%02x ", c);
    }
    printf("\n");

    buf += sizeof(T);
}

inline
void stream_move(const char*& buf, size_t offset) {
    buf += offset;
}

void decode_key(unsigned int* key_addr, const char* key_value_addr, unsigned int offset) {
    unsigned int key_offset = *(key_addr + offset);
    printf("key offset = %u\n", key_offset);

    uint16_t* size_ptr = (uint16_t*) (key_value_addr + *(key_addr + offset));
    key_type k = (key_type) (((*size_ptr) & (0x8000)) >> 15);
    unsigned int key_size = (*size_ptr) & (0x7fff);

    if (k == NUM) {
        double* data_ptr = (double*) (size_ptr + 1);
        printf("num key: %f, size:%u\n", *data_ptr, key_size);
    } else {
        const char* data_ptr = (char*) (size_ptr + 1);
        printf("char key: %s, size:%u\n", data_ptr, key_size);
    }
}

void decode_keys(const char* buf) {
    const char* ptr = buf;

    unsigned int key_size;
    stream_read(ptr, key_size);
    printf("key size = %d\n", key_size);

    unsigned int arr_size;
    stream_read(ptr, arr_size);
    printf("arr size = %d\n", arr_size);

    unsigned int* key_addr = (unsigned int*) ptr;
    stream_move(ptr, key_size * sizeof(unsigned int));
    // stream_move(ptr, key_size* sizeof(unsigned int));
    
    for (int i = 0; i < key_size; i++) {
        decode_key(key_addr, ptr, i);
    }
}

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

    /* decode_keys(buf); */
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

    decode_value(buf);

    return buf;
}

void create_from_file_as_userdata(const char* path, lua_State* L) {
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    std::streamsize size = inf.tellg();
    inf.seekg(0, std::ios::beg);

    printf("all size = %ld\n", size);

    char* buf = (char*) lua_newuserdata(L, size);

    printf("create  size: %zu\n", size);

    inf.read(buf, size);
    inf.close();

    decode_value(buf);
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
