# FLOYD TOP-LEVEL MANUAL

Floyd Script is the basic way to create logic. The code is isolated from the noise and troubles of the real world: everything is immutable and pure. Time does not advance. There is no concurrency of communication with other systems etc.

Floyd toplevel is how you make software that lives in the real world, where all those things happens all the time. Floyd allows you create huge software systems spanning computers and processes, handling communication and time advancing.

Floyd uses the C4 model to organize all of this. C4 model https://c4model.com/


### GOALS

1. A high-level way to organise huge code bases and systems with many threads, processes and computer, beyond functions, classes and modules.
2. Represent those high level concepts at the code level too, not just on whiteboards. 
3. Automatically visualize the system in tools like profilers, code navigators, debuggers and IDEs.
4. Visual design of the system at the top level.


### NON-GOALS

1. Have notation like UML for all the details.


# PERFORMANCE

Floyd is designed to make it practical to make big systems with performance better than what you get with average optimized C code.

It does this by splitting the design into two different concepts:

1. Encourage your logic and processing code to be simple and correct and to declare where there is opportunity to execute code independely of eachother.

2. At the top level profile execution and make high-level improvements that dramatically alter how the code is executed on the available hardware -- caching things, working batches, running in thread teams, running in parallell, ordering work for different localities, memory layouts and access patterns.


# TOP-LEVEL CONCEPTS


### PERSON

Various human users of your software system


### Software System

Highest level of abstraction and describes something that delivers value to its users, whether they are human or not. 

Floyd file: **software-system.floyd**


### CONTAINER

A container represents something that hosts code or data. A container is something that needs to be running in order for the overall software system to work.
Mobile app, Server-side web application, Client-side web application, Microservice

This is usually a single OS-process, with mutation, time, several threads. It looks for resources and knows how to string things together inside the container.


### COMPONENT

Grouping of related functionality encapsulated behind a well-defined interface. Executes in one process - process agnostic. Requires using container to do mutation / concurrency.
JPEGLib, JSON lib. Custom component for syncing with your server.
Passive, pure.

	jpeglib.component.floyd -- declares a component, including docs and exposed API


### CODE

Classes. Instance diagram. Examples.
Passive. Pure.

	jpeg_quantizer.floyd, colortab.floyd -- implementation source files for the jpeglib


### DIAGRAMS

Level 1: System Context diagram
Level 2: Container diagram
Level 3: Component diagram
Level 4: Code

Notice: a component used in several components or a piece of code that appears in several components = appears as duplicates. The perspective is: logic dependencies. There is no diagram for showing which source files or libraries that depends on eachother.


# MOTHERBOARD = CONTAINER

The motherboard is how you implement a Floyd-based container. Other containers in your system may be implemented some other way and will be represented using a placeholder instead. The motherboard connects all code together into a product / app / executable. WORLD: The exposition between client code and the outside world. This includes sockets, file systems, messages, screens, UI.


### RESPONSIBILITIES

- Instantiate all parts of the final product and connect them together.
- Introduce time and mutation into the system.
- Use Floyd-script modules
- Call C functions
- Connect to the outside world, communicating with sockets, reading / writing files etc.
- Profile control and optimize performance of system.
- There are limits to how advanced logic code you can do in motherboard -- force programmer to do advanced coding in Floyd Script.
- Handle comuncation and timeouts.
- Handle errors
- Control performance by balancing memory, CPU and other resources.


### NON-GOALS

- Be reusable.
- To be composable
- Pure / free of side effects


The motherboard is a declarative system, based on JSON. Real-world performance decisions, profiling, optimizations, caching etc.

Boards are statically designed, new / delete are not possible. Instead collections of objects are used

No dynamic allocation - introduce concept of fixed set of infinity instances, using indexes, like a pool / collection of instances.

A motherboard is usually its own OS process.
Dynamic instancing. Create more HW-sockets and custom parts on demand. (*)-setting.



