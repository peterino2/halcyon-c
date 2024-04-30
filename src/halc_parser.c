#include "halc_parser.h"
#include "halc_allocators.h"
#include "halc_errors.h"

#include <stdio.h>
#include <inttypes.h>

static errc aindex_init(struct aindex_list* list, i32 initialCap)
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

static errc aindex_push(struct aindex_list* list, i32 newIndex)
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

static void aindex_free(struct aindex_list* list)
{
    if(list->cap == 0)
    {
        return;
    }

    hfree(list->children, list->cap * sizeof(list->children[0]));
    list->cap = 0;
}

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

static errc p_print_node(struct s_parser* p, struct anode* n)
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

static errc parser_new_node(struct s_parser* p, struct anode** newNode)
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

#define PARSER_INIT_NODESTACK_SIZE 64
#define PARSER_INIT_NODECOUNT 256

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

static errc parser_push_stack(struct s_parser* p, struct anode* node); // forward decl for parser_init

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

static errc parser_push_stack(struct s_parser* p, struct anode* node)
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
static void pop_stack_discard(struct s_parser* p)
{
    p->stackCount -= 1;
}

static errc match_reduce_space(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;
    if(p->ast[stackEnd[-1]].typeTag == SPACE)
    {
        pop_stack_discard(p);
        *didReduce = TRUE;
    }
    end;
}

static errc match_reduce_tab(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
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

static errc match_reduce_errors(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;

    i32 len = (i32)(stackEnd - stackStart);
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

static errc match_reduce_expression(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
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
static errc match_reduce_segment_label(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    // calculate length of the window as that's quite useful usually
    i32 len = (i32)(stackEnd - stackStart);

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
        stackStart = (i32*)p->stack;}}
        //assertMsg(p->stackCount != oldStackCount, "function " #X " reported reduce, but stackCount did not change, this will result in an infinite loop %d", p->stackCount);
        // oldStackCount = p->stackCount;}} // this assert might not be correct, cross that bridge when i get there.
                                        // it's possible that there may be reductions which merely change the 
                                        // state of existing nodes on the stack. rather than changing the stack


static errc parser_reduce(struct s_parser* p)
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

static errc parser_advance(struct s_parser* p)
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
