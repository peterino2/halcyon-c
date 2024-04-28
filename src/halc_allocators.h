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
errc setup_default_allocator();

// allocator interface
struct allocator{
    void* (*malloc_fn) (size_t);
    void (*free_fn) (void*);
};


// default allocator used internally within halcyon. 
// unless a special allocator is needed
extern struct allocator gDefaultAllocator;

// overrides gDefaultAllocator with a custom malloc and free function
errc setup_default_custom_allocator(void* (*malloc_fn) (size_t), void (*free_fn) (void*));


// Will go to cleanup if alloc fails for any reason.
#define halloc(ptr, size) try(halloc_advanced((void**) ptr, size, __FILE__, __LINE__, __func__))

#define halloc_cleanup(ptr, size) try_cleanup(halloc_advanced((void**) ptr, size))

// deletes selected pointer
// size of old allocation is needed for potential perf optimizations and debugging
#define hfree(ptr, size) hfree_advanced(ptr, size, __FILE__, __LINE__, __func__)

// reallocates selected pointer to new pointer size
// size of old allocation is needed for potential perf optimizations and  debugging
//
// modifies existing pointer, on failure no reallocation happens
#define hrealloc(ptr, size, newSize, allowShrink) try(hrealloc_advanced(ptr, size, newSize, allowShrink, __FILE__, __LINE__, __func__))

// backing code for halloc
errc halloc_advanced(void** ptr, size_t size, const char* file, i32 lineNumber, const char* func);
void hfree_advanced(void* ptr, size_t size, const char* file, i32 lineNumber, const char* func);
errc hrealloc_advanced(void** ptr, size_t size, size_t newSize, b8 allowShrink, const char* file, i32 lineNumber, const char* func);
void track_allocs(const char* contextString);
errc untrack_allocs(struct allocatorStats* outTrackedAllocationStats);
void primt_memory_statistics();
errc enable_allocation_tracking();

EXTERN_C_END

#endif
