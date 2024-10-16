#ifndef __HALCYON_PARSER
#define __HALCYON_PARSER

#include "halc_errors.h"
#include "halc_strings.h"
#include "halc_tokenizer.h"
#include "halc_allocators.h"

EXTERN_C_BEGIN

struct anode;
struct s_parser;
typedef u32 s_link;

// how should we resolve links?

#define LINK_ENDNODE 0
#define LABELS_MAP_INITIAL_CAP 32

struct hash_entry
{
    u32 hash;
    void* value;
};

struct s_label_map
{
    struct hash_entry* entries;
    u32 len;
    u32 cap;
};

errc label_map_init(struct s_label_map* map);

errc label_map_insert(struct s_label_map* map, s_link link);

// this should be part of the public API?
struct s_graph 
{
    hstr* strings;
    u32 stringsLen;
    u32 stringsCap;

    // fully linked story nodes
    struct s_node* nodes;
    u32 nodesLen;
    u32 nodesCap;

    // oh god... I have to make a hashmap
    struct s_label_map labels;
};

struct s_choice {
    const hstr* choiceText;
    s_link link; // link to another node in the same story
};

struct s_choices_list {
    const struct s_choice* choice;
    u32 len;
    u32 cap;
};

struct s_node 
{
    const hstr* text; // nullable. if null we are to automatically link to the next one
    const struct s_choices_list* choices; // nullable
    s_link link;
};

errc graph_init(struct s_graph* graph);
void graph_free(struct s_graph* graph);

errc graph_append(struct s_graph* graph, struct s_node newNode);

errc graph_clone_string(struct s_graph* graph, const hstr* string, hstr** out);

struct s_node* find_node_from_link(struct s_graph* graph, u32 link);

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
// - node stores end of list as it's start index for new children being created
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
    ANODE_END,
    ANODE_DIRECTIVE,
    ANODE_EXPRESSION,
    ANODE_EXTENSION,
    ANODE_GRAPH,
    ANODE_BADNODE,
    ANODE_INVALID
};
 
// index into tokenstream as a token
typedef i32 anode_token_t;

struct anode_bad_node
{
    anode_token_t token;
};

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

struct anode_extension {
    anode_token_t extension;
    i32 tabCount;
};

struct anode_goto {
    struct anode_list_alloc label;
    i32 tabCount;
};

// ast node
struct anode
{
    i32 index;
    i32 parent; // parent of this node, following this node when we successfully parse should always get us to a graph node.

    enum ANodeType typeTag;
    union NodeData{
        anode_token_t token; // this node is just a token
        struct anode_selection selection;
        struct anode_speech speech;
        struct anode_extension extension;
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

// parser operations

EXTERN_C_END

#endif
