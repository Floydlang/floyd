
# NORMALIZED CODE
This is coded in Floyd Script. This is the normal code. Try to have as much of your code normalized. It is composable, easy to understand and write. It is pure, have no side effects. No optimizations are done here (like lookups, lazy evaluation, batching). It's perfect to create complex logic, control states etc.
You cannot do callbacks or interfaces. You cannot read or affect the world.


# CLOCKS, CHANNELS, MOTHERBOARD

In the real world, normalized code isn't enough. Normalized code can be used to make a simple input -> program -> output program, like a simple terminal app.

Most programs in the world are processes that handles inputs, updates its state and controls side effects. In Floyd, this type of code is done in a motherboard and uses clocks and channels to advance time and communicate to the world.


# SIMULATION, SIDE EFFECTS & PURETIMES

Floyd functions are pure and can have no side effects. There are of course always side effects when running code on hardware. When a pure function runs, the CPU:s caches change state, the CPU:s registers change, memory is allocated and freed etc. Logging happens. CPU cycles are consumed, heat is generated etc.

In Floyd we make a distinction between inside-simulation and outside simulation.

### Inside simulation
This is what the world looks from within the simulation, where pure Floyd functions run in an ideal world:

- All functions executes in zero real-world time.
- All functions are completely side effect free.
- All functions executes as specified by source code.
- No functions fail because of external or exotic problems.
- Nothing around the code every changes -- the world is frozen while the functions execute (well they are run inifinitely fast).


### Outside simulation
But outside the simulation the real world happens. This is *hidden* from the simulation:
- Exceptions are thrown when running out of simulation memory, when exotic things or external events happens. Inside the simulation you cannot observer this.
- Functions can be precomputed or lazely computed instead of called as specified in source code.
- Functions have all sorts of side effects outside the simulation, that cannot be observed from within the simulation.


# Runtimes ("puretimes")
This is a Floyd mechanism that lets you build stacks of systems and run pure functions on top. Just like you can run a pure function ontop of a physical CPU and mutable RAM-memory and ontop of malloc() and free().

Goals:

- Allow you to cleanly compose sub-systems ontop of eachother to build huge and reliable software = composability.
- Explicit dependencies between systems.
- Allow Floyd to be used to create mechanisms like a photo cache, resource caches, memory pooling, background processes, logging.
- Explicit access control to systems: you can only allocate an image if your function has been handed that puretime.

 

These are systems that provide basic services to normalized code. Just like a memory manager does. Normalized code uses these but cannot observe any side effects (but there really are side-effects).

Runtimes are done outside of simulation and cannot be observed inside the simulation. Examples:
- Memory allocation. All normalized code assumes there is unlimited memory and cannot measure the memory pools. In out-of-memory conditions, the runtimes will roll back normalized code. The normalized code never knows this happens.
- Logging. Normalized code can log, but can never observe the logs or be affected by the logs.
- Optimizations (see list)

Runtimes are not globals / singletones -- you can run several at the same time. In realtime apps you might want a separate memory runtime to avoid blocking the threads.

A client needs to supply runtimes to all pure functions, often done by grouping them together into a meta-runtime.
You can create your own runtimes too. Use to make your own image caching.

**A runtime is run in its own environment and cannot leak details into the simulation. It contains its own clock.**








-----------------------------------------





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



# motherboard code
This is where time and externals and concurrency is handled.

Not composable, not usually reusable (copy & modify). This code takes application-level decisions. Has side-effects. Here all performance decisions are made, like memory vs CPU performance.



#ABI
All communication is done via immutable values. Conceptually they are JSON-values. C-ABI


#Floyd Runtime optimizations
- Lazy evaluation
- Caching recent data
- Batching
- Amortising
- Pooling
- Precalculating
- Paralellization
- Accellerate on GPUs


Amazon S3-persistens

# THE LONG STICK

1) Write C++ code in text editor.

2) Compile C++ code to another representation, machine code.

3) Load machine code on CPU and start executing it.

4) Program makes instances of objects and connects together in graphics, mutating graph and nodes through out runtime.

5) Program uses operating system to present parts of its state and mutations on the screen, or via TCP etc. 
