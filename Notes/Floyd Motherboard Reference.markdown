

# MOTHERBOARD
The motherboard is the top-level design that connects all code together into a product / app / executable.

WORLD: The exposition between client code and the outside world. This includes sockets, file systems, messages, screens, UI.

### Responsibilities:
- Instantiate all parts of the final product and connect them together.
- Introduce time and mutation into the system.
- Use Floyd-script modules
- Call C functions
- Connect to the outside world, communicating with sockets, reading / writing files etc.
- Profile control and optimize performance of system.

### Non-goal
- Be reusable.
- To be composable
- Pure / free of side effects

The motherboard is a declarative system, based on JSON. Real-world performance decisions, profiling, optimizations, caching etc.

Most logic is done using Floyd Script, which is a pure, referential transparent language.

# VALUES AND SIGNALS
All signals and values use Floyd's immutable types, provided by the Floyd runtime. Whenever a value, queue element or signal is mentioned, *any* of Floyd's types can be used -- even huge nested structs or collections.

# MOTHERBOARD PARTS
- Clock
- Channel
- Glue expressions, calling FLoyd functions
- File system access: Read and write files, rename directory, swap temp files
- Socket access: Send REST command and handle it's response.
- UI access
- Non-pure OS features
- Interfaces with command line arguments / returns.
- Tweaks
- Performance probes
- Log probes
- Timeout
- Perform background work using another thread / clock / process
- Audio output buffers

		my first design.mboard

		{
			"major_version": 1,
			"minior_version": 0,

			"nodes": {
			}
		}

# CLOCKS AND CHANNELS-parts
A motherboard has one to many **clock**-parts. A clock-part is a little virtual process that perform a sequence of operations, call FloydScript, read and write to channels and other i/o. The clock-part introduces *time* and *mutation* to an otherwise pure and timeless program.

Clocks are modelled after Golang's Goroutines, complete with the **select**-statement. A clock-value is a first-class type but can only be used from the motherboard, never in Floyd Script.

Simple apps, like a basic command line apps, have only one clock-part that gathers input from the command line arguments, maybe calls some pure FloydScript functions on the arguments, reads and writes to the world, then finally return an integer result. A server app may have a lot more concurrency.

The clocks cannot share any non-const data, only const data. They can be connected together using channels.

A **channel** is a first-class type that channels a specific value. It is modelled after Golang's channels. Channels can only be used from the motherboard, not from FloydScript, since they have sideeffects. This is the only way to communicate between clocks.

Clocks and channels can be created dynamically in the Motherboard scripts too.



# FILE SYSTEM-part
??? Simple file API or use channels?

# REST-part
Channels?

# LOG-probe

# Profiler-probe

# Command-line-part

# Optimizations
These are settings you apply on wires.

- Insert read cache
- Insert write cache
- Precalculate / prefetch, eager
- Lazy-buffer
- Content de-duplication
- Cache in local file system
- Cache on Amazon S3
- Parallellize pure function
- Increase mutability
- Increase random access speed
- Increase forward read speed, stride
- Increase backward read speed, stride
- Rearrange nested composite (turn vec<pixel> to struct{ vec<red>, vec<green>, vec<blue> }
- Batching: make 64 value each time?
- Speculative batching with rewind.


# Floyd Script
All scripts are pure, cannot do file handling or communication at all. They need to return data so script in motherboard has enough info to perform mutations / communication. Return queues with commands / work on snapshots of the world, then let motherboard code diff / merge snapshot into world.


# ISSUES

1. Floyd script is referential transparent. Cannot access the world -- read files, write files, do socket communication etc.
2. Floys script is composable: cannot support completion callbacks, futures (as composition solution) or blocking. Instead the scripts works on big, composite snapshots.


# DIFF AND MERGE
Diff and merge are important to Motherboard code to detect what changes needs to be performed in the world.




# EXAMPLES

## "Halo 4"
A video game may have several clocks:

- UI event loop clock
- Prefetch assets clock
- World-simulation / physics clock
- Rendering pass 1 clock
- Commit to OpenGL clock
- Audio streaming clock


## VST-plug

- process() callback
- main ui()
- parameters -- separate clocks since it's undefined which thread it runs on.
- midi input
- transport



## Basic console app
- main() one clock only.


## Instagram app
- main ui thread()
- rendering / scaling clock
- server comm clock