### MOTHERBOARD FEATURES

The motherboard consists of a discrete number of parts connected together with wires. The wires carries messages. A message is a Floyd value, usually a an enum. The messages can carry an integer or a string or a huge value describing an entire document or database.

Parts:
- Clocks
- States (memory)
- Component instances (see parts library below)
- Glue expressions, calling FLoyd functions
- Profiling probe
- Log probes
- Watchdogs
- Pipeline part
- Multiplexer part

??? parts vs components?


### MOTHERBOARD PARTS LIBRARY
Notice: these are all non-singletons - you can make many instances in one container

- A component written in Floyd Script.
- Built in pipeline component
- Built in local FS component: Read and write files, rename directory, swap temp files
- Built in S3 component
- Built in socket component
- Built in REST-component
- Built in UI-component
- Built in cache-component
- Built in command line component: Interfaces with command line arguments / returns.
- VST-component
- A component written in C
- Background worker component - perform background work using another thread / clock / process
- Audio component

		my first design.mboard

		{
			"major_version": 1,
			"minior_version": 0,

			"nodes": {
			}
		}


### MOTHERBOARD MAIN

Top level function

```
motherboard_main()
```

This is the motherboard's start function. It will find, create and and connect all resources, runtimes and other dependencies to boot up the motherboard and to do executibe decisions and balancing for the motherboard.


### VALUES AND SIGNALS

All signals and values use Floyd's immutable types, provided by the Floyd runtime. Whenever a value, queue element or signal is mentioned, *any* of Floyd's types can be used -- even huge multi-gigabyte nested structs or collections.


### CLOCKS

This it the means of implement time / mutation / concurrency.







### STATE

This is an independant memory on the motherboard. It is used to implement a controller or mediator and often used inside clocks to give them independent state.
???



	
	# CLOCKS
	
	Main-thread clock
		Floyd GUI clock ---^
		My GUI ---^
	Buffer-switch clock
		vst_instance ---^
		My Audio Engine ---^
	Background sample-loader clock
	
	
	
	# STATES
	
	Clock 1 -- main
		Floyd GUI
		My GUI
		vst_instance::GUI
	Clock 2 -- buffer-switch
		vst_instance::buffer-switch
		My Audio Engine
	Clock 3 -- background loader
		Background Sample Loading
	






# CLOCKS AND CONCURRENCY


### GOALS

- Small set of explicit and focused and restricted tools. Avoid general-purpose!
- No explicit instantiation of processes via code — they are declared statically inside a top-level process. All dynamic allocation of processes are done implicitly via pmap(), via build-in parallelism-setting on process.
- Composable
- Handle blocking, threading, concurrency, parallelism, async, distributed into OS processes, machines.
- Declarative and easy to understand.
- No DSL like nested completions -- all code should be regular code. No callback, no inversion of control or futures.
- No threads or mutaxes -- operate at a higher level where it's possible to reason about concurrency.
- OS threading hidden.


### NON-GOALS

- Detailed control over individual OS threads



### CLOCKS

A clock is statically instantiated in a motherboard -- you cannot allocate them at runtime.

Uses a mutable process function to handle messages in inbox. Function returns when it has handled its message.

The clock function is called once per input value, then returns when that value has been processed. It CAN chose to block on further input values, which makes it a small state machine.

Floyd uses a simple way to model concurreny, threading and timing losely based on the Erlangs processes, called a "Clock".

A clock has:
	1. a function
	2. an inbox of 0...any values. This is an input queue with values. The values can be big objects or primitives.
	3. an address, unique in the worl
	4. a previous-state value
	5. Its own address space


Clients send values into a clocks input queue using its address. At some world-time, the clock's function will be called with the first value in the queue and the clock's previous-state. When the clock function returns, it provides the next current-value. Only the runtime can call the clock-function.

Clocks consume messages one at a time, in series.

