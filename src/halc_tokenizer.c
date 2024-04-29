#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "halc_tokenizer.h"
#include "halc_allocators.h"


errc ts_initialize(struct tokenStream* ts, i32 source_length_hint)
{
    ts->len = 0;
    // I'm estimating an average of 5 tokens every 80 characters.
    // Then doubling that. We can be far more conservative but memory feels cheap these days.
    
    ts->capacity = ((source_length_hint / 40) + 1) * 8 * 2;
    halloc(&ts->tokens, ts->capacity * sizeof(struct token));

    end;
}

errc ts_resize(struct tokenStream* ts)
{
    struct token* oldTokens = ts->tokens;
    usize oldCapacity = ts->capacity;

    // resize the array
    ts->capacity *= 2;
    track_allocs("resize event");
    struct token* newTokens;
    halloc(&newTokens, ts->capacity * sizeof(struct token)); // FIXME_GOOD

    // copy over data
    for (i32 i = 0; i < oldCapacity; i += 1)
    {
        newTokens[i] = oldTokens[i];
    }
    
    // free old array
    hfree(oldTokens, sizeof(struct token));

    ts->tokens = newTokens;

    end;
}

errc ts_push(struct tokenStream* ts, struct token* tok)
{
    if(ts->len >= ts->capacity)
    {
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

// temporary struct created during tokenize()
// to represent the current state of the tokenize operation
struct tokenizer {

    // arguments
    struct tokenStream* ts;
    const hstr* source;
    const hstr* filename;

    const char* r; // read pointer
    const char* rEnd; // end of source read pointer
    const char* c; // end of read source pointer
    b8 directiveParenCount; // nested directives parentheses count 
    i32 lineNumber; // current line number in source
    i32 state;
};

errc tokenizer_advance(struct tokenizer* t)
{
    b8 shouldBreak = FALSE;

    const hstr* filename = t->filename;
    const hstr* source = t->source;

    // comment clause
    if(*t->r == '#' && !shouldBreak)
    {
        t->c = t->r;
        while(*t->c != '\n' && t->c < t->rEnd) t->c++;

        hstr view = {(char*) t->r, (u32)(t->c - t->r)};

        struct token newToken = {COMMENT, view, *filename, t->lineNumber};
        try(ts_push(t->ts, &newToken));
        t->r += view.len - 1;
        shouldBreak = TRUE;
    }

    if ((*t->r == ':' || *t->r == '>' ) && !shouldBreak && t->directiveParenCount == 0)
    {
        if (*t->r == ':')
        {
            struct token nt = { COLON, {(char*)t->r, 1}, *filename, t->lineNumber };
            try(ts_push(t->ts, &nt));
        }
        if (*t->r == '>')
        {
            struct token nt = { R_ANGLE, {(char*)t->r, 1}, *filename, t->lineNumber };
            try(ts_push(t->ts, &nt));
        }
        t->r += 1;

        while (*t->r == ' ') t->r++;

        t->c = t->r;
        while (*t->c != '\n' && *t->c != '#' && t->c < t->rEnd) t->c++;

        t->c--;

        while (*t->c == ' ') t->c--;

        t->c++;

        hstr view = { (char*) t->r, (u32)(t->c - t->r)};
        struct token newToken = { STORY_TEXT, view, *filename, t->lineNumber };
        try(ts_push(t->ts, &newToken));
        t->r += view.len;
        if (t->r > t->rEnd)
        {
            if(!is_supressed_errors())
            {
                fprintf(stderr, RED("Tokenizer pointer overflow: \n"));
                if(t->ts->len > 0)
                {
                    fprintf(stderr, "Last token parsed: ");
                    ts_print_token(t->ts, t->ts->len - 1, FALSE, RED_S);
                }
            }
            raise(ERR_TOKENIZER_POINTER_OVERFLOW);
        }
        while (*t->r == ' ') t->r++;
        t->r--;
        shouldBreak = TRUE;
    }

    // terminals clause
    if (!shouldBreak) {
        for (i32 i = 0; i < arrayCount(Terminals); i += 1)
        {
            // build a view based on the current character and look ahead.
            hstr view = { (char*) t->r, Terminals[i].len };

            // clamp it so we don't look at the null terminator or look past the end.
            if(view.buffer + view.len >= t->rEnd) 
            {
                view.len = (u32) (t->rEnd - view.buffer);
            }

            if(hstr_match(&view, &Terminals[i]))
            {
                struct token newToken = {i, view, *filename, t->lineNumber};
                t->r += Terminals[i].len - 1;

                try(ts_push(t->ts, &newToken));

                if(i == NEWLINE)
                {
                    t->directiveParenCount = 0;
                    t->lineNumber += 1;
                }

                if (i == L_PAREN)
                {
                    t->directiveParenCount += 1;
                }

                if (i == R_PAREN)
                {
                    t->directiveParenCount -= 1;
                }
                shouldBreak = TRUE;
                break;
            }
        }
    }

    // try to build a label instead
    if (!shouldBreak && isAlphaNumeric(*t->r))
    {
        t->c = t->r;
        while (t->c < t->rEnd && isAlphaNumeric(*t->c)) t->c++;

        hstr view = { (char*) t->r, (u32)(t->c - t->r)};
        struct token newToken = { LABEL, view, *filename, t->lineNumber };

        try(ts_push(t->ts, &newToken));
        t->r += view.len - 1;
        shouldBreak = TRUE;
    }

    if (!shouldBreak)
    {
        if(!is_supressed_errors())
        {
            fprintf(stderr, RED("Unrecognized token: \n"));
            if(t->ts->len > 0)
            {
                fprintf(stderr, "Last token parsed\n");
                ts_print_token(t->ts, t->ts->len - 1, FALSE, RED_S);
            }
        }
        raise(ERR_UNRECOGNIZED_TOKEN);
    }

    t->r += 1;
    shouldBreak = FALSE;

    end;
}

errc tokenize(struct tokenStream* ts, const hstr* source, const hstr* filename)
{
    track_allocs("ts_initialize");
    try(ts_initialize(ts, source->len));
    ts->source = *source;
    ts->filename = *filename;

    struct tokenizer t;

    t.ts = ts;
    t.source = source;
    t.filename = filename;

    t.state = TOK_MODE_DEFAULT;

    // initialize a pointer and start walking through the source
    t.r = source->buffer;
    t.rEnd = source->buffer + source->len;
    
    t.lineNumber = 1;
    t.directiveParenCount = 0;

    while(t.r < t.rEnd)
    {
        tryCleanup(tokenizer_advance(&t));
    }

    end;

cleanup:
    ts_free(ts);
    end;
}

void ts_free(struct tokenStream* ts)
{
    hfree(ts->tokens, sizeof(struct tokenStream) * ts->capacity);
}

errc tok_get_sourceline(const struct token* tok, const hstr* source, hstr* out, struct tok_view* offsets)
{
    // find the line of the source file that the token resides on and print out the line.
    i32 linelen = 0;
    out->buffer = NULL;
    out->len = 0;

    const char* l = tok->tokenView.buffer;
    const char* sEnd = source->buffer + source->len;

    if (l > sEnd) raiseCleanup(ERR_TOKEN_OUT_OF_RANGE);
    if (l < source->buffer) raiseCleanup(ERR_TOKEN_OUT_OF_RANGE);

    if (*tok->tokenView.buffer == '\n')
    {
        l--;
        while (l > source->buffer && *l != '\n') l--;
        l += 1;

        out->buffer = (char*) l;
        out->len = (u32)(tok->tokenView.buffer - l);

        offsets->tok_start = (u32) (tok->tokenView.buffer - l);
        offsets->tok_end = offsets->tok_start + 1;

        return ERR_OK;
    }

    // walk backwards until we hit the line start
    while (l > source->buffer && *l != '\n') l--;
    if(*l == '\n')
        l += 1;

    const char* lstart = l;
    offsets->tok_start = (u32) (tok->tokenView.buffer - lstart);
    offsets->tok_end = offsets->tok_start + tok->tokenView.len - 1;

    l = tok->tokenView.buffer + tok->tokenView.len - 1;

    // walk forward until we hit line end
    while (l < sEnd && *l != '\n') l++;

    const char* lend = l;

    out->buffer = (char*) lstart;
    out->len = (u32)(lend - lstart);

cleanup:
    end;
}

errc ts_print_token_inner(const struct tokenStream* ts, struct token tok, b8 dryRun, const char* color)
{
    hstr sl;
    struct tok_view offsets;
    try(tok_get_sourceline(&tok, &ts->source, &sl, &offsets));

    if (dryRun)
    {
        return ERR_OK;
    }

    printf("token at: %.*s \n", ts->filename.len, ts->filename.buffer);

    printf("line %6d: ", tok.lineNumber);

    for(u32 j = 0; j < sl.len; j += 1)
    {
        if(sl.buffer[j] == '\t')
        {
            printf("-->|");
        }
        else
        {
            putchar(sl.buffer[j]);
        }
    }

    printf("\n             ");

    for (i32 i = 0; i < offsets.tok_start; i++)
    {
        if(sl.buffer[i] == '\t')
        {
            printf("   ");
        }
        printf(" ");
    }

    printf("%s", color);
    for (i32 i = offsets.tok_start; i <= offsets.tok_end; i++)
    {
        if(sl.buffer[i] == '\t')
        {
            printf("^^^");
        }
        printf("^");
    }
    printf(RESET_S);

    printf("%s(%d)\n",tokenTypeStrings[tok.tokenType], tok.tokenType);
    end;
}

errc ts_print_token(const struct tokenStream* ts, const i32 index, b8 dryRun, const char* color)
{
    if(index >= ts->len || index < 0)
    {
        fprintf(stderr, YELLOW("Warning: attempted to print invalid token reference in stream: %" PRId32), index);
        end;
    }

    struct token tok = ts->tokens[index];
    try(ts_print_token_inner(ts, tok, dryRun, color));
    end;
}

// we can easily implement a multi_tok version of this which takes two tokens and merges them from start to finish,
// assuming there's no difference between their lines

const char* tok_id_to_string(i32 id)
{
    if(id < sizeof(tokenTypeStrings) / sizeof(tokenTypeStrings[0]))
    {
        return tokenTypeStrings[id];
    }
    return "UNKNOWN_TOKEN_TYPE";
}

const char* ts_get_token_as_buffer(const struct tokenStream* ts, const i32 index)
{
    return ts->tokens[index].tokenView.buffer;
}

const struct token* ts_get_tok(const struct tokenStream* ts, i32 index)
{
    if(index == -1 ||  index >= ts->len)
    {
        return NULL;
    }

    return ts->tokens + index;
}















