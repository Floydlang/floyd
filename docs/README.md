![](readme_floyd_logo.png)

License: MIT

Status: Language spec for 1.0 almost done. Compiles to byte code and interprets it. Not ready for production yet. 

# FLOYD

Floyd is a general-purpose static and compiled programming language with strong opinions on how to write complex and robust software products with minimum pain and minimum waste of time. Fast programs. It's tools are visual and interactive.

Floyd replaces languages like C++, C#, Rust, Swift and Java -- but also scripting languages like Python and Javascript.

It's designed to allow you to write the next Photoshop and Grand Theft Auto and mobile apps.

The end goal is always to ship great *products* that execute extremely well on the hardware and that are a delight to develop further.

Floyd has a bunch of novel concepts, how it separates the programming work in new ways, imposes a clean system-wide structure and has simple built in ways to do processing and concurrency.

# SO EXACTLY WHAT IS FLOYD?

Floyd consists of a small and elegant toolkit of features built directly into the language that all go together to support you to make great products. Every feature has been carefully picked, polished and adopted as a first class feature in the language. The goal is to have *one* simple way to do each common thing.

**What's *not* included and what you *can't* do is one of Floyds best features**

##### PRIMARY GOALS

1. Make it simpler to create robust large scale software products

2. Replace many bad programming ideas with a few good ones, critical to make code composable

3. Introduce high-level features needed for large scale software

4. Extreme execution speed, faster than practical in majority of C systems

5. Next-gen visual and interactive tools to support exploration and experimentation


# UNIQUE FEATURES

1. Floyd splits your system into three areas: a) your program's logic, b) your program's processing and c) mapping its execution to the hardware.

2. Floyd as a clean built-in structure skeleton for *complete* software systems including processes, people, concurrency and communication between processes

3. Visual and interactive tools

4. Carefully distilled set of syntactical features to move focus to the *system* rather than the inside of functions

5. Feels like simple imperative programming but with the best bits of functional programming sneaked in discretely


Floyd has three big parts:

1. **Floyd Speak** - a neat new programming language for doing logic. It's statically typed with inference, compiles to byte code or native code. Here you program the bulk of your system.
![](readme_cheat_sheet.png)

2. **Floyd Systems** - does two things: A) defines your complete software system and its internal interactions and processes and concurrency, and B) Allows you to use Tweakers to map your software onto the hardware resources (cores, caches, RAM) 
![](readme_floyd_systems_vst.png)

3. **Floyd Studio** - a fast interactive and visual tool to play around with your code and visually wire things together, profile your system and apply heavy optimizations to it



Read more about Floyd: [Read more](readme_deeper.md)

# INSTALLATION

TBD (for now, explore dev directory)

# STATUS

The core of the Floyd Speak language is up and running. The compiler compiles the source code to byte code that runs quite fast on an interpreter. There are approximately 750 tests the make sure compiler is solid. The manual is complete.
A handful small but critical features needs to be implemented before it's truly useful for real world programming.

Floyd Systems finally has its design in place but needs to be implemented in the compiler and runtime.

The details on Tweakers, probes and Floyd Studio are on the drawing board.


|Item				| Feature					| Link
|:---				|:---					|:---
| **Floyd Speak Manual**		|				|[Floyd Speak Manual](floyd_speak.md)
| **Floyd Speak compiler**		|Compiles Floyd Speak source code to byte code.
| **Floyd Speak interpreter**	|Runs your programs at approx. 7-15% of native speeds.
| **Floyd Systems Manual**		|				|[Floyd Systems Manual](floyd_systems.md)
| **Floyd Systems Reference**	|				|[Floyd Systems Reference](floyd_systems_ref.md)
| **TODO POC: Floyd Systems compiler**			|compiles floyd systems and containers etc to byte code.|
| Standard library				|A number of basic components|

Floyd compilers and tools are written in portable C++11.


##### BACKLOG
These are upcoming features, listed here to give you an idea of what the plan is: [Floyd Backlog](backlog.md)
