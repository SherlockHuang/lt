#ifndef __PLT_DECODE__
#define __PLT_DECODE__

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <memory.h>

#ifdef __cplusplus
}
#endif

enum key_type {
    NUM,
    STR,
    TABLE,
};

template<typename T>
inline
const void* read_value(T* dst, const void* src) {
    size_t size = sizeof(T);
    memcpy(dst, src, size);
    return (char*)src + size;
}

template<typename T>
inline
const void* read_value(T* dst, const void* src, size_t size) {
    size = sizeof(T) * size;
    memcpy(dst, src, size);
    return (char*)src + size;
}


inline
const void* addr_offset(const void* root, size_t offset) {
    return (char*)root + offset;
}

const char* create_from_file(const char* path);
void create_from_file_as_userdata(const char* path, lua_State* L);
void decode_file(const char* path);
void decode_value(const char* buf);
void decode_table(const char* buf);
void decode_num(const char* buf);
void decode_str(const char* buf);

/* lua_Number read_key_num(const char* buf); */
/* lua_Number read_value_num(const char* buf); */

/* const char* read_key_str(const char* buf); */
/* const char* read_value_str(const char* buf); */

#endif
