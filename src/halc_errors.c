#include <stdio.h>
#include "halc_errors.h"

errc gErrorCatch = ERR_OK; b8 gErrorFirst = FALSE;

const char* errcToString(errc code)
{
    switch(code)
    {   
        case ERR_OK:
            return "Ok, this is not an error";
        case ERR_OUT_OF_MEMORY:
            return "Out of Memory";
        case ERR_UNKNOWN:
            return "Unknown Error";
        case ERR_STR_BAD_RESIZE:
            return "Bad String Resize arguments, new size must be equal or larger.";
        case ERR_UNABLE_TO_OPEN_FILE:
            return "Unable to open file";
        case ERR_FILE_SEEK_ERROR:
            return "File Seek error";
        case ERR_ASSERTION_FAILED:
            return "Assertion failed";
        default: 
            break;
    }

    return "UNKNOWN_ERROR_CODE";
}

void errorPrint(errc code, const char* C, const char* F, int L)
{
    if(gErrorFirst)
    {
        fprintf(stderr, "\n");
        gErrorFirst = FALSE;
    }
    fprintf(stderr, "  > Error \"%s\": '%s' %s:%d\n", errcToString(code), C, F, L);
}

void setupErrorContext()
{
    gErrorCatch = ERR_OK;
    gErrorFirst = TRUE;
}
