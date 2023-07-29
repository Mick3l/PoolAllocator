#pragma once

#include <algorithm>
#include <stack>
#include <string>

#include "nlohmann/json.hpp"


namespace mtl {
    template<typename Sequence = std::deque<std::byte*>>
    class Pool {
    public:
        using pointer = std::byte*;
        using const_pointer = std::byte* const;

        Pool();

        Pool(const Pool& other) = default;

        Pool& operator=(Pool&& other) noexcept;

        Pool(Pool&& other) noexcept;


        Pool& operator=(const Pool& other);

        Pool(std::size_t chunks_in_pool, std::size_t size_of_chunk);

        ~Pool();

        pointer allocate();

        void deallocate(pointer p);

        bool operator<(const Pool& other);

        bool operator==(const Pool& other);

        bool operator!=(const Pool& other);

        bool operator>(const Pool& other);

        bool operator==(size_t requested);

        bool operator!=(size_t requested);

        template<typename T>
        bool operator==(T* requested);

        template<typename T>
        bool operator!=(T* requested);

    private:
        pointer begin_of_memory;
        size_t chunks;
        size_t size_of_chunk;
        pointer end_of_memory;
        std::stack<std::byte*, Sequence> free_chunks;
    };

    template<typename T, typename Sequence = std::deque<std::byte*>>
    class PoolAllocator {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = T* const;

        PoolAllocator() : pool_list(*(new std::vector<Pool<Sequence>>())),
                          count(*(new uint16_t)) {
            count = 1;
        }

        template<typename U>
        explicit PoolAllocator(const PoolAllocator<U>& other);

        PoolAllocator(const PoolAllocator& other);

        explicit PoolAllocator(const std::string& config);

        template<typename U>
        PoolAllocator& operator=(const PoolAllocator& other);

        template<typename U>
        PoolAllocator& operator=(PoolAllocator&& other);


        template<typename U>
        PoolAllocator(PoolAllocator<U>&& other) noexcept;

        ~PoolAllocator();

        template<typename... Args>
        explicit PoolAllocator(Args... args);

        void AddPool(std::size_t chunks_in_pool, std::size_t size_of_chunk);

        bool operator==(const PoolAllocator& other);

        bool operator!=(const PoolAllocator& other);

        pointer allocate(size_t n);

        void deallocate(pointer p, size_t n);

        template<typename... Args>
        void construct(pointer p, Args... args);

        void destroy(pointer p);

        std::vector<Pool<Sequence>>& pool_list;
        uint16_t& count;
    };

    template<typename T, typename Sequence>
    PoolAllocator<T, Sequence>::PoolAllocator(const std::string& config) : pool_list(*(new std::vector<Pool<Sequence>>())),
                                                                           count(*(new uint16_t)) {
        count = 1;
        const auto root = nlohmann::json::parse(std::ifstream(config));
        if (!root.contains("pools")) {
            throw std::runtime_error("unvalid config");
        }
        for (int i = 0; i < root["pools"].size() - 1; ++i) {
            pool_list.emplace_back(root["pools"][i], root["pools"][i + 1]);
        }
    }


    template<typename T, typename... Args>
    T Unpack(size_t index, size_t inc, T arg1, Args... args) {
        if (index == inc) {
            return arg1;
        } else {
            return Unpack(index, inc + 1, args..., arg1);
        }
    }

    template<typename T, typename... Args>
    T GetArgFromPack(size_t index, T arg1, Args... args) {
        return Unpack(index, 0, arg1, args...);
    }

}// namespace mtl

template<typename T, typename Sequence>
template<typename... Args>
mtl::PoolAllocator<T, Sequence>::PoolAllocator(Args... args) : pool_list(*(new std::vector<Pool<Sequence>>(sizeof...(args) / 2))),
                                                               count(*(new uint16_t)) {
    count = 1;
    for (int i = 0; i < sizeof...(args) / 2; ++i) {
        pool_list[i] = std::move(Pool<Sequence>(GetArgFromPack(2 * i, args...), GetArgFromPack(2 * i + 1, args...)));
    }
}