Clocks tracks their own time separately from other clocks in the system.

The clock's function is unpure. The function can call OS-function, block on writes to disk, use sockets etc. Clocks needs to take all runtimes they require as arguments. Clock function also gets input token with access rights to systems.

A clock can send messages to other clocks, optionally blocking on a reply, handling replies via its inbox or not use replies.

The runtime can place clocks on different cores and processors or servers. You can have several clocks running in parallell, even on separate hardware. These forms separate clock circuits that are independent of eachother.

Clocks are inexpensive, you can use 100.000-ands of clocks. Clocks typically runs as M x N threads. They may be run from the main thread, a thread team or cooperatively.


Who advances the clocks? Runtime advances a clock when it has at least one message in its inbox. Runtime settings controls how runtime prioritizes the clocks when many have waiting messages.


### CLOCK LIMITATIONS

Cannot find assets / ports / resources — those are handed to it.
Cannot go find resources, processes etc. These must be handed to it.
Clocks cannot be created or deleted at runtime.
Cannot create other clocks!


- ??? Supervisor: this is a function that creates and tracks and restarts clocks.
- ??? System provices clocks for timers, ui-inputs etc. When setup, these receive user input messages etc.


### STATIC DEFINITION

```
	struct clock_xyz_state_t {
		int timestamp
		[xyz_record_t] recs
	}

	struct clock_xyz_message_st {
		int timestamp
		int mouse_x
		int mouse_y
		case stop: struct { int duration }
		case on_mouse_move:
		case on_click: struct { int button_index }
	}
	clock_xyz_state_t tick_clock_xyz([clock_xyz_state_t] history, clock_xyz_message_st message){
	}
```


### CLOCK SYNC

A clock can be synced to another clock. All posts to the inbox will be sync calls, that blocks.

These types of clock still have their own state and can be used as controllers / mediators -- even when it doesnt need its own thread.


### Ex: Simple app

This is a basic command line app, have only one clock that gathers ONE input value from the command line arguments, calls some pure Floyd Script functions on the arguments, reads and writes to the world, then finally return an integer result. A server app may have a lot more concurrency.



### CONCURRENCY SCENARIOS

1. Move data between concurrent modules (instead of atomic operation / queue / mutex)
2. Do blocking call, like REST request.
3. Start non-blocking lambda operations - referential transparent - like calculating high-quality image in the background.
4. Start non-blocking unpure background calculation (auto save doc)
5. Start lengthy operation, non-blocking using passive future
6. Run process concurrently, like analyze game world to prefetch assets
7. handle requests from OS quickly, like call to audio buffer switch process()
8. Implement a generator / seq / iterator for lazy calculation during foreach
9. Enable parallelization by hiding function behind channel (allows it to be parallellized)
10. Coroutines let you time slice heavy computation so that it runs a little bit in each frame.
11. Concept: State-less lambda. Takes time but is referential transparent. These can be callable from pure code. Not true!! This introduces cow time.
12. Model a process, like Erlang. UI process, realtime audio process etc.
13. Supply a service that runs all the time, like GO netpoller. Hmm. Maybe and audio thread works like this too?
14. Wait on timer
15. Wait on message from socket
16. Send message, await reply
17. Make sequence of calls, block on each
18. Simple command line app, with only one clock.
19. Small server
20. Fan-in fan-out
21. low-priority, long running background thread
22. Processing pipeline with many stages
23. 

