#ifndef _HALC_ALLOCATORS_H_
#define _HALC_ALLOCATORS_H_

#include <stdlib.h>
#include "halc_errors.h"

#define TRACK_ALLOCATIONS 1

// ==================== Allocators ======================
// mostly an internal library

// uses default C allocator that comes with your os support package.
errc setupDefaultAllocator();

struct allocator{
    void* (*malloc_fn) (size_t);
    void (*free_fn) (void*);
};

extern struct allocator gDefaultAllocator;

errc setupCustomDefaultAllocator(void* (*malloc_fn) (size_t), void (*free_fn) (void*));
errc halloc_advanced(void** ptr, size_t size);

#define halloc(ptr, size) halloc_advanced((void**) ptr, size)

void hfree(void* ptr);
void trackAllocs(const char* contextString);
void untrackAllocs();
void printMemoryStatistics();
void enableAllocationTracking();

#endif
