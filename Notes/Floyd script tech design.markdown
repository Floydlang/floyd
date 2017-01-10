


- Make each function have example usage code and example data by default.




# GOALS

- Make language "symmetrical" so it's easy to copy-paste / refactor, generate etc. C++ is nightmare.

- Be a platform to build higher-level constructs on top (high level LLVM IR) and tools.  Simple parsing / processing.  Have built-in specific designs for important programming concepts, not misuse or reuse of existing ones.

- Parse script. Parse tree is json and official encoding 2. Supports LISP -style macros. Roundtrip, tools, runtime code generation etc.

- Parse script. Parse tree is json and official encoding 2. Supports LISP -style macros. Roundtrip, tools, runtime code generation etc.

- No need to code for debugging. Visually unfold multi-expression lines so you can set breakpoints in between etc.

- Easy to parse / regenerate Floyd script code. Use to reformat your code in debugger, do processing of source code, for serialize / deserialize.

- Easy to make new structs/tuples and use / convert / mix them

- round trip script -> JSOn -> script


# INSIGHTS & QUESTIONS

- Pure code can't access opticouplers.
??? Ptr, ref, uuid



# NO CLASSES?
Floyd replaces the class and inheritance thinking with other primitives. What are traditional OOP classes used for?

- Interface
- Bundle values
- Keep data invariant
- Represent external gadget
- Layer APIs
- Represent start / end = lifetime
- Share state between several clients.

Floyd uses struct to make new composites.


# PROBLEM 1 - POINTERS
Use path-objects Instead of ref in Floyd. Specifies location of a value, not the value.

- Paths / pointers / links vs values?

### Why user a pointer in C++?

1) To avoid copying and construction of big object when passing it around.

2) To let several clients communicate by read / writing a shared object.

3) To make comparison of big objects fast.

4) To be able to forward declare a type.

5) To iterate collection, array quickly.

6) To allow function to update something passed in via argument (a view adding itself to its parent view).

7) To keep track other objects. A list of views to redraw, or a list the current view focus chain.

8) To implement low-level collections (linked list).

9) To *identify* a specific object by comparing the pointers, not the values.


ML: Traveling salesman algo. How without pointers?

MZ: Pairing up two parallelle data trees often use pointers. View tree is attached to a tree of modelel objects


# PROBLEM 6 - SOURCE FILE FORMAT

Define source format: mix JSON, markdown, tests and code.


# PROBLEM 7 - Polymorphisms: interface and supports_interface() query

Don't support generic interfaces or protocols

- Provide toolbox to client
- support variants of object
- callbacks for wiring together components into a system
- support testing
- lambdas/completions/closures

Replace with several more specific concepts
- lambda
- runtime
- motherboards
- variant objects


# PROBLEM 8 - how to expose built-in functions, like size()?
How does built-in feature support queries and request: do they have normal functions that have special built-in functionality? Ex: vector, map, clock, externals etc.
