#include <fstream>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
}

enum key_type {
    NUM,
    STR,
};

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

void decode(const char* path) {
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    std::streamsize size = inf.tellg();
    inf.seekg(0, std::ios::beg);
    printf("all size = %ld\n", size);

    char* buf = new char[size]();
    inf.read(buf, size);
    inf.close();

    decode_keys(buf);

    delete[](buf);
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("require path\n");
        return -1;
    }

    decode(argv[1]);

    return 0;
}
