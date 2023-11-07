//
// Created by tanisme on 2023/11/7.
//

#include "MemoryPool.h"

MemoryPool::MemoryPool() {
    for (int i = 0; i < __list_number; ++i)
        m_list[i] = nullptr;
    m_ptrStart = m_ptrEnd = nullptr;
}

MemoryPool::~MemoryPool() {
    for (auto iter = m_memVecs.begin(); iter != m_memVecs.end(); ++iter) {
        if (*iter) {
            free(*iter);
        }
    }
    m_memVecs.clear();
}

int MemoryPool::RoundUp(int size, int align) {
    return ((size + align - 1) & ~(align - 1));
}

int MemoryPool::ListIndex(int size, int align) {
    return (size + align - 1) / align - 1;
}

void* MemoryPool::ReFill(int size, int grow_number) {
    int number = grow_number;
    char* chunk = nullptr;
    try {
        chunk = (char*) Chunk(size, number);
    } catch (const std::exception& e) {
        printf("Malloc failed!!! %s\n", e.what());
        return nullptr;
    }

    MemNode* volatile* first;
    MemNode* res, *cur, *next;
    if (number == 1) return chunk;
    res = (MemNode*)chunk;
    first = &(m_list[ListIndex(size)]);
    *first = next = (MemNode*)(chunk + size);
    for (int i = 1; ; i++) {
        cur = next;
        next = (MemNode*)((char*) next + size);
        if (number - 1 == i) {
            cur->_next = nullptr;
            break;
        } else {
            cur->_next = next;
        }
    }
    return res;
}

void* MemoryPool::Chunk(int size, int& number) {
    char* ret;
    int need_size = size * number;
    int left_size = (int)(m_ptrEnd - m_ptrStart);
    if (left_size >= need_size) {
        ret = m_ptrStart;
        m_ptrStart += need_size;
        return ret;
    }
    else if (left_size >= size) {
        number = left_size / size;
        need_size = size * number;
        ret = m_ptrStart;
        m_ptrStart += need_size;
        return ret;
    }
    int get_size = size * number;
    if (left_size > 0) {
        MemNode* first = m_list[ListIndex(left_size)];
        ((MemNode*)m_ptrStart)->_next = first;
        m_list[ListIndex(size)] = (MemNode*)m_ptrStart;
    }

    m_ptrStart = (char*) malloc(get_size);
    if (!m_ptrStart) {
        throw std::exception(std::logic_error("Memory is not enough!"));
        return nullptr;
    }

    m_memVecs.push_back(m_ptrStart);
    m_ptrEnd = m_ptrStart + get_size;
    return Chunk(size, number);
}
