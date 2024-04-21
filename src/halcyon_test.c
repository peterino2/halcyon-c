#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "halc_types.h"
#include "halc_errors.h"
#include "halc_allocators.h"
#include "halc_files.h"
#include "halc_strings.h"
#include "halc_tokenizer.h"

// utilities tests
errc loading_file_test() 
{
    hstr decoded;
    const hstr filePath = HSTR("testfiles/terminals.halc");
    try(loadAndDecodeFromFile(&decoded, &filePath));
    hstr_free(&decoded);
    end;
}

// ===================== tokenizer tests ========================
errc testing_directives()
{
    hstr testString = HSTR("[helloworld]\n@setVar(wutang clan coming at you)\n @jumpIf(x >= 2)");
    hstr testFileName = HSTR("no file");
    i32 tokens[] = {
        L_SQBRACK, LABEL, R_SQBRACK, NEWLINE,

        AT, LABEL, L_PAREN, 
        LABEL, SPACE,
        LABEL, SPACE,
        LABEL, SPACE,
        LABEL, SPACE,
        LABEL, R_PAREN,
        NEWLINE,

        SPACE,
        AT,
        LABEL,
        L_PAREN,
        LABEL,
        SPACE,
        GREATER_EQ,
        SPACE,
        LABEL,
        R_PAREN
    };
    struct tokenStream ts;
    try(tokenize(&ts, &testString, &testFileName));

    for(i32 i = 0; i < ts.len; i += 1)
    { 
        assert(ts.tokens[i].tokenType == tokens[i]);
    }

    assert(ts.len == arrayCount(tokens));

cleanup:
    ts_free(&ts);
    end;
}

errc tokenizer_full()
{
    const hstr filename = HSTR("testfiles/storySimple.halc");

    hstr fileContents;
    try(loadAndDecodeFromFile(&fileContents, &filename));
    
    struct tokenStream ts;
    try(tokenize(&ts, &fileContents, &filename));

    i32 tokens[] = {
        L_SQBRACK,
        LABEL,
        R_SQBRACK,
        NEWLINE,

        SPEAKERSIGN,
        COLON,
        STORY_TEXT,
        COMMENT,
        NEWLINE,

        L_SQBRACK,
        LABEL,
        R_SQBRACK,
        NEWLINE,

        SPEAKERSIGN,
        COLON,
        STORY_TEXT,
        NEWLINE,

        COLON,
        STORY_TEXT,
        NEWLINE,

        SPACE, SPACE, SPACE, SPACE,
        R_ANGLE, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,
        
        SPACE, SPACE, SPACE, SPACE,
        R_ANGLE, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
        LABEL, COLON, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE,
        R_ANGLE, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,

        SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
        AT, LABEL, SPACE, LABEL, NEWLINE,

        AT, LABEL, NEWLINE
    };

    assert(arrayCount(tokens) == ts.len);
    for (int i = 0; i < ts.len; i += 1)
    {
        assert(tokens[i] == ts.tokens[i].tokenType);
    }

    const hstr label1 = HSTR("hello");
    assert(hstr_match(&label1, &ts.tokens[1].tokenView));
    const hstr label2 = HSTR("question");
    assert(hstr_match(&label2, &ts.tokens[10].tokenView));
    const hstr comment1 = HSTR("#first comment");
    assert(hstr_match(&comment1, &ts.tokens[7].tokenView));
    const hstr storyText = HSTR("I'm going to ask you a question.");
    assert(hstr_match(&storyText, &ts.tokens[15].tokenView));

    assert(ts.tokens[0].lineNumber == 1);
    assert(ts.tokens[4].lineNumber == 2);
    assert(ts.tokens[8].lineNumber == 2);
    assert(ts.tokens[9].lineNumber == 3);
    assert(ts.tokens[104].lineNumber == 14);

cleanup:
    ts_free(&ts);
    hstr_free(&fileContents);
    end;
}

errc test_random_utf8()
{
    hstr filename = HSTR("testfiles/random_utf8.halc");
    hstr content;
    try(loadAndDecodeFromFile(&content, &filename));

    struct tokenStream ts;
    supressErrors();
    assert(tokenize(&ts, &content, &filename) == ERR_UNRECOGNIZED_TOKEN);

cleanup:
    hstr_free(&content);
    return ERR_OK;
}

b8 gPrintouts;

errc token_printouts()
{
    const hstr filename = HSTR("testfiles/storySimple.halc");

    hstr fileContents;
    try(loadAndDecodeFromFile(&fileContents, &filename));
    
    struct tokenStream ts;
    try(tokenize(&ts, &fileContents, &filename));

    for (i32 i = 0; i < ts.len; i += 1)
    {
        ts_print_token(&ts, i, !gPrintouts, YELLOW_S);
    }

    ts_free(&ts);
    hstr_free(&fileContents);
    return ERR_OK;
}

// ====================== storygraph ========================

