


# Normalized code
This is coded in Floyd Script.
This is the normal code. Try to have as much of your code normalized.

It is composable, easy to understand and write.
It is pure, have no side effects. No optimizations are done here (like lookups, lazy evaluation, batching).

You cannot do callbacks or interfaces. You cannot read or affect the world.



#motherboard code
This is where time and externals and concurrency is handled.

Not composable, not usually reusable (copy & modify). This code takes application-level decisions. Has side-effects. Here all performance decisions are made, like memory vs CPU performance.


#Runtimes ("puretimes")
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
