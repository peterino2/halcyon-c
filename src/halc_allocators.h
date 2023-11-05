#ifndef _HALC_ALLOCATORS_H_
#define _HALC_ALLOCATORS_H_

#include "halc_errors.h"

// ==================== Allocators ======================

errc setupDefaultAllocator();

// These are the only two functions I'm going to support in my allocator interface.
struct allocator{
    void* (*malloc_fn) (unsigned long long);
    void (*free_fn) (void*);
};

extern struct allocator gDefaultAllocator;

// uses default cstandard library allocators

// allows you to bring whatever malloc and free function you want
errc setupCustomDefaultAllocator(
        void* (*malloc_fn) (usize),
        void (*free_fn) (void*));

// halcyon default allocation function
errc halloc_advanced(void** ptr, usize size);

#define halloc(ptr, size) halloc_advanced((void**) ptr, size)

// halcyon default free function
void hfree(void* ptr);


#endif