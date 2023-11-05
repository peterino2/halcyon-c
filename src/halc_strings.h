#ifndef _HALC_STRINGS_H_
#define _HALC_STRINGS_H_

#include "halc_types.h"
#include "halc_errors.h"

struct hstr{
    char* buffer;
    u32 len;
};
typedef struct hstr hstr;

b8 hstr_match(const hstr* left, const hstr* right);
void hstr_free(hstr* str);
errc hstr_decodeUtf8(const hstr* istr, hstr* ostr);

// Do not count the null terminator as part of the length
#define HSTR(X) {(char*) X, sizeof(X)/sizeof(char) - 1} 

#define arrayCount(X) sizeof(X) / sizeof(X[0])

#define BLACK(X) "\033[0;30m" X "\033[0m" 
#define RED(X) "\033[0;31m" X "\033[0m" 
#define GREEN(X) "\033[0;32m" X "\033[0m" 
#define YELLOW(X) "\033[0;33m" X "\033[0m" 
#define BLUE(X) "\033[0;34m" X "\033[0m" 
#define PURPLE(X) "\033[0;35m" X "\033[0m" 
#define CYAN(X) "\033[0;36m" X "\033[0m" 

#endif
