# QUICK PITCH

Small C/JS-like language. Bare minimals. Struct are values & automatic. Functions are pure. String, vector, dict and seq are built-in and persistent. No pointers or references. Composable.

JSON is IL, templates.
Native C ABI. RC. JSON type.

Runtime stacks. FS-part

Time is recorded. Debugging.
Embeddable
Compiled

C-performance.

Top-level supports clocks, transformers, tweaks.caching/lazy, channels and gofuncts.


Test & examples part of syntax. Interactive sandbox.



Function complexity is expressed on functions.
Visual IDE, game-engine style.















# BENEFITS


# Killer apps for languages
- C: port operating systems when HW change a lot
- Fortran

# Why do I want Floyd instead of C++, Rust or Swift?

- Build huge systems. Easily compose parts into bigger ones
- Programs are very fast (native or interpreted)
- No fad distractions, just industrial-strength tools
- Best practises defined and enforced (naming and layout)
- Full set of nextgen interactive tools
- Programming is easy and risk free
- Programs are very small - very lite accidental complexity
- Simple built in toolkit for all basics
- Easy to learn, Javascript-like.
- Fun and reliable concurrency
- Embeddable and mixable with other languages

- Supports C ABI
- Compiled nate


# parts

Cache
FileSystem
Server comm
S3 share
user-account w server
HTML DOM
App Data Model
Document File with versions
ABI
Internet stream
Audio
Encode / decode binary and text formats
Images
Emails
Open GLs

# External file system value, like React

? Can the fs be a normal Floyd value and file data stored as [uint8]? Or do we need to have special accessors?

Read and write entire fs and every byte in files and meta data is if it was only values.

	struct Permissions {
		reading: bool
		writing: bool
		execution: bool
	}
	
	struct FSNode {
		fileSize: size_t
		ownerUserID: uint32
		ownerGroupD: uint32
		userPermissions: Permissions
		groupPermissions: Permissions
		othersPermissions: Permissions
		file: [uint8]?
		dir: [FSNode]?
		inodeChangeTime: uint32
		contentChangeTime: uint32
		lastAccessTime: uint32
	}
	
	struct FileSystem {
		root: INode
	}

	struct APath {
		nodeNames: [string]
	}

- APath: a vector or names from root to a node (file or dir). Nodes may or may not exist in fs right now.


How it would look with wrapping

	let fs = SampleFileSystem("c:")
	let path = APath("users/marcus/desktop/hello.txy")
	let fileContents = fs.GetNodeAsFile(path)
	let unicodeText = unpackUTF8(fileContents)
	let fs2 = fs.SetNodeFileContents(path, unicodeText + "Hello, this is the new file contents")

With mirrored data structures

	let fs = SampleFileSystem("c:")
	let path = APath("users/marcus/desktop/hello.txt")
	let fileContents = fs.root.dir!["users"].dir!["marcus"].dir!["desktop"].dir!["hello.txt"].file!
	let fs2 = fs.root.dir!["users"].dir!["marcus"].dir!["desktop"].dir!["hello.txt"].file! = "hello"


# Other notes

- Support setin and getin, it uses introspection? Don't use string paths, include tech in language. Maybe use vector of keyword-type.
??? every object, value i system has 64bit ID. You can always write code that tracks that ID, for example a view can track the data values it represents as subviews. Mirror tree -- feature.






























# OVERVIEW


1. Floyd script. Pure compiled language. Pure / referential transparent. Built-in vec/map/seq, JSON. Compilter + tool chain.
2. Motherboard. JSON format. Concrete layout. Can be visual. Clocks, optocouplers, sockets, REST, files, UI.
3. Runtime: Optimizing/profiling runtime. Optimization injection, dynamic data structure.

![title](images/VST-plug%20motherboard.png)


Floyd has an opinon how to make great software and takes a stand.

Limit programmer’s freedom where it’s not needed. Little freedom of expression about bread and butter stuff.

95% of the general-purpose features you need are already built-into the language and you don’t have to think about it more.

Collections, optimisation strategy, serialisation, exceptions, unicode. No need to invent basics.



# FLOYD PARTS
- Floyd Script langugage. Pure, typesafe, compiled+interpreted, no refs. JS-like syntax, single assignment SSA
- Persistent DS
- Standard intermediate format in JSON
- Persistent values/structs
- Clocks
- Optocouplers
- Profiling/optimizing runtime. Dynamic collection backends, caching-precalc-readcache, write cache, parrallelization.
- Scriptable runtime params for optimization
- Embeddable

 

# CLIENT CODE
Code that Floyd programmer writes.
- Has no concept of time and order. (Except locally inside a function.)
- Cannot store state
- Pure. Cannot mutate outside world. No side effects.
- Cannot observer the world changing.
- Any call can be replaced by its output value
- 

# INSIGHTS & QUESTIONS

??? Three languages: pure logic, com/process, tweaks?




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





# NEWS

A piece of code depends on: runtime, functions, types and modules. Callbacks. Custom runtimes

A journal of all inputs is the purest of all datastructures

Floyd •cache• primitive: immutable on the outside, mutable on the inside

Reference: Ipfs

Peer to peer deduplication between data structures of same types

Built-in statemachine construct (variant of a process / clock?)

Cache / prefetch / lazy / deduplicate / pair-sync / diff are language features

Runtimes can have internal state, side-effects invisible to clients. Pure bulk code (good code) then time code on top

Anonymous functions and concise lambda syntax: x => x * 2 is a valid function expression in JavaScript. Concise lambdas make it easier to work with higher-order functions.

