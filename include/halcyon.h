// public-facing single header for everything you need to interact with halcyon.

#ifndef __HALCYON_API_C
#define __HALCYON_API_C


// Don't bring in EXTERN_C_BEGIN from halc_types, in-case end user libraries define it. Additionally, this entire file should only contain safe-to-consume
//
// defines and symbols

#ifdef __cplusplus
extern "C" {
#endif

// object-like interface, everything here is an opaque pointer
struct HalcyonRuntime;
struct HalcyonStory;
struct HalcyonWorld;
struct HalcyonInteractor;

// compiler access things (not usually used in runtime), used to drive the debugger, 

// custom tooling, etc..
struct HalcGraph; // each halcyon file contains a graph associated with it
struct HalcRegion; // Each graph is part of a region, HalcyonStory contains 
                   // a list of all the regions
                   // a world is a runtime instantiation of a story

struct HalcFacts; // Facts database

// runtime operations
// - setup memory allocator
// - setup perf/monitoring functionalities
// - compile story (target root story.halc)
// - create world (instance of a story)
// - install directive
// - uninstall directive
// - install callbacks
// - tick runtime
//
// dev:
// - access compiler
//      - get compiler status ( including errors and warnings )
//      - get story graph for a file
//      - annotate for localization

// HalcyonStory
//  
// Compiled, ready to use, representation of your entire story
//
// - load from compiled
//
// dev:
// - export as compiled
// - generate/view reports
//      - story table
//      - localization database (with reference to existing)

// World operations
//
// world is a copy of the facts database with versioned values that represent
// a specific running instance of the story.
//
// serialization for a world is stable and should always be safe.
//
// - save world
//      - to bytes
//      - to disk
// - load world
//      - from disk
//      - from memory
// - destroy world
//
// dev:
// - update to new story
// - takes the current running instance of the world and attempts to update 
// to a new story

// Interaction
//
// An interaction is an object which represents a specific session for traversing 
// the graph for a given world.
//
// an Interaction can branch and create multiple branching interactors at the
// same time.
//
// World has a list of these things, and active interactions are saved if they
// are persistent
//
// - start interaction
// - force-end interaction
// - set if the interaction should be persistent or not
// - get story text, including localization key
// - walk
// - select choice
// - install callbacks


#ifdef __cplusplus
}
#endif 

#endif // ifndef __HALCYON_API_C
