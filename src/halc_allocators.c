#include "halc_allocators.h"

#include <stdlib.h>

struct allocator gDefaultAllocator;

errc setupDefaultAllocator() 
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;

    ok;
}

errc setupCustomDefaultAllocator(
    void* (*malloc_fn) (usize),
    void (*free_fn) (void*))
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;

    ok;
}

errc halloc_advanced(void** ptr, usize size)
{
    *ptr = gDefaultAllocator.malloc_fn(size);
    if(!*ptr)
    {
        herror(ERR_OUT_OF_MEMORY);
    }

    ok;
}

void hfree(void* ptr)
{
    gDefaultAllocator.free_fn(ptr);
}