Often when you use a map you want an array, but with quick access via one key

keyword-string is valuable. Text = user, String = 8bit data, keyword = hash/string pair

Diffing is hard to get right. Caching and invalidating caches are hard to get right. Never do it off the cuff inside other code, always externalize it. Use off-the-shelf mechanisms

Simple to make dll or exe with standard args. Or start Floyd app in server runtime on the fly.


Idea for making tests of pure functions: run your app and tool collects all ins/outs of your function, collapses and sorts. You can just check the tests you want to keep

Feature that updates a collection to same topology of another collection. Use to "decorate" or match a lowlevel collection with UI data.

All values are always futures = composable in time!

User can create and debug boards on iPhone. Apps live on github. Tools work directly on GitHub. 

Source code can have compile time binds to external files containing ID:s like resource files.

Floyd though: GetHughStruct().only.access.subset => only evaluate what's needed to calculate .subset

Make chip for runtime stacks

Make chip for git-style storage.

Make chip for prefetch, jit, read cache, write cache.

Make chip for g2 caching, hashing, stackable ram-cache-algo-disk-s3



Use JSON conceptually as the only values. Internally we can keep it static / structs / binary. Use "RESTFul" concept in language = composable.

Built-In RT profiling

Fast diff. Compare or track? Research git

Source code tweaker + profiler. How to encode? Profiler records stats like big O and DS-usage on high level - not by running code on hw.

Diffing-broadcasting (record changes compared to base version?), sockets, ui, FS. Referenser vs by-value.






Remove file, file system, version and load/restore from apps and put in runtime

Special part for automatic validated JSON serialization / deserialisation, debugging

Special part for binary serialisation / deserialisation, debugging

Special part that represents an external resource (open file, socket, window). Or make snapshot and diff the interface between simulation and the world?


#Playgrounds

A playground is an interactive Swift coding environment that evaluates each statement and displays results as updates are made, without the need to compile and run a project.

Use playgrounds to learn and explore Swift, prototype parts of your app, and create learning environments for others. The interactive Swift environment lets you experiment with algorithms, explore system APIs, and even create custom views, all without the need to create a project.

Share your learning with others by adding notes and guidance using rich comments. Create an explorable learning environment by grouping related concepts into pages and adding navigation

# RoundtripPersistentTree 
Create C++ collection class (not templetized) that supports JSON roundtrip (maybe extensions, hints) and is persistent.

Use as data model of apps.

Simple diff, simple server sync / diff.
Merge

# Ternary operators 
operate on three targets. Like C, Swift has only one ternary operator, the ternary conditional operator (a ? b : c). 

Use instead of switches, with several ranges. Like pattern matching.

let a = (signal ? _ == 0 ? calc_zero(), _ >= 0 && < _< 0.1)


























# MANIFESTO


# FLOYD (NAME TBD)

Floyd is a nextgen programming language that has an opinion how you create great software. It simplifes and guides by getting rid of many traditional programming concepts that were wrong and it attempts to have one built-in solution for each important concept. It steals shamelessly from both the functional and imperative school of programming.

Goals are: easy of programming, correctness, composability, concurrency, performance. Floyd is both a better scripting language _and_ a better high-performance language.

Floyd programs have distinct layers of different capabilites:

- the top level is the motherboard, where you connect stuff together into a product. Here you design concurrency, communicate with the rest of the world. You also control all optimizations on this level. This is normally quite few lines of code, even for huge projects.
- FloydScript is used for the bulk of the logic. It looks and feels like Javascript, but is typesafe, can be compiled or interpreted and uses persistent data structures. It's simplified OOP - there are no classes or inheritance but there are objects and types and encapsulation. It's not functional - you have local variables, loops etc - but functions are all pure.


# OPINIONS

- Programs have two types of code: friendly code that is the simple, fast and flexible way to write software and process-code that does communication, concurreny and tracks changes. Make as much code as possible friendly!

- Functions are no place to be creative.
- Side effects sucks
- Caching, lazy evaluation, precompute: these are system-level decisions, not part of friendly code.
- Callbacks suck. Observers suck. Futures suck. Do not track changes of data manually, let computer do that. How? Diffing!
- The language shall define strict rules for friendly code - the "bulk" code. ??? term? Red and blue functons? This code follows all ideals.
- Software spent to my energy converting data back and forth
- Optizations, concurrency, external communication -- these are composability killers and should never
- Making correct code is done in each function, optimzation is done globally.
- It's fine to have local variables and mutate stuff, as long as the final function is pure
- Correct, error-tolerant, composable, multithreadable, parallelizable and understandable GOES TOGETHER, they are not in conflict.
- BTW: Programming is not math
- All basics must be built-in to allow composability and simple programs
- Reduce dependencies: modules should in best case only depend on language, libraries and interfaces it exposes - no other modules.
- Only way to do every day things, but it's great
- The language is the languge, libraries are libraries.


MANIFEST: To Create a small general purpose languages that promotes great software engineering- for small and gigantic programs, embedded and distributed scripting and optimized engines - that has one simple way to do all basics. It raises the level of abstraction to let human, compilers, runtimes and hardware to do what they do best. Based on best practices, take a set of most important bits of modern languages and abandons most. Support visual programming / composition / architecture


Correctness first - program A - theoretical: Imagine all functions executing on an infinitely fast computer and design the program to *work* correctly.
Optimize second - Program B: Next optimise the program by using the Optimizer to do caching, lazy evaluation and other optimisations *on top* of the correct design. Other types of time: communication latency, screen refresh rate etc must be part of program A - theoretical design.


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
