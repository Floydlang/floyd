![](readme_floyd_logo.png)

License: MIT

Status: All the basics are finished. Runs on a bytecode interpreter. Will run natively soon.

# WHAT IS FLOYD?

Floyd is a general purpose programming language designed for making very large and robust programs that run really fast.

Learn more here: [www.floydlang.org](https://www.floydlang.org "Floyd language's Homepage")


# USE

Use Floyd to build the next Photoshop, Grand Theft Auto or mobile app. But also for short scripts and toy programs. Probably not embedded software, though.

Floyd wants to replace C++, Rust, C#, Python. And Javascript. Only C can stay. Maybe Closure too, just out of respect.


# FLOYD IN NUTSHELL

Floyd separates your program into three separate concerns:

1. Writing the program logic: Floyd Speak.
2. Programming the interaction between your internal process and the outside world: Floyd Systems and Floyd Processes.
3. Mapping the program to the cpu and memory system: Floyd's augmentation mechanism and parallelism functions.

Floyd is a small and friendly language that is easy to learn. It looks like Javascript or C. It is statically typed with type inference and runs on a bytecode interpreter (now) and natively using LLVM (not implemented). It's got built in types for vector, dictionaries, JSON, a struct type and strings. All values are immutable / persistent data structures using HAMT and other techniques. Floyd uses reference counting internally.

It's a mashup of imperative, functional and OOP. Functions defaults to pure (but with normal local variables). TBD: protocol type allows polymorphism. There is no encapsulation (yet).

There are no classes, pointers / references, no tracing GC, lambdas (yet), closures, pattern matching, generics (yet), threads, mutexes, atomics, header files, encapsulation (yet).

Floyd compilers and tools are written in C++ 17.


Processing and concurrency is done using Floyd's virtual processes and message passing. Each process has its own private state. Floyd processes can call impure functions and interact with the world. The rest of your program is pure.


# FLOYD PERFORMANCE
*(Implementation in progress)*


Parallelism is simple and safe with map(), reduce(), filter() and supermap() functions. Like shaders running on a GPU. They share an internal OS thread team with the Floyd processes.

You optimise your program by running it and *augmenting* your Floyd process functions (and the function calls they make, downward the call graph). This automatically generates new optimized versions of lots of affected functions. This cannot introduce defects! Examples:

- Change memory layout of structs, order, split, merge, array-of-structs vs struct-of-arrays.
- Select backend for collections: a dictionary can be an array with binary search, a HAMT, a hash table or a red-black tree - all with different performance tradeoffs.
- Control thread priority, affinity, how many threads to use for the parallelization features.
- Insert read or write caches, introduce batching.


# QUICK REFERENCE

![](readme_cheat_sheet.png)


# STATUS

The essentials of Floyd are up and running and very robust (approx 1000 tests), including the concurrent Floyd processes. The compiler generates bytecode that runs at approx 5-15% of native speed on the interpreter. The manual is complete.

A handful features are needed for a satisfying 1.0: rounding out the language features somewhat and then *it's all about performance*.


# IN THE BOX

|Item				| Feature	
|:---				|:---
| [Floyd Speak Manual](floyd_speak.md) | Programming language manual
| [Core Library Manual](floyd_speak_corelibs.md) | File system access, JSON support
| [Floyd Systems Manual](floyd_systems.md) | How to use Floyd processes to change the world
| **Floyd compiler** | Compiles Floyd source code to byte code
| **Floyd byteCode interpreter**	|Runs your program



# LOOKING FORWARD

#### PERFORMANCE
- LLVM codegen
- Better internal threaded task manager
- Control over collection backend types, caching
- Visual profiler, debugger

#### LANGUAGE FEATURES
- More int and float types
- Sum-type (enum/union) with switch
- Proper support for libraries
- Protocol type
- Basic Unicode functions
- Binary packing of data
- C language integration
- Built-in REST library


