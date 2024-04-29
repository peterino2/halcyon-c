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

#ifndef NO_TESTS
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

errc test_tokenizer_stress()
{
    const hstr filename = HSTR("testfiles/stress_easy.halc");

    hstr fileContents;
    try(load_and_decode_from_file(&fileContents, &filename));
    
    struct tokenStream ts;
    try(tokenize(&ts, &fileContents, &filename));

    ts_free(&ts);
    hstr_free(&fileContents);
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

// append-only bump allocator list of children
//
// basically creating a seperate allocation for each child list 
// within each node is going to be too much of a pain in the ass.
//
// this simple bump allocator is going to be much easier to work wtih
//
// operation: 
//
// - node starts being created
// - node stores end of list as it's start index for it's list of children
// - node writes all of it's children into the list
// - node then stores the end offset
// - then the node is done writing and never gets to write to the list index ever again.
// - node stores the length of the number of children it wrote to
//
// node can get a view of it's children at any time by calling anode_get_children 
// on the index list
struct aindex_list 
{
    i32* children;
    i32 len;
    i32 cap;
};

errc aindex_init(struct aindex_list* list, i32 initialCap)
{
    if(list->cap != 0)
    {
        raise(ERR_UNEXPECTED_REINITIALIZATION);
    }

    list->children = NULL;
    if (initialCap > 0)
    {
        halloc(&list->children, initialCap * sizeof(i32));
    }

    list->len = 0;
    list->cap = initialCap;

    end;
}

errc aindex_push(struct aindex_list* list, i32 newIndex)
{
    if(list->len == list->cap)
    {
        i32 newCap = list->cap * 2;
        hrealloc(&list->children, list->cap * sizeof(i32), newCap, FALSE);
        list->cap = newCap;
    }

    list->children[list->len] = newIndex;
    end;
}

void aindex_free(struct aindex_list* list)
{
    if(list->cap == 0)
    {
        return;
    }

    hfree(list->children, list->cap * sizeof(list->children[0]));
    list->cap = 0;
}


// anode types are an extension of tokenType
enum ANodeType {
    ANODE_SELECTION = COMMENT + 1, // ok
    ANODE_SPEECH, // ok
    ANODE_SEGMENT_LABEL,
    ANODE_EXPRESSION,
    ANODE_GRAPH,
    ANODE_INVALID
};
 
const char* node_id_to_string(i32 id)
{
    if(id <= COMMENT)
    {
        return tok_id_to_string(id);
    }

    switch(id)
    {
        case ANODE_SELECTION:
            return YELLOW("SELECTION");
        case ANODE_SPEECH:
            return YELLOW("SPEECH");
        case ANODE_SEGMENT_LABEL:
            return YELLOW("SEGMENT_LABEL");
        case ANODE_GRAPH:
            return GREEN("GRAPH");
        case ANODE_EXPRESSION:
            return CYAN("EXPRESSION");
    }

    return "BROKEN NODE ID";
}

// index into tokenstream as a token
typedef i32 anode_token_t;

struct anode_selection{
    anode_token_t storyText;
    anode_token_t comment;
    i32 tabCount;
    i32 extension;
    i32 extensionCount;
};

struct anode_speech{
    anode_token_t speaker;
    anode_token_t storyText;
    anode_token_t comment;
    i32 tabCount;
    i32 extension;
    i32 extensionCount;
};

struct anode_directive{
    anode_token_t commandLabel;
    i32 children;
    i32 childrenCount;
    i32 tabCount;
};

struct anode_segment_label {
    anode_token_t label;
    anode_token_t comment;
    i32 tabCount;
};

// goal of parser is to reduce, reduce, reduce until we get a graph
struct anode_graph {
    i32 children; // statically allocated childlist handle
    i32 childrenCount; // number of children in the indexlist
};

// main reduction, used to generate the graph
struct anode_expression {
    // children indexes storage
    // we don't dynamically resize or destroy or unload any of this shit so..
    // we could just have two pointers into a bump-allocated a_index_list

    i32 children; // statically allocated childlist handle
    i32 childrenCount; // number of children in the indexlist

    // reference to start and end tokens
    anode_token_t tokenStart;
    anode_token_t tokenEnd;
};

// ast node
struct anode
{
    i32 index;
    i32 parent; // parent of this node, following this node when we successfully parse should always get us to a graph node.

    enum ANodeType typeTag;
    union NodeData{
        i32 token; // this node is just a token
        struct anode_selection selection;
        struct anode_speech speech;
        struct anode_segment_label label;
        struct anode_expression expression;
        struct anode_directive directive;
        struct anode_graph graph;
    } nodeData;
};


#define PSTATE_DEFAULT 0
#define PSTATE_DIRECTIVE 1

struct s_parser
{
    struct anode* ast; // container for all ast nodes;
    u32 ast_cap;
    u32 ast_len;

    struct aindex_list list; // container for all indexes

    // ast view
    struct anode* ast_view;
    u32 ast_view_len;

    i32 state;

    const struct token* t;
    const struct token* tend;
    const struct tokenStream* ts;

    i32* stack;
    i32 stackCount;
    i32 stackCap;

    i32 tabCount;
};


errc p_print_node(struct s_parser* p, struct anode* n)
{
    printf("index: %" PRId32 " (%s) parent: %" PRId32" (%s)\n", 
            n->index, node_id_to_string(n->typeTag),
            n->parent, node_id_to_string(p->ast[n->parent].typeTag));

    if(n->typeTag <=  COMMENT)
    {
        ts_print_token(p->ts, n->nodeData.token, FALSE, GREEN_S);
    }
    else if(n->typeTag == ANODE_SEGMENT_LABEL)
    {
        hstr label = ts_get_tok(p->ts, n->nodeData.label.label)->tokenView;
        printf("tab: % " PRId32 " label: "YELLOW("%.*s"), n->nodeData.label.tabCount, label.len, label.buffer);
        const struct token* comment = ts_get_tok(p->ts, n->nodeData.label.comment);
        if(comment)
        {
            printf(" comment: "YELLOW("%.*s") "\n", comment->tokenView.len, comment->tokenView.buffer);
        }
        else 
        {
            printf("\n");
        }
    }
    end;
}



#define PARSER_INIT_NODESTACK_SIZE 64
#define PARSER_INIT_NODECOUNT 256

errc parser_new_node(struct s_parser* p, struct anode** newNode)
{
    if(p->ast_len == p->ast_cap)
    {
        u32 newSize = p->ast_cap * sizeof(struct anode) * 2;
        hrealloc(&p->ast, p->ast_cap * sizeof(struct anode), newSize, FALSE);
        p->ast_cap = newSize;
    }

    *newNode = p->ast + p->ast_len;
    (*newNode)->index = p->ast_len;
    (*newNode)->parent = 0;
    (*newNode)->typeTag = ANODE_INVALID;
    p->ast_len += 1;

    end;
}
errc parser_push_stack(struct s_parser* p, struct anode* node); // forward decl

errc parser_init(struct s_parser* p, const struct tokenStream* ts)
{
    p->ast_cap = PARSER_INIT_NODECOUNT;
    halloc(&p->ast, p->ast_cap * sizeof(struct anode));
    p->ast_len = 0;

    p->ast_view_len = 0;
    p->ast_view = p->ast;

    p->state = PSTATE_DEFAULT;

    p->list.cap = 0;
    aindex_init(&p->list, ts->len);

    p->ts = ts;
    p->t = ts->tokens;
    p->tend = ts->tokens + ts->len;

    halloc(&p->stack, PARSER_INIT_NODESTACK_SIZE * sizeof(i32));
    p->stackCap = PARSER_INIT_NODESTACK_SIZE;
    p->stackCount = 0;

    struct anode* newNode;
    try(parser_new_node(p, &newNode));

    // node 0's parent is itself, and is the core s_graph
    newNode->parent = 0;
    newNode->typeTag =  ANODE_GRAPH;
    newNode->nodeData.graph.children = -1;
    newNode->nodeData.graph.childrenCount = 0;
    
    p->tabCount = 0;

    try(parser_push_stack(p, newNode));

    end;
}

void parser_free(struct  s_parser* p)
{
    hfree(p->stack, sizeof(i32) * p->stackCap);
    hfree(p->ast, sizeof(struct anode) * p->ast_cap);
    aindex_free(&p->list);
}

/**
 * overall design spec for a large world
 *
 * project/
 *      dresden/
 *          dresden.halc # encounters and stuff
 *          characters/
 *              steward.halc # talking to the steward in neutral
 *          oneshots/
 * */

/**
 *
$: Does that make sense?
    > No, can you repeat that?
        @goto main_menu_dialogue

    @if(condition = )
    > Yes, I'm ready to start.
        $: Thanks for playing. And good luck!
        @changeRooms(1 content/BreakRoom)
@end
 *
 *
 * shitty ebnf
 *
 * s_graph -> [expression+]
 * 
 * expression -> (speech | selection | directive | segmentLabel)
 *
 * ----------- these guys could be folded into a custom type but it would add complexity and not much perf gain b/c the tab is optional
 * directive -> [TAB+] AT L_PAREN * R_PAREN [COMMENT] NEWLINE
 *
 * speech -> [TAB+] (SPEAKERSIGN | LABEL) COLON STORY_TEXT [COMMENT] NEWLINE
 *
 * selection -> [TAB+] R_ANGLE STORY_TEXT [COMMENT] NEWLINE
 * ------------
 *
 * segmentLabel -> L_SQBRACK LABEL R_SQBRACK NEWLINE
 *
 * are there any disadvantages to this layout? metadata layout is limited without being 
 * recursive descent. however recoverability is really good.
 *
 * s_graph is the main unit of forward parsing.
 *
 * in order of matching: 
 *
 * segmentLabel -> [[hello_world]]
 * selection -> [> hello, how's it going\n]
 * speech -> [$: this is a thing!#[commentthingy] \n]
 * dialogue -> selection | speech
 *  
 */

errc parser_push_stack(struct s_parser* p, struct anode* node)
{
    if(p->stackCount == p->stackCap)
    {
        const i32 newCap = p->stackCap * 2 * sizeof(i32);
        hrealloc(&p->stack, p->stackCap * sizeof(i32), newCap, FALSE);
        p->stackCap = newCap;
    }

    p->stack[p->stackCount] = node->index;
    p->stackCount += 1;
    end;
}

// pops one value off the active stack in the parser and discards it
void pop_stack_discard(struct s_parser* p)
{
    p->stackCount -= 1;
}

errc match_reduce_space(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;
    if(p->ast[stackEnd[-1]].typeTag == SPACE)
    {
        pop_stack_discard(p);
        *didReduce = TRUE;
    }
    end;
}

errc match_reduce_tab(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;
    if(p->ast[stackEnd[-1]].typeTag == TAB)
    {
        pop_stack_discard(p);
        *didReduce = TRUE;
        p->tabCount += 1;
    }
    end;
}

errc match_reduce_errors(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;

    i32 len = stackEnd - stackStart;
    if(p->ast[stackStart[0]].typeTag == ANODE_GRAPH)
    {
        // check if there are errors.
        //
        // one key indication of an error is walking past a newline still on the stack
        //

        i32 newLineCount = 0;
        for(int i = 0; i < len; i+=1)
        {
            if(p->ast[stackStart[i]].typeTag == NEWLINE)
            {
                newLineCount += 1;
            }
            else {
                
                if(newLineCount >= 1)
                {
                    fprintf(stderr, RED("Unable to parse line, evicting previous tokens\n")); // another stray thought. logging should really be something that we give hooks for the end user code to call into.
                                         // directly writing to stderr or stdout should be fully redirectible
                    // raise(ERR_UNABLE_TO_PARSE_LINE); // What to do here. 
                                                     // We can report the error, evict the current line and just continue parsing from there.
                                                     // Because this is such a recoverable error, maybe 
                                                     // its better to not use raise() here.
                    
                    i32 currentStackEnd = stackEnd[-1];
                    while(p->ast[p->stack[p->stackCount-1]].typeTag <= COMMENT)
                    {
                        pop_stack_discard(p);
                    }

                    parser_push_stack(p, &p->ast[currentStackEnd]);

                    *didReduce = TRUE;
                    break;
                }
            }
        }
    }
    end;
}

errc match_reduce_expression(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    end;
}

// attempts to create an ast node for segment label, if successful, pops the stack by the token size count 
// and also pushes a new ast node for segment label.
//
// this is also generally how the match reduce functions work for this parser, they are also responsible for individually 
// evicting bad values
//
// this will also be over commented to describe how this is implemented
errc match_reduce_segment_label(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    // calculate length of the window as that's quite useful usually
    i32 len = stackEnd - stackStart;

    // by default our results return false
    // when we return out true, the reduce function will exit the current iteration and conduct a new iteration 
    // this will continue until no further reductions are done.
    *didReduce = FALSE;
    if(len < 3)
    {
        end;
    }

    // this clause can only match if there is a newline at the end of it and we see a label within the newline
    // now one question... how can we support spaces?
    //
    // well that can be done with another reduction, if there is a space on the top of the stack, 
    //
    // like:
    //
    // L_SQBRACK SPACE | LABEL 
    //
    // we can reduce SPACE by popping it from the stack immediately
    //
    // also what happens if we have some goofy things like this?
    //
    // @if([my_butt])\n
    //
    if(p->ast[stackStart[len - 1]].typeTag == NEWLINE &&
       p->ast[stackStart[0]].typeTag == L_SQBRACK && 
       p->ast[stackStart[1]].typeTag == LABEL && 
       p->ast[stackStart[2]].typeTag == R_SQBRACK
    )
    {
        struct anode* label;
        try(parser_new_node(p, &label));

        p->ast[stackStart[0]].parent = label->index;
        p->ast[stackStart[1]].parent = label->index;
        p->ast[stackStart[2]].parent = label->index;

        label->typeTag = ANODE_SEGMENT_LABEL;
        label->nodeData.label.label = p->ast[stackStart[1]].nodeData.token;
        label->nodeData.label.tabCount =  p->tabCount;

        if(len > 4)
        {
            if(p->ast[stackStart[3]].typeTag != COMMENT)
            {
                fprintf(stderr, RED("Unexpected token when matching a segment, expected a comment or a newline\nIssue with token:\n"));
                if(p->ast[stackStart[3]].typeTag <= COMMENT)
                {
                    ts_print_token(p->ts, p->ast[stackStart[3]].nodeData.token, FALSE, RED_S);
                }
                
                raise(ERR_UNEXPECTED_TOKEN);
            }
            label->nodeData.label.comment = p->ast[stackStart[3]].nodeData.token;
        }
        else {
            label->nodeData.label.comment = -1;
        }

        // pop 3 times
        for(i32 i = len; i > 0; i--)
            pop_stack_discard(p);

        parser_push_stack(p, label);
        p_print_node(p, label);

        p->tabCount = 0;

        *didReduce = TRUE;
        end;
    }

    end;
}


#define PARSER_MATCH_REDUCE(X) if(!didReduce){X;\
    if(didReduce){\
        continueReducing = TRUE;\
        stackStart = p->stack;}}
        //assertMsg(p->stackCount != oldStackCount, "function " #X " reported reduce, but stackCount did not change, this will result in an infinite loop %d", p->stackCount);\
        oldStackCount = p->stackCount;}} // this assert might not be correct, cross that bridge when i get there.
                                        // it's possible that there may be reductions which merely change the 
                                        // state of existing nodes on the stack. rather than changing the stack


