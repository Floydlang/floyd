# FLOYD (NAME TBD)

Floyd is a nextgen programming language that gets rid of many problematic traditional programming concepts and introduces a few key new ones. It borrows from both the functional and imperative school of programming. It attempts to solve programming problems like correctness, composability, concurrency, performance. Floyd is both a better scripting language _and_ a better high-performance language.


MANIFEST: To Create a small general purpose languages that promotes great software engineering- for small and gigantic programs, embedded and distributed scripting and optimized engines - that has one simple way to do all basics. It raises the level of abstraction to let human, compilers, runtimes and hardware to do what they do best. Based on best practices, take a set of most important bits of modern languages and abandons most. Support visual programming / composition / architecture


# GOALS

- Simple and minimal
- Guides you into writing simple and correct code
	- problematic constructs are gone, all basics are ready out of the box (strings, streams, collections)
- Faster than C
	- by avoiding mutation and aliasing problems and carefully mapping data structures into cache lines. Optimizations are performed on top of correct code and dynamic for
- Easy to learn
	- Simplified C / Javascript / Java / C#
	- One prefered and built-in way to do every basic task
- Ideal both for scripting web servers and writing optimized game engines
- Composable
	- Immutability and pure functions and simple struct creation / agregation
- Explicit and portable and future-proof
	- No undefined state, no hardware details in language.
- Distributable / concurrent
	- Immutability and pure functions.
	- Hashing for deduplication and merging
	- Explicit safe mechanisms for introducing time and mutation, even across systems.
- Interoperates with web APIs and FS and UIs effortlessly
	- Built-in automatic JSON thunking
	- Has built in REST
	- Automatic UI creation for terminals, 
- Not designed for clever coding
- Promote lots of custom structs
- Support almost-system programming
- Easy to parse


# NON-GOALS

- C like hardware vs code equivalence. You cannot look at source code and know what instructions or bytes are generated.
- Real functional language
- All modern bells and whistles like generators from other hot languages
- Support creating custom low-level data structures in libraries, like C++ vs STL. Floyd has a clear distinction what is built into the language and what is libraries written ontop of the language. Collections and strings are part of language, not libraries on top of it.
- Detail control over threads and mutexes


# FEATURES

- Looks like JS or C
- Pure and immutable
- Explicit time / mutation / concurrency control
- Separate correctness from optimuzation
- Strict and static typing
- Persistent data structures built in
- Supports OOP classes and encapsulation (objects are persistent/immutable)
- Supports OOP interfaces
- DbC -- documentation -- tests: built-in and integrated
- Use LLVM or interpreter
- Tool pipeline is hackable and you can make stacks
- Dynamic collection backends
- Normalized source code format. Allow roundtrips
- Auto map structs to JSON
- Auto map structs to terminal UI
- Built in RC - No collecting GC: control over pooling
- Built-in JSON support
- Super simple to create new value classes with "clone" comparisons etc.
- DRY: no header files, contracts-tests-docs are the same thing.
- Simple module system with unique versioning, identifies modules using hashes.
- Connects easily to C ABIs.


# PARTS

- Language specification
- Language documentation
- Runtime configuration file format specification
- Intermediate code format specification IC
- Module file format specification
- Multi-phase compiler: Floyd source -> IC -> LLVM byte code -> x86. Implemented in C++
- Runtime implemented in C++
- Development tool for developing, debugging and optimizing code

=========================================================================================


# PURPOSE

1. Allow you to quickly create non-trivial software that is correct, fast, composable and long-lasting.
2. Replace  Java, C++ and C# as systems programming languages.
3. Be SIMPLE to learn and use - remove noise of traditional languages. 


# GOALS

1. Make correct and reliable software you can reason about.

2. Allow huge, complex software using truly composable software design.

3. Speed - goal is higher practical speed than the equivalent C code, taking  threading and trimming into account.

4. Elevate software design from expressions, functions and class to a higher level constructs.

5. Target imperativ programmers, making transition to Floyd and mixing Floyd with existing C / C++ code simple and worthwhile. Floyd appears as a utility.


