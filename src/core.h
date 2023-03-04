#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t Size;

#define intern static
#define local_persist static



template<typename T>
struct __Defer {
    T t;
    __Defer(T t) : t(t) { }
    ~__Defer() { t(); }
};

template<typename T>
__Defer<T> __DoDefer(T t) { return __Defer<T>(t); }

#define __DEFER_MERGE(x, y) x##y
#define __DEFER_MERGE_1(x, y) __DEFER_MERGE(x, y)
#define __DEFER_COUNTER(x) __DEFER_MERGE_1(x, __COUNTER__)
#define defer(code) auto __DEFER_COUNTER(__defer) = __DoDefer([&](){code;})

#define cast(type, ctx) ((type)ctx)



// allocation
void* Alloc(size_t size);
void* Realloc(void* memory, size_t newSize);
void Free(void* memory);

#define StackAlloc(x) alloca(x)


// looping
#define For(x) for (auto it = (x).data; it != (x).data + (x).size; it++)
#define ForIt(x, it) for (auto it = (x).data; it != (x).data + (x).size; it++)
#define ForIdx(x, idx) for (Size idx = 0; idx < (x).size; idx++)

// size macros
#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
