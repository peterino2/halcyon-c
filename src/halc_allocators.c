#include "halc_allocators.h"

#include <stdlib.h>
#include <stdio.h>

struct allocator gDefaultAllocator;

#if TRACK_ALLOCATIONS
b8 gTrackAllocations;
b8 gAllowTrackAllocations;
const char* gContextString = "Unnamed";
#endif

struct allocatorStats gAllocatorStats;

static void initAllocatorStats()
{
    gAllocatorStats.allocations = 0;
    gAllocatorStats.allocatedSize = 0;
    gAllocatorStats.peakAllocatedSize = 0;
}

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


errc untrackAllocs(struct allocatorStats* outTrackedAllocationStats)
{
#if TRACK_ALLOCATIONS
    gTrackAllocations = FALSE;
#endif

    if(gAllowTrackAllocations)
    {
        if(gAllocatorStats.allocations > 0)
        {
            fprintf(stderr, "untrack called, leaked memory: %lld bytes in %lld allocations", gAllocatorStats.allocatedSize, gAllocatorStats.peakAllocatedSize);
            raise(ERR_TEST_LEAKED_MEMORY);
        }
    }

    end;
}

errc setupDefaultAllocator() 
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;
    gTrackAllocations = FALSE;
    gAllowTrackAllocations = FALSE;

    end;
}

errc enableAllocationTracking()
{
    gAllowTrackAllocations = TRUE;
    end;
}

errc setupCustomDefaultAllocator(
    void* (*malloc_fn) (size_t),
    void (*free_fn) (void*))
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;
    gTrackAllocations = FALSE;
    gAllowTrackAllocations = FALSE;

    end;
}

errc hallocAdvanced(void** ptr, size_t size)
{
    *ptr = gDefaultAllocator.malloc_fn(size);
    if(!*ptr)
    {
        raiseCleanup(ERR_OUT_OF_MEMORY);
    }

#if TRACK_ALLOCATIONS
    if(gTrackAllocations)
    {
        fprintf(stderr, "alloc->\"0x%p\" # %s\n", *ptr, gContextString);
    }
#endif

cleanup:
    end;
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

void printMemoryStatistics()
{
}
