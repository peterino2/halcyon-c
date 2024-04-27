#ifndef _HALC_ERRORS_H_
#define _HALC_ERRORS_H_

#include "halc_types.h"

EXTERN_C_BEGIN

// =================== halc_errors =====================================
//  errors in C are really annoying
//
//  in this way "error-able" functions are functions which return an errc
//
//  type. This type will be commonly used and it should be the only typedef 
//  throughout halcyon in general.
//
//  functions returning an errc, must have a default call to the macro 'end';
//
//  eg.
//
//  errc myErrorableFunction (int parameter)
//  {
//      if(parameter == -1)
//      {
//          herror("Negative 1 passed in, this is a big issue");
//      }
//
//      end;
//  }
//
//  end is basically a natural 'return ERR_OK;'
//
//
// any errorable function can be called and their error value can be checked
// 
// if(myErrorableFunction() != ERR_OK) // != 0, or func() are ok as well.
// {
//      printf("myErrorableFunction failed, handling the error case here");
// }
//
//
//  however if the calling convention is also an errorable function you can use the macro
//  'try(...)' which implements an incredibly helpful default handler for 
//  calling error-able function functions.
//
//  eg:
//
//      try(myErrorableFunction(2));
//
//  roughly equivalent to:
//
//      int errorcode = myErrorableFunction();
//      if(errorcode)
//      {
//          raise(errorcode);
//      }
//
//
// there is also _Cleanup() versions of both raise() and try()
//
// which kick the code execution to a cleanup label in your function, 
// useful if you want to leave some code in to back-out side effects on error.
//
// eg:
//
// errc myErrorableFunction(const char* openFile, int expectedSize)
// {
//      char* bufferContents = halloc(expectedSize);
//      int bytesRead = readFileIntoBuffer(openFile, bufferContents, expectedSize);
//
//      if(bytesRead != expectedSize )
//      {
//          raiseCleanup(ERR_UNABLE_TO_OPEN_FILE);
//      }
//
//  cleanup:
//     hfree(bufferContents);
//     end;
// }
//
// you'll notice a limitation here, what if we have more than failable operations 
// that require cleanup? 
//
// eg. 
//
//  try(createFile(&file1, ..))
//  tryCleanup(createFile(&file2, ..))
//  tryCleanup(createFile(&file3, ..)) // if this one fails file2 will be leaked.
//  destroyFile(file3);
//  destroyFile(file2);
// cleanup:
//  destroyFile(file1);
//  
// this is indeed a limitation. these kinds of situations are not easy to deal with in C.
// As the language itself lacks destructors or a defer mechanisim. For this reason, a 
// code smell during code review of this library is the usage of more than two failable
// initializers in one function.
//
// for situations like that I reccomend manual checking of errcs returned from each 
// function along with raise() and manual cleanup is preferred.
//
// eg.
//
// errc error;
//
// if(error = !createFile(&file1, ..))
// {
//    raise(error);
// }
//
// if(error = createFile(&file2, ..))
// {
//    destroyFile(file1);
//    raise(error);
// }
//
// if(error = createFile(&file3, ..))
// {
//    destroyFile(file1);
//    destroyFile(file2);
//    raise(error);
// }
//
// end;
// 
// ======================================================================

typedef int errc;

// not an error
#define ERR_OK 0

// core and memory issues
// note to self: ERR_UNKNOWN should never be used, therefore it shouldn't be implmented.
#define ERR_OUT_OF_MEMORY 100
#define ERR_DOUBLE_FREE 200

// string errors
#define ERR_STR_BAD_RESIZE 1000

// File IO errors
#define ERR_UNABLE_TO_OPEN_FILE 2000
#define ERR_FILE_SEEK_ERROR 2100
#define ERR_INCONSISTENT_FILE_FORMAT 2200

// assertions
#define ERR_ASSERTION_FAILED 3100

#define ERR_UNRECOGNIZED_TOKEN 4100
#define ERR_TOKENIZER_POINTER_OVERFLOW 4200
#define ERR_TOKEN_OUT_OF_RANGE 4300

// testing based error tokens
#define ERR_TEST_LEAKED_MEMORY 101 // codes that end in a 1 indicate they are supposed to only be used by the testing framework.

const char* errc_to_string(errc code);
void error_print(errc code, const char* C, const char* F, int L);

// ==================== Errors Library ==================
#define try(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    return gErrorCatch;\
}

#define tryCleanup(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    goto cleanup;\
}

#define ensure(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    abort();\
} \

#define end return gErrorCatch;
#define end_ok do{gErrorCatch = ERR_OK;}while(0)

// use if your code has a cleanup: section
#define raiseCleanup(X) do{ \
    gErrorCatch = X;\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    goto cleanup; }while(0)

// use only if your code doesn't have a cleanup section
#define raise(X) do {\
        gErrorCatch = X;\
        error_print(gErrorCatch, #X, __FILE__, __LINE__);\
        return gErrorCatch;\
    }while(0)

#define assert(X) if(!(X)) {raise(ERR_ASSERTION_FAILED); }

#define assertCleanup(X) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n"); raiseCleanup(ERR_ASSERTION_FAILED); }

#define assertCleanupMsg(X, FMT, ...) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n with message:\n " FMT, __VA_ARGS__); raiseCleanup(ERR_ASSERTION_FAILED); }

#define assertMsg(X, FMT, ...) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n with message:\n " FMT, __VA_ARGS__); raise(ERR_ASSERTION_FAILED); }

extern errc gErrorCatch;
extern b8 gErrorFirst;

void setup_error_context();

// supresses error printouts.
// this has no impact on error handling code, but it prevents downgrades
void supress_errors();
b8 is_supressed_errors();
void unsupress_errors();

EXTERN_C_END;

#endif