### EXAMPLE CLOCK

	defclock mytype1_t myclock1(mytype1_t prev, read_transformer<mytype2_t> transform_a, write_transformer<mytype2_t> transform_b){
		... init stuff.
		...	state is local variable.

		let a = readfile()	//	blocking

		while(true){	
			let b = transform_a.read()	//	Will sample / block,  depending on transformer mode.
			let c = a * b
			transform_b.store(c)	//	Will push on tranformer / block, depending on transformer mode.

			//	Runs each task in parallell and returns when all are complete. Returns a vector of values of type T.
			//	Use return to finish a task with a value. Use break to finish and abort any other non-complete tasks.
			//??? Split to parallell_tasks_race() and parallell_tasks_all().
			let d = parallell_tasks<T>(timeout: 10000) {
				1:
					sleep(3)
					return 2
				2:
					sleep(4)
					break 100
				3:
					sleep(1)
					return 99
			}	
			let e = map_parallell(seq(i: 0..<1000)){ return $0 * 2 }
			let f = fold_parallell(0, seq(i: 0..<1000)){ return $0 * 2 }
			let g = filter_parallell(seq(i: 0..<1000)){ return $0 == 1 }
		}
	}


# IDEA: FAN-IN-FAN-OUT PART

Like a clock but distributes messages on a number of paralell threads, then records the (out of order) results as they happen.

Has parallelism setting: increase > 1 to put on more threads with multiplex + merge. Built in supervision. Declarative - only settings, no code.


# IDEA: PIPELINE PART

This allows you to configure a number of steps with queues between them. You supply a function for each step. All settings can be altered via UI or programatically.

This setting allows you to process more than one message in the inbox at once. If you set it to 100, up to 100 messages can be processed at the same time. Each message will be stamped 




# IDEA: TASK DISPATCHER

This part lets you queue up tasks that have dependencies between them, and it will r
??? This only increases performance = part of optimization. Make this something you declare to expose potential for paralell task processing. Supports a stream of jobs, like a game engine does.


# ??? OPEN ISSUES

	??? All OS-services are implemented as clock: you send them messages and they execute asynchronously.??? Nah, better to have blocking calls.


	- ??? Does UI_Part support several instances of VST plugin? Is plugin instance a value? Or do we make several instances of the board? When how can design know about all boards to do communication between them, for example sample-caching?

	- ??? CAN ONE PART HAVE SEVERAL PATHS FOR DIFFERENT CLOCKS? VST-HOST PART?



# PARALLELISM

They are not for straight parallelism (like a graphics shader).
Accelerating computations (parallelism) is done using tweaks — a separate mechanism. It supports moving computations in time (lazy, eager, caching) and running work in parallell.
Often clocks are introduced in a system to expose oppotunity for parallelism.


Making software faster by using distributing work to all CPUs and cores.

Future: add job-graph for parallellising more complex jobs with dependencies.

- parallelism: runtime can chose to use many threads to accelerate the processing. This cannot be observed in any way by the user or programmer, except program is faster. Programmer controls this using tweakers. Threading problems eliminated.

??? BIG DEAL IN GAMING: MAKING DEPENDENCY GRAPH OF JOBS TO PARALLELIZE


### PARALLISABLE FUNCTIONS

map(), fold() filter()

Expose possible parallelism of pure function, like shaders, at the code level (not declarative). The supplied function must be pure.


??? What if you want a motherboard-like setup for each document?
??? make pipeline part. https://blog.golang.org/pipelines



