#include "halc_parser.h"
#include "halc_allocators.h"
#include "halc_errors.h"

#include <stdio.h>
#include <inttypes.h>

static b8 gParserRunVerbose;
static b8 gParserRunNoPrint;

struct anode_list_ref {
    i32* start;
    i32* listEnd;
};

void halc_set_parser_noprint() 
{
    gParserRunNoPrint = TRUE;
}

void halc_clear_parser_noprint() 
{
    gParserRunNoPrint = FALSE;
}

void halc_set_parser_run_verbose() 
{
    gParserRunVerbose = TRUE;
}

void halc_clear_parser_run_verbose()
{
    gParserRunVerbose = FALSE;
}

static i32 p_getTokenFromNode(struct s_parser* p, i32 node)
{
    return p->ast[node].nodeData.token;
}


static void print_error_at_node(struct s_parser* p, i32 node, const char* errorText)
{
    fprintf(stderr, "\n");
    p_print_node(p, p->ast + node, RED_S);

    fprintf(stderr, RED("Error: %s \n"), errorText);
}

static errc aindex_init(struct aindex_list* list, i32 initialCap)
{
    if(list->cap != 0)
    {
        halc_raise(ERR_UNEXPECTED_REINITIALIZATION);
    }

    list->children = NULL;
    if (initialCap > 0)
    {
        halloc(&list->children, initialCap * sizeof(i32));
    }

    list->len = 0;
    list->cap = initialCap;

    halc_end;
}

static errc p_getTokenLabelText(struct s_parser* p, i32 node, hstr *gotoString)
{
    // assert(p_getTypeTag(p, node) == LABEL);

    *gotoString = p->ts->tokens[p->ast[node].nodeData.token].tokenView;

    halc_end;
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
    halc_end;
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

// kind of a lie, all this does is return the current end length of the anode
static errc p_push_index_list_entry(struct s_parser* p, i32 newIndex)
{
    halc_try(aindex_push(&p->list, newIndex));
    halc_end;
}

static i32 p_get_index_list_start(struct s_parser* p)
{
    return p->list.len;
}

static void p_assign_parent(struct s_parser* p, i32 target, i32 newParent)
{
    p->ast[target].parent = newParent;
}

static errc p_create_index_list(struct s_parser* p, i32* stackStart, i32* stackEnd, struct anode_list_alloc* newList)
{
    // executive decision. I dont think we're going to create any kind of index list with 
    // a set of tokens longer than 1000, 
    halc_assert(stackEnd - stackStart < 1000);

    newList->entry = p_get_index_list_start(p);
    newList->count = 0;

    while(stackStart < stackEnd)
    {
        newList->count += 1;
        p_push_index_list_entry(p, *stackStart);
        stackStart += 1;
    }
    halc_end;
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
        case ANODE_EXPRESSION:
            return CYAN("EXPRESSION");
        case ANODE_EXTENSION:
            return YELLOW("EXTENSION");
        case ANODE_DIRECTIVE:
            return YELLOW("DIRECTIVE");
        case ANODE_GOTO:
            return PURPLE("GOTO");
        case ANODE_END:
            return PURPLE("END");
        case ANODE_GRAPH:
            return GREEN("GRAPH");
    }

    return RED("BROKEN_NODE_ID");
}

