#ifndef __PLT_DECODE__
#define __PLT_DECODE__

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#ifdef __cplusplus
}
#endif

const char* create_from_file(const char* path);
void create_from_file_as_userdata(const char* path, lua_State* L);
void decode_file(const char* path);
void decode_value(const char* buf);
void decode_table(const char* buf);
void decode_num(const char* buf);
void decode_str(const char* buf);

#endif
