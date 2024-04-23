# Halcyon Core Services

After long and careful deliberation. I think it is best to move the core of halcyon's runtime and compiler into a single c99 compatible library. 

I intend to just have the core runtime as completely embeddable into any platform that supports compiling c99.

Many of the other tools such as the editor and debugger will still be written in zig.

Building: 

```
mkdir build && cd build
cmake ../
```

the .bat files should work on windows and run_tests.sh should work on linux.

## integrating and embedding

Just add an include path to `src/` and compile all the `.c` files that start with `halc` to integrate halcyon.

This should work on literally every C compiler that supports c99. If you find one that it doesn't work, file a bug on github, as that's a bug.

run `HalcyonInit(&InitParams);` from your main function and it should be started up.

## Roadmap

1. Reach feature parity with zig-halcyon
2. Debug server and gui tool