errc p_print_node(struct s_parser* p, struct anode* n, const char* pointerColor)
{
    if(n->parent >= 0 )
    {
        printf("index: %" PRId32 " (%s) parent: %" PRId32" (%s)\n", 
                n->index, node_id_to_string(n->typeTag),
                n->parent, node_id_to_string(p->ast[n->parent].typeTag));
    }
    else 
    {
        printf("index: %" PRId32 " (%s)\n", 
                n->index, node_id_to_string(n->typeTag));
    }

    if(n->typeTag <=  COMMENT)
    {
        ts_print_token(p->ts, n->nodeData.token, FALSE, pointerColor);
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
    halc_end;
}

static errc parser_new_node(struct s_parser* p, struct anode** newNode)
{
    if(p->ast_len == p->ast_cap)
    {
        u32 newSize = HALC_MAX(p->ast_cap * 2, 16);
        hrealloc(&p->ast, p->ast_cap * sizeof(struct anode), newSize * sizeof(struct anode), FALSE);
        p->ast_cap = newSize;
    }

    *newNode = p->ast + p->ast_len;
    (*newNode)->index = p->ast_len;
    (*newNode)->parent = 0;
    (*newNode)->typeTag = ANODE_INVALID;
    p->ast_len += 1;

    halc_end;
}

#define PARSER_INIT_NODESTACK_SIZE 64
#define PARSER_INIT_NODECOUNT 256

static errc parser_push_stack(struct s_parser* p, struct anode* node); // forward decl for parser_init

errc parser_init(struct s_parser* p, const struct tokenStream* ts)
{
    p->ast_cap = PARSER_INIT_NODECOUNT;
    if(p->ast_cap)
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

    p->stackCap = PARSER_INIT_NODESTACK_SIZE;
    if(p->stackCap)
        halloc(&p->stack, PARSER_INIT_NODESTACK_SIZE * sizeof(i32));
    p->stackCount = 0;

    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));

    // node 0's parent is itself, and is the core s_graph
    newNode->parent = 0;
    newNode->typeTag =  ANODE_GRAPH;
    newNode->nodeData.graph.children.entry = -1;
    newNode->nodeData.graph.children.count = 0;
    
    p->tabCount = 0;

    halc_try(parser_push_stack(p, newNode));

    halc_end;
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
    if(p->stackCount + 1 >= p->stackCap)
    {
        const i32 oldCap = p->stackCap;
        const i32 newCap = HALC_MAX(oldCap * 2, 16);
        p->stackCap = newCap;

        hrealloc(&p->stack, oldCap * sizeof(i32), newCap * sizeof(i32), FALSE);
    }

    p->stack[p->stackCount] = node->index;
    p->stackCount += 1;
    halc_end;
}

// pops one value off the active stack in the parser and discards it
static void pop_stack_discard(struct s_parser* p)
{
    p->stackCount -= 1;
}

static void pop_stack_discard_multi(struct s_parser* p, i32 count)
{
    while (count --> 0)
    {
        pop_stack_discard(p);
    }
}

// ---- helper functions ----
static b8 isTagTerminal(i32 tag)
{
    return tag <= COMMENT;
}

static b8 isNodeTerminal(struct s_parser* p, i32 node)
{
    return isTagTerminal(p->ast[node].typeTag);
}

static i32 p_getTypeTag(struct s_parser* p, i32 node)
{
    return p->ast[node].typeTag;
}

static errc match_forward_newline(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce) 
{
    i32 stackLen = (i32) (stackEnd - stackStart);

    if((stackLen == 1 && p_getTypeTag(p, stackStart[0]) == NEWLINE) || 
        (stackLen == 2 && p_getTypeTag(p, stackStart[0]) == COMMENT && p_getTypeTag(p, stackStart[1]) == NEWLINE)
    )
    {
        pop_stack_discard_multi(p, stackLen);
    }

    halc_end;
}

// -- match reduce functions --
static errc match_reduce_space(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;
    if(p->ast[stackEnd[-1]].typeTag == SPACE)
    {
        pop_stack_discard(p);
        *didReduce = TRUE;
    }
    halc_end;
}


