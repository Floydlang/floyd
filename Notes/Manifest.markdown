# FLOYD (NAME TBD)
Floyd is a nextgen programming language that gets rid of many traditional programming concepts and introduces a few new ones. It borrows heavily from both the functional and imperative school of programming.

Floyd is a new programming language. It attempts to inject some new thinking into programming computers, specific higher-level constructs than functions and classes.

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

# ROADMAP

### PHASE 1 - minimal but useful
Prove minimal design works
Make paper 1.0
Hello world
Package

### PHASE 1 - speed proof of concept
Make optimization experiments, high-level.
Make fast, compiled version
Make fast persistent containers

### IDEAS
port
bus
rising and falling edges of the clock signal

	edge-detection triggers new generation
testbench

transitivity of immutability
http://lua-users.org/wiki/ImmutableObjects

state
combinational logic
sequential logic
https://en.wikipedia.org/wiki/State_(computer_science)

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


### Example

{
	External_OpenGL openGL;
	External_KeyboardInput keyboard;
	External_COWClock worldClock(100hz);
	External_COWClock videoClock(60hz);

	//	Every time world is stored to, a new generation is created.
	//	“All” old generations are still available and referenced existing objects. 
	GENERATIONS WorldSimulation worldGenerations;
	WorldDrawerFunction worldDrawer;
	WorldAdvancerFunction worldAdvancer;
	Latcher<World> latcher;

	//	Setup so world is updated at 100 Hz.
	worldAdvancer.inputWorldPin = world;
	worldAdvancer.clocksPin = worldClock.clockOutPin;
	worldGenerations.nextPin = worldAdvancer;

	//	Setup so world is painted at 60 Hz.
	latcher.inputPin = world;
	latcher.clockPin = videoClock.clockOutPin;
	worldDrawer.inputWorldPin = latcher;

	OpenGL.commandsPin = worldDrawer.outputPin;
}


	External_AudioStream audioStream;
	AudioGenerator audioGenerator;
	//	Audio stream requests buffers of 64 audio frames at a time from audio generator.
	audioStream.bufferInputPin = audioGenerator.bufferOutputPin;


# ISSUES

### Requires trim pots
### Requires measurements
### How to solve mutability
### Requires language to describe dependencies between chips.
### Requires efficient reference counting, across threads, cores and network.
### Requires efficient persistent vector and map.
### Use sha1 internally to do de-duplication.

### Make JSON format for entire simulation. Normalise format so it can be generated, editted in GUI.

Content-addressable objects?

??? Hash objects?

??? Always do de-duplication aka "interning".

??? How to do fast, distributed reference counting? When to copy? Trim settings.


### Example programs

Example: Hello, world
Example: Copy a file.
Example: Game of life
Example: space invaders
Example program: read a text file and generate JSON file.



# Why circuit boards?

* Calculations: functions, data structures, collections - no time.
* Time and externals: mutability, clocks, concurrency, transformers  - circuit boards
* Optimizations: measure / profile / tweak runtime.


int function f1(int a) is a value that is equivalent to a int myVector[int]


There are only immutable values, no iterators, generators, ranges etc. Use SEQ, VEC and MAP and INSTREAM and OSTREAM.

You can only have mutable state as local variables and in clock.

Runtime optimization: Only MUTABLE<SONG> a = x;  will use atomic operations on internal RC for the value X, all other access to value X will use normal inc/dec.




### SIMULATION OVERVIEW

Simulation is a controlled environment managed by an active runtime.

0 to many functions
0 to many packages
0 to many types
0 to many objects
0 to many external artefacts
0 to many time bases
0 to many chips


PACKAGE
0 to many functions
0 to many CHIPs
	0 to many objects
	0 to many wirings
0 to many types

0 to many external artefacts

