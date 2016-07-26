# FLOYD (NAME TBD)

Floyd is a nextgen programming language that gets rid of many problematic traditional programming concepts and introduces a few key new ones. It borrows from both the functional and imperative school of programming. It attempts to solve programming problems like correctness, composability, concurrency, performance. Floyd is both a better scripting language _and_ a better high-performance language.


MANIFEST: To Create a small general purpose languages that promotes great software engineering- for small and gigantic programs, embedded and distributed scripting and optimized engines - that has one simple way to do all basics. It raises the level of abstraction to let human, compilers, runtimes and hardware to do what they do best. Based on best practices, take a set of most important bits of modern languages and abandons most. Support visual programming / composition / architecture


# OPINIONS

- Programming is not math
- All basics must be built-in
- Reduce dependencies: modules should in best case only depend on language, libraries and interfaces it exposes - no other modules.
- Only way to do every day things, but it's great
- Functions are no place to be creative.
- Do not confuse optimization and correctness. Correctness is done locally, optimizations are done globally.
- Side effects are evil
- The language is the languge, libraries are libraries.


# GOALS

- Simple and minimal
- Elevate software design from expressions, functions and class to a higher level constructs.
- Guides you into writing correct and reliable software you can reason about. Little room to do creative coding
- Language has one built-in way of do each common thing: concurrency, strings, streams, collections, logging, tests, JSON, memory handling.
- Problematic features are gone (pointers, threads, mutation, aliasing, side-effects, manual operator implementation).
- Programs are faster than C, taking  threading and trimming into account.
- Ideal both for scripting web servers and writing optimized game engines
- Write and compose huge and reliable programs. Also quick to use for scripting.
- Great and practical way to think and implement mutation / time / side-effects.
- Litte learning friction for Javascript / Java / C# /C++ programmers
- Practical, pragmatic and hands-not - not theoretical mathy concepts.
- Allow huge, complex software using truly composable software design.
- Explicit and portable and future-proof
- Supports distributable / concurrent programs
- Interoperates with web, users and file system effortlessly
- Easy to write tools
- Advanced built-in profiling and optimization tools.
- Simple reliable module feature


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
- Separate correctness from optimisation
- Strict and static typing
- Next generation runtime
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

- Normalized source code format allows round-trip tools / visual editing. Easy language parsing, use as data language.
- Easy to parse source code
- Static and runtime optimization / profiling using complexities and approximations of hardware.
- Dedicated module file format supports versioning, renaming, backward compatibility etc. DLL-hell not possible.
- Promotes lots of custom structs. Making new structs is zero or one line of code. Can be unnamed.
- Interoperates with web APIs and FS and UIs effortlessly
	- C-ABI support
	- Built-in automatic JSON thunking
	- Has built in REST
	- Automatic UI creation for terminals, 


# PARTS

- Language specification
- Language documentation
- Runtime configuration file format specification
- Intermediate code format specification IC
- Module file format specification
- Multi-phase compiler: Floyd source -> IC -> LLVM byte code -> x86. Implemented in C++
- Runtime implemented in C++
- Development tool for developing, debugging and optimizing code



# Built in
- Strings / unicode
- Collections: key/value and index->value.
- Streams
- Seq
- Functions
- High level REST-like, more like RPC for objects than HTTP.
- Concurrency
- Hashing

# Built in plug-ins
- File system access
- Sockets
- JSON support
- Terminal
- Optimized native collections
- Web UI

- Development environment is web based: run webserver. App looks like gmail.

	- by avoiding mutation and aliasing problems and carefully mapping data structures into cache lines. Optimizations are performed on top of correct code and dynamic for




=========================================================================================



# MAIN TECHNIQUES
1. Write imperative functions but make all functions pure and work on immutable data. Allow local mutation inside functions.
2. Have classes but objects are immutable / persistent.
3. Special mechanism for introducing time / mutation.
4. Separate where and how you write correct code vs where you optimize the application that uses that code. Different source code, different tools.
5. Defer optimization and performance choices to the application-optimization phase and level. Collection backends are chosen on a per-instances basis
6. Performance optimization is focused on memory accesses caching and synchronization, not removing CPU instructions.
7. Memory handling is automatic using reference counting. No pointers.
8. Aggressive static and dynamic optimizations with detailed control, profiling, rules for different targets, switching collection backends, lazy/eager computation.





# PERFORMANCE

Why is Floyd faster than functional languages?

1. Static typing - all types are defined and verified at compile-time.
2. Compiled to native code (usually).
3. Uses native basic types just like C: int32, uint8, char, float etc. These are not wrapped in abstractions.
4. No collecting garbage collector. Memory and other resources have explicit life times controlled by the runtime and client program.
5. Concurrency is not only done by parallelizing fold and loops, it can also be done for separate sub-systems.
6. All types of optimizations are detailed controlled on a systems-level by the developer - per instance of collections etc. In a composable way.

Why is Floyd faster than C?


###################################################


# COMPARISON

1. Imperativ programming: functions change variables and objects. Program always performs what programmer tells it, step by step. A lot of intermediate state is visible to other code. Program can reach out and read / write global state. Uses objects that can be updated.

2. Functional programming: functions have no side effects and function may be evaluated lazily. Immutability.

3. Floyd: functions are pure, like functional programming, but can be implemented imperatively. Then you string functions and objects together statically.

In floyd, you statically allocate objects and string them together in a dependency graph, as part of the programming. In object oriented programs this graph is usually created by the program at runtime.

??? Floyd has limited support for generic programming: it has built-in generic containers. ### Add explicit traits-protocols

??? Concept of 0-1-2-3-many instances. No pointers.
??? Construction and destruction with sideffectcts = RAII. Use "externals" for this.








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