// minimum set:
//  > TEXT \n
//  1 2    3
static errc match_forward_selection(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* matched)
{
    *matched = FALSE;

    i32 stackLen = (i32) (stackEnd - stackStart);

    if(stackLen < 3 || stackLen > 4 ||
        p_getTypeTag(p, stackEnd[-1]) != NEWLINE ||
        p_getTypeTag(p, stackStart[1]) != STORY_TEXT ||
        p_getTypeTag(p, stackStart[0]) != R_ANGLE
    )
    {
        halc_end;
    }

    struct anode* newNode;

    halc_try(parser_new_node(p, &newNode));

    newNode->typeTag = ANODE_SELECTION;
    newNode->nodeData.selection.tabCount = p->tabCount;
    newNode->nodeData.selection.comment = -1;
    newNode->nodeData.selection.extensionCount = 0;
    newNode->nodeData.selection.storyText = stackStart[1];

    if(p_getTypeTag(p, stackEnd[-2]) == COMMENT)
    {
        newNode->nodeData.selection.comment = stackEnd[-2];
    }

    for (i32 i = 0; i < stackLen; i += 1)
    {
        pop_stack_discard(p);
    }
    halc_try(parser_push_stack(p, newNode));


    halc_end;
}

// minimum:
// : lmfao 2 nova \n
// 1 2            3
static errc match_forward_extension(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* matched)
{
    *matched = FALSE;

    i32 stackLen = (i32) (stackEnd - stackStart);

    if(stackLen < 3 || 
        stackLen > 4 ||
        p_getTypeTag(p, stackEnd[-1]) != NEWLINE || 
        p_getTypeTag(p, stackStart[0]) != COLON ||
        p_getTypeTag(p, stackStart[1]) != STORY_TEXT)
    {
        halc_end;
    }


    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));

    newNode->typeTag = ANODE_EXTENSION;
    newNode->nodeData.extension.extension = p_getTokenFromNode(p, stackStart[1]);
    newNode->nodeData.extension.tabCount = p->tabCount;

    for(i32 i = 0; i < stackLen; i += 1)
    {
        pop_stack_discard(p);    
    }

    halc_try(parser_push_stack(p, newNode));

    *matched = TRUE;
    
    halc_end;
}

// speech is important but it needs to capture the optional comment at the end.
//
// $ : lmfao 2 nova \n
// 1 2 3            4  
// $ : lmfao 2 nova #[]comment \n
// 1 2 3            4           5
static errc match_forward_speech(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* matched)
{
    *matched = FALSE;

    i32 stackLen = (i32) (stackEnd - stackStart);

    if(stackLen < 4 || p_getTypeTag(p, stackEnd[-1]) != NEWLINE)
    {
        halc_end;
    }

    if(p_getTypeTag(p, stackStart[0]) != SPEAKERSIGN && p_getTypeTag(p, stackStart[0]) != LABEL)
    {
        halc_end;
    }

    if(p_getTypeTag(p, stackStart[1]) != COLON)
    {
        halc_end;
    }

    struct anode* newNode;

    halc_try(parser_new_node(p, &newNode));
    newNode->typeTag = ANODE_SPEECH;
    newNode->nodeData.speech.speaker = stackStart[0];
    newNode->nodeData.speech.storyText = stackStart[2];
    newNode->nodeData.speech.comment = -1;
    newNode->nodeData.speech.tabCount = p->tabCount;
    newNode->nodeData.speech.extensionCount = 0; // to be filled out later during graph linking step.

    for(i32 i = 0; i < stackLen; i += 1)
    {
        pop_stack_discard(p);
    }

    halc_try(parser_push_stack(p, newNode));
    *matched = TRUE;

    halc_end;
}

static errc match_forward_end(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* matched)
{
    *matched = FALSE;
    i32 stackLen = (i32) (stackEnd - stackStart);

    if(stackLen < 3 || stackLen > 4)
    {
        halc_end;
    }

    
    if(p_getTypeTag(p, stackStart[0]) != AT || 
       p_getTypeTag(p, stackStart[1]) != LABEL ||
       p_getTypeTag(p, stackEnd[-1]) != NEWLINE)
    {
        halc_end;
    }

    hstr directiveString;
    const hstr compare = HSTR("end");
    halc_try(p_getTokenLabelText(p, stackStart[1], &directiveString));

    if (!hstr_match(&directiveString, &compare))
    {
        halc_end;
    }


    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));

    newNode->typeTag = ANODE_END;
    newNode->nodeData.token = p_getTokenFromNode(p, stackStart[1]);

    pop_stack_discard_multi(p, stackLen);
    parser_push_stack(p, newNode);

    halc_end;
}

