#ifndef __HALCYON_PARSER
#define __HALCYON_PARSER

#include "halc_errors.h"
#include "halc_strings.h"
#include "halc_tokenizer.h"


struct anode;

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

const char* node_id_to_string(i32 id);
errc p_print_node(struct s_parser* p, struct anode* n, const char* pointerColor);
errc parser_init(struct s_parser* p, const struct tokenStream* ts);
void parser_free(struct  s_parser* p);
errc parse_tokens(struct s_graph* graph, const struct tokenStream* ts);

// anode types are an extension of tokenType
enum ANodeType {
    ANODE_SELECTION = COMMENT + 1, // ok
    ANODE_SPEECH, // ok
    ANODE_SEGMENT_LABEL,
    ANODE_GOTO,
    ANODE_DIRECTIVE,
    ANODE_EXPRESSION,
    ANODE_GRAPH,
    ANODE_INVALID
};
 
// index into tokenstream as a token
typedef i32 anode_token_t;

struct anode_list_alloc {
    i32 entry;
    i32 count;
};

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
    struct anode_list_alloc innerTokens;
    i32 tabCount;
};

struct anode_segment_label {
    anode_token_t label;
    anode_token_t comment;
    i32 tabCount;
};

// goal of parser is to reduce, reduce, reduce until we get a graph
struct anode_graph {
    struct anode_list_alloc children;
};

// main reduction, used to generate the graph
struct anode_expression {
    // children indexes storage
    // we don't dynamically resize or destroy or unload any of this shit so..
    // we could just have two pointers into a bump-allocated a_index_list
    struct anode_list_alloc children;

    // reference to start and end tokens
    anode_token_t tokenStart;
    anode_token_t tokenEnd;
};

struct anode_goto {
    struct anode_list_alloc label;
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
        struct anode_goto directiveGoto;
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

#endif
