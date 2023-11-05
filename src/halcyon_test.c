#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "halc_types.h"
#include "halc_errors.h"
#include "halc_allocators.h"
#include "halc_files.h"
#include "halc_strings.h"
#include "halc_tokenizer.h"

errc loading_file_test() 
{
    fprintf(stderr, "\n");
    hstr decoded;
    const hstr filePath = HSTR("src/halcyon_test.c");
    try(loadAndDecodeFromFile(&decoded, &filePath));
    hstr_free(&decoded);
    ok;
}

errc tokenizer_test() 
{
    fprintf(stderr, "\n");
    hstr filePath = HSTR("testfiles/terminals.halc");

    hstr decoded;
    try(loadAndDecodeFromFile(&decoded, &filePath));

    struct tokenStream ts;
    try(tokenize(&decoded, &ts));

    assert(ts.len > 0);
    assert(ts.tokens[0].tokenType == L_SQBRACK);
    assert(ts.tokens[1].tokenType == R_SQBRACK);

    ts_free(&ts);
    hstr_free(&decoded);
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
