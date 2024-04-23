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

// uses default C allocator that comes with your os support package.
errc setupDefaultAllocator();

// allocator interface
struct allocator{
    void* (*malloc_fn) (size_t);
    void (*free_fn) (void*);
};


// default allocator used internally within halcyon. 
// unless a special allocator is needed
extern struct allocator gDefaultAllocator;

// overrides gDefaultAllocator with a custom malloc and free function
errc setupCustomDefaultAllocator(void* (*malloc_fn) (size_t), void (*free_fn) (void*));

// backing code for halloc
errc hallocAdvanced(void** ptr, size_t size, const char* file, i32 lineNumber, const char* func);

// Will go to cleanup if alloc fails for any reason.
#define halloc(ptr, size) try(hallocAdvanced((void**) ptr, size, __FILE__, __LINE__, __func__))

#define hallocCleanup(ptr, size) tryCleanup(hallocAdvanced((void**) ptr, size))

#define hfree(ptr, size) hfreeAdvanced(ptr, size, __FILE__, __LINE__, __func__);

void hfreeAdvanced(void* ptr, size_t size, const char* file, i32 lineNumber, const char* func);
void trackAllocs(const char* contextString);
errc untrackAllocs(struct allocatorStats* outTrackedAllocationStats);
void printMemoryStatistics();
errc enableAllocationTracking();

EXTERN_C_END

#endif