### SUPERMAP FUNCTION

	[int:R] supermap(tasks: [T, [int], f: R (T, [R]))

This function runs a bunch of tasks with dependencies between them and waits for them all to complete.

- Tasks can block.
- Tasks cannot generate new tasks. A task *can* call supermap.

Notice: supermap() shares threads with other mechanisms in the Floyd runtime. This mean that even if your tasks cannot be distributed to all execution units, other things going on can fill those execution gaps with other work.

- **tasks**: a vector of tasks and their dependencies. A task is a value of type T. T can be an enum to allow mixing different types of tasks. Each task also has a vector of integers tell which other tasks it depends upon. The task will not be executed until those tasks have been run. The indexes are the indexes into the tasks-vector.

- **f**: this is your function that processes one T and returns a result R. The function must not depend on the order in which tasks execute. When f is called, all the tasks dependency tasks have already been executed and you get their results in [R].

- **result**: a vector with one element for each element in the tasks-argument. The order of the elements are undefined. The int specifies which task, the R is its result.

When supermap() returns all tasks have been completed.

Notice: your function f can send messages to a clock — this means another clock can start consuming results while supermap() is still running.


??? IDEA: Make this a two-step process. First analyse tasks into an execution description. Then use that description to run the tasks.

This lets you keep the execution description for next time, if tasks are the same.

Also lets you inspect the execution description & improve it or create one for scratch.


- If IO is bottleneck, try to spread out IO over time. If IO blocks is bottleneck, try to start IO ASAP.

- Try to keep instructions and data in CPU caches.






# OTHER MOTHERBOARD FEATURES


### FILE SYSTEM-part

??? Simple file API or use channels?

### REST-part

Channels?

### LOG-probe

- Pulse everytime a function is called
- Pulse everytime a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log

### Profiler-probe

### Command-line-part

### Tweaks - Optimizations

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



# MOTHERBOARD EXAMPLES

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

# File API

		file_handle open_fileread(string path) unpure
		file_handle make_file(string path) unpure
		void close_file(file_handle h) unpure

		vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) unpure
		delete_fsnode(string path) unpure

		make_dir(string path, string name) unpure
		make_file(string path, string name) unpure

# Command line

		int on_commandline_input(string args) unpure
		void print(string text) unpure
		string readline() unpure

# helloworld.board
This is a declarative file that describes the top-level structure of an app.


# VST Plug -- my_vst_plug.board

		//	IMPORTED FROM GUI
		struct uievent {
			time timestamp;
			variant<>
				struct {
					int x;
					int y;
					int mouse_button
					uint32 mods
				} click;
				struct {
					int keycode;
					int x;
					int y;
					uint32 mods;
				} key;
				struct {
					rect dirty
					channel<obuf> reply
				} draw;
				struct {} activate;
				struct {} deactivate;
			} type;
		}


		struct ibuf {
			time timestamp;
			[float] left_input;
			[float] right_input;
			channel<obuf> reply;
		}

		struct obuf {
			[float] left_output;
			[float] right_output;
		}

		struct param {
			int param_id;
			float time;
			float value;
		}

		int ui_process(){
			m = ui_state()
			while(){
				select {
					case events.pop() {
						r = on_uievent(m, _)
						if(_.data.type == draw){
							_.reply.push(r._paint_image)
							m = r.m
						}
						else{
							control.push_vec(r.control_params)
							m = r.m
						}
					}
					case close {
						return nil
					}
				}
			}
		}
	
		int audio_stream_part(){
			m = audio_stream_ds(2, 44100)
			while(){
				select {
					case audiostream.pop() {
						m = process(m, _)
					}
					case control.pop {
						return nil
					}
					case close {
						return nil
					}
				}
			}
		}

		board = JSON
			[
				{
					"doc": "you need to import pins.",
					"label": "vsthost",
					"type": "vsthost",
					"def_pins" : [
						[ "outpin", "request_param", "request_parameter()" ],
						[ "inpin", "midi_in", "on_midi_input()" ],
						[ "inpin", "set_param", "on_set_parameter()" ],
						[ "inpin", "process", "process()" ]
					]
				},
			
				{
					"doc": "A channel always have an input pin called *in* and an output port called *out*",
					"label": "events",
					"type": "channel", 
					"element": "uievent",
					"mode": "block"
				},
				{
					"label": "control", "type": "channel", "element": "param", "mode": "block"
				},
				{
					"label": "audiostream", "type": "channel", "element": "ibuf", "mode": "block"
				},
			
				{
					"type": "ui_part", "process": "ui_process"
				},
			
				{
					"type": "audio_stream_part", "process": "ui_process"
				},
			
				{
					"type": "wires",
					"wires": [
						[ "vsthost.midi", "control.in" ],
						[ "vsthost.gui", "events.in" ],
						[ "events.out", "ui_part.uievent" ],
						[ "ui_part.control", "control.in" ],
						[ "vsthost.set_param", "control.in" ],
						[ "vsthost.process", "audiostream.in" ],
						[ "control.out", "audio_stream_part.control" ],
						[ "audiostream.out", "audio_stream_part.audio" ]
					]
				}
			
			]



