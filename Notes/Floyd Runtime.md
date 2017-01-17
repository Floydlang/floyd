# RUNTIME

The Floyd runtime has more responsibilities than most language runtimes.

- Memory management
- Basic types
- Strings
- Collections and backend colletions
- Composites / structs handling
- Reference counting
- Profiliing
- Tweaks
- Optimizations: lazy, precalc, read-cache, write-cache, alternative deep-struct layout etc.


??? Always do de-duplication aka "interning".


# FLOYD RUNTIME
- Static profiler - knows ballpark performance and complexity directly in editor. Optimize complexity when data sets are big, else brute force.
- Runtime does optimizations in the same way as Pentium does optimizations at runtime. Caches, write buffers, speculation, batching, concurrency. Nextgen stuff.
- No aliasing or side-effects = maximum performance.
- ### Compile for GPUs too.



# PROBLEM 5 - HOW TO EFFICIENTLY MUTATE STRUCTS?

One struct can have several different memory layouts depending on its use. 

Vector of pixels may be stored as one struct with vector of red, vector of blue.

Struct that gets modified may be stored as as a tree of structs.

ALSO: Support many alternaitve layouts for structs and nestesd structs / vectors etc. Generate alternative functions that use them? RUntime vtable for struct member access?


ALSO: How to efficiently switch between collection types at runtime
