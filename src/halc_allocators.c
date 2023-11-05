#include "halc_allocators.h"

#include <stdlib.h>
#include <stdio.h>

struct allocator gDefaultAllocator;

#if TRACK_ALLOCATIONS
b8 gTrackAllocations;
b8 gAllowTrackAllocations;
const char* gContextString = "Unnamed";
#endif

void trackAllocs(const char* contextString) 
{
#if TRACK_ALLOCATIONS
    if(!gAllowTrackAllocations)
    {
        return;
    }
    gTrackAllocations = TRUE;
    gContextString = contextString;
#endif
}

void untrackAllocs() 
{
#if TRACK_ALLOCATIONS
    gTrackAllocations = FALSE;
#endif
}

errc setupDefaultAllocator() 
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;
    gTrackAllocations = FALSE;
    gAllowTrackAllocations = FALSE;

    ok;
}

void enableAllocationTracking()
{
    gAllowTrackAllocations = TRUE;
}

errc setupCustomDefaultAllocator(
    void* (*malloc_fn) (size_t),
    void (*free_fn) (void*))
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;

    ok;
}

errc halloc_advanced(void** ptr, size_t size)
{
    *ptr = gDefaultAllocator.malloc_fn(size);
    if(!*ptr)
    {
        herror(ERR_OUT_OF_MEMORY);
    }

#if TRACK_ALLOCATIONS
    if(gTrackAllocations)
    {
        fprintf(stderr, "alloc->\"0x%p\" # %s\n", *ptr, gContextString);
    }
#endif

    ok;
}

void hfree(void* ptr)
{
#if TRACK_ALLOCATIONS
    if(gTrackAllocations)
    {
        fprintf(stderr, "free->\"0x%p\" # %s\n", ptr, gContextString);
    }
#endif
    gDefaultAllocator.free_fn(ptr);
}
