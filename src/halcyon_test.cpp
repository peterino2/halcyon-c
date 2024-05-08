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
#include "halc_parser.h"

#ifndef NO_TESTS

extern "C" void halc_set_parser_noprint();
extern "C" void halc_clear_parser_noprint();

extern "C" void halc_set_parser_run_verbose();
extern "C" void halc_clear_parser_run_verbose();

// utilities tests
static errc loading_file_test() 
{
    hstr decoded;
    const hstr filePath = HSTR("testfiles/terminals.halc");
    halc_try(load_and_decode_from_file(&decoded, &filePath));
    hstr_free(&decoded);
    halc_end;
}

// ===================== tokenizer tests ========================
static errc tokenizer_directives_basic()
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
    halc_try(tokenize(&ts, &testString, &testFileName));

    for(i32 i = 0; i < ts.len; i += 1)
    { 
        assertCleanupMsg(ts.tokens[i].tokenType == tokens[i], " i == %d ", i);
    }

    halc_assertCleanup(ts.len == arrayCount(tokens));

cleanup:
    ts_free(&ts);
    halc_end;
}


static errc test_ts_matches_expected_stream(const struct tokenStream* ts, i32* tokens, i32 len)
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

    halc_end;
}

static errc tokenizer_full()
{
    const hstr filename = HSTR("testfiles/storySimple.halc");

    hstr fileContents;
    halc_try(load_and_decode_from_file(&fileContents, &filename));
    
    struct tokenStream ts;
    halc_try(tokenize(&ts, &fileContents, &filename));

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


    halc_try(test_ts_matches_expected_stream(&ts, tokens, arrayCount(tokens)));
    const hstr label1 = HSTR("hello");
    const hstr label2 = HSTR("question");
    const hstr comment1 = HSTR("#first comment");
    const hstr storyText = HSTR("I'm going to ask you a question.");

    halc_assertCleanup(arrayCount(tokens) == ts.len);
    halc_assertCleanup(hstr_match(&label1, &ts.tokens[1].tokenView));
    halc_assertCleanup(hstr_match(&label2, &ts.tokens[10].tokenView));
    halc_assertCleanup(hstr_match(&comment1, &ts.tokens[7].tokenView));
    halc_assertCleanup(hstr_match(&storyText, &ts.tokens[15].tokenView));

    halc_assertCleanup(ts.tokens[0].lineNumber == 1);
    halc_assertCleanup(ts.tokens[4].lineNumber == 2);
    halc_assertCleanup(ts.tokens[8].lineNumber == 2);
    halc_assertCleanup(ts.tokens[9].lineNumber == 3);

cleanup:
    ts_free(&ts);
    hstr_free(&fileContents);
    halc_end;
}

static errc tokenizer_test_random_utf8()
{
    hstr filename = HSTR("testfiles/random_utf8.halc");
    hstr content;
    halc_try(load_and_decode_from_file(&content, &filename));

    supress_errors();

    struct tokenStream ts;
    errc errorcode = tokenize(&ts, &content, &filename);

    unsupress_errors();

    halc_assertCleanup(errorcode == ERR_UNRECOGNIZED_TOKEN);

    hstr_free(&content);
    halc_end_ok;

cleanup:
    hstr_free(&content);
    halc_end;
}

static errc test_tokenizer_stress()
{
    const hstr filename = HSTR("testfiles/stress_easy.halc");

    hstr fileContents;
    halc_try(load_and_decode_from_file(&fileContents, &filename));
    
    struct tokenStream ts;
    halc_try(tokenize(&ts, &fileContents, &filename));

    ts_free(&ts);
    hstr_free(&fileContents);
    halc_end;
}

b8 gPrintouts;

static errc token_printouts()
{
    const hstr filename = HSTR("testfiles/storySimple.halc");

    hstr fileContents;
    halc_try(load_and_decode_from_file(&fileContents, &filename));
    
    struct tokenStream ts;
    halc_try(tokenize(&ts, &fileContents, &filename));

    for (i32 i = 0; i < ts.len; i += 1)
    {
        ts_print_token(&ts, i, !gPrintouts, YELLOW_S);
    }

    ts_free(&ts);
    hstr_free(&fileContents);
    return ERR_OK;
}

static errc parse_test_with_string(hstr* testString)
{
    hstr filename = HSTR("no file");
    hstr normalizedTestString;
    
    halc_try(hstr_normalize(testString, &normalizedTestString));

    struct tokenStream ts;
    struct s_graph graph;

    halc_try(tokenize(&ts, &normalizedTestString, &filename));
    halc_try(parse_tokens(&graph, &ts));

    ts_free(&ts);
    hstr_free(&normalizedTestString);
    graph_free(&graph);
    // hfree(graph.nodes, graph.capacity * sizeof(struct s_node));,
    halc_end;
}

