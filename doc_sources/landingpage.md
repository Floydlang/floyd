

##### THE MATRIX
Floyd is inspired by the movie The Matrix, which is obviously a great way to design a programming language!

- **Red Pill**: Knowledge, freedom, uncertainty and the brutal truths of reality
- **Blue Pill**: Security, happiness, beauty, and the blissful ignorance of illusion

Learn more here - things like **green processes**, **black magic**, the **red and blue pill**, and other cool and colorful things: [www.floydlang.org](https://www.floydlang.org "Floyd language's Homepage")


##### VERTICAL LANGUAGE
Floyd is a vertical language: it cares deeply about **cache lines** and **hardware cores** and at the same time has keywords like **software-system** and primitives for interacting between concurrent processes.

It's still very small and easy to learn, by dropping a lot of cruft other programming languages think is needed. C++ - how many ways do we need to initialise a variable? Really?







### NOVEL CONCEPTS

Floyd separates the programming work in new ways, imposes a clean system-wide structure and has simple built-in ways to do hard things like concurrency and optimization.

Floyd is not a high-level or low-level language -- it's both. Think of it as a vertical language Floyd is surprisingly a high-level and low-level language at the same time. It cares about cores and caches *and* concurrency, interactions between systems and APIs.

### THE MATRIX

- **Red Pill**: Knowledge, freedom, uncertainty and the brutal truths of reality
- **Blue Pill**: Security, happiness, beauty, and the blissful ignorance of illusion

Working interactive software needs both red and blue code.

1. **Blue pill**: Write your code inside the blissful matrix illusion. You are Neo here. Make sure most of your code goes here. It's a fantastic place to write solid programs.

2. **Red pill**: The outside world is harsh. Floyd's built-in containers and processes helps you deal with moving time, interactions and communication with an every changing outside world.

Keep the red pill code as small as possible.


### SO EXACTLY WHAT IS FLOYD?

Floyd consists of a small and elegant toolkit of features built directly into the language that all go together to support you to make great products. Every feature has been carefully picked, polished and adopted as a first class feature in the language. The goal is to have one simple way to do each common thing.

**What's *not* included and what you *can't* do is a core of Floyd's design**

##### PRIMARY GOALS

1. Make it simpler to create robust large scale software products
2. Replace many bad programming ideas with a few good ones, critical to make code composable
3. Introduce high-level features needed for large scale software
4. Extreme execution speed, faster than practical in majority of C systems
5. Next-gen visual and interactive tools to support exploration and experimentation


### UNIQUE FEATURES

1. Floyd splits your system into three areas: a) your program's logic, b) your program's processing and c) mapping its execution to the hardware.
2. Floyd has a clean and minimal skeleton structure for *complete* software systems - including processes, people, concurrency and how they all interact.
3. Carefully distilled set of syntactical features to move focus to the *system* rather than the inside of functions
4. Feels like simple imperative programming but with the best bits of functional programming sneaked in discretely


Floyd has three big parts:

1. **Floyd Speak** - a neat new programming language for doing logic. It's statically typed with inference, compiles to byte code or native code. Here you program the bulk of your system.
![](readme_cheat_sheet.png)

2. **Floyd Systems** - does two things: A) defines your complete software system and its internal interactions and processes and concurrency, and B) Allows you to use Tweakers to map your software onto the hardware resources (cores, caches, RAM) 
![](readme_floyd_systems_vst.png)

3. **Floyd Studio** - a fast interactive and visual tool to play around with your code and visually wire things together, profile your system and apply heavy optimizations to it



Read more about Floyd: [Read more](readme_deeper.md)

# TECH OVERVIEW

- Static and compiled language.
- Runs on byte code interpreter. Will also run natively using LLVM.
- Mashup of C, Erlang, Javascript and Closure. Stealing the best bits in a comfortable package!
- Not functional, imperative or OOP. Mix. You have structs and polymorphism and pure functions.




# LEARN MORE

|Item				| Feature					| Link
|:---				|:---					|:---
| **Floyd Speak Manual**		|About the language itself				|[Floyd Speak Manual](floyd_speak.md)
| **Floyd Speak Core Library Manual**		|Small kit of essential functions, like JSON, file system access				|[Floyd Speak Core Library Manual](floyd_speak_corelibs.md)
| **Floyd Speak compiler**		|Compiles Floyd Speak source code to byte code.
| **Floyd Speak interpreter**	|Interprets your programs at approx. 7-15% of native speeds.
| **Floyd Systems Manual**		|				|[Floyd Systems Manual](floyd_systems.md)
| **Floyd Systems compiler**			|Compiles floyd systems and containers etc to byte code.|