static errc match_forward_goto(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* matched)
{
    *matched = FALSE;

    i32 stackLen = (i32) (stackEnd - stackStart);

    if(stackLen < 4)
    {
        halc_end;
    }

    if(p_getTypeTag(p, stackStart[0]) != AT || 
       p_getTypeTag(p, stackStart[1]) != LABEL ||
       p_getTypeTag(p, stackEnd[-1]) != NEWLINE
       )
    {
        halc_end;
    }

    // this case will get caught by the reduce case later on
    if (p_getTypeTag(p, stackStart[2]) == L_PAREN)
    {
        halc_end;
    }

    hstr directiveString;
    const hstr compare = HSTR("goto");
    halc_try(p_getTokenLabelText(p, stackStart[1], &directiveString));

    if (!hstr_match(&directiveString, &compare))
    {
        halc_end;
    }

    if (p_getTypeTag(p, stackStart[2]) != LABEL)
    {
        // todo implement error context here
        if(!gParserRunNoPrint)
            fprintf(stderr, RED("Expected a label after goto\n"));
        halc_end;
    }

    *matched = TRUE;

    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));
    newNode->typeTag = ANODE_GOTO;
    newNode->nodeData.directiveGoto.tabCount = p->tabCount;

    i32 delta = -1;
    // -1 must be newline
    // -2 might be a comment
    if(p_getTypeTag(p, stackEnd[-2]) == COMMENT)
    {
        delta = -2;
    }

    halc_try(
        p_create_index_list(p, stackStart + 2, stackEnd + delta, &(newNode->nodeData.directiveGoto.label))
    );

    for (i32 i = 0; i < stackLen; i += 1)
    {
        pop_stack_discard(p);
    }

    halc_try(parser_push_stack(p, newNode));

    halc_end;
}

// Test Cases for directive
//
// note that GOTO is a special directive.
//
// directives are always 
//
// @ LABEL L_PAREN Tokenlist R_PAREN [COMMENT] NEWLINE
// 1  2    3         4        5         5       6
//
static errc match_forward_directive(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce) 
{
    *didReduce = FALSE;
    i32 stackLen = (i32)(stackEnd - stackStart);


    if(stackLen < 4)
    {
        halc_end;
    }

    i32* s = stackStart;

    // first symbol must always be an @
    // second symbol must be a LABEL
    // last symbol must be NEWLINE
    if (p_getTypeTag(p, stackStart[0]) != AT || 
        p_getTypeTag(p, stackStart[1]) != LABEL || 
        p_getTypeTag(p, stackStart[2]) != L_PAREN || 
        p_getTypeTag(p, stackEnd[-1]) != NEWLINE)
    {
        halc_end;
    }

    // walk forward until we find the L_PAREN
    i32* lParen = stackStart;
    i32* rParen = stackEnd - 1; // walk backwards until we find the R_PAREN

    while (lParen != rParen && p_getTypeTag(p, *lParen) != L_PAREN)
    {
        lParen++;
    }

    // match incomplete, this could be a goto
    if(p_getTypeTag(p, *lParen) != L_PAREN)
    {
        halc_end;
    }

    while (rParen != lParen && p_getTypeTag(p, *rParen) != R_PAREN)
    {
        rParen--;
    }

    // match incomplete, we might not have finished reading the whole line yet
    if(p_getTypeTag(p, *rParen) != R_PAREN)
    {
        halc_end;
    }

    // create a token list between lparen and rParen
    i32 indexList = -1;
    i32 indexListLen = 0;
    // ( a b c ) 
    // 0 1 2 3 4 diff = 4
    //
    // ( a )
    // 0 1 2 diff = 2;
    //
    // ( ) 
    // 0 1 diff = 1;
    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));
    newNode->typeTag = ANODE_DIRECTIVE;


    // assign parents to the nodes that we touched here.
    p_assign_parent(p, stackStart[0], newNode->index);
    p_assign_parent(p, stackStart[1], newNode->index);
    p_assign_parent(p, *lParen, newNode->index);
    p_assign_parent(p, *rParen, newNode->index);

    // if the parentheses start and the parenthese have a delta smaller than 2 then we do not need to allocate a nodelist
    if (rParen - lParen >= 2)
    {
        i32* nodeStart = lParen + 1;
        i32* nodeEnd = rParen - 1;

        indexListLen  = (i32)(rParen - nodeStart);
        halc_assert(indexListLen > 0); // note to self, UNKNOWN_ERRORs should get removed
        indexList = p_get_index_list_start(p);

        for (i32 i = 0; i < indexListLen; i += 1)
        {
            halc_try(p_push_index_list_entry(p, nodeStart[i]));
            p_assign_parent(p, nodeStart[i], newNode->index);
        }
    }

    newNode->nodeData.directive.commandLabel = stackStart[1];
    newNode->nodeData.directive.tabCount = p->tabCount;
    newNode->nodeData.directive.innerTokens.entry = indexList;
    newNode->nodeData.directive.innerTokens.count = indexListLen;
    
    for(i32  j = 0; j < stackLen; j += 1)
    {
        pop_stack_discard(p);
    }

    halc_try(parser_push_stack(p, newNode));

    *didReduce = TRUE;

    halc_end;
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
    halc_end;
}

