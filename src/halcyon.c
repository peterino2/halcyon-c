#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define _CRT_SECURE_NO_WARNINGS
// =================== my c helpers ==============

typedef int errc;

#define ERR_OK 0
#define ERR_OUT_OF_MEMORY 1
#define ERR_UNKNOWN 2

// string based ones
#define ERR_STR_BAD_RESIZE 1000

// File IO based ones
#define ERR_UNABLE_TO_OPEN_FILE 2000
#define ERR_FILE_SEEK_ERROR 2001

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
        default: 
            break;
    }

    return "UNKNOWN_ERROR_CODE";
}

typedef char b8;
typedef char i8;
typedef unsigned char u8;
typedef char bool;

#define true 1
#define false 0

typedef int i32;
typedef unsigned int u32;

typedef long long i64;
typedef long long isize;
typedef unsigned long long u64;
typedef unsigned long long usize;

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
#define herror(X) do{ gErrorCatch = X; errorPrint(gErrorCatch, #X, __FILE__, __LINE__); return gErrorCatch; }while(0)

errc gErrorCatch;
bool gErrorFirst;

void errorPrint(errc code, const char* C, const char* F, int L)
{
    if(gErrorFirst)
    {
        fprintf(stderr, "\n");
        gErrorFirst = false;
    }
    fprintf(stderr, "  > Error \"%s\": '%s' %s:%d\n", errcToString(code), C, F, L);
}

// ==================== Allocators ======================

// These are the only two functions I'm going to support in my allocator interface.
struct allocator{
    void* (*malloc_fn) (usize);
    void (*free_fn) (void*);
};

struct allocator gDefaultAllocator;

// uses default cstandard library allocators
errc setupDefaultAllocator() 
{
    gDefaultAllocator.malloc_fn = malloc;
    gDefaultAllocator.free_fn = free;

    ok;
}

void setupErrorContext()
{
    gErrorCatch = ERR_OK;
    gErrorFirst = true;
}

// allows you to bring whatever malloc and free function you want
void setupCustomDefaultAllocator(
    void* (*malloc_fn) (usize),
    void (*free_fn) (void*))
{
    gDefaultAllocator.malloc_fn = malloc_fn;
    gDefaultAllocator.free_fn = free_fn;
}

// halcyon default allocation function
errc halloc_advanced(void** ptr, usize size)
{
    *ptr = gDefaultAllocator.malloc_fn(size);
    if(!*ptr)
    {
        herror(ERR_OUT_OF_MEMORY);
    }

    ok;
}

#define halloc(ptr, size) halloc_advanced((void**) ptr, size)

// halcyon default free function
void hfree(void* ptr)
{
    gDefaultAllocator.free_fn(ptr);
}

// ==================== CRT Safeties ====================
FILE* h_fopen(const char* filePath, const char* opts) 
{
    FILE* file;
#if defined(_WIN32) || defined(_WIN64)
    errno_t err = fopen_s(&file, filePath, opts);
    if (err != 0) {
        return NULL;
    }
#else
    file = fopen(filePath, opts);
#endif

    return file;
}

// ==================== Strings Library =================
// primitve, a string view
struct hstr{
    char* buffer;
    u32 len;
};
typedef struct hstr hstr;

errc loadFile(hstr* out, const hstr* filePath)
{
    errc error_code = ERR_OK; 

    FILE* file = h_fopen(filePath->buffer, "rb");
    if(!file)
    {
        herror(ERR_UNABLE_TO_OPEN_FILE);
    }

    // seek for the size of the file
    if(fseek(file, 0, SEEK_END) != 0)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    isize fileSize = ftell(file);
    if(fileSize == -1)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    char* buffer;
    try(halloc(&buffer, fileSize));

    if(fseek(file, 0, SEEK_SET) != 0)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    usize bytesRead = fread(buffer, 1, fileSize, file);
    if(bytesRead != fileSize)
    {
        fprintf(stderr,
            "read file but didn't get the right size? expected %lld got %lld", 
            fileSize, bytesRead);
    }

    out->buffer = buffer;
    out->len = bytesRead;

    fclose(file);
    ok;
    
exitCloseFile:
    fclose(file);
    herror(error_code);
    ok;
}

// try to convert a given binary buffer into equivalent utf8 code points
// also converts \r\n into \n
//
// This does not fixup in place, this allocates a new buffer into outBuffer and outLen
errc decodeUtf8(const hstr* istr, hstr* ostr)
{
    // Allocate working buffer
    try(halloc(&ostr->buffer, istr->len));
    ostr->len = 0;

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

    ostr->len = w - ostr->buffer;

    herror(ERR_UNKNOWN);

    ok;
}

#define HSTR(X) {(char*) X, sizeof(X)/sizeof(char)}

void hstr_free(hstr* str) 
{
    if(str->len == 0)
    {
        return;
    }
    hfree(str->buffer);
    str->len = 0;
    str->buffer = NULL;
}

errc loadAndDecodeFromFile(hstr* out, const hstr* filePath)
{
    hstr file;
    try(loadFile(&file, filePath));
    try(decodeUtf8(&file, out));

    hstr_free(&file);
    ok;
}

// ============= unit tests =================

errc loading_file_test() 
{
    hstr decoded;
    const hstr filePath = HSTR("src/halcyon.c");
    try(loadAndDecodeFromFile(&decoded, &filePath));
    hstr_free(&decoded);
    ok;
}

// ====================== test registry ======================
struct testEntry {
    const char* testName;
    errc (*testFunc)();
};

struct testEntry gTests[] = {
    {"simple file loading test", loading_file_test},
};

errc setupTestHarness() 
{
    setupDefaultAllocator(); // TODO swap this with a testing allocator
    ok;
}

void runAllTests()
{
    for(int i = 0; i < sizeof(gTests) / sizeof(gTests[0]); i += 1)
    {
        printf("Running test: %s... ", gTests[i].testName);
        if(gTests[i].testFunc() != ERR_OK)
        {
            printf("Test Failed! %s\n", gTests[i].testName);
        }
        else 
        {
            printf("Test Passed!\n");
        }
    }
}

int main(int argc, char** argv)
{
    setupErrorContext();
    ensure(setupTestHarness());
    runAllTests();
}
