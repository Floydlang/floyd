![](readme_floyd_logo.png)

License: MIT

Status: All the basics are finished. Runs on a byte code interpreter. Will run natively soon.

# NEW TO FLOYD?

Floyd is a industrial grade general purpose programming language. It has strong opinions on how to write complex and robust software products. Fast programs! While you are getting actual things done. Think H-beams and RCCB:s. Floyd is designed 2019, not in the 50-80:ies.


##### USE
Use Floyd to build the next Photoshop, Grand Theft Auto and any type of mobile application. But also to make short scripts and toy programs. Probably not embedded software, though. The end goal is always to ship great *products* that execute extremely well on the hardware and that you are happy to develop further.

Floyd wants to replace C++, Rust, C#, Python. And Javascript. Only C can stay. Maybe Closure too, just out of respect.

##### VERTICAL LANGUAGE
Floyd is a vertical language: it cares deeply about **cache lines** and **hardware cores** and at the same time has keywords like **software-system** and primitived for interacting between concurrent processes.

It's still very small and easy to learn, by dropping a lot of cruft other programming languages think is needed. C++ - how many ways do we need to initialise a variable? Really?

##### THE MATRIX
Floyd is inspired by the movie The Matrix, which is obviously a great way to design a programming language!

- **Red Pill**: Knowledge, freedom, uncertainty and the brutal truths of reality
- **Blue Pill**: Security, happiness, beauty, and the blissful ignorance of illusion

Learn more here - things like **green processes**, **black magic** and the **red and blue pill**,  and other cool and colorful things: [www.floydlang.org](https://www.floydlang.org "Floyd language's Homepage")


# QUICK REFERENCE

![](readme_cheat_sheet.png)


# STATUS

The essentials of the Floyd are up and running and very robust (approx 1000 tests). The compiler generates byte code that runs quite fast on an interpreter. The manual is complete.

A handful features are needed for a proper 1.0: rounding out the language features somewhat and then *all about hardware performance*.


# IN THE BOX

|Item				| Feature	
|:---				|:---
| [Floyd Speak Manual](floyd_speak.md) | Programming language manual
| [Core Library Manual](floyd_speak_corelibs.md) | File system access, JSON support
| [Floyd Systems Manual](floyd_systems.md) | How to setup concurrency and change the world.
| **Floyd compiler** | Compiles Floyd Speak source code to byte code.
| **Floyd byte Code interpreter**	|Interprets your programs at approx. 7-15% of native speeds.

Floyd compilers and tools are written in portable C++17.


# LOOKING FORWARD

#### LANGUAGE FEATURES
- More int and float types
- Sum-type (enum/union) with switch
- Proper support for libraries
- Protocol type
- Basic Unicode functions
- Binary packing of data
- C language integration
- Built-in REST library

#### PERFORMANCE
- LLVM codegen
- Better internal threaded task manager
- Visual profiler, debugger
- Control over collection backend types, caching
