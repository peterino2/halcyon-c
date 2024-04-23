#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
        assertCleanup(ts.tokens[i].tokenType == tokens[i]);
    }

    assertCleanup(ts.len == arrayCount(tokens));

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
    tryCleanup(tokenize(&ts, &fileContents, &filename));

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

    assertCleanup(arrayCount(tokens) == ts.len);
    for (int i = 0; i < ts.len; i += 1)
    {
        assertCleanup(tokens[i] == ts.tokens[i].tokenType);
    }

    const hstr label1 = HSTR("hello");
    assertCleanup(hstr_match(&label1, &ts.tokens[1].tokenView));
    const hstr label2 = HSTR("question");
    assertCleanup(hstr_match(&label2, &ts.tokens[10].tokenView));
    const hstr comment1 = HSTR("#first comment");
    assertCleanup(hstr_match(&comment1, &ts.tokens[7].tokenView));
    const hstr storyText = HSTR("I'm going to ask you a question.");
    assertCleanup(hstr_match(&storyText, &ts.tokens[15].tokenView));

    assertCleanup(ts.tokens[0].lineNumber == 1);
    assertCleanup(ts.tokens[4].lineNumber == 2);
    assertCleanup(ts.tokens[8].lineNumber == 2);
    assertCleanup(ts.tokens[9].lineNumber == 3);
    assertCleanup(ts.tokens[104].lineNumber == 14);

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

    supressErrors();

    struct tokenStream ts;
    errc errorcode = tokenize(&ts, &content, &filename);

    unSupressErrors();

    assertCleanup(errorcode == ERR_UNRECOGNIZED_TOKEN);

    hstr_free(&content);
    end_ok;

cleanup:
    hstr_free(&content);
    end;
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

    end;
}

void parser_free(struct  s_parser* p)
{
    hfree(p->ast, sizeof(struct p_obj) * p->ast_cap);
}

// Minimum matching is 3 segments long
b8 parser_matchDialogue(struct s_parser* p)
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
        // printf(YELLOW("Dialogue!\n"));
        p->t += 3 - 1;
        rv = TRUE;
    }

    return rv;
}

// Minimum matching is 3 segments long
b8 parser_matchLabel(struct s_parser* p)
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
        // printf(YELLOW("LABEL!!\n"));
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
        if(parser_matchDialogue(&p))
        {
            // output a dialogue node to the graph
        }
        else if(parser_matchLabel(&p)) 
        {
            // output a label node to the graph
        }
        p.t++;
    }

    parser_free(&p);

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
    // hfree(graph.nodes, graph.capacity * sizeof(struct s_node));
    end;
}


// ====================== test registry ======================
struct testEntry {
    const char* testName;
    errc (*testFunc)();
};

#define TEST_IMPL(X, DESC) {#X ": " DESC, X}

struct testEntry gTests[] = {
    TEST_IMPL(loading_file_test, "simple file loading test"),
    TEST_IMPL(testing_directives, "testing tokenization of directives"),
    TEST_IMPL(tokenizer_full, "tokenizer full test"),
    TEST_IMPL(test_random_utf8, "testing safety on random utf8 strings being passed in"),
    TEST_IMPL(token_printouts, "debugging token printouts"),
    TEST_IMPL(tokens_into_graph, "parsing tokens into a graph")
};

i32 runAllTests()
{
    i32 failures = 0;
    i32 passes = 0;
    for(int i = 0; i < sizeof(gTests) / sizeof(gTests[0]); i += 1)
    {
        gErrorCatch = ERR_OK;
        printf("Running test: ... %s  ", gTests[i].testName);
        setupErrorContext();
        trackAllocs(gTests[i].testName);
        errc errorCode = gTests[i].testFunc();

        struct allocatorStats stats;
        errc errorCodeFromUntrack = untrackAllocs(&stats);

        if (!errorCode && errorCodeFromUntrack)
        {
            errorCode = errorCodeFromUntrack;
        }

        printf("stats - memory remaining at end of test: %0.3lfK (peak: %" "0.3lf" "K)\n", ((double) stats.allocatedSize) / 1000.0, ((double)stats.peakAllocatedSize) / 1000.0);

        if(errorCode != ERR_OK)
        {
            fprintf(stderr, "> " RED("Test Failed") ": " YELLOW("%s") "\n", gTests[i].testName);
            failures += 1;
        }
        else 
        {
            passes += 1;
            printf("\r                                                    \r");
        }
        unSupressErrors();
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
    const hstr testOut = HSTR("--printout");
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