// misc. error matching, attempts to clean up the parser stack.
static errc match_reduce_errors(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    *didReduce = FALSE;

    i32 len = (i32)(stackEnd - stackStart);
    b8 shouldEvict = FALSE;

    if(p->ast[stackStart[0]].typeTag == ANODE_GRAPH)
    {
        // check if there are errors.
        //
        // one key indication of an error is walking past a newline still on the stack
        
        if(!shouldEvict)
        {
            // find start of terminal list.
            i32* terminalStart = stackStart;
            if(p->ast[stackStart[0]].typeTag == L_SQBRACK && p->ast[stackStart[1]].typeTag != LABEL)
            {
                if(!gParserRunNoPrint)
                    fprintf(stderr, RED("Unexpected token %s after "), node_id_to_string(p->ast[stackStart[1]].typeTag));
                ts_print_token(p->ts, p->ast[stackStart[1]].nodeData.token, FALSE, RED_S);
                shouldEvict = TRUE;
            }
        }
        

        if(!shouldEvict)
        {
            i32 newLineCount = 0;
            for(int i = 0; i < len; i+=1)
            {
                if(p->ast[stackStart[i]].typeTag == NEWLINE)
                {
                    newLineCount += 1;
                }
                else if(newLineCount >= 1)
                {
                    if(!gParserRunNoPrint)
                    {
                        p_print_node(p, &p->ast[stackStart[i - 1]], GREEN_S);
                        fprintf(stderr, RED("Unable to parse line after reaching end of line\n")); // another stray thought. logging should really be something that we give hooks for the end user code to call into.
                    }
                    shouldEvict = TRUE;
                    break;
                }
            }
        }


        if(shouldEvict)
        {
            // directly writing to stderr or stdout should be fully redirectible... thats one for the todo.
            // halc_raise(ERR_UNABLE_TO_PARSE_LINE); // What to do here. 
            // We can report the error, evict the current line and just continue parsing from there.
            // Because this is such a recoverable error, maybe 
            // its better to not use halc_raise() here.
            
            i32 currentStackEnd = stackEnd[-1];
            while(p->ast[p->stack[p->stackCount-1]].typeTag <= COMMENT)
            {
                pop_stack_discard(p);
            }

            halc_try(parser_push_stack(p, &p->ast[currentStackEnd]));

            *didReduce = TRUE;
        }

    }
    halc_end;
}