errc parser_reduce(struct s_parser* p)
{
    
    assertMsg(p->stackCount > 0, "parser active stack was %d", p->stackCount);
    
    b8 continueReducing = TRUE;
    b8 didReduce = FALSE;

    i32* stackStart = p->stack + p->stackCount - 1;
    i32* stackEnd = p->stack + p->stackCount;

    i32 oldStackCount = p->stackCount;
    
    // stray thought:
    // if we encounter an impoossible sequence, we should be able to evict the broken sequence 
    // and continue parsing as normal.

    while(continueReducing)
    {
        continueReducing = FALSE;
        stackEnd =  p->stack + p->stackCount;
        stackStart = stackEnd - 1;
        while(stackStart >= p->stack)
        {
            // printf("stackCount %ld \n", stackEnd - stackStart);
            // if anything gets popped from the stack, restart reduction

            PARSER_MATCH_REDUCE(match_reduce_segment_label(p, stackStart, stackEnd, &didReduce))
            PARSER_MATCH_REDUCE(match_reduce_space(p, stackStart, stackEnd, &didReduce))
            PARSER_MATCH_REDUCE(match_reduce_errors(p, stackStart, stackEnd, &didReduce))
            PARSER_MATCH_REDUCE(match_reduce_tab(p, stackStart, stackEnd, &didReduce))

            stackStart -= 1;
        }
    }

    end;
}

