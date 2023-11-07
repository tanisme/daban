//
// Created by tanisme on 2023/11/7.
//

#ifndef DABAN_MEMORYPOOL_H
#define DABAN_MEMORYPOOL_H

#include <new>
#include <map>
#include <mutex>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <functional>

static const int __align = 8;
static const int __max_bytes = 1024;
static const int __list_number = __max_bytes/__align;
static const int __grow_number = 50;

class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    template<class T>
    T* Malloc(int size);
    template<class T>
    void Free(T* &mem, int size);

private:
    int RoundUp(int size, int align = __align);
    int ListIndex(int size, int align = __align);
    void* ReFill(int size, int grow_number = __grow_number);
    void* Chunk(int size, int& number);

private:
    union MemNode {
        MemNode* _next;
        char _data[1];
    };

    MemNode* m_list[__list_number];
    char* m_ptrStart;
    char* m_ptrEnd;
    std::vector<char*> m_memVecs;
    std::recursive_mutex m_mutex;
};

template<class T>
T* MemoryPool::Malloc(int size) {
    if (size > __max_bytes) {
        void* mem = malloc(size);
        memset(mem, 0, size);
        return (T*)mem;
    }

    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    MemNode** first = &(m_list[ListIndex(size)]);
    MemNode* node = *first;
    if (!node) {
        void* mem = ReFill(RoundUp(size));
        memset(mem, 0, size);
        return (T*)mem;
    }
    *first = node->_next;
    memset(node, 0, size);
    return (T*)node;
}

template<class T>
void MemoryPool::Free(T *&mem, int size) {
    if (!mem) return;
    if (size > __max_bytes) {
        free(mem);
        mem = nullptr;
        return;
    }

    MemNode* node = (MemNode*)mem;
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    MemNode** first = &(m_list[ListIndex(size)]);
    node->_next = *first;
    *first = node;
    mem = nullptr;
}

#endif //DABAN_MEMORYPOOL_H