# USPs

- Nextgen systems programming langugage
- Composable, correct, robust, compact
- Correctness is separated from optimizations
- Uses pure functions, persistent and immutable data structures
- Visual
- Static typing (not dynamic)
- Compact code
- Explicit mechanisms for time, mutation, mutable externals
- Native, not interpreted - uses LLVM
- Aggressive static and dynamic optimizations with detailed control, profiling, rules for different targets, switching collection backends, lazy/eager computation.
- Support for visual programming
- Unique support for concurrency, distribution.
- Collections 
- Resource handling automatic but explicit, not collecting GC

It’s a compiled, static systems programming language and speed is comparable to C / C++.
It uses LLVM as backend so is very portable across processor architectures.


###################################################


# COMPARISON

1. Imperativ programming: functions change variables and objects. Program always performs what programmer tells it, step by step. A lot of intermediate state is visible to other code. Program can reach out and read / write global state. Uses objects that can be updated.

2. Functional programming: functions have no side effects and function may be evaluated lazily. Immutability.

3. Floyd: functions are pure, like functional programming, but can be implemented imperatively. Then you string functions and objects together statically.

In floyd, you statically allocate objects and string them together in a dependency graph, as part of the programming. In object oriented programs this graph is usually created by the program at runtime.

??? Floyd has limited support for generic programming: it has built-in generic containers. ### Add explicit traits-protocols

??? Concept of 0-1-2-3-many instances. No pointers.
??? Construction and destruction with sideffectcts = RAII. Use "externals" for this.


# SPEED

Why is Floyd faster than functional languages?
1. Static typing - all types are defined and verified at compile-time.
2. Compiled to native code (usually).
3. Uses native basic types just like C: int32, uint8, char, float etc. These are not wrapped in abstractions.
4. No collecting garbage collector. Memory and other resources have explicit life times controlled by the runtime and client program.
5. Concurrency is not only done by parallelizing fold and loops, it can also be done for separate sub-systems.
6. All types of optimizations are detailed controlled on a systems-level by the developer - per instance of collections etc. In a composable way.


# IMPLEMENTATION LEVELS

1) Running simulation interactively. All nodes and each function in simulation is executed by runtime.

2) Generate C-code for simulation, inject client C-functions. Runtime is in charge, calling all nodes and functions.

3) Same as 2, but optimise-away runtime when possible, making nodes and functions call each-other intrusively.


A) Multithreaded supervisor, work stealing.

B) Advanced profiling.

C) Execute on speculation, lazy etc. Tweakable.


# TRACTION STRATEGY

Getting traction for new visual programming: same problem as with electric cars. Must be *bad ass* compared to existing solutions (speed, coolness) - not only healthier and better for the world.


### Main approaches

# Functions and data structures

1) Statically typed, but has support for dynamic types where it’s needed.

2) Mutation is carefully controlled. Data is per default immutable, persistent data structures are used.

3) Not object oriented. Supports objects-like constructs but not traditional classes with inheritance, encapsulation and virtual functions.

4) Functions are written in any C, C++. ### Javascript or Python. ### Floyd language.

5) All functions are pure. In debug mode, all native code is sandboxed.

6) Design by contract built in.

7) Unit tests are part of function definition.

8) There is no difference between a function and a table lookup.

9) All data structures have an efficient structure, mappable directly to a function written in C. (C function still cannot have side effects or mutate data directly, except as controlled by Floyd).


### Runtime, dependencies

1) System is designed by defining dependency between parts. This graph is statically designed.

2) The runtime evaluated the dependencies and controls execution order of functions. 

3) Optimisations like lazy evaluation, caching, trading memory vs cpu, treading etc is controlled at runtime by the runtime, but inspected and tweaked by programmer, as a system. Futures are used internally, so are thread pools etc. Execute on speculation, with rollback. Batching control.

4) No traditional garbage collection.

5) Heavily scalable to many cores. No explicit control over threading.

6) Programs cannot observe or control concurrency.

