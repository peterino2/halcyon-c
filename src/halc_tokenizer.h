#ifndef _HALC_TOKENIZER_H_
#define _HALC_TOKENIZER_H_

#include "halc_types.h"
#include "halc_strings.h"

struct token {
    i32 tokenType;
    hstr tokenView;
    hstr fileName;
    i32 lineNumber;
};

struct tokenStream {
    hstr source; // 16
    struct token* tokens; //8
    i32 len; // 4
    i32 capacity; //4
};

struct tokenizer {
    struct tokenStream* ts;
    i32 state;
    hstr view;
};


errc tokenize(const hstr* source, struct tokenStream* ts);

errc ts_initialize(struct tokenStream* ts, i32 source_length_hint);

errc ts_resize(struct tokenStream* ts);

errc ts_push(struct tokenStream* ts, struct token* tok);

void ts_free(struct tokenStream* ts);

#define TOK_MODE_ERROR -1
#define TOK_MODE_DEFAULT 0
#define TOK_MODE_STORY 1
#define TOK_MODE_LABEL 2

enum tokenType{
    NOT_EQUIV,
    EQUIV,
    LESS_EQ,
    GREATER_EQ,
    L_SQBRACK,
    R_SQBRACK,
    AT,
    L_ANGLE,
    R_ANGLE,
    COLON,
    L_PAREN,
    R_PAREN,
    DOT,
    SPEAKERSIGN,
    SPACE,
    NEWLINE,
    CARRIAGE_RETURN,
    TAB,
    EXCLAMATION,
    EQUALS,
    L_BRACE,
    R_BRACE,
    HASHTAG,
    PLUS,
    MINUS,
    COMMA,
    SEMICOLON,
    AMPERSAND,
    DOUBLE_QUOTE,
    QUOTE,
    LABEL,
    STORY_TEXT,
    COMMENT
};

extern const char* tokenTypeStrings[];
extern const hstr Terminals[];


#endif
