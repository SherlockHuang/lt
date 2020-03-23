#include <cmath>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <vector>
#include <exception>
#include <cstring>
#include <algorithm>
#include <string>
#include <memory>
#include <fstream>
#include <cassert>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
}

unsigned int pack_table(lua_State* L, int t, std::ofstream& of);

void print_hex(const char* buf, size_t size) {
    for (int i = 0; i < size; i++) {
        printf("%02x", buf[i] & 0xff);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        } else if ((i + 1) % 2 == 0) {
            printf(" ");
        }
    }

    printf("\n");
}

template<typename T>
size_t write_data(std::ofstream& of, T& data) {
#ifdef DEBUG
    auto pos = of.tellp();
    std::cout << "write data pos: " << pos << std::endl;
    print_hex((const char*) &data, sizeof(T));
#endif

    size_t write_size = sizeof(T);
    of.write((char*) &data, write_size);
    return write_size;
}

template<typename T>
size_t write_data(std::ofstream& of, T* data, size_t size) {
#ifdef DEBUG
    auto pos = of.tellp();
    std::cout << "write data pos: " << pos << std::endl;
    print_hex((const char*) data, sizeof(T) * size);
#endif

    size_t write_size = sizeof(T) * size;
    of.write((char*) data, write_size);
    return write_size;
}

template<typename T>
void write_buf_data(char* buf, T data) {
    memcpy(buf, (char*) &data, sizeof(T));
}

struct bt_node {
    uint32_t start;
    uint32_t end;
    bt_node* next;
};

enum key_type {
    NUM,
    STR,
    TABLE,
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
        return *this;
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

    size_t get_pack_size() {
        size_t size = sizeof(uint16_t);
        if (type == NUM) {
            size += sizeof(double);
        } else {
            size += (strlen(str) + 1) * sizeof(char);
        }
        return size;
    }

    void write_data_to_buf(char* buf) {
        uint16_t* size_info = (uint16_t*) buf;
        char* data = (char*) (buf + sizeof(uint16_t));

        if (type == NUM) {
            *size_info = (uint16_t) sizeof(double);
            write_buf_data(data, num);
        } else {
            size_t size = (strlen(str) + 1) * sizeof(char);
            assert(size < 0x8000);

            *size_info = (uint16_t) (size | 0x8000);
            printf("size = %zu, size info = %d, 0x%x\n", size, *size_info, *size_info);

            std::memcpy(data, str, strlen(str) + 1);
        }
    }