// "@directive([with some oddities])\n"
// "$:this is a sample dialogue\n" );
// "homer: give me another dialogue \n"
// "    > give him a real dialogue\n"
// "        homer: thanks for the new dialogue\n"
// "    > nah, don't do that #[123456] why would you pick this\n"
// "        homer: you are the worst\n"
// "$:end of dialogue\n" );

// tests first phase of parsing, converting a tokenstream into a graph of nodes
static errc test_parser_labels()
{
    hstr testString = HSTR(
        "[label]\n"
        "\t[label2 ]\n"
        "[@label2 ] # with a comment, this one should error with unexpected token in @\n"
        "[label2 label2 ] # with a comment, this one should error with unexpected multiple tokens\n"
        "[label2 32132 + - 2 32] absolutely fucked label\n"
        "$: this is some speech to end it with\n"
        );


    halc_try(parse_test_with_string(&testString));

    halc_end;
}

static errc test_parser_directives() 
{
    // halc_set_parser_run_verbose();
    hstr testString = HSTR(
        "[label]\n"
        "@directive([with some oddities])\n"
        "@directive([with some oddities] h jdhskah 2y811 !)\n"
        "@directive()\n" // one label only
        "\n"
        "\n"
        "@goto\n" // special goto directive
        "@goto .\n" // special goto directive
        "@goto region.characters.something\n" // special goto directive
        "$:this is a sample dialogue\n" );

    halc_try(parse_test_with_string(&testString));   
    halc_end;
}

#include <chrono>
#include <iostream>

static errc test_parser_speed()
{
    using namespace std::chrono;
    // halc_set_parser_run_verbose();
    halc_set_parser_noprint();
    const hstr filename = HSTR("testfiles/stress_easy.halc");

    hstr fileContents;
    halc_try(load_and_decode_from_file(&fileContents, &filename));

    i32 i = 500;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    do {
        struct tokenStream ts;
        struct s_graph graph;

        halc_try(tokenize(&ts, &fileContents, &filename));
        halc_try(parse_tokens(&graph, &ts));

        ts_free(&ts);
        graph_free(&graph);
    } while(i-- > 0);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    std::cout << "Time elapsed for test 10000 ast generations " << time_span.count() << " seconds average time: " << time_span.count() / 500 * 1000 << "ms" << std::endl;

    hstr_free(&fileContents);
    halc_end;
}

static errc test_parser_speech()
{
    // halc_set_parser_run_verbose();
    hstr testString = HSTR(
        "[label]\n"
        "@directive([with some oddities])\n"
        "$:this is a sample dialogue\n" 
        "personA: this is another dialogue\n" 
        "PersonB: dialogue2 #[deadbeef] this is a comment with a Lockey\n" 
        "$:this is a sample dialogue # this is just a comment, lockey can be generated\n" 
        );

    halc_try(parse_test_with_string(&testString));   
    halc_end;
}

errc debug_print_sizes()
{
    if(gPrintouts)
    {
        printf("struct anode size = %" PRId64 "\n", (i64)sizeof(struct anode));
    }
    halc_end;
}


// ====================== test registry ======================
struct testEntry {
    const char* testName;
    errc (*testFunc)();
};

// test imports

// halc_strings.c

errc test_hstr_printf();

#define TEST_IMPL(X, DESC) {#X ": " DESC, X}

static struct testEntry gTests[] = {
    TEST_IMPL(debug_print_sizes, "checking sizes of various structs"),
    TEST_IMPL(loading_file_test, "simple file loading test"),
    TEST_IMPL(tokenizer_directives_basic, "testing tokenization of directives"),
    TEST_IMPL(tokenizer_full, "tokenizer full test"),
    TEST_IMPL(tokenizer_test_random_utf8, "testing safety on random utf8 strings being passed in"),
    TEST_IMPL(test_tokenizer_stress, "testing tokenization on a larger set"),
    TEST_IMPL(token_printouts, "debugging token printouts"),
    TEST_IMPL(test_hstr_printf, "testing printf stuff in hstr"),
    TEST_IMPL(test_parser_labels, "parsing tokens into a graph, specifically with error cases for labels"),
    TEST_IMPL(test_parser_directives, "parsing tokens into a graph, specificially testing cases for directives"),
    TEST_IMPL(test_parser_speed, "parses tokens into a graph, specifically measuring speed")
};

static i32 runAllTests()
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
        halc_clear_parser_run_verbose();
        halc_clear_parser_noprint();

        struct allocatorStats stats;
        errc errorCodeFromUntrack = untrack_allocs(&stats);

        if (!errorCode && errorCodeFromUntrack)
        {
            errorCode = errorCodeFromUntrack;
        }

        printf(GREEN("stats - memory remaining at end of test: %0.3lfK (peak: %" "0.3lf" "K) peakAllocations: %" PRId32"\n"), ((double) stats.allocatedSize) / 1000.0, ((double)stats.peakAllocatedSize) / 1000.0, stats.peakAllocationsCount);

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

    i32 results = runAllTests();

    return results;
}
#endif
