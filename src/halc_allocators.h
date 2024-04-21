#ifndef _HALC_ALLOCATORS_H_
#define _HALC_ALLOCATORS_H_

#include <stdlib.h>
#include "halc_errors.h"

EXTERN_C_BEGIN

#ifndef TRACK_ALLOCATIONS
#define TRACK_ALLOCATIONS 1
#endif


// == statistics ==
struct allocatorStats{
    i32 allocations;
    i64 allocatedSize;
    i64 peakAllocatedSize;
};

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
errc hallocAdvanced(void** ptr, size_t size);

#define halloc(ptr, size) tryCleanup(hallocAdvanced((void**) ptr, size))

void hfree(void* ptr);
void trackAllocs(const char* contextString);
errc untrackAllocs(struct allocatorStats* outTrackedAllocationStats);
void printMemoryStatistics();
errc enableAllocationTracking();



EXTERN_C_END

#endif
