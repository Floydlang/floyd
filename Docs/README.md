![Floyd logo](floyd_logo2.png)

License: at the moment Floyd is closed software, (c) Marcus Zetterquist. Plan is to change license to MIT or similar.

# WHAT IS FLOYD?

Floyd is a programming language designed to have ready made answers for how to do great, robust and composable programs. Both small programs and huge systems. Floyd prioritises solving the bigger questions: how to make big composable software the really works, rather on dweilling on neat syntactic details.



##### GOALS

1. Make it simple to create robust large scale software

2. Replace many bad programming ideas with a few good ones, also make code composable

3. Introduce high-level features needed for large scale software

4. Extreme execution speed, faster than practical in C systems

5. Nextgen visual tools


Floyd consists of three parts:

1. **Floyd Speak** - a neat new programming language that feels familiar to Javascript / C(++) / Javaprogrammers but gets rid of a lot of the troublesome features and replaces it with new and better ways. Used to write composable

2. **Floyd Systems** - a language that defines your complete software system and its internal interactions: components, concurrency, parallelism, communication, errors, time and mutation

3. **Floyd Studio** - a fast interactive and visual tool to playaround with your code and visually wire things together, profile your system and apply heavy optimizations to it.


Floyd separates the logic (written in Floyd Speak) from the processing part. Processing is where you do communication, interaction between components, concurrency, mutation etc.


# FLOYD SPEAK

1. Built-in building blocks like strings, vectors, dictionaries and JSON literals and serialization. Avoid DYI primitives!
2. Statically typed with type inference.
3. Higher level (slightly) than C or Javascript. No pointers
4. Interpreted or compiled (in future)
5. Simple to make tools
6. Values, persistent data structures. Structs are automatically comparable, copyable etc.
7. Roundtrip values - string - files - protocols - source code - messages
8. Pure functions, imperative. Encapsulation and protocols but not OOP
9. Visual playground



##### CHEAT SHEET

![Floyd Speak Cheat Sheet](floyd_cheat_sheet.png)

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

##### UNSUPPORTED BY FLOYD SPEAK

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

The concurrency is done in a very clean way by actors, inspired by Erlang processes. There are no threads, locks, atomics, await-async, nested callback hell etc.



# FLOYD STUDIO

1. TODO FUTURE: Software System Navigator

2. TODO 1.0: Function playground

3. TODO 1.0: Container designer

4. TODO 1.0: Systems profiler view


##### CONTAINER EDITOR
You string together components with actor

![VST](floyd_systems_vst.png)



# IN THE BOX

1. **Floyd Speak Reference Manual**: [Floyd Speak Reference Manual](Floyd Speak.md).

2. **Floyd Speak compiler**, that compiles Floyd Speak source code to byte code.

3. **Floyd Speak interpreter** runs your programs at approx 10% of native speeds.

	TODO 1.0: The compiler can also compile to ".ast.json" files, which is the AST of the program in JSON format, then compile ".ast.json" to byte code. Great for making cool tools that analyses, manipulates the AST or generates code.

4. **Floyd Systems Reference Manual**: [Floyd Systems Reference Manual](Floyd Systems.md).

5. **TODO POC: Floyd Systems compiler**, compiles floyd systems and containers etc to byte code.

6. **TODO POC: Floyd Studios**, a web-based interactive IDE for Floyd Systems and Floyd Script.

Floyd compilers and tools are written in portable C++11.



# TODO

TODO POC: Floyd Speak - sum-type with match-expression
TODO POC: Floyd Speak - implement map()

TODO 1.0: Floyd Speak - implement filter(), reduce()
TODO 1.0: Floyd Speak - shortcut && and ||
TODO 1.0: Floyd Speak - float, int8, int16, int32, int64
TODO 1.0: Floyd Speak - nested update
TODO 1.0: Floyd Speak - protocols
TODO 1.0: Floyd Speak - implement supermap()
TODO 1.0: Floyd Speak - implement floyd exceptions


TODO POC: Floyd Systems - research and implement tweakers

TODO POC: Floyd Systems - research and implement probes

TODO POC: Floyd Systems - implement clocks and actors

TODO POC: Floyd Systems - implement file system component


TODO 1.0: Floyd Systems - implement socket component

TODO 1.0: Floyd Studio: Function playground

TODO 1.0: Floyd Studio: Container designer

TODO 1.0: Floyd Studio: Systems profiler view


TODO FUTURE: Floyd Speak - Use LLVM to generate native code

TODO FUTURE: Floyd Speak - Encapsulation of structs

TODO FUTURE: Floyd Speak - Bit manipulation

TODO FUTURE: Floyd Speak - Protocol buffers-style features

TODO FUTURE: Floyd Speak - Text library, Unicode

TODO FUTURE: Floyd Speak - Round-trip code -> AST -> executable function in runtime = optimization / generics

TODO FUTURE: Floyd Speak - Call C functions

TODO FUTURE: Floyd Speak - Embeddable in C programs

TODO FUTURE: Floyd Studio: Software System Navigator
