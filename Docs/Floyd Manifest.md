# FLOYD (NAME TBD)

Floyd is a nextgen programming language that gets rid of many problematic traditional programming concepts and introduces a few key new ones. It borrows from both the functional and imperative school of programming. It attempts to solve programming problems like correctness, composability, concurrency, performance. Floyd is both a better scripting language _and_ a better high-performance language.


MANIFEST: To Create a small general purpose languages that promotes great software engineering- for small and gigantic programs, embedded and distributed scripting and optimized engines - that has one simple way to do all basics. It raises the level of abstraction to let human, compilers, runtimes and hardware to do what they do best. Based on best practices, take a set of most important bits of modern languages and abandons most. Support visual programming / composition / architecture



Correctness first - program A - theoretical: Imagine all functions executing on an infinitely fast computer and design the program to *work* correctly.
Optimize second - Program B: Next optimise the program by using the Optimizer to do caching, lazy evaluation and other optimisations *on top* of the correct design. Other types of time: communication latency, screen refresh rate etc must be part of program A - theoretical design.





# OPINIONS

- Programming is not math
- All basics must be built-in
- Reduce dependencies: modules should in best case only depend on language, libraries and interfaces it exposes - no other modules.
- Only way to do every day things, but it's great
- Functions are no place to be creative.
- Do not confuse optimization and correctness. Optimizations are done globally.
- Side effects are evil
- The language is the languge, libraries are libraries.
- Observers, callbacks and similar are EVIl.


# GOALS

- Simple and minimal
- Elevate software design from expressions, functions and class to a higher level constructs.
- Guides you into writing correct and reliable software you can reason about. Little room to do creative coding
- Language has one built-in way of do each common thing: concurrency, strings, streams, collections, logging, tests, JSON, memory handling.
- Problematic features are gone (pointers, threads, mutation, aliasing, side-effects, manual operator implementation).
- Programs are faster than C, taking  threading and trimming into account.
- Ideal both for scripting web servers and writing optimized game engines
- Write and compose huge and reliable programs. Also quick to use for scripting.
- Great and practical way to think and implement mutation / time / side-effects.
- Litte learning friction for Javascript / Java / C# /C++ programmers
- Practical, pragmatic and hands-not - not theoretical mathy concepts.
- Allow huge, complex software using truly composable software design.
- Explicit and portable and future-proof
- Supports distributable / concurrent programs
- Interoperates with web, users and file system effortlessly
- Easy to write tools
- Advanced built-in profiling and optimization tools.
- Simple reliable module feature


# NON-GOALS

- C like hardware vs code equivalence. You cannot look at source code and know what instructions or bytes are generated.
- Real functional language
- All modern bells and whistles like generators from other hot languages
- Support creating custom low-level data structures in libraries, like C++ vs STL. Floyd has a clear distinction what is built into the language and what is libraries written ontop of the language. Collections and strings are part of language, not libraries on top of it.
- Detail control over threads and mutexes
