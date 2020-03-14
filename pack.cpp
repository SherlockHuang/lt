#include <cstdio>
#include <iostream>
#include <vector>
#include <exception>
#include <cstring>
#include <algorithm>
#include <string>
#include <memory>
#include <fstream>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

template<typename T>
void write_data(std::ofstream& of, T& data) {
    of.write((char*) &data, sizeof(T));
}

template<typename T>
void write_data(std::ofstream& of, T* data, size_t size) {
    of.write((char*) data, sizeof(T) * size);
}

struct bt_node {
    uint32_t start;
    uint32_t end;
    bt_node* next;
};

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
            size += sizeof((strlen(str) + 1) * sizeof(char));
        }
        return size;
    }

    void write_data_to_buf(char* buf) {
        uint16_t* size_info = (uint16_t*) buf;
        void* data = (void*) (buf + sizeof(uint16_t));

        if (type == NUM) {
            *size_info = (uint16_t) sizeof(double);
            *((double *)data) = num;
        } else {
            size_t size = (strlen(str) + 1) * sizeof(char);
            assert(size < 1 << 15);

            size |= 1 << 15;
            *size_info = (uint16_t) size;

            std::memcpy(data, str, strlen(str) + 1);
        }
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
    assert(key_vec.size() <= 1 << 31);
    uint32_t key_size = key_vec.size();

    write_data(of, key_size);
    std::cout << "write size " << key_size << std::endl;

    uint32_t total_key_size = 0;
    for (uint32_t i = 0; i < key_size; i++) {
        total_key_size += key_vec[i].get_pack_size();
    }
    std::cout << "total size " << total_key_size << std::endl;

    char* key_buf = (char*) malloc(sizeof(uint32_t) * key_size + total_key_size);
    uint32_t* key_addr_arr = (uint32_t*) key_buf;
    char* key_data = key_buf + sizeof(uint32_t) * key_size;

    std::cout << "key_buf size " << sizeof(uint32_t) * key_size + total_key_size << std::endl;
    printf("key_buf: %p, key_addr_arr: %p, key_data: %p\n", key_buf, key_addr_arr, key_data);

    uint32_t key_offset = 0;
    uint32_t array_size = 0;
    for (uint32_t i = 0; i < key_size; i++) {
        auto& key = key_vec[i];
        if (array_size == i && key.type == NUM and key.num == i + 1) {
            std::cout << "write array key " << key.num << ", key offset = " << key_offset << std::endl;

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

    while(head) {
        size_t mid = avl_choose_root(head->start, head->end);
        key_addr_arr[mid] = key_offset;
        key_vec[mid].write_data_to_buf(key_data + key_offset);
        key_offset += key_vec[mid].get_pack_size();

        printf("start: %d, end: %d, get mid: %d, key = ", head->start, head->end, mid);
        if (key_vec[mid].type == NUM) {
            std::cout << key_vec[mid].num << std::endl;
        } else {
            std::cout << key_vec[mid].str << std::endl;
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

    int count = 0;
    while(unused) {
        bt_node* node = unused;
        unused = unused->next;

        free(node);
    }

    /* uint32_t key_offset = 0; */
    /* for (tkey& key : key_vec) { */
    /*     write_data(of, key_offset); */
    /*     key_offset += get_pack_size(); */
    /* } */
}

void pack(lua_State* L, int t, std::ofstream& of) {
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
            std::cout << "key num " << key.num << std::endl;
        } else {
            std::cout << "key str " << key.str << std::endl;
        }
    }

    std::cout << "---------- pack key ----------" << std::endl;
    pack_keys(key_vec, of);
    std::cout << "---------- pack key end ----------" << std::endl;

    for (auto& key : key_vec) {
        if (key.type == NUM) {
            lua_pushnumber(L, key.num);
            std::cout << "key num " << key.num << std::endl;
        } else {
            lua_pushstring(L, key.str);
            std::cout << "key str " << key.str << std::endl;
        }
        lua_rawget(L, -2);

        if (lua_istable(L, -1)) {
            pack(L, -1, of);
            std::cout << "---------- value table end ----------" << std::endl;
        }
        else if(lua_isnumber(L, -1)) {
            std::cout << "value num: " << lua_tonumber(L, -1) << std::endl;
        } else if(lua_isstring(L, -1)) {
            std::cout << "value str: " << lua_tostring(L, -1) << std::endl;
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
    
    std::ofstream of("output.pl", std::ios::binary | std::ios::out);
    pack(L, -1, of);
    of.close();

    return 0;
}
