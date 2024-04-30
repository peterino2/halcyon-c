
#include "halc_allocators.h"
#include "halc_strings.h"
#include "halc_errors.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>

#define HSTR_VALIDATE_NOT_STATIC(X) do{if(X->cap == -1) raise(ERR_STR_OPERATION_ON_STATIC_HSTR);\
    } while(0)

#define HSTR_VALIDATE_NOT_STATIC_VOID(X) do{if(X->cap == -1) return;\
    } while(0)

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
    HSTR_VALIDATE_NOT_STATIC_VOID(str);

    if(str->len <= 0)
    {
        return;
    }

    hfree(str->buffer, str->cap);
    str->len = 0;
    str->cap = 0;
    str->buffer = NULL;
}

void hstr_empty(hstr* str)
{
    HSTR_VALIDATE_NOT_STATIC_VOID(str);

    str->len = 0;
}

errc hstr_reserve(hstr* str, u32 len)
{
    HSTR_VALIDATE_NOT_STATIC(str);

    if (str->cap >= (i32)len)
    {
        end; // nothing to do, we're already the correct size
    }

    if(str->cap > 0)
    {
        hrealloc(&str->buffer, str->cap, len, FALSE);
    }
    else 
    {
        halloc(&str->buffer, len);
    }

    str->cap = len;

    end;
}

errc hstr_normalize(const hstr* istr, hstr* ostr)
{
    // Allocate working buffer, it's garunteed to be smaller the input buffer.
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

errc hstr_printf(hstr* str, const char* fmt, ...)
{
    HSTR_VALIDATE_NOT_STATIC(str);
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    int charsToWrite = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);

    if(str->cap < (i32)(str->len + charsToWrite + 1))
    {
        hstr_reserve(str, str->len + charsToWrite + 1);
    }

    char* start = str->buffer + str->len;
    vsnprintf(start, charsToWrite + 1, fmt, args);
    str->len += charsToWrite;
    va_end (args);

    end;
}

void hstr_init(hstr* str)
{
    str->len = 0;
    str->cap = 0;
    str->buffer = NULL;
}

#ifndef NO_TESTS
errc test_hstr_printf()
{
    hstr str;
    hstr_init(&str);
    try(hstr_printf(&str, "lol this is a printf %d", 32));
    try(hstr_printf(&str, ". lol this is another one %s", "wu tang forever"));

    hstr_free(&str);
    end;
}
#endif
