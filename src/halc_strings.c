
#include "halc_allocators.h"
#include "halc_strings.h"

#include <stdio.h>


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

errc hstr_normalize(const hstr* istr, hstr* ostr)
{
    // Allocate working buffer
    halloc(&ostr->buffer, istr->len); // FIXME_GOOD
    ostr->len = 0;
    ostr->cap = istr->len;

    char* w = ostr->buffer;
    usize wLen = ostr->len;

    const char* r = istr->buffer;
    const char* rEnd = istr->buffer + istr->len;

    b8 isNewLine = TRUE;
    i32 spaceCount = 0;

    while(r < rEnd)
    {

        if(isNewLine)
        {
            if(*r != '\t' && *r != ' ')
            {
                isNewLine = FALSE;
                if(spaceCount % 4 != 0)
                {
                    *w = 0; // write out a null so we can debug print it
                    fprintf(stderr, RED("attempted to normalize file with inconsistent file format content normalized so far:") " %s\n", ostr->buffer);
                    raise(ERR_INCONSISTENT_FILE_FORMAT);
                }

                while(spaceCount > 0)
                {
                    *w = '\t';
                    w++;
                    spaceCount -= 4;
                }
                spaceCount = 0;
            }
        }

        if(*r == '\n')
        {
            isNewLine = TRUE;
        }

        if(*r == '\r')
        {
            // no-op
        }
        else if(isNewLine)
        {
            if(*r == ' ')
            {
                spaceCount += 1;
            }
            else
            {
                *w = *r;
                w++;
            }
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
