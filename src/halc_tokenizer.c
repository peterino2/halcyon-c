#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "halc_tokenizer.h"
#include "halc_allocators.h"


errc ts_initialize(struct tokenStream* ts, i32 source_length_hint)
{
    ts->len = 0;
    // I'm estimating an average of 5 tokens every 80 characters.
    // Then doubling that. we can be more conservative but memory feels cheap these days.
    
    ts->capacity = ((source_length_hint / 40) + 1) * 8 * 2;
    halloc(&ts->tokens, ts->capacity * sizeof(struct token));

cleanup:
    end;
}

errc ts_resize(struct tokenStream* ts)
{
    struct token* oldTokens = ts->tokens;
    usize oldCapacity = ts->capacity;

    // resize the array
    ts->capacity *= 2;
    trackAllocs("resize event");
    struct token* newTokens;
    halloc(&newTokens, ts->capacity * sizeof(struct token));

    // copy over data
    for (i32 i = 0; i < oldCapacity; i += 1)
    {
        newTokens[i] = oldTokens[i];
    }
    
    // free old array
    hfree(oldTokens);

    ts->tokens = newTokens;

cleanup:
    end;
}

errc ts_push(struct tokenStream* ts, struct token* tok)
{
    if(ts->len >= ts->capacity)
    {
        fprintf(stderr, "resizing!\n");
        try(ts_resize(ts));
    }

    ts->tokens[ts->len] = *tok;
    ts->len += 1;

    end;
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
    "QUOTE",

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
    HSTR("'"), // DOUBLE_QUOTE,
};

b8 isAlphaNumeric(char ch)
{
    if ((ch >= 'a' && ch <= 'z') || 
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '_')
        )
    {
        return TRUE;
    }
    return FALSE;
}

errc tokenize(struct tokenStream* ts, const hstr* source, const hstr* filename)
{
    trackAllocs("ts_initialize");
    try(ts_initialize(ts, source->len));
    ts->source = *source;

    // initialize a pointer and start walking through the source
    const char* r = source->buffer;
    const char* rEnd = source->buffer + source->len;

    struct tokenizer tokenizer = {ts, TOK_MODE_DEFAULT};
    i32 lineNumber = 1;
    b8 shouldBreak = FALSE;
    b8 directiveParenCount = 0;

    trackAllocs("ts_tokenize");
    while(r < rEnd)
    {
        // comment clause
        if(*r == '#' && !shouldBreak)
        {
            const char* c = r;
            while(*c != '\n' && c < rEnd) c++;

            hstr view = {(char*) r, (u32)(c - r)};

            struct token newToken = {COMMENT, view, *filename, lineNumber};
            try(ts_push(ts, &newToken));
            r += view.len - 1;
            shouldBreak = TRUE;
        }

        if ((*r == ':' || *r == '>' ) && !shouldBreak && directiveParenCount == 0)
        {
            if (*r == ':')
            {
                struct token nt = { COLON, {(char*)r, 1}, *filename, lineNumber };
                try(ts_push(ts, &nt));
            }
            if (*r == '>')
            {
                struct token nt = { R_ANGLE, {(char*)r, 1}, *filename, lineNumber };
                try(ts_push(ts, &nt));
            }
            r += 1;

            while (*r == ' ') r++;

            const char* c = r;
            while (*c != '\n' && *c != '#' && c < rEnd) c++;

            c--;

            while (*c == ' ') c--;

            c++;

            hstr view = { (char*) r, (u32)(c - r)};
            struct token newToken = { STORY_TEXT, view, *filename, lineNumber };
            try(ts_push(ts, &newToken));
            r += view.len;
            if (r > rEnd)
                herror(ERR_TOKENIZER_POINTER_OVERFLOW);
            while (*r == ' ') r++;
            r--;
            shouldBreak = TRUE;
        }

        // terminals clause
        if (!shouldBreak) {
            for (i32 i = 0; i < arrayCount(Terminals); i += 1)
            {
                // build a view based on the current character and look ahead.
                hstr view = { (char*) r, Terminals[i].len };

                // clamp it so we don't look at the null terminator or look past the end.
                if(view.buffer + view.len >= rEnd) 
                {
                    view.len = (u32) (rEnd - view.buffer);
                }

                if(hstr_match(&view, &Terminals[i]))
                {
                    struct token newToken = {i, view, *filename, lineNumber};
                    r += Terminals[i].len - 1;

                    try(ts_push(ts, &newToken));

                    if(i == NEWLINE)
                    {
                        directiveParenCount = 0;
                        lineNumber += 1;
                    }

                    if (i == L_PAREN)
                    {
                        directiveParenCount += 1;
                    }

                    if (i == R_PAREN)
                    {
                        directiveParenCount -= 1;
                    }
                    shouldBreak = TRUE;
                    break;
                }
            }
        }

        // try to build a label instead
        if (!shouldBreak && isAlphaNumeric(*r))
        {
            const char* c = r;
            while (c < rEnd && isAlphaNumeric(*c)) c++;

            hstr view = { (char*) r, (u32)(c - r)};
            struct token newToken = { LABEL, view, *filename, lineNumber };

            try(ts_push(ts, &newToken));
            r += view.len - 1;
            shouldBreak = TRUE;
        }

        if (!shouldBreak)
        {
            fprintf(stderr, "Unknown character: %c\n", *r);
            herror(ERR_UNRECOGNIZED_TOKEN);
        }

        r += 1;
        shouldBreak = FALSE;
    }

    end;

cleanup:
    ts_free(ts);
    end;
}


void ts_free(struct tokenStream* ts)
{
    hfree(ts->tokens);
}
