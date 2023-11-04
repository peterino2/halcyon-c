# Halcyon Core Services

After long and careful deliberation. I think it is best to move the core of halcyon's runtime and compiler into a single C99 compatible library. 

I intend to just have the core runtime as completely embeddable.

Many of the other tools such as the editor and debugger will still be written in zig.

Building: 

```
mkdir build && cd build
cmake ../
```

## Roadmap

1. Reach feature parity with zig-halcyon
2. Debug server and gui tool
