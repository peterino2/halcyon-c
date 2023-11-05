#ifndef _HALC_STRINGS_H_
#define _HALC_STRINGS_H_

#include "halc_types.h"
#include "halc_errors.h"

struct hstr{
    char* buffer;
    u32 len;
};
typedef struct hstr hstr;

b8 hstr_debug_print(const hstr* str);
b8 hstr_match(const hstr* left, const hstr* right);
void hstr_free(hstr* str);
errc hstr_decodeUtf8(const hstr* istr, hstr* ostr);

// Do not count the null terminator as part of the length
#define HSTR(X) {(char*) X, sizeof(X)/sizeof(char) - 1} 

#define arrayCount(X) sizeof(X) / sizeof(X[0])

#endif