errc parser_advance(struct s_parser* p)
{
    // - look at the next token and produce emit a new astNode, 
    
    struct anode* newNode;
    try(parser_new_node(p, &newNode));
    printf("nodeIndex = %d\n", newNode->index);

    // - add it to ast, then add it to stack
    newNode->parent = -1;
    newNode->typeTag = (enum ANodeType) p->t->tokenType; // type tags <= COMMENT are Token Terminals
    newNode->nodeData.token = (i32)(p->t - p->ts->tokens);

    printf("token offset %d\n", newNode->nodeData.token);
    ts_print_token(p->ts, newNode->nodeData.token, FALSE, GREEN_S);

    try(parser_push_stack(p, newNode));

    printf("stack: ");

    for(i32 i = 0; i < p->stackCount; i += 1)
    {
        printf(" %s ", node_id_to_string(p->ast[p->stack[i]].typeTag));
    }
    printf("\n");

    // - call parser_reduce to merge the active stack if merges are possible
    parser_reduce(p);

    // - errors: 
    //      if we have anything other than just a single s_graph at the end.
    p->t++;

    end;
}

errc parse_tokens(struct s_graph* graph, const struct tokenStream* ts)
{
    struct s_parser p;
    try(parser_init(&p, ts));

    while (p.t < p.tend)
    {
        tryCleanup(parser_advance(&p));
    }

    printf("parser lenght = %d\n", p.ast_len);

    // graph construction is should now happen here.
cleanup:
    parser_free(&p);

    end;
}

