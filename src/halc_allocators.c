#include "halc_allocators.h"
#include "halc_strings.h"

#include <inttypes.h>
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

    *outTrackedAllocationStats = gAllocatorStats;

        if(gAllocatorStats.allocations > 0)
        {
            fprintf(stderr, "untrack called, leaked memory: %" PRId64 
                    " bytes in %" PRId32 
                    " allocations (peakAllocatedSize: %" PRId64 ")\n", 
                    gAllocatorStats.allocatedSize,
                    gAllocatorStats.allocations,
                    gAllocatorStats.peakAllocatedSize);
            raise(ERR_TEST_LEAKED_MEMORY);
        }

    gAllocatorStats.allocations = 0;
    gAllocatorStats.allocatedSize = 0;
    gAllocatorStats.peakAllocatedSize = 0;

#if TRACK_ALLOCATIONS
    gTrackAllocations = FALSE;
#endif

    end;
}

errc setupDefaultAllocator() 
{
    initAllocatorStats();
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

errc hallocAdvanced(void** ptr, size_t size, const char* file, i32 lineNumber, const char* func)
{
    *ptr = gDefaultAllocator.malloc_fn(size);
    if(!*ptr)
    {
        raiseCleanup(ERR_OUT_OF_MEMORY);
    }

#if TRACK_ALLOCATIONS
    if(gTrackAllocations)
    {
        fprintf(stderr, YELLOW("alloc(%" PRId64 ")->\"0x%p\" # %s %s() %s:%d\n"), (i64)size, *ptr, gContextString, func, file, lineNumber);
    }
#endif

    gAllocatorStats.allocations += 1;
    gAllocatorStats.allocatedSize += size;
    if(gAllocatorStats.allocatedSize > gAllocatorStats.peakAllocatedSize)
    {
        gAllocatorStats.peakAllocatedSize = gAllocatorStats.allocatedSize;
    }

cleanup:
    end;
}

// ======================= hash allocator =================

void hfreeAdvanced(void* ptr, size_t size, const char* file, i32 lineNumber, const char* func)
{
#if TRACK_ALLOCATIONS
    if(gTrackAllocations)
    {
        fprintf(stderr, GREEN("free(%" PRId64 ")->\"0x%p\" # %s %s() %s:%d\n"), (i64)size, ptr, gContextString, func, file, lineNumber);
    }
#endif
    gAllocatorStats.allocatedSize -= size;
    gAllocatorStats.allocations -= 1;
    gDefaultAllocator.free_fn(ptr);
}

void printMemoryStatistics()
{
}
