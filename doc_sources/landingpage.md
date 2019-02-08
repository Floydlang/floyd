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