// what the hell was i thinking about this one.
// 
// what is an expression?
static errc match_reduce_expression(struct s_parser* p, i32* stackStart, i32* stackEnd, b8* didReduce)
{
    halc_end;
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
        halc_end;
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
        halc_try(parser_new_node(p, &label));

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
                
                halc_raise(ERR_UNEXPECTED_TOKEN);
            }
            label->nodeData.label.comment = p->ast[stackStart[3]].nodeData.token;
        }
        else {
            label->nodeData.label.comment = -1;
        }

        // pop 3 times
        for(i32 i = len; i > 0; i--)
            pop_stack_discard(p);

        halc_try(parser_push_stack(p, label));
        if(gParserRunVerbose)
            p_print_node(p, label, GREEN_S);

        p->tabCount = 0;

        *didReduce = TRUE;
        halc_end;
    }

    halc_end;
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
    //
    // parser should maybe contain an error list?

    i32* tokenStackStart = p->stack;

    while(!isNodeTerminal(p, *tokenStackStart))
    {
        tokenStackStart++;
    }

    // forward matching 
    {
        b8 matched = FALSE;

        if(!matched)
            halc_try(match_forward_goto(p, tokenStackStart, stackEnd, &matched));
        if(!matched)
            halc_try(match_forward_end(p, tokenStackStart, stackEnd, &matched));
        if(!matched)
            halc_try(match_forward_directive(p, tokenStackStart, stackEnd, &matched));
        if(!matched) 
            halc_try(match_forward_speech(p, tokenStackStart, stackEnd, &matched));
        if(!matched)
            halc_try(match_forward_extension(p, tokenStackStart, stackEnd, &matched));
        if(!matched)
            halc_try(match_forward_selection(p, tokenStackStart, stackEnd, &matched));
        if(!matched)
            halc_try(match_forward_newline(p, tokenStackStart, stackEnd, &matched));
    }
    

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

    halc_end;
}

static errc parser_advance(struct s_parser* p)
{
    // - look at the next token and produce emit a new astNode, 
    
    struct anode* newNode;
    halc_try(parser_new_node(p, &newNode));

    if(gParserRunVerbose)
        printf("nodeIndex = %d\n", newNode->index);

    // - add it to ast, then add it to stack
    newNode->parent = -1;
    newNode->typeTag = (enum ANodeType) p->t->tokenType; // type tags <= COMMENT are Token Terminals
    newNode->nodeData.token = (i32)(p->t - p->ts->tokens);

    if(gParserRunVerbose)
    {
        printf("token offset %d\n", newNode->nodeData.token);
        ts_print_token(p->ts, newNode->nodeData.token, FALSE, GREEN_S);
    }

    halc_try(parser_push_stack(p, newNode));

    if(gParserRunVerbose)
    {
        printf("stack: ");
        for(i32 i = 0; i < p->stackCount; i += 1)
        {
            printf(" %s ", node_id_to_string(p->ast[p->stack[i]].typeTag));
        }
        printf("\n");
    }

    // - call parser_reduce to merge the active stack if merges are possible
    parser_reduce(p);

    // - errors: 
    //      if we have anything other than just a single s_graph at the end.
    p->t++;

    halc_end;
}


void graph_free(struct s_graph* graph)
{
    // no-op for now
}

errc parse_tokens(struct s_graph* graph, const struct tokenStream* ts)
{
    struct s_parser p;
    halc_try(parser_init(&p, ts));

    while (p.t < p.tend)
    {
        halc_tryCleanup(parser_advance(&p));
    }

    if(!gParserRunNoPrint)
        printf("parser nodes constructed = %d\n", p.ast_len);

    // graph construction should happen at this point
cleanup:
    parser_free(&p);

    halc_end;
}


