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
    try(load_and_decode_from_file(&decoded, &filePath));
    hstr_free(&decoded);
    end;
}

// ===================== tokenizer tests ========================
errc tokenizer_directives_basic()
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
        assertCleanupMsg(ts.tokens[i].tokenType == tokens[i], " i == %d ", i);
    }

    assertCleanup(ts.len == arrayCount(tokens));

cleanup:
    ts_free(&ts);
    end;
}


errc test_ts_matches_expected_stream(const struct tokenStream* ts, i32* tokens, i32 len)
{
    for(i32 i = 0; i < len; i+=1)
    {
        if(ts->tokens[i].tokenType != tokens[i])
        {
            ts_print_token(ts, i, FALSE, RED_S);
        }
        assertMsg(ts->tokens[i].tokenType == tokens[i], " i == %d expected %s got %s", i, 
            tok_id_to_string(tokens[i]), 
            tok_id_to_string(ts->tokens[i].tokenType));
    }

    end;
}

errc tokenizer_full()
{
    const hstr filename = HSTR("testfiles/storySimple.halc");

    hstr fileContents;
    try(load_and_decode_from_file(&fileContents, &filename));
    
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

        TAB,
        R_ANGLE, STORY_TEXT, NEWLINE,

        TAB, TAB,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,
        
        TAB,
        R_ANGLE, STORY_TEXT, NEWLINE,

        TAB, TAB,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,

        TAB, TAB,
        LABEL, COLON, STORY_TEXT, NEWLINE,

        TAB,
        R_ANGLE, STORY_TEXT, NEWLINE,

        TAB, TAB,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,

        TAB, TAB,
        AT, LABEL, SPACE, LABEL, NEWLINE,

        AT, LABEL, NEWLINE
    };


    try(test_ts_matches_expected_stream(&ts, tokens, arrayCount(tokens)));

    assertCleanup(arrayCount(tokens) == ts.len);

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

cleanup:
    ts_free(&ts);
    hstr_free(&fileContents);
    end;
}

errc tokenizer_test_random_utf8()
{
    hstr filename = HSTR("testfiles/random_utf8.halc");
    hstr content;
    try(load_and_decode_from_file(&content, &filename));

    supress_errors();

    struct tokenStream ts;
    errc errorcode = tokenize(&ts, &content, &filename);

    unsupress_errors();

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
    try(load_and_decode_from_file(&fileContents, &filename));
    
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

// ast node
struct a_node
{
    b8 x;
};

struct s_parser
{
    struct a_node* ast; // backing storage for asts
    u32 ast_cap;
    u32 ast_len;

    // ast view
    struct a_node* ast_view;
    u32 ast_view_len;

    const struct token* t;
    const struct token* tend;
    const struct tokenStream* ts;
};

errc parser_init(struct s_parser* p, const struct tokenStream* ts)
{
    p->ast_cap = 256;
    halloc(&p->ast, p->ast_cap * sizeof(struct a_node));
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
    hfree(p->ast, sizeof(struct a_node) * p->ast_cap);
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

/**
 * shitty ebnf
 *
 * s_graph -> [expression+]
 * 
 * expression -> (dialogue | directive | extension) [COMMENT]
 * directive -> AT L_PAREN * R_PAREN NEWLINE
 *
 * dialogue -> speech | selection
 *
 * speech -> [TAB] (SPEAKERSIGN | LABEL) COLON STORY_TEXT NEWLINE
 *
 * selection -> 
 */

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

    parser_free(&p);

    end;
}

// tests first phase of parsing, converting a tokenstream into a graph of nodes
errc tokens_into_graph()
{
    hstr testString = HSTR(
        "[label]\n"
        "$:this is a sample dialogue\n" 
        "homer: give me another dialogue \n"
        "    > give him a real dialogue\n"
        "        homer: thanks for the new dialogue\n"
        "    > nah, don't do that #[123456] why would you pick this\n"
        "        homer: you are the worst\n"
        "$:end of dialogue\n" );
    hstr filename = HSTR("no file");

    hstr normalizedTestString;
    
    try(hstr_normalize(&testString,&normalizedTestString));

    i32 tokens[] = {
        L_SQBRACK, LABEL, R_SQBRACK, NEWLINE,
        SPEAKERSIGN, COLON, STORY_TEXT, NEWLINE,
        LABEL, COLON, STORY_TEXT, NEWLINE,
        TAB, R_ANGLE, STORY_TEXT, NEWLINE,
        TAB, TAB, LABEL, COLON, STORY_TEXT, NEWLINE,
        TAB, R_ANGLE, STORY_TEXT, COMMENT, NEWLINE,
        TAB, TAB, LABEL, COLON, STORY_TEXT, NEWLINE,
        SPEAKERSIGN, COLON, STORY_TEXT,
    };

    struct tokenStream ts;

    try(tokenize(&ts, &normalizedTestString, &filename));

    for (i32 i = 0; i < ts.len; i += 1)
    {
        ts_print_token(&ts, i, FALSE, GREEN_S);
    }

    try(test_ts_matches_expected_stream(&ts, tokens, sizeof(tokens) / sizeof(tokens[0])));

    struct s_graph graph;
    try(parse_tokens(&graph, &ts));

    ts_free(&ts);
    hstr_free(&normalizedTestString);
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
    TEST_IMPL(tokenizer_directives_basic, "testing tokenization of directives"),
    TEST_IMPL(tokenizer_full, "tokenizer full test"),
    TEST_IMPL(tokenizer_test_random_utf8, "testing safety on random utf8 strings being passed in"),
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
        setup_error_context();
        track_allocs(gTests[i].testName);
        errc errorCode = gTests[i].testFunc();

        struct allocatorStats stats;
        errc errorCodeFromUntrack = untrack_allocs(&stats);

        if (!errorCode && errorCodeFromUntrack)
        {
            errorCode = errorCodeFromUntrack;
        }

        printf(GREEN("stats - memory remaining at end of test: %0.3lfK (peak: %" "0.3lf" "K)\n"), ((double) stats.allocatedSize) / 1000.0, ((double)stats.peakAllocatedSize) / 1000.0);

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
        unsupress_errors();
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
    setup_default_allocator(); // TODO swap this with a testing allocator
    setup_error_context();

    const hstr trackAllocs = HSTR("-a");
    const hstr doPrintouts = HSTR("--printout");
    const hstr testOut = HSTR("--printout");
    gPrintouts = FALSE;

    for(i32 i = 0; i < argc; i += 1)
    {
        hstr arg = {argv[i], (u32)strlen(argv[i])};
        if(hstr_match(&arg, &trackAllocs))
        {
            enable_allocation_tracking();
        }

        if(hstr_match(&arg, &doPrintouts))
        {
            gPrintouts = TRUE;
        }
    }

    return runAllTests();
}
