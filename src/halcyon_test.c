#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "halc_types.h"
#include "halc_errors.h"
#include "halc_allocators.h"
#include "halc_files.h"
#include "halc_strings.h"
#include "halc_tokenizer.h"

errc comment_test() 
{
    fprintf(stderr, "\n");
    hstr testString = HSTR("    # this is a comment\"'\n # this is another comment");
    hstr testFileName = HSTR("no file");
    i32 tokens[] = {
        SPACE,
        SPACE,
        SPACE,
        SPACE,
        COMMENT,
        NEWLINE,
        SPACE,
        COMMENT,
    };
    i32 lineNumbers[] = {
        1,
        1,
        1,
        1,
        1,
        1,
        2,
        2,
    };
    struct tokenStream ts;
    try(tokenize(&ts, &testString, &testFileName));

    assert(ts.len > 0);
    for(i32 i = 0; i < ts.len; i += 1)
    {
        assert(ts.tokens[i].tokenType == tokens[i]);
        assert(ts.tokens[i].lineNumber == lineNumbers[i]);
    }

    ts_free(&ts);
    ok;
}

errc loading_file_test() 
{
    fprintf(stderr, "\n");
    hstr decoded;
    const hstr filePath = HSTR("testfiles/terminals.halc");
    try(loadAndDecodeFromFile(&decoded, &filePath));
    hstr_free(&decoded);
    ok;
}

errc tokenizer_test() 
{
    fprintf(stderr, "\n");
    hstr testString = HSTR("[]()&<!= = # \"'\n");
    hstr testFileName = HSTR("no file");
    i32 tokens[] = {
        L_SQBRACK,
        R_SQBRACK,
        L_PAREN,
        R_PAREN,
        AMPERSAND,
        L_ANGLE,
        NOT_EQUIV,
        SPACE,
        EQUALS,
        SPACE,
        COMMENT,
        NEWLINE
    };
    struct tokenStream ts;
    try(tokenize(&ts, &testString, &testFileName));

    assert(ts.len > 0);
    for(i32 i = 0; i < ts.len; i += 1)
    {
        assert(ts.tokens[i].tokenType == tokens[i]);
    }

    ts_free(&ts);
    ok;
}

errc testing_labels()
{
    hstr testString = HSTR("storytext'\n storytext2 # this is another comment");
    hstr testFileName = HSTR("no file");
    i32 tokens[] = {
        LABEL,
        QUOTE,
        NEWLINE,
        SPACE,
        LABEL,
        SPACE,
        COMMENT
    };
    struct tokenStream ts;
    try(tokenize(&ts, &testString, &testFileName));
    assert(ts.len == arrayCount(tokens));

    for(i32 i = 0; i < ts.len; i += 1)
        assert(ts.tokens[i].tokenType == tokens[i]);

    ok;
}

// ====================== test registry ======================
struct testEntry {
    const char* testName;
    errc (*testFunc)();
};

struct testEntry gTests[] = {
    {"simple file loading test", loading_file_test},
    {"tokenizer test all terminals", tokenizer_test},
    {"testing comment detection test", comment_test},
    {"testing labels", testing_labels},
};

i32 runAllTests()
{
    i32 failures = 0;
    i32 passes = 0;
    for(int i = 0; i < sizeof(gTests) / sizeof(gTests[0]); i += 1)
    {
        printf("Running test: ... %s", gTests[i].testName);
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
        untrackAllocs();
    }

    printf("total: %d\npassed: %d\nfailed: %d\n", passes + failures, passes, failures);

    return failures;
}

int main(int argc, char** argv)
{
    setupDefaultAllocator(); // TODO swap this with a testing allocator
    setupErrorContext();

    const hstr trackAllocs = HSTR("-a");
    for(i32 i = 0; i < argc; i += 1)
    {
        hstr arg = {argv[i], strlen(argv[i])};
        if(hstr_match(&arg, &trackAllocs))
        {
            enableAllocationTracking();
        }
    }

    return runAllTests();
}