    unsigned int write_to(std::ofstream& of) {
        unsigned int pack_size = 0;

        char t = (char) type;
        pack_size += write_data(of, t);

        if (type == NUM) {
            printf("write key %f\n", num);
            pack_size += write_data(of, num);
        } else {
            printf("write key %s\n", str);

            uint16_t size = strlen(str) + 1;
            pack_size += write_data(of, size);
            pack_size += write_data(of, str, size);
        }

        return pack_size;
    }
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

uint32_t avl_choose_root(uint32_t start, uint32_t end) {
    assert(end >= start);

    uint32_t delta = end - start + 1;
    if (delta <= 3) {
        return start + delta / 2;
    }

    uint32_t right_node_num = 1;
    uint32_t left_node_num = 1;
    uint32_t threshold = 1;

    for (int i = 2; i < 32; i++) {
        threshold = 1 << i;
        if (delta + 1 == threshold) {
            return start + threshold / 2 - 1;
        }

        if (delta < threshold) {
            threshold /= 2;
            left_node_num = threshold / 2 - 1;
            right_node_num = delta - left_node_num - 1;
            if (right_node_num > threshold - 1) {
                left_node_num += right_node_num - threshold + 1;
                right_node_num = threshold - 1;
            }

            return start + right_node_num;
        }
    }

    left_node_num = threshold / 2 - 1;
    right_node_num = delta - left_node_num - 1;
    if (right_node_num > threshold - 1) {
        left_node_num += right_node_num - threshold + 1;
        right_node_num = threshold - 1;
    }

    return start + right_node_num;
}

bt_node* get_or_create_free_node(bt_node** free_head) {
    if (*free_head) {
        bt_node* head = *free_head;
        *free_head = head->next;
        return head;
    }

    bt_node* node = new bt_node();

    return node;
}

void pack_keys(std::vector<tkey>& key_vec, std::ofstream& of) {
    assert(key_vec.size() <= (size_t) (1 << 31));
    uint32_t key_size = key_vec.size();

    write_data(of, key_size);
    std::cout << "write key size " << key_size << std::endl;

    uint32_t key_data_size = 0;
    for (uint32_t i = 0; i < key_size; i++) {
        key_data_size += key_vec[i].get_pack_size();
    }

    size_t key_addr_size = sizeof(uint32_t) * key_size;
    size_t buf_size = key_addr_size + key_data_size;

    char* key_buf = (char*) malloc(buf_size);
    memset(key_buf, 0, buf_size);

    char* key_data = key_buf + sizeof(uint32_t) * key_size;
    uint32_t* key_addr_arr = (uint32_t*) key_buf;

    uint32_t key_offset = 0;
    uint32_t array_size = 0;
    for (uint32_t i = 0; i < key_size; i++) {
        auto& key = key_vec[i];
        if (array_size == i && key.type == NUM and key.num == i + 1) {

            key_addr_arr[i] = key_offset;
            key.write_data_to_buf(key_data + key_offset);

            key_offset += key_vec[i].get_pack_size();
            array_size++;
        } else {
            break;
        }
    }

    bt_node* head = nullptr;
    bt_node* tail = nullptr;
    bt_node* unused = nullptr;

    if (array_size < key_size) {
        head = new bt_node();
        head->start = array_size;
        head->end = key_size - 1;
        head->next = nullptr;

        tail = head;
    }

    for (uint32_t i = array_size; i < key_size; i++) {
        uint32_t mid = avl_choose_root(head->start, head->end);

        tkey& key = key_vec[mid];
        size_t pack_size = key.get_pack_size();

        key.write_data_to_buf(key_data + key_offset);
        key_addr_arr[i] = key_offset;
        key_offset += pack_size;

        if (key.type == NUM) {
            printf("write num key %f\n", key.num);
        } else {
            printf("write str key %s\n", key.str);
        }

        if (mid > head->start) {
            bt_node* left = get_or_create_free_node(&unused);
            left->start = head->start;
            left->end = mid - 1;
            left->next = nullptr;

            tail->next = left;
            tail = left;
        }

        if (mid < head->end) {
            bt_node* right = get_or_create_free_node(&unused);
            right->start = mid + 1;
            right->end = head->end;
            right->next = nullptr;

            tail->next = right;
            tail = right;
        }

        bt_node* next_head = head->next;

        if (unused) {
            head->next = unused;
            unused = head;
        } else {
            unused = head;
            head->next = nullptr;
        }

        head = next_head;
    }

    while(unused) {
        bt_node* node = unused;
        unused = unused->next;

        free(node);
    }

    write_data(of, array_size);
    write_data(of, key_buf, buf_size);
    free(key_buf);
}

unsigned int get_arr_size(std::vector<tkey>& key_vec) {
    uint32_t array_size = 0;
    for (uint32_t i = 0; i < key_vec.size(); i++) {
        auto& key = key_vec[i];
        if (array_size == i && key.type == NUM and key.num == i + 1) {
            array_size++;
        } else {
            break;
        }
    }
    return array_size;
}

inline
unsigned int pack_key(std::ofstream& of, tkey& key) {
    return key.write_to(of);
}

void put_value_on_top(lua_State* L, int t, tkey& key) {
    /* printf("t = %d, type = %s\n", t, lua_typename(L, lua_type(L, t))); */

    if (key.type == NUM) {
        lua_pushnumber(L, key.num);
    } else {
        lua_pushstring(L, key.str);
    }

    lua_rawget(L, t - 1);
}

unsigned int pack_value(lua_State* L, int t, std::ofstream& of, tkey& key) {
    put_value_on_top(L, t, key);

    unsigned int pack_size = 0;

    if (lua_istable(L, -1)) {
        pack_size += pack_table(L, -1, of);
    } else if(lua_isnumber(L, -1)) {
        char type = (char) NUM;
        lua_Number num = lua_tonumber(L, -1);

        printf("write value %f\n", num);
        
        pack_size += write_data(of, type);
        pack_size += write_data(of, num);
    } else if(lua_isstring(L, -1)) {
        char type = (char) STR;
        const char* str = lua_tostring(L, -1);
        uint16_t size = strlen(str) + 1;

        printf("write value %s\n", str);

        pack_size += write_data(of, type);
        pack_size += write_data(of, size);
        pack_size += write_data(of, str, size);
    } else {
        printf("Unsupport value type %s\n", lua_typename(L, lua_type(L, -1)));
        assert(false);
    }

    lua_pop(L, 1);

    return pack_size;
}

struct LuaTableInfo {
    lua_State* L;
    int index;
};

struct TKeyInfo {
    std::vector<tkey>& vec;
    unsigned int start;
    unsigned int end;
};

struct AddrInfo {
    unsigned int* key_addr;
    unsigned int* val_addr;
};

unsigned int pack_arr_part(std::ofstream& of, LuaTableInfo& table, TKeyInfo& keys, AddrInfo& addr, unsigned int base_size) {
    unsigned int start = keys.start;
    unsigned int end = keys.end;
    std::vector<tkey>& key_vec = keys.vec;

    unsigned int* key_addr = addr.key_addr;
    unsigned int* val_addr = addr.val_addr;
    unsigned int pack_size = base_size;

    for (unsigned int i = start; i < end; i++) {
        auto& key = key_vec[i];

#ifdef DEBUG
        printf("write key_addr %d, pack_size %u\n", i, pack_size);
#endif

        key_addr[i] = pack_size;
        pack_size += pack_key(of, key);

        val_addr[i] = pack_size;
        pack_size += pack_value(table.L, table.index, of, key);
    }

    unsigned inc_size = pack_size - base_size;
    return inc_size;
}

unsigned int pack_hash_part(std::ofstream& of, LuaTableInfo& table, TKeyInfo& keys, AddrInfo& addr, unsigned int base_size) {
    unsigned int start = keys.start;
    unsigned int end = keys.end;
    std::vector<tkey>& key_vec = keys.vec;

    unsigned int* key_addr = addr.key_addr;
    unsigned int* val_addr = addr.val_addr;
    unsigned int pack_size = base_size;

    bt_node* head = nullptr;
    bt_node* tail = nullptr;
    bt_node* unused = nullptr;

    if (start < end) {
        head = new bt_node();
        head->start = start;
        head->end = end - 1;
        head->next = nullptr;

        tail = head;
    }

    for (uint32_t i = start; i < end; i++) {
        uint32_t mid = avl_choose_root(head->start, head->end);
#ifdef DEBUG
        printf("i = %u, start, end = %u, %u, mid = %u, head_start, head_end = %u, %u\n", i, start, end, mid, head->start, head->end);
#endif

        tkey& key = key_vec[mid];

#ifdef DEBUG
        printf("write key_addr %d, pack_size %u\n", i, pack_size);
#endif

        key_addr[i] = pack_size;
        pack_size += pack_key(of, key);

        val_addr[i] = pack_size;
        pack_size += pack_value(table.L, table.index, of, key);

        if (mid > head->start) {
            bt_node* left = get_or_create_free_node(&unused);
            left->start = head->start;
            left->end = mid - 1;
            left->next = nullptr;

            tail->next = left;
            tail = left;
        }

        if (mid < head->end) {
            bt_node* right = get_or_create_free_node(&unused);
            right->start = mid + 1;
            right->end = head->end;
            right->next = nullptr;

            tail->next = right;
            tail = right;
        }

        bt_node* next_head = head->next;

        if (unused) {
            head->next = unused;
            unused = head;
        } else {
            unused = head;
            head->next = nullptr;
        }

        head = next_head;
    }

    while(unused) {
        bt_node* node = unused;
        unused = unused->next;

        free(node);
    }


    /* for (unsigned int i = start; i < end; i++) { */
    /*     auto& key = key_vec[i]; */

    /*     key_addr[i] = pack_size; */
    /*     pack_size += pack_key(of, key); */

    /*     val_addr[i] = pack_size; */
    /*     pack_size += pack_value(table.L, table.index, of, key); */
    /* } */

    unsigned int inc_size = pack_size - base_size;
    return inc_size;
}

unsigned int pack_table(lua_State* L, int t, std::ofstream& of) {
    printf("write table\n");
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
            snprintf(msg, 64, "unsupport key type %s\n", lua_typename(L, lua_type(L, -2)));
            printf("%s\n", msg);
            throw msg;
        }

