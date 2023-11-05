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
    ok;
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
    ts_free(&ts);
    ok;
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
        // printf("expected: %d %s got: %s\n", i, tokenTypeStrings[tokens[i]], tokenTypeStrings[ts.tokens[i].tokenType]);
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

    ts_free(&ts);
    hstr_free(&fileContents);
    ok;
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
