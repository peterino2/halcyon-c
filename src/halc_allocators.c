#include "halc_allocators.h"
#include "halc_strings.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h> // might get rid of this one...
#include <stdio.h>

struct allocator gDefaultAllocator;

#if TRACK_ALLOCATIONS
b8 gTrackAllocations;
b8 gAllowTrackAllocations;
const char* gContextString = "Unnamed";
#endif

struct allocatorStats gAllocatorStats;

static void init_allocator_stats()
{
    gAllocatorStats.allocations = 0;
    gAllocatorStats.allocatedSize = 0;
    gAllocatorStats.peakAllocatedSize = 0;
}

void track_allocs(const char* contextString) 
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


errc untrack_allocs(struct allocatorStats* outTrackedAllocationStats)
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

        raiseCleanup(ERR_TEST_LEAKED_MEMORY);
    }


cleanup:
    gAllocatorStats.allocations = 0;
    gAllocatorStats.allocatedSize = 0;
    gAllocatorStats.peakAllocatedSize = 0;

#if TRACK_ALLOCATIONS
    gTrackAllocations = FALSE;
#endif

    end;
}

errc setup_default_allocator() 
{
    init_allocator_stats();
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;
    gTrackAllocations = FALSE;
    gAllowTrackAllocations = FALSE;

    end;
}

errc enable_allocation_tracking()
{
    gAllowTrackAllocations = TRUE;
    end;
}

errc setup_default_custom_allocator(
    void* (*malloc_fn) (size_t),
    void (*free_fn) (void*))
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;
    gTrackAllocations = FALSE;
    gAllowTrackAllocations = FALSE;

    end;
}

errc halloc_advanced(void** ptr, size_t size, const char* file, i32 lineNumber, const char* func)
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

void hfree_advanced(void* ptr, size_t size, const char* file, i32 lineNumber, const char* func)
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

void print_memory_statistics()
{
}

errc hrealloc_advanced(void** ptr, size_t size, size_t newSize, b8 allowShrink,const char* file, i32 lineNumber, const char* func)
{
    if(newSize == size)
    {
        end;
    }

    if(!allowShrink && (newSize <= newSize))
    {
        raise(ERR_REALLOC_SHRUNK_WHEN_NOT_ALLOWED);
    }

    // allocate new memory
    void* new = gDefaultAllocator.malloc_fn(newSize);
    if(!new)
    {
        raise(ERR_OUT_OF_MEMORY);
    }
    // copy from old to new
    memcpy(new, *ptr, min(size, newSize));
    
    // clean up old ptr
    gDefaultAllocator.free_fn(ptr);
    *ptr = new;

    gAllocatorStats.allocatedSize -= size;
    gAllocatorStats.allocatedSize += newSize;

    end;
}
