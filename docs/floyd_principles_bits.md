# RISKY CODE


- Store reference to mutable data as optimization
- Caching, precalculating, lazy evaluation. Keep cache correct.
- Many paths to storing the same output property
- Several functions with parasitic behaviour between them (Ex: two functions both updating the same dialog box fields, from different user inputs. Both updates same field).
- Mutation
- Store data to avoid recomputation
- Incremental changes over object with invariant over time -- instead: compute correct object each time.
- Callbacks, observers that have side effects. Instead: no side effects. Make complete change, then diff new value with old value and 
- File systems: many unknown clients
- Keep two parallell systems in sync: data model graph, server database, view hiearchy. Store each in a variable. Use diff and merge.

- Pointers and backlinks. Weak and strong.

- Many mediators making independant decisions.





# TESTS & examples

Test are always run from the bottom up in the physical dependency tree.

Compiler warns on input combinations not covered by tests.

Tests show RED / GREEN and are interactively evaluated.


# Source file formats
Uses text files for all data - designed for easy search and replace, merge and diff etc.

All data formats are normalized for easy tool support and round tripping.

JSON?

Markdown-style with ASCII art?




# TIME AND PLACE & MOTHERBOARD

# Why circuit boards?

* Calculations: functions, data structures, collections - no time.
* Time and externals: mutability, clocks, concurrency, transformers  - circuit boards
* Optimisations: measure / profile / tweak runtime.z

* To get 2D spatial reasoning.
* To use instances rather than code that creates instances = cutting one level of abstraction.






# THE LONG STICK

1) Write C++ code in text editor.

2) Compile C++ code to another representation, machine code.

3) Load machine code on CPU and start executing it.

4) Program makes instances of objects and connects together in graphics, mutating graph and nodes through out runtime.

5) Program uses operating system to present parts of its state and mutations on the screen, or via TCP etc. 






# RUNTIME

Jag har en design för floyd som kallas "runtime" tills vidare. Det är en sorts kontext man skickar med funktioner. Man kan aggregera runtimes. Klientkoden kan inte observera förändringar i runtimen, den vet bara att runtimens features är tillgängliga. Tänkt att användas för att loggning, heaps och sådant, trots a Floyd är pure. Också tänk för att ersätta subsystem med singletons, som är ett helvete i större program.
Man kan t.ex. dra igång en runtime för bildcachning, eller för att paga ljud till/från disk asynkront eller för att hantera resursfiler eller för att göra interning på värden etc.


RUNTIME INSTEAD OF INTRUSIVE CALL GRAPHS

The runtime evaluates the flow of function calls can chose to cache calls etc.

This is different to traditional languages where functions of the program directly call eachother without any supervising runtime being involved at all.







# NORMALIZED CODE
This is coded in Floyd Script.
This is the normal code. Try to have as much of your code normalized.

It is composable, easy to understand and write.
It is pure, have no side effects. No optimizations are done here (like lookups, lazy evaluation, batching).

You cannot do callbacks or interfaces. You cannot read or affect the world.



# motherboard code
This is where time and externals and concurrency is handled.

Not composable, not usually reusable (copy & modify). This code takes application-level decisions. Has side-effects. Here all performance decisions are made, like memory vs CPU performance.


# Runtimes ("puretimes")
These are systems that provide basic services to normalized code. Just like a memory manager does. Normalized code uses these but cannot observe any side effects (but there really are side-effects).

Runtimes are done outside of simulation and cannot be observed inside the simulation. Examples:
- Memory allocation. All normalized code assumes there is unlimited memory and cannot measure the memory pools. In out-of-memory conditions, the runtimes will roll back normalized code. The normalized code never knows this happens.
- Logging. Normalized code can log, but can never observe the logs or be affected by the logs.
- Optimizations (see list)

Runtimes are not globals / singletones -- you can run several at the same time. In realtime apps you might want a separate memory runtime to avoid blocking the threads.

A client needs to supply runtimes to all pure functions, often done by grouping them together into a meta-runtime.
You can create your own runtimes too. Use to make your own image caching. A runtime is run in its own environment and cannot leak details into the simulation.



#ABI
All communication is done via immutable values. Conceptually they are JSON-values. C-ABI


#Runtime optimizations
- Lazy evaluation
- Caching recent data
- Batching
- Amortising
- Pooling
- Precalculating
- Paralellization
- Accellerate on GPUs


Amazon S3-persistens