        lua_pop(L, 1);
    }

    std::sort(key_vec.begin(), key_vec.end(), sort_tkey);
    assert(key_vec.size() <= (size_t) (0xffffffff));

    unsigned int pack_size = 0;

    char type = (char) TABLE;
    pack_size += write_data(of, type);

    uint32_t key_size = key_vec.size();

    printf("write key size %u\n", key_size);
    pack_size += write_data(of, key_size);

    uint32_t arr_size = get_arr_size(key_vec);
    printf("write arr size %u\n", arr_size);
    pack_size += write_data(of, arr_size);

    uint32_t* key_addr = new uint32_t[key_size]();
    uint32_t* val_addr = new uint32_t[key_size]();

    auto ka_pos = of.tellp();
    pack_size += write_data(of, key_addr, key_size);

    auto va_pos = of.tellp();
    pack_size += write_data(of, val_addr, key_size);

    LuaTableInfo table = { L, t };
    TKeyInfo keys = { key_vec, 0, arr_size};
    AddrInfo addr = { key_addr, val_addr };

    pack_size += pack_arr_part(of, table, keys, addr, pack_size);

    keys.start = arr_size;
    keys.end = key_vec.size();
    pack_size += pack_hash_part(of, table, keys, addr, pack_size);

    auto end_pos = of.tellp();

    of.seekp(ka_pos);
    write_data(of, key_addr, key_size);

    of.seekp(va_pos);
    write_data(of, val_addr, key_size);

    of.seekp(end_pos);

    return pack_size;
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
    
    std::ofstream of("output.pl", std::ios::binary | std::ios::out);

    pack_table(L, -1, of);

    of.close();

    return 0;
}