// tests first phase of parsing, converting a tokenstream into a graph of nodes
errc test_parser_labels()
{
    hstr testString = HSTR(
        "[label]\n"
        "\t[label2 ]\n"
        "[@label2 ] # with a comment, this one should error with unexpected token in @\n"
        "[label2 label2 ] # with a comment, this one should error with unexpected multiple tokens\n"
        "[label2 32132 + - 2 32] absolutely fucked label\n"
        "@directive([with some oddities])\n"
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

    struct tokenStream ts;

    try(tokenize(&ts, &normalizedTestString, &filename));

    struct s_graph graph;
    try(parse_tokens(&graph, &ts));

    ts_free(&ts);
    hstr_free(&normalizedTestString);
    // hfree(graph.nodes, graph.capacity * sizeof(struct s_node));
    end;

}

errc debug_print_sizes()
{
    if(gPrintouts)
    {
        printf("struct anode size = %" PRId64 "\n", (i64)sizeof(struct anode));
    }
    end;
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

struct testEntry gTests[] = {
    TEST_IMPL(debug_print_sizes, "checking sizes of various structs"),
    TEST_IMPL(loading_file_test, "simple file loading test"),
    TEST_IMPL(tokenizer_directives_basic, "testing tokenization of directives"),
    TEST_IMPL(tokenizer_full, "tokenizer full test"),
    TEST_IMPL(tokenizer_test_random_utf8, "testing safety on random utf8 strings being passed in"),
    TEST_IMPL(test_tokenizer_stress, "testing tokenization on a larger set"),
    TEST_IMPL(token_printouts, "debugging token printouts"),
    TEST_IMPL(test_hstr_printf, "testing printf stuff in hstr"),
    TEST_IMPL(test_parser_labels, "parsing tokens into a graph, specifically with error cases for labels")
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

    printf("sizeof(struct token) = %ld", sizeof(struct token));

    return results;
}
#endif
