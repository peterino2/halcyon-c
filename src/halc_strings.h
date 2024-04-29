#ifndef _HALC_STRINGS_H_
#define _HALC_STRINGS_H_

#include "halc_types.h"
#include "halc_errors.h"

struct hstr{
    char* buffer;
    u32 len;
    u32 cap;
};

typedef struct hstr hstr;

b8 hstr_match(const hstr* left, const hstr* right);
void hstr_free(hstr* str);

errc hstr_append(hstr* base, hstr* right);
errc hstr_resize(hstr* ostr, u32 len);

// Gives back a string buffer with normalized line endings, 
// we only support \n line ending and convert 
// all line start spaces to TABs, will raise an halc_error 
// if we detect a line that does not start on a multiple of 4
// spaces
errc hstr_normalize(const hstr* istr, hstr* ostr);

// Do not count the null terminator as part of the length
#define HSTR(X) {(char*) X, sizeof(X)/sizeof(char) - 1, 0} 

#define arrayCount(X) sizeof(X) / sizeof(X[0])

#define BLACK(X) "\033[0;30m" X "\033[0m" 
#define RED(X) "\033[0;31m" X "\033[0m" 
#define GREEN(X) "\033[0;32m" X "\033[0m" 
#define YELLOW(X) "\033[0;33m" X "\033[0m" 
#define BLUE(X) "\033[0;34m" X "\033[0m" 
#define PURPLE(X) "\033[0;35m" X "\033[0m" 
#define CYAN(X) "\033[0;36m" X "\033[0m" 

#define RESET_S "\033[0m" 

#define BLACK_S "\033[0;30m" 
#define RED_S "\033[0;31m" 
#define GREEN_S "\033[0;32m" 
#define YELLOW_S "\033[0;33m" 
#define BLUE_S "\033[0;34m" 
#define PURPLE_S "\033[0;35m" 
#define CYAN_S "\033[0;36m" 

enum Color {
    cBlack,
    cRed,
    cGreen,
    cYellow,
    cBlue,
    cPurple,
    cCyan
};

#endif
