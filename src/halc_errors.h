#ifndef _HALC_ERRORS_H_
#define _HALC_ERRORS_H_

#include "halc_types.h"

EXTERN_C_BEGIN

// =====
// 
// there are two main classes of issues, these are the ones that are covered by halc_errors these are the ones emitted by errc
// 
//
// However within halcyon b/c this is intended to be a runtime, we should never crash on any kind of 
// garbage input from the user.
//
// I think these should be called exceptions. eg if someone feeds in 
//
// [!THISIs mY Label!!!]
//
// this won't compile and the label is now malformed, this will raise a malformed label warning
//
// I know a lot of programs generally overuse warnings and 
//
// =====

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
//      halc_end;
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
//      halc_try(myErrorableFunction(2));
//
//  roughly equivalent to:
//
//      int errorcode = myErrorableFunction();
//      if(errorcode)
//      {
//          halc_raise(errorcode);
//      }
//
//
// there is also _Cleanup() versions of both halc_raise() and halc_try()
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
//          halc_raiseCleanup(ERR_UNABLE_TO_OPEN_FILE);
//      }
//
//  cleanup:
//     hfree(bufferContents);
//     halc_end;
// }
//
// you'll notice a limitation here, what if we have more than failable operations 
// that require cleanup? 
//
// eg. 
//
//  halc_try(createFile(&file1, ..))
//  halc_tryCleanup(createFile(&file2, ..))
//  halc_tryCleanup(createFile(&file3, ..)) // if this one fails file2 will be leaked.
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
// function along with halc_raise() and manual cleanup is preferred.
//
// eg.
//
// errc error;
//
// if(error = !createFile(&file1, ..))
// {
//    halc_raise(error);
// }
//
// if(error = createFile(&file2, ..))
// {
//    destroyFile(file1);
//    halc_raise(error);
// }
//
// if(error = createFile(&file3, ..))
// {
//    destroyFile(file1);
//    destroyFile(file2);
//    halc_raise(error);
// }
//
// halc_end;
// 
// ======================================================================

typedef int errc;

// not an error
#define ERR_OK 0

// core and memory issues
// note to self: ERR_UNKNOWN should never be used, therefore it shouldn't be implmented.
#define ERR_OUT_OF_MEMORY 100
#define ERR_DOUBLE_FREE 200
#define ERR_REALLOC_SHRUNK_WHEN_NOT_ALLOWED 300
#define ERR_BAD_REALLOC_PARAMETERS 400

// string errors
#define ERR_STR_BAD_RESIZE 1000
#define ERR_STR_OPERATION_ON_STATIC_HSTR 1100

// File IO errors
#define ERR_UNABLE_TO_OPEN_FILE 2000
#define ERR_FILE_SEEK_ERROR 2100
#define ERR_INCONSISTENT_FILE_FORMAT 2200


// assertions
#define ERR_ASSERTION_FAILED 3100

#define ERR_UNRECOGNIZED_TOKEN 4100
#define ERR_TOKENIZER_POINTER_OVERFLOW 4200
#define ERR_TOKEN_OUT_OF_RANGE 4300

#define ERR_UNEXPECTED_REINITIALIZATION 4400

// parser specific tokens

#define ERR_UNEXPECTED_TOKEN 5100
#define ERR_UNABLE_TO_PARSE_LINE 5200

// testing based error tokens
#define ERR_TEST_LEAKED_MEMORY 101 // codes that end in a 1 indicate they are supposed to only be used by the testing framework.

const char* errc_to_string(errc code);
void error_print(errc code, const char* C, const char* F, int L);

// ==================== Errors Library ==================
#define halc_try(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    return gErrorCatch;\
}

#define halc_tryCleanup(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    goto cleanup;\
}

#define halc_ensure(X) if((gErrorCatch = X)) {\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    abort();\
} \

#define halc_end return gErrorCatch;
#define halc_end_ok do{gErrorCatch = ERR_OK;}while(0)

// use if your code has a cleanup: section
#define halc_raiseCleanup(X) do{ \
    gErrorCatch = X;\
    error_print(gErrorCatch, #X, __FILE__, __LINE__);\
    goto cleanup; }while(0)

// use only if your code doesn't have a cleanup section
#define halc_raise(X) do {\
        gErrorCatch = X;\
        error_print(gErrorCatch, #X, __FILE__, __LINE__);\
        return gErrorCatch;\
    }while(0)

#define halc_assert(X) if(!(X)) { halc_raise(ERR_ASSERTION_FAILED); }

#define halc_assertCleanup(X) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n"); halc_raiseCleanup(ERR_ASSERTION_FAILED); }

#define assertCleanupMsg(X, FMT, ...) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n with message:\n " FMT, __VA_ARGS__); halc_raiseCleanup(ERR_ASSERTION_FAILED); }

#define assertMsg(X, FMT, ...) if(!(X)) { fprintf(stderr, "Assertion failed: " RED(#X) "\n with message:\n " FMT, __VA_ARGS__); halc_raise(ERR_ASSERTION_FAILED); }

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