template<typename T, typename Sequence>
template<typename U>
mtl::PoolAllocator<T, Sequence>::PoolAllocator(const PoolAllocator<U>& other) : pool_list(other.pool_list),
                                                                                count(other.count) {
    ++count;
}


template<typename T, typename Sequence>
mtl::PoolAllocator<T, Sequence>::PoolAllocator(const PoolAllocator& other) : pool_list(other.pool_list),
                                                                             count(other.count) {
    ++count;
}

template<typename T, typename Sequence>
template<typename U>
mtl::PoolAllocator<T, Sequence>::PoolAllocator(PoolAllocator<U>&& other) noexcept : pool_list(other.pool_list),
                                                                                    count(other.count) {
    ++count;
}

template<typename T, typename Sequence>
mtl::PoolAllocator<T, Sequence>::~PoolAllocator() {
    if (count == 1) {
        delete &pool_list;
        delete &count;
    } else {
        --count;
    }
}

template<typename T, typename Sequence>
void mtl::PoolAllocator<T, Sequence>::AddPool(std::size_t chunks_in_pool, std::size_t size_of_chunk) {
    Pool new_pool(chunks_in_pool, size_of_chunk);
    pool_list.insert(std::upper_bound(pool_list.begin(), pool_list.end(), new_pool), new_pool);
}


template<typename T, typename Sequence>
template<typename U>
mtl::PoolAllocator<T, Sequence>& mtl::PoolAllocator<T, Sequence>::operator=(const PoolAllocator& other) {
    if (this == &other) {
        return *this;
    }
    if (count == 1) {
        delete &pool_list;
        delete &count;
    } else {
        --count;
    }
    pool_list = other.pool_list;
    count = other.count;
    ++count;
    return *this;
}

template<typename T, typename Sequence>
template<typename U>
mtl::PoolAllocator<T, Sequence>& mtl::PoolAllocator<T, Sequence>::operator=(PoolAllocator&& other) {
    if (this == &other) {
        return *this;
    }
    if (count == 1) {
        delete &pool_list;
        delete &count;
    } else {
        --count;
    }
    pool_list = other.pool_list;
    count = other.count;
    ++count;
    return *this;
}

template<typename T, typename Sequence>
bool mtl::PoolAllocator<T, Sequence>::operator==(const PoolAllocator& other) {
    if (pool_list.size() != other.pool_list.size()) {
        return false;
    }
    for (int i = 0; i < pool_list.size(); ++i) {
        if (pool_list[i] != other.pool_list[i]) {
            return false;
        }
    }
    return true;
}

template<typename T, typename Sequence>
bool mtl::PoolAllocator<T, Sequence>::operator!=(const PoolAllocator& other) {
    return !operator==(other);
}

template<typename T, typename Sequence>
typename mtl::PoolAllocator<T, Sequence>::pointer mtl::PoolAllocator<T, Sequence>::allocate(size_t n) {
    auto pool = std::find(pool_list.begin(), pool_list.end(), n * sizeof(T));
    if (pool == pool_list.end()) {
        throw std::bad_alloc{};
    }
    return reinterpret_cast<pointer>(pool->allocate());
}

template<typename T, typename Sequence>
void mtl::PoolAllocator<T, Sequence>::deallocate(PoolAllocator::pointer p, size_t n) {
    std::find(pool_list.begin(), pool_list.end(), p)->deallocate(reinterpret_cast<typename Pool<Sequence>::pointer>(p));
}

template<typename T, typename Sequence>
template<typename... Args>
void mtl::PoolAllocator<T, Sequence>::construct(PoolAllocator::pointer p, Args... args) {
    *p = T(args...);
}

template<typename T, typename Sequence>
void mtl::PoolAllocator<T, Sequence>::destroy(PoolAllocator::pointer p) {
    p->~T();
}

