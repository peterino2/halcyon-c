#ifndef _HALC_ERRORS_H_
#define _HALC_ERRORS_H_

#include "halc_types.h"

// =================== halc_errors ==============
typedef int errc;

#define ERR_OK 0
#define ERR_OUT_OF_MEMORY 1
#define ERR_UNKNOWN 2

// string based ones
#define ERR_STR_BAD_RESIZE 1000

// File IO based ones
#define ERR_UNABLE_TO_OPEN_FILE 2000
#define ERR_FILE_SEEK_ERROR 2001

// assertions
#define ERR_ASSERTION_FAILED 3000

const char* errcToString(errc code);
void errorPrint(errc code, const char* C, const char* F, int L);

// ==================== Errors Library ==================
#define try(X) if((gErrorCatch = X)) {\
    errorPrint(gErrorCatch, #X, __FILE__, __LINE__);\
    return gErrorCatch;\
}

#define ensure(X) if((gErrorCatch = X)) {\
    errorPrint(gErrorCatch, #X, __FILE__, __LINE__);\
    abort();\
} \

#define ok return ERR_OK;
#define herror(X) do{ \
    gErrorCatch = X;\
    errorPrint(gErrorCatch, #X, __FILE__, __LINE__);\
    return gErrorCatch; }while(0)

#define assert(X) if(!(X)) {herror(ERR_ASSERTION_FAILED); }

extern errc gErrorCatch;
extern b8 gErrorFirst;

void setupErrorContext();

#endif
