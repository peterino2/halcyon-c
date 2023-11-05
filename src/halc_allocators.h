#ifndef _HALC_ALLOCATORS_H_
#define _HALC_ALLOCATORS_H_

#include <stdlib.h>
#include "halc_errors.h"

#define TRACK_ALLOCATIONS 1

// ==================== Allocators ======================

errc setupDefaultAllocator();

struct allocator{
    void* (*malloc_fn) (size_t);
    void (*free_fn) (void*);
};

extern struct allocator gDefaultAllocator;

errc setupCustomDefaultAllocator(
    void* (*malloc_fn) (size_t),
    void (*free_fn) (void*));

errc halloc_advanced(void** ptr, size_t size);

#define halloc(ptr, size) halloc_advanced((void**) ptr, size)

void hfree(void* ptr);

void trackAllocs(const char* contextString);
void untrackAllocs();

#endif
