![alt text](./docs/floyd_logo.png "Floyd Logo")

License: at the moment Floyd is closed software, (c) Marcus Zetterquist. Plan is to change license to MIT or similar.

# WHAT IS FLOYD?

Floyd is a new programming language design that is designed to have ready made answers for how to do great, roboust and composable programs, both small programs and huge systems.

It feels familiar to Javascript / C / Java / C++ programmers but gets rid of some of core troublesome features and replaces it with new and better features.

**FEATURES:** Functions, structs as value objects, strings, vectors, dictionaries, deep JSON integration, static typing, type deducing, automatic deep behaviour of all types, like comparison and copying.

**REMOVED:** Pointers and references and aliasing, null, classes with mutation and aliasing, inheritance, memory management, header files, constructors and manual lifetime operators.

**COMING SOON:** protocols, sumtype, double-type, file system access, clocks & channels, HAMT collection backend.

Floyd consists of the language specification, a compiler and a byte code interpreter. Floyd is written in portable C++ 11.

For details, read the [Floyd reference docs](./docs/floyd_reference.md).

# CHEAT SHEET

![alt text](./docs/floyd_cheat_sheet2.png "Floyd Cheat Sheet")

# EXAMPLE

	//  Make simple, ready-for use struct.
	struct photo {
		int width;
		int height;
		[float] pixels;
	};

	//  Try the new struct.
	a = photo(1, 3, [ 0.0, 1.0, 2.0 ]);
	assert(a.width == 1);
	assert(a.height == 3);
	assert(a.pixels[2] == 2.0);

	b = photo(0, 3, []);
	c = photo(1, 3, [ 0.0, 1.0, 2.0 ]);

	//	Try automatic features for equality
	asset(a == a);
	asset(a != b);
	asset(a == c);
	asset(c > b);


# INSTALLATION

Floyd is distributed as C++ source code.


# PLANNED LANGUAGE FEATURES

- Closures
- Add explicit language features for binary / text protocols. Protocol buffers.
- Add runtime-concepts -- a way to layer systems on top of others.
- Map, filter, reduce, lambda.
- Struct encapsulation
- Support integers with range.
- Bit manipulation.
- Text type and basic toolkit.
- High-level constructs, also generates docs. https://trello.com/c/cIYePtrb/669-impose-high-level-structure-of-the-code-base-above-classes

# PLANNED TOOLS & RUNTIME

- Playground-like system around Floyd. With visual help. Interactive, graphs, tests, docs.
- Interactive motherboard designer / debugger / profiler where you assemble Floyd apps that interacts with the world and runs over time, from pure parts and libraries written in Floyd.
- Tweaks, tracing & dynamic collection backends selection.
- LLVM backend for making native code.
- Features for embedding Floyd runtime easily in an application.
- Add N x M green threads.
- Round-trip code -> AST -> executable function in runtime = optimization / generics.
- Standardized pretty printer.