# ROADMAP

#### PERFORMANCE
- LLVM codegen
- Better internal threaded task manager
- Visual profiler, debugger
- Control over collection backend types, caching

#### LANGUAGE FEATURES
- More int and float types
- Sum-type (enum/union) with switch
- Proper support for libraries
- Protocol type
- Basic Unicode functions
- Binary packing of data
- C language integration
- Built-in REST library





# PERFORMANCE - PAGE


# How are Floyd programs simper to make:

BLUE PILL CODE
You implement the simplest program that does the job. Optimise later by augmenting your code, without risk of introducing defects!

- There are no pointers or references
- There is no aliasing
- There are no ownerships or lifetimes
- There are no globals
- Function's don't have side effects, you can call any function without surprises
- Dead simple to write tests
- Everything is guaranteed to be 100% reproducible = simple debugging
- There are no threading concerns, no need to think about this at all
- You don't write any caching that go out of sync
- You don't attempt to do the regular optimisations with caching, batching and tweaking collection types
- You don't do error handling

RED PILL CODE
This is done by writing code for your processes, usually 1 to 4. Try to have very little code here, preferably delegate all logic to blue pill code.

- Processes have one variable.
- They run in a sandbox.
- All communication between processes is done via immutable messages, no mutexes, threads or futures.






# RUNTIME CODEGEN FOR OPTIMIZATION: GENERATE BITMAP SCALER


# How is concurrency handled in Floyd?

Floyd does simple and safe concurrency using Floyd processes. These are sandboxed and communicate only by sending atomic messages to each-other. A messages can be a string or a huge data structure. Similar to Erlang. (Sweden rules!).

An advanced xbox game might have 5-10 of these processes and send world states and render graphs and textures between them. Posting a huge data structure between two processes only costs bumping an atomic reference count.
# How are Floyd programs fast?

DISCLAIMER: The structure for this is in Floyd right now - processes, supermap() etc. The byte code interpreter does very little optimisation. Next on the roadmap is LLVM codegen from Floyd's byte code.

Floyd makes it simple to control how your program cab exploit the memory sub system and CPU cores of the hardware it runs on.

You dictate memory layout and collection types on-top of existing code, interactively, without changing your code. Several version of your functions are generated and optimised automatically. Match up with hardware cache lines.
You can insert caches and batching in existing code, without changing it.
You can assign thread priority, affinity to function calls, without changing your code.
Profiling is integrated into the language itself, not a separate clunky tool you run once per release.
There is no pointer type thus no aliasing problems = compiler generates fast-than-C code.
You can exploit hardware threads by using map(), reduce() and supermap(). Crunch a collection of 1000 JPEG:s or 3840 x 2160 float pixel images. Your function executes in a sandbox on many hardware cores. This is how shaders work on a GPU. The supermap() function even allows you to do this when elements have dependencies between them.
Floyd uses an internal OS thread team.
Floyd uses green-threads and does not consume OS threads when blocking on I/O, only when executing instructions.

==================
Floyd code is written with generic vectors, dictionaries and structs to get the logic right. Which type of collection (hash, array with binary search, HAMT, red-black trees etc) is controlled separately. Your struct:s have undefined memory layout. When your program works, you can interactively explore how to use the hardware best. This is impossible with C, all these decisions are locked down into every line of C code.

- Augument your program to control memory layouts and collection backend on a per-function-call level.

Floyd automatically generates optimized machine code for the function and all functions it calls. There can be many versions of the *same Floyd functions*, optimised for different purposes. This is done without introducing defect - it doesn't affect your program's logic at all.

- You can augument your code by insert caches and batching - without altering your logic. 

- You can augment your Floyd process and functions with hardware core affinities and priorities to control how they are mapped to available cores.

- Floyd has no pointer thus has no aliasing problems, this is an advantage compared to C.


- Floyd does parallelism using small set of safe primitives: map(), reduce(), filter() and supermap(). You supply a collection (1000 JPEG? 3840 x 2160 float pixels?) Your code execute sandboxed on many hardware cores. This is how shaders work on a GPU. The supermap() allows you to do the same even when elements have dependencies between them.

- Floyd uses an internal OS thread team.

- Floyd does not consume OS threads when it blocks on IO, only when executing instructions.


