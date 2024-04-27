#include <stdio.h>
#include "halc_errors.h"
#include "halc_strings.h"

errc gErrorCatch = ERR_OK;
b8 gErrorFirst = FALSE;

const char* errc_to_string(errc code)
{
    switch(code)
    {   
        // not an error
        case ERR_OK:
            return "Ok, this is not an error. You should never see this message.";

        // core and memory errors
        case ERR_OUT_OF_MEMORY:
            return "Attempted out of Memory access";
        case ERR_DOUBLE_FREE:
            return "Attempted to free memory that was already freed";

        // string errors
        case ERR_STR_BAD_RESIZE:
            return "Bad String Resize arguments, new size must be equal or larger.";

        // File IO errors
        case ERR_UNABLE_TO_OPEN_FILE:
            return "Unable to open file";
        case ERR_FILE_SEEK_ERROR:
            return "File Seek error";

        // assertions
        case ERR_ASSERTION_FAILED:
            return "Assertion failed";

        // tokenizer errors
        case ERR_TOKENIZER_POINTER_OVERFLOW:
            return "Pointer Ran off the end while tokenizing";
        case ERR_UNRECOGNIZED_TOKEN:
            return "Unrecognized token";
        case ERR_TOKEN_OUT_OF_RANGE:
            return "Token is not part of this source file, pointer not in range.";

        // testing
        case ERR_TEST_LEAKED_MEMORY:
            return "Memory tracking finished but allocations are outstanding.\n This indicates code path will leak memory at runtime";
        default: 
            break;
    }

    return "UNKNOWN_ERROR_CODE";
}

b8 gSupressErrors;

void error_print(errc code, const char* C, const char* F, int L)
{
    if(!gSupressErrors)
    {
        if(gErrorFirst)
        {
            fprintf(stderr, "\n");
            gErrorFirst = FALSE;
        }
        fprintf(stderr, "  > " RED("Error") RED(" \"%s\"(%d):") YELLOW(" '%s'") CYAN(" %s:%d\n"), errc_to_string(code), code, C, F, L);
    }
}

void supress_errors()
{
    gSupressErrors = TRUE;
}

void unsupress_errors()
{
    gSupressErrors = FALSE;
}

void setup_error_context()
{
    gErrorCatch = ERR_OK;
    gErrorFirst = TRUE;
    gSupressErrors = FALSE;
}