struct s_node
{
    struct tokenStream* ts;
    i32 index;
};

struct s_graph {
    struct s_node* nodes;
    i32 capacity;
    i32 len;
};

struct p_obj
{
    b8 x;
};

struct s_parser
{
    // ast
    struct p_obj* ast;
    u32 ast_cap;
    u32 ast_len;

    // ast view
    struct p_obj* ast_view;
    u32 ast_view_len;

    const struct token* t;
    const struct token* tend;
    const struct tokenStream* ts;
};

errc parser_init(struct s_parser* p, const struct tokenStream* ts)
{
    p->ast_cap = 256;
    halloc(&p->ast, p->ast_cap * sizeof(struct p_obj));
    p->ast_len = 0;

    p->ast_view = p->ast;
    p->ast_view_len = 0;

    p->ts = ts;
    p->t = ts->tokens;
    p->tend = ts->tokens + ts->len;

cleanup:
    end;
}

// Minimum matching is 3 segments long
b8 parser_match_dialogue(struct s_parser* p)
{
    if ((p->tend - p->t) < 3)
    {
        return FALSE;
    }

    b8 rv = FALSE;

    if (p->t[0].tokenType == SPEAKERSIGN || p->t[0].tokenType == LABEL &&
        p->t[1].tokenType == COLON &&
        p->t[2].tokenType == STORY_TEXT)
    {
        printf(YELLOW("Dialogue!\n"));
        p->t += 3 - 1;
        rv = TRUE;
    }

    return rv;
}

// Minimum matching is 3 segments long
b8 parser_match_label(struct s_parser* p)
{
    if ((p->tend - p->t) < 3)
    {
        return FALSE;
    }

    b8 rv = FALSE;

    if (p->t[0].tokenType == L_SQBRACK &&
        p->t[1].tokenType == LABEL &&
        p->t[2].tokenType == R_SQBRACK)
    {
        printf(YELLOW("LABEL!!\n"));
        p->t += 3 - 1;
        rv = TRUE;
    }

    return rv;
}

errc parse_tokens(struct s_graph* graph, const struct tokenStream* ts)
{
    struct s_parser p;
    try(parser_init(&p, ts));

    while (p.t < p.tend)
    {
        // go through each possiblity of a match, and check if it is each one.
        if(parser_match_dialogue(&p))
        {
            // output a dialogue node to the graph
        }
        else if(parser_match_label(&p)) 
        {
            // output a label node to the graph
        }
        p.t++;
    }

    end;
}

errc tokens_into_graph()
{
    hstr testString = HSTR(
        "[label]\n"
        "$:this is a sample dialogue\n" 
        "$:this is another dialogue\n" );
    hstr filename = HSTR("no file");

    struct tokenStream ts;
    try(tokenize(&ts, &testString, &filename));

    struct s_graph graph;
    try(parse_tokens(&graph, &ts));

    ts_free(&ts);
    end;
}


// ====================== test registry ======================
struct testEntry {
    const char* testName;
    errc (*testFunc)();
};

struct testEntry gTests[] = {
    {"simple file loading test", loading_file_test},
    {"testing directives", testing_directives},
    {"tokenizer full test", tokenizer_full},
    {"random utf8 safety",  test_random_utf8},
    {"debugging token printouts",  token_printouts},
    {"parsing tokens into a graph", tokens_into_graph}
};

i32 runAllTests()
{
    i32 failures = 0;
    i32 passes = 0;
    for(int i = 0; i < sizeof(gTests) / sizeof(gTests[0]); i += 1)
    {
        gErrorCatch = ERR_OK;
        printf("Running test: ... %s\n", gTests[i].testName);
        setupErrorContext();
        trackAllocs(gTests[i].testName);
        if(gTests[i].testFunc() != ERR_OK)
        {
            printf("Test Failed! %s\n", gTests[i].testName);
            failures += 1;
        }
        else 
        {
            passes += 1;
            printf("\r                                                    \r");
        }
        unSupressErrors();
        untrackAllocs();
    }

    printf("total: %d\npassed: %d\nfailed: %d\n", passes + failures, passes, failures);

    if (failures == 0)
    {
        printf(GREEN("All tests passed! :)"));
    }
    else 
    {
        printf(RED("Tests have failed!!\n"));
    }

    return failures;
}

int main(int argc, char** argv)
{
    setupDefaultAllocator(); // TODO swap this with a testing allocator
    setupErrorContext();

    const hstr trackAllocs = HSTR("-a");
    const hstr doPrintouts = HSTR("--printout");
    gPrintouts = FALSE;

    for(i32 i = 0; i < argc; i += 1)
    {
        hstr arg = {argv[i], (u32)strlen(argv[i])};
        if(hstr_match(&arg, &trackAllocs))
        {
            enableAllocationTracking();
        }

        if(hstr_match(&arg, &doPrintouts))
        {
            gPrintouts = TRUE;
        }
    }

    return runAllTests();
}
