![](readme_floyd_logo.png)

License: MIT

# WHAT IS THE PROBLEM?

Most software sucks. Both to use and to improve. There is already a lot of dogma and conservatism that holds programming back.

Floyd intends to move the discussion.

While hardware designers have grown from thousands of transistors to billions over the last 40 years, programmers are still programming exactly the same way - typing individual if-statements and for-loops in emacs into 7 bit ASCII files. Exactly like our parents and grandparents did in the 60ies and 70ies.

![](readme_emacs.png)

Programming at this levels is the software equivalent of building the nextgen NVIDIA graphis card by placing NAND-gates onto the silicon, one by one.

From the other side, hardware designers have spent decades and enormous amount of on-chip transistors to try to work around the software industry's lack of progress. The hardware today tries to figure out the intent and structure of your code *from the machine code while executing it* to boost performance. These are things the software already knows!

Floyd will help us solve this.

BUT:

- If you love spending brainpower creating your own clever string classes, exploring new ways to think about software, feel strongly about *expressing yourself* in your code or just love the bad-ass feeling of *finally* tracking down an exciting threading problem -- then C++ is for you, not Floyd. Eventually you too may stumble across the finishing line of releasing a product.

- If you need intellectual rushes from mastering clever theoretical programming concepts, go read about set theory and monads or something, you are clearly not into programming to make great products.

- If you are obsessed about getting the compiler to generate optimal CPU instructions for every function no matter the cost - but don't care to learn how CPUs and caches actually work and how to best structure your *entire* program for best results -- use C++ with turned-off exceptions and no RTTI and make your own collection classes. You may be able to release a product at some point, but everyone will hate working on that code base.

- If you love emacs or vi and the only keyboard you care to use weights 2 kg and sounds like klockety-klock -- well maybe your children may be ready for Floyd.

When you've gotten all above crap out of your system and feel ready to make great and solid products that execute *extremely fast* you are welcome to Floyd!


# WHAT FLOYD WANTS TO DO

Floyd is intolerant of programming religion and conservativsm, tired of all distractions with shiny new language syntax.

Floyd knows programming is about engineering and making insightful compromises about how your *system* works and performs. About exploring your system from different angles and try improvements and learn what works.

Your code needs to be reshaped during its lifetime and Floyd makes sure this is smooth.

- You have limited brainpower, limited time, limited hardware -- great design is about figuring out how to spend these where it has maximum impact for the *product*.

Also at all cost avoid getting "design lumps" into your system that constantly limits how you can improve the software. "It can't be changed".

- Typing code into a function is **not** the right place to express yourself or do clever things.
- Programming is **not** math, programming is engineering.
- The new neat language syntax won't help you a bit.
- You don't make programs fast by trying to optimise the hell out of every single function in isolation. You don't make multithreaded programs by sprinkling thread-stuff all over your code.
- If you can't on request draw a clear overview picture how your system is structured -- then you can't possibly make system-wide decisions like which functions to optimize, what to cache and how to do *anything* with concurreny.

Floyd solves much of this by setting a simple but clear structure for how big systems (and small) needs to be organised and *enforces* this structure in the language from top to bottom to make us concentrate on making good software. All while trying not to scare conservative programmers with too big changes to their source code typing.

Just like structured programming replaced goto hell with more specific but limited if-else and while loops, Floyd tries to name and impose another structure on systems and how they interact and are composed.


# SO EXACTLY WHAT IS FLOYD?

Floyd is a general-purpose programming language with strong opinions on how to write complex and robust software products with minimum pain and minimum waste of time.

Floyd replaces languages like C++, C# and Java but also scripting languages like Python and Javascript. You can write something like Photoshop or Grand Theft Auto V or mobile apps using Floyd.

The end goal is *always* to ship great *products* that are a delight to develop further.

Floyd consists of a small and elegant toolkit of features built into the language that all go together to support you in this work. Every feature has been carefully picked, polished and adopted as a first class feature in the language. The goal is to have *one* simple way to do each common thing.

**What's *not* included and what you *can't* do is one of Floyds best features**

##### PRIMARY GOALS

1. Make it simpler to create robust large scale software products

2. Replace many bad programming ideas with a few good ones, critical to make code composable

3. Introduce high-level features needed for large scale software

4. Extreme execution speed, faster than practical in C systems

5. Nextgen visual and interactive tools

# UNIQUE FEATURES

1. Floyd splits your system into three areas: a) your program's logic, b) your program's processing and c) mapping its execution to the hardware.

2. Built-in structure for *complete* software systems including processes, people, concurrency and communication between processes

3. Visual and interactive tools

4. Carefully destilled set of syntactical features to move focus to the *system* rather than the inside of functions

5. Fusion of imperative programming but with the best bits of functional programming sneaked in discretely


Floyd has three big parts:

1. **Floyd Speak** - a neat new programming language for doing logic. It's statically typed with inference, compiles to byte code or native code. Here you program the bulk of your system.

