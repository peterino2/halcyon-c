
#include "halc_allocators.h"
#include "halc_strings.h"


// try to convert a given binary buffer into equivalent utf8 code points
// also converts \r\n into \n

// check if two stringviews match
b8 hstr_match(const hstr* left, const hstr* right)
{
    if(left->len != right->len)
        return FALSE;

    const char* r = left->buffer;
    const char* r2 = right->buffer;
    const char* rEnd = left->buffer + left->len;

    while(r < rEnd)
    {
        if(*r != *r2)
            return FALSE;

        r2++;
        r++;
    }

    return TRUE;
}

void hstr_free(hstr* str) 
{
    if(str->len == 0)
    {
        return;
    }
    hfree(str->buffer, str->cap);
    str->len = 0;
    str->buffer = NULL;
}

errc hstr_normalize_lf(const hstr* istr, hstr* ostr)
{
    // Allocate working buffer
    halloc(&ostr->buffer, istr->len); // FIXME_GOOD
    ostr->len = 0;
    ostr->cap = istr->len;

    char* w = ostr->buffer;
    usize wLen = ostr->len;

    const char* r = istr->buffer;
    const char* rEnd = istr->buffer + istr->len;

    while(r < rEnd)
    {
        if(*r == '\r')
        {
            // no-op
        }
        else 
        {
            *w = *r;
            w++;
        }
        
        r++;
    }

    *w = '\0';

    ostr->len = (u32)(w - ostr->buffer);

    end;
}
