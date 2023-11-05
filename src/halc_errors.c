#include <stdio.h>
#include "halc_errors.h"
#include "halc_strings.h"

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
        case ERR_TOKENIZER_POINTER_OVERFLOW:
            return "Pointer Ran off the end while tokenizing";
        case ERR_UNRECOGNIZED_TOKEN:
            return "Unrecognized token";
        case ERR_TOKEN_OUT_OF_RANGE:
            return "Token is not part of this source file, pointer not in range.";
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
    fprintf(stderr, "  > " RED("Error") RED(" \"%s\"(%d):") YELLOW(" '%s'") CYAN(" %s:%d\n"), errcToString(code), code, C, F, L);
}

void setupErrorContext()
{
    gErrorCatch = ERR_OK;
    gErrorFirst = TRUE;
}