2. **Floyd Systems** - does two things: defines your complete software system and its internal interactions and processes and concurrencyand B) Use Tweakers to map your software onto the hardware resources (cores, caches, RAM) 

3. **Floyd Studio** - a fast interactive and visual tool to play around with your code and visually wire things together, profile your system and apply heavy optimizations to it

This sounds really complicated and heavy-weight but it's not, as you'll see.


# SOLUTION PART 1/2 - FLOYD SPEAK

Floyd Speak is a fast and modern C-like program language that makes writing correct programs simpler and faster than any other programming language.

It's focus is composability (avoid lumbs in your system), simplicity and robust programming techniques.

##### FEATURES

1. All critical building blocks are in the language itself: strings, collections and JSON literals and serialization. Avoid DYI primitives!
2. Statically typed with type inference.
3. Higher level (slightly) than C or Javascript. No pointers
4. Byte code interpreter. Future: compiled to native using LLVM
5. Designed for simple parsing and tool creation
6. Values, persistent data structures
7. Automatic behaviors: comparable, copyable, roundtrip values -- no need for boilerplate code
8. Pure functions, imperative. Encapsulation and protocols but not OOP
9. Visual playground
10. Familiar look and feel for most programmers to reduce friction

##### COMPOSABILITY
Normally, Floyd Speak functions don't have side effects or write to files etc -- they are only doing logic (aka Pure functions). This makes them composable!

But when a function needs to call the OS or have other types of side effects they are called "unpure" functions. An unpure function can't just decide to access the OS - it needs to have one or several Toolboxes provided as argument(s) so it can get to those features. This makes those side effects explicit and part of the static signature of the function. You can't accidentally call an unpure function because you won't have the correct toolbox.

Floyd is not a functional language. You write statements, change local variables etc. But those changes cannot escape the function -- the are hidden inside the function. All to make the functions composable.

#### NON-GOALS

- Have neat syntactical features that do not improve final product.
- Be multiparadigm
- Be a real functional language
- Be a real object oriented
- Provide choice and expressing yourself
- Let programmer be 100% in control of the hardware
- Be an interesting intellectual challenge to master language


## CHEAT SHEET

![](readme_cheat_sheet.png)



## EXAMPLE CODE

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



## DETAILS

**Floyd Speak Manual:** [Floyd Speak Manual](floyd_speak.md).



## BAD IDEAS

There feature have been excluded from Floyd on purpose, because they are a bad idea:

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
16. Local threading decisions and assumptions
17. Shared mutable state
18. “Express your self in code”
19. Make your own linked list



# SOLUTION PART 2/3 - FLOYD SYSTEMS

This is a language that defines your complete software system and its internal interactions: components, concurrency, parallelism, communication, errors, time and mutation. The goals are these:

1. You can reason about it, present to no-developers and use to navigate visually

2. Clean concepts of robustness, concurrency, parallelism, communication, errors, time and mutation

3. Composable components

4. Simple to create tools

5. Visual profiling and editing

The second important feature of Floyd Systems is to configures how the system maps to the available hardware -- things like cores and threads, caches and RAM.

## SYSTEM STRUCTURE

Your software system is composed of containers (apps, servers and other programs), components (libraries) and code.

![](readme_software_system.png)



## EXAMPLE CONTAINER

This is a container with a bunch of actors wired together:

![](readme_floyd_systems_vst.png)

Concurrency is done using actors -- small processes inspired by Erlang processes. Those are the blue green and red boxes in the picture above. There are no threads, locks, atomics, await-async, nested callback hell etc.


Read more here: **Floyd Systems Manual**: [Floyd Systems Manual](floyd_systems.md), **Floyd Systems Reference**: [Floyd Systems Reference](floyd_systems_ref.md).



# SOLUTION PART 3/3 - FLOYD STUDIO

TODO 1.0
This is a webbased interactive tool for making, exploring and tuning complete software systems.

**Floyd Studio Manual**: [Floyd Studio Manual](floyd_studio.md).



# FLOYD - IN THE BOX

|Item				| Feature					| Link
|:---				|:---					|:---
| **Floyd Speak Manual**		|				|[Floyd Speak Manual](floyd_speak.md)
| **Floyd Speak compiler**		|Compiles Floyd Speak source code to byte code.
| **Floyd Speak interpreter**	|Runs your programs at approx 10% of native speeds.
| **Floyd Systems Manual**		|				|[Floyd Systems Manual](floyd_systems.md)
| **Floyd Systems Reference**	|				|[Floyd Systems Reference](floyd_systems_ref.md)
| **TODO POC: Floyd Systems compiler**			|compiles floyd systems and containers etc to byte code.|
| Standard library				|A number of basic components|

Floyd compilers and tools are written in portable C++11.


# BACKLOG
These are upcoming features, listed here to give you an idea of what the plan is: [Floyd Backlog](backlog.md)
