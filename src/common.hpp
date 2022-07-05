#if !defined(METAVM_COMMON_HPP)
#define METAVM_COMMON_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utils/math.hpp>
#include <utils/assert.hpp>
#include <utils/memory.hpp>

using namespace achilles;

inline bool aassert_handler(const char *conditionCode, const char *report) {
    std::printf("assertion %s failed, %s \n", conditionCode, report);
    return true;
}

inline void *default_allocator(u64 size, u64 alignment = sizeof(u64), void *previousMemory = nullptr, u64 previousSize = 0) {
    if (size == previousSize) return previousMemory;
    u64 aligned = memory::align(size, alignment);
    if (previousMemory == nullptr) {
        void *result = std::malloc(aligned);
        std::memset(result, 0, aligned);
        return result;
    }
    void *newMemory = std::realloc(previousMemory, aligned);
    if (size > previousSize) {
        std::memset(((u8 *) newMemory) + previousSize, 0, size - previousSize);    
    }
    return newMemory;
}

inline void default_deallocator(void *mem, u64 size) {
    (void)size; // shutting off the compiler warning
    return free(mem);
}

template<typename T>
using region = memory::region<T, default_allocator, default_deallocator, true>;

template<typename T>
using array = memory::array<T, default_allocator, default_deallocator, true>;

template<typename T, u64 size>
using static_region = memory::static_region<T, size>;

template<typename T, u64 size>
using static_array = memory::static_array<T, size>;

template<typename T>
using memory_view = memory::memory_view<T>;

template<typename T>
using array_view = memory::array_view<T>;

#endif
