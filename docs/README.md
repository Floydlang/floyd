![](floyd_logo_banner.png)


**PLEASE PLAY AROUND WITH FLOYD**


# TL;DR
**WHAT:** General-purpose programming language with a unique take on programming and performance.

**LICENCE:** MIT
 
**STATUS:** Alpha 2. All the basics of the language are finished and robust. Runs natively using LLVM JIT backend (passes all tests, but not optimised). A byte code interpreter is also available. Production ready for smaller programs.

**THIS REPO:** This Github repository holds the compiler, the bytecode interpreter and documentation.

**2019:** 1) first generation of Injector (optimiser), 2) Flesh out language, approx 15% more features.

[**[FLOYD BLOG]**](https://medium.com/floyd-programming-language "Floyd Blog") - [**[ROADMAP]**](https://github.com/marcusz/floyd/projects/1 "Floyd Roadmap") - [**[CURRENT MILESTONE]**](https://github.com/marcusz/floyd/projects/4 "Milestone") - [**[MANUAL]**](floyd_manual/floyd_manual.md "Manual") - [**[LIBRARY]**](floyd_manual/floyd_corelib.md "Core library")



# WHAT IS FLOYD?

Floyd is a general-purpose programming language designed for making very large and robust programs that run really fast, while ducking many of the problems of older languages. Floyd wants to become a better choice than C++, Rust, C#, Python and Javascript for any project.

The goal is to make a programming language that:

- Executes faster than the same programming written in C or C++ (with the same skills & time) - it should become the preferred language to write a video game engine with, for example

- Makes it fast and simple to program - less accidental complexity

- Helps you make big programs that are fun and straight forward to work on for a long time

- Simple built-in support for concurrency and parallelism

How can this level of execution speed be reached? By designing Floyd to give exceptional freedom to the compiler and runtime to drastically control the mapping of the program's execution to the hardware. The programmer supervises this mapping interactively using a profiler-like tool. This is done separately from writing the program logic. Precise selection of data structures, exact memory layouts, data packing, hot-cold data, hardware caches, thread tasks priorities, thread affinity and so on.

Floyd separates your program into three separate concerns:

1. Writing the program logic
2. Programming internal concurrent processes and how they access the outside world
3. Mapping the program to the CPU and memory system

Floyd compilers and tools are written in C++ 17 and compiles with Clang and GCC.



## LANGUAGE SYNTAX

Floyd looks like Javascript and has a lot fewer features, syntax and quirks than most languages. Floyd is **statically typed** with **type inference**. It's got built in types for vectors, dictionaries, JSON, a struct type and strings. All values are **immutable** / **persistent data structures** using HAMT and other techniques.

NOTICE: The manual is full of examples in text form: [Floyd Manual](floyd_manual/floyd_manual.md).


![](floyd_snippets.png)

It's a mashup of imperative, functional and OOP. Functions defaults to **pure** (but with normal local variables).

Floyd has no classes, no pointers / references, no tracing GC (uses copying and RC), no threads, mutexes, atomics and no header files. No Closures. No generics.


![](floyd_quick_reference.png)

Roadmap: protocol type for simple polymorphism, basic encapsulation feature, sum-type and limited lambdas.



## CONCURRENCY, STATE AND THE WORLD

Processing and concurrency is done using Floyd's virtual processes and message passing. Each Floyd process has its own private state and is sandboxed. Floyd processes can interact with the world, calling OS APIs and accessing files. Keep this code small, with minimal logic.

![](floyd_container_example.png)

The bulk of your program should be blue code - pure code.

## PARALLELISM

*(Implementation in progress)*

Safe parallelism is built in using map() reduce() filter() and map_dag(). Like shaders running on a GPU. They share an internal OS thread team with the Floyd processes.



## NATIVE PERFORMANCE AND BYTE CODE INTERPRETER

Floyd compiles to optimised native x86 and x64 machine code, using an LLVM-based backend. This is the same backend Xcode uses for Swift, C and C++.

Floyd also has a byte code compiler & interpreter. It is useful for embedding / scripting using Floyd but also for making tools for Floyd. 



## OPTIMIZATION

*(Implementation in progress)*

You optimise your program by running it and *augmenting* your Floyd processes and their function call graph. Each process has its own optimisation settings. This automatically generates new optimized versions of affected functions. This cannot introduce defects! Examples:


- Change memory layout of structs, order, split, merge, array-of-structs vs struct-of-arrays.
- Select backend for collections: a dictionary can be an array with binary search, a HAMT, a hash table or a red-black tree - all with different performance tradeoffs.
- Control thread priority, affinity, how many threads to use for the parallelization features.
- Insert read or write caches, introduce batching.

![](floyd_optimization.png)

# INSTALLATION

There is no compiled distribution of Floyd yet. You need to clone the github repository and build yourself.

#### MACOS

1. Clone the Floyd repository from Github. Use the master branch

2. Install the Homebrew package manager, if you don't already have it. https://brew.sh/

3. In your terminal, run "brew install llvm@8.0.0" -- this installs the LLVM library on your Mac. It's installed in "/usr/local/Cellar/llvm/8.0.0_1" - so it won't conflict with Xcode or other versions of LLVM.

4. Open the Floyd xcode project: Floyd/dev/floyd.xcodeproj

5. Make sure the current xcode scheme is "unit tests". Select from the top-left popup menu, looking like a stop-button.

6. Select menu Product/Run

	This builds the project and runs the unit tests. Output in the Xcode console.
	
#### UNIX

Cmake is used to build floyd 




##### Building with arch linux

1. Install llvm, sudo pacman -Sy llvm

2. cd floyd/dev/floyd

3. mkdir build;cd build

4. cmake ..

5. make

##### Builing quicker with ninja
 
 0. sudo pacman -Sy ninja

 1-3. as above 

 4. cmake .. -G Ninja

 5. ninja

#### WINDOWS

TBD


# STATUS

The essentials of Floyd are up and running and very robust (more than 1000 tests), including the concurrent Floyd processes. The manual is complete but needs polish.

A handful features are needed for a satisfying 1.0: rounding out the language features somewhat and then *it's all about performance*.


# IN THE BOX

|Item				| Feature	
|:---				|:---
| [Floyd Manual](floyd_manual/floyd_manual.md) | Programming language manual
| [Core Library Manual](floyd_manual/floyd_corelib.md) | File system access, hashing, time & date etc.
| **Floyd compiler** | Compiles Floyd source code
| **Floyd LLVM backend**	| Optimises your Floyd program and generates native code x86 / x64 code
| **Floyd byte code compiler & interpreter**	| Runs your program in the byte code interpeter



# MOVING FORWARD

[ROADMAP](https://github.com/marcusz/floyd/projects/1 "Floyd Roadmap")

[CURRENT MILESTONE](https://github.com/marcusz/floyd/projects/4 "Milestone 2")


**PLEASE PLAY AROUND WITH FLOYD**
