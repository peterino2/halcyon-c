#include <string.h>
#include <stdlib.h>

#include "halc_tokenizer.h"
#include "halc_allocators.h"


errc ts_initialize(struct tokenStream* ts, i32 source_length_hint)
{
    ts->len = 0;
    // I'm estimating an average of 5 tokens every 80 characters.
    // Then doubling that. we can be more conservative but memory feels cheap these days.
    
    ts->capacity = ((source_length_hint / 80) + 1) * 5 * 2;
    halloc(&ts->tokens, ts->capacity * sizeof(struct tokenStream));

    ok;
}

errc ts_resize(struct tokenStream* ts)
{
    struct token* oldTokens = ts->tokens;
    usize oldCapacity = ts->capacity;

    // resize the array
    ts->capacity *= 2;
    trackAllocs("resize event");
    try(halloc(&ts->tokens, ts->capacity * sizeof(struct tokenStream)));

    // copy over data
    memcpy(ts->tokens, oldTokens, oldCapacity * sizeof(struct tokenStream));
    
    // free old array
    hfree(oldTokens);

    ok;
}

errc ts_push(struct tokenStream* ts, struct token* tok)
{
    if(ts->len + 1 > ts->capacity)
    {
        try(ts_resize(ts));
    }

    *(ts->tokens + ts->len) = *tok;
    ts->len += 1;

    ok;
}

const char* tokenTypeStrings[] = {
    "NOT_EQUIV",
    "EQUIV",
    "LESS_EQ",
    "GREATER_EQ",
    "L_SQBRACK",
    "R_SQBRACK",
    "AT",
    "L_ANGLE",
    "R_ANGLE",
    "COLON",
    "L_PAREN",
    "R_PAREN",
    "DOT",
    "SPEAKERSIGN",
    "SPACE",
    "NEWLINE",
    "CARRIAGE_RETURN",
    "TAB",
    "EXCLAMATION",
    "EQUALS",
    "L_BRACE",
    "R_BRACE",
    "HASHTAG",
    "PLUS",
    "MINUS",
    "COMMA",
    "SEMICOLON",
    "AMPERSAND",
    "DOUBLE_QUOTE",

    "LABEL",
    "STORY_TEXT",
    "COMMENT"
};

const hstr Terminals[] = {
    HSTR("!="), // NOT_EQUIV
    HSTR("=="), // EQUIV
    HSTR("<="), // LESS_EQ
    HSTR(">="), // GREATER_EQ
    HSTR("["),  // L_SQBRACK
    HSTR("]"),  // R_SQBRACK
    HSTR("@"),  // AT
    HSTR("<"),  // L_ANGLE
    HSTR(">"),  // R_ANGLE
    HSTR(":"),  // COLON
    HSTR("("),  // L_PAREN,
    HSTR(")"),  // R_PAREN,
    HSTR("."),  // DOT,
    HSTR("$"),  // SPEAKERSIGN,
    HSTR(" "),  // SPACE,
    HSTR("\n"), // NEWLINE,
    HSTR("\r"), // CARRIAGE_RETURN, - this one is an error
    HSTR("\t"), // TAB,
    HSTR("!"),  // EXCLAMATION,
    HSTR("="),  // EQUALS,
    HSTR("{"),  // L_BRACE,
    HSTR("}"),  // R_BRACE,
    HSTR("#"),  // HASHTAG,
    HSTR("+"),  // PLUS,
    HSTR("-"),  // MINUS,
    HSTR(","),  // COMMA,
    HSTR(";"),  // SEMICOLON,
    HSTR("&"),  // AMPERSAND,
    HSTR("\""), // DOUBLE_QUOTE,
};

errc tokenize(const hstr* source, struct tokenStream* ts)
{
    trackAllocs("ts_initialize");
    try(ts_initialize(ts, source->len));
    ts->source = *source;

    // initialize a pointer and start walking through the source
    const char* r = source->buffer;
    const char* rEnd = source->buffer + source->len;

    struct tokenizer tokenizer = {ts, TOK_MODE_DEFAULT, {ts->source.buffer, 0}};

    trackAllocs("ts_tokenize");
    while(r < rEnd)
    {
        // rules for what is a token.
        // - comments
        // - story text
        // - terminals

        tokenizer.view.len += 1;
        for(i32 i = 0; i < arrayCount(Terminals); i += 1)
        {
            if(hstr_match(&tokenizer.view, &Terminals[i]))
            {
                struct token newToken = {i, tokenizer.view, HSTR("no file"), 0};
                tokenizer.view.buffer = (char*) (r + 1);
                tokenizer.view.len = 0;

                try(ts_push(ts, &newToken));
                break;
            }
        }
        r += 1;
    }

    ok;
}

void ts_free(struct tokenStream* ts)
{
    hfree(ts->tokens);
}
