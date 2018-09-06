![Floyd logo](readme_floyd_logo.png)

License: at the moment Floyd is closed software, (c) Marcus Zetterquist. Plan is to change license to MIT or similar.

# WHAT IS FLOYD?

Floyd is a programming language designed to have ready made answers for how to do great, robust and composable programs. Both quick hacks and huge systems. It's statically typed, compiler or interpreted and focused very much on performance. It consists of three separate layers: a programming language, a system-level language and interactive tools.

##### GOALS

1. Make it simple to create robust large scale software

2. Replace many bad programming ideas with a few good ones, also make code composable

3. Introduce high-level features needed for large scale software

4. Extreme execution speed, faster than practical in C systems

5. Nextgen visual tools


##### Floyd consists of three parts:

1. **Floyd Speak** - a neat new programming language

2. **Floyd Systems** - a language that defines your complete software system and its internal interactions

3. **Floyd Studio** - a fast interactive and visual tool to playaround with your code and visually wire things together, profile your system and apply heavy optimizations to it.


Floyd prioritises solving the bigger questions: how to make big composable software the really works, rather on dweilling on neat syntactic details.

Floyd separates the logic (written in Floyd Speak) from the processing part. Processing is where you do communication, interaction between components, concurrency, mutation etc.


# FLOYD SPEAK

A programming language that feels familiar to Javascript / C(++) / Javaprogrammers but gets rid of a lot of the troublesome features and replaces it with new and better ways. Used to write composable components and programs.

1. All critical builing blocks are built-in mechanisms: strings, vectors, dictionaries and JSON literals and serialization. Avoid DYI primitives!
2. Statically typed with type inference.
3. Higher level (slightly) than C or Javascript. No pointers
4. Byte code interpreter. Future: compiled to native using LLVM
5. Simple to make tools
6. Values, persistent data structures
7. Automatic behaviors: comparable, copyable, roundtrip values -> strings -> files -> protocols -> source code
8. Pure functions, imperative. Encapsulation and protocols but not OOP
9. Visual playground
10. Familiar looks an feel for most programmers

**Floyd Speak Manual:** [Floyd Speak Manual](floyd_speak.md).


##### CHEAT SHEET

![Floyd Speak Cheat Sheet](readme_cheat_sheet.png)


##### EXAMPLE

	//  Make simple, ready-for use struct.
	struct photo {
		int width;
		int height;
		[float] pixels;
	};

	//  Try the new struct.
	let a = photo(1, 3, [ 0.0, 1.0, 2.0 ]);
	assert(a.width == 1);
	assert(a.height == 3);
	assert(a.pixels[2] == 2.0);

	let b = photo(0, 3, []);
	let c = photo(1, 3, [ 0.0, 1.0, 2.0 ]);

	//	Try automatic features for equality
	asset(a == a);
	asset(a != b);
	asset(a == c);
	asset(c > b);


##### FEATURES LEFT OUT OF FLOYD SPEAK

1. Aliasing
2. Pointers & references
3. Null and null-exceptions
4. Tracing garbage collection
5. Threads, locks, atomics, await, async, nested callback hell
6. OOP
7. Memory management
8. Mutable state
9. Callbacks, observers or broadcasting
10. Singletons and globals
11. Side effects
12. Find your resources
13. Header files
14. Error codes
15. Local optimisation, caching 
16. Local threading decisions
17. Shared mutable state
18. “Express your self in code”
19. Make your own linked list


# FLOYD SYSTEMS

This is a language that defines your complete software system and its internal interactions: components, concurrency, parallelism, communication, errors, time and mutation. The goals are these:

1. You can reason about it, present to no-developers and use to navigate visually

2. Clean concepts of robustness, concurrency, parallelism, communication, errors, time and mutation

3. Composable components

4. Simple to create tools

5. Visual profiling and editing

Your software system is composed of containers (apps, servers and other programs), components (libraries) and code.


![](readme_software_system.png)


Concurrency is done using actors -- small processes inspired by Erlang processes. There are no threads, locks, atomics, await-async, nested callback hell etc.

**Floyd Systems Manual**: [Floyd Systems Manual](floyd_systems.md).

##### A container with a bunch of actors wired together:

![VST](readme_floyd_systems_vst.png)


# FLOYD STUDIO

TODO 1.0
This is a webbased interactive tool for making, exploring and tuning complete software systems.

**Floyd Studio Manual**: [Floyd Studio Manual](floyd_studio.md).



# IN THE BOX

1. **Floyd Speak Manual**: [Floyd Speak Manual](floyd_speak.md).

2. **Floyd Speak compiler**, that compiles Floyd Speak source code to byte code.

3. **Floyd Speak interpreter** runs your programs at approx 10% of native speeds.

4. **Floyd Systems Manual**: [Floyd Systems Manual](floyd_systems.md).

5. **TODO POC: Floyd Systems compiler**, compiles floyd systems and containers etc to byte code.

Floyd compilers and tools are written in portable C++11.


# BACKLOG
These are upcoming features, listed here to give you an idea of what the plan is.

## POC - PROOF OF CONCEPT

|Area				| Feature
|:---				|:---
|Floyd Systems		| clocks and actors
|Floyd Systems		| software-systems, components, components
|Floyd Speak			| sum-type with match-expression
|Floyd Systems		| 3 most important tweakers
|Floyd Systems		| 3 most important probes
|Floyd Systems		| file system component



## FOR 1.0 - PUBLIC USE

|Area				| Feature
|:---				|:---
|Floyd Speak			| float, int8, int16, int32, int64
|Floyd Speak			| floyd exceptions
|Floyd Systems		| socket component
|Floyd Speak			| map() filter(), reduce()
|Floyd Speak			| shortcut && and ||
|Floyd Speak			| protocols
|Floyd Speak			| nested update
|Floyd Speak			| supermap()
|Floyd Studio			| Function playground
|Floyd Studio			| Container designer
|Floyd Studio			| Systems profiler view



## FUTURE BACKLOG

|Area				| Feature
|:---				|:---
|Floyd Speak			| Call C functions
|Floyd Speak			| Bit manipulation
|Floyd Speak			| Text library, Unicode
|Floyd Speak			| Protocol buffers-style features
|Floyd Speak			| Use LLVM to generate native code
|Floyd Speak			| Round-trip code -> AST -> executable function in runtime = optimization / generics
|Floyd Speak			| Embeddable in C programs
|Floyd Studio			| Software System Navigator
|Floyd Speak			| Encapsulation of structs