template<typename Sequence>
mtl::Pool<Sequence>::Pool(std::size_t chunks_in_pool, std::size_t size_of_chunk)
    : begin_of_memory(new std::byte[chunks_in_pool * size_of_chunk]),
      size_of_chunk(size_of_chunk),
      chunks(chunks_in_pool),
      end_of_memory(begin_of_memory + size_of_chunk * chunks),
      free_chunks() {
    for (size_t i = 0; i < chunks_in_pool; ++i) {
        free_chunks.emplace(begin_of_memory + i * size_of_chunk);
    }
}

template<typename Sequence>
mtl::Pool<Sequence>::Pool() : begin_of_memory(nullptr),
                              size_of_chunk(0),
                              chunks(0),
                              end_of_memory(begin_of_memory + size_of_chunk * chunks),
                              free_chunks() {
}

template<typename Sequence>
mtl::Pool<Sequence>::~Pool() {
    delete[] begin_of_memory;
}

template<typename Sequence>
mtl::Pool<Sequence>::Pool(Pool<Sequence>&& other) noexcept : begin_of_memory(other.begin_of_memory),
                                                             size_of_chunk(other.size_of_chunk),
                                                             chunks(other.chunks),
                                                             end_of_memory(other.begin_of_memory + other.size_of_chunk * other.chunks),
                                                             free_chunks(std::move(other.free_chunks)) {
    other.size_of_chunk = other.chunks = 0;
    other.begin_of_memory = nullptr;
}

template<typename Sequence>
mtl::Pool<Sequence>& mtl::Pool<Sequence>::operator=(Pool<Sequence>&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    begin_of_memory = other.begin_of_memory;
    size_of_chunk = other.size_of_chunk;
    chunks = other.chunks;
    end_of_memory = other.end_of_memory;
    free_chunks = std::move(other.free_chunks);
    other.size_of_chunk = other.chunks = 0;
    other.begin_of_memory = nullptr;
    return *this;
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator>(const Pool& other) {
    return (size_of_chunk > other.size_of_chunk) ||
           (size_of_chunk == other.size_of_chunk && free_chunks.size() > other.free_chunks.size());
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator==(const Pool& other) {
    return (size_of_chunk == other.size_of_chunk && free_chunks.size() == other.free_chunks.size());
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator!=(const Pool& other) {
    return !operator==(other);
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator<(const Pool& other) {
    return (size_of_chunk < other.size_of_chunk) ||
           (size_of_chunk == other.size_of_chunk && free_chunks.size() < other.free_chunks.size());
    ;
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator==(size_t requested) {
    return !free_chunks.empty() && size_of_chunk >= requested;
}

template<typename Sequence>
bool mtl::Pool<Sequence>::operator!=(size_t requested) {
    return !operator==(requested);
}

template<typename Sequence>
template<typename T>
bool mtl::Pool<Sequence>::operator==(T* requested) {
    return reinterpret_cast<std::byte*>(requested) >= begin_of_memory &&
           reinterpret_cast<std::byte*>(requested) <= end_of_memory;
}

template<typename Sequence>
template<typename T>
bool mtl::Pool<Sequence>::operator!=(T* requested) {
    return !operator==(requested);
}

template<typename Sequence>
void mtl::Pool<Sequence>::deallocate(Pool::pointer p) {
    free_chunks.emplace(reinterpret_cast<std::byte*>(p));
}

template<typename Sequence>
typename mtl::Pool<Sequence>::pointer mtl::Pool<Sequence>::allocate() {
    auto result = reinterpret_cast<pointer>(free_chunks.top());
    free_chunks.pop();
    return result;
}

template<typename Sequence>
mtl::Pool<Sequence>& mtl::Pool<Sequence>::operator=(const Pool& other) {
    if (this == &other) {
        return *this;
    }
    delete[] begin_of_memory;
    begin_of_memory = other.begin_of_memory;
    size_of_chunk = other.size_of_chunk;
    chunks = other.chunks;
    free_chunks = other.free_chunks;
    return *this;
}