# NOTES AND IDEAS FOR FUTURE



### IDEA: SUPERVISOR CLOCK

A special type of process that mediates life and death and com of child processes.
Declarative - only settings, no code.


clock<my_clock_state_t> tick(clock<my_clock_state_t> s, message<my_clock_message_t> m){
	... 
}


### IDEA: DIFF AND MERGE

Diff and merge are important to Motherboard code to detect what changes needs to be performed in the world.

### REFERENCES

- CSP
- Erlang
- Go routies and channels
- Clojure Core.Async



### IDEAS

- ??? The clock records a LOG of all generations of its value, in a forever-growing vector of states. In practice, those older generations are not kept or just kept for a short time. RESULT: this is not done automatically. It can be implemented as a vector in clock's state.

- Add preset-containers": for S3, for local FS cache etc. Has settings. Can do static initialization and also update settings from code.

- Split concept of go routine and channel / Erlang processes etc into several more specific concepts.

- Idea: Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?

- Use golang for motherboards. Visual editor/debugger. Use FloydScript for logic. Use vec/map/seq/struct for data.


### INSIGHTS

- Synchronization points between systems (state or concurrent) always breaks composition. Move to top level of product. ONE super-mediator per complete server-spanning solution.

- A system requires a master controller that may use and control sub-processes.

- A system can be statically layed out on a top-level. Some processes may be dynamically created / deleted -- this is shown using 1-2-3-many.

- Initialization often instances and connects parts together. In Floyd this is done using declaration / visual design outside of runtime.

- Sometimes we introduce concurreny to allow parallelism: multithreading a game engine is taking a non-concurrent design and making it concurrent to improve throughput. This is different to using concurrency to model real-world concurrency like UI vs background cloud com vs realtime audio processing. Maybe have different concepts for this?



### GOLANG
	
	func main() {
		c1 := make(chan string)
		c2 := make(chan string)
		go func() {
			time.Sleep(1 * time.Second)
			c1 <- "one"
		}()
		go func() {
			time.Sleep(2 * time.Second)
			c2 <- "two"
		}()
		for i := 0; i < 2; i++ {
			select {
			case msg1 := <-c1:
				fmt.Println("received", msg1)
			case msg2 := <-c2:
				fmt.Println("received", msg2)
			}
		}
	}



### IDEA: TRANSFORMER / OPTO-COUPLER WITH MODES



- ??? This is how you connect wires between two different clock circuits. This is the ONLY way to send data beween clocks circuits. Use as few transformers as possible - prefer making a composite value out of smaller values and transform that as ONE signal.

- ??? Transformers have settings specifying how it moves values from one clock to another. The default is that it samples the current input of the transformer. Alternatively you can get a vector of every change to the input since the last. Notice that a transformer is NOT a queue. It holds ONE value and may records its history.


These are setup to one of these:

1. sample-value: Make snapshot of current value. If no current value, returns none.

2. get-new-writes: Get a vector with every value that has been written (including double-writes) since last read of the transformer.

3. get-all-writes: Get a vector of EVERY value EVER written to the transformer. This includes double-writes. Doesn't reset the history.

4. block-until-value: Block execution until a value exists in the transformer. Pops the value. If value already exists: returns immediately.


READER modes:
- Default: reader blocks until new value is written be writer.
- Sample the latest value in the optocoupler, don't block.
- Sample queue of all values since last read.
Notice that the value can be a huge struct with all app state or just a message or a UI event etc.

Notice: pure functions cannot access optocouplers. They would not be referential transparent.
