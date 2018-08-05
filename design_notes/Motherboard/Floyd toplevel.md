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


# Top-LEVEL CONCEPTS


### Person
Various human users of your software system

### Software System
Highest level of abstraction and describes something that delivers value to its users, whether they are human or not. 

Floyd file: **software-system.floyd**

### Container
A container represents something that hosts code or data. A container is something that needs to be running in order for the overall software system to work.
Mobile app, Server-side web application, Client-side web application, Microservice

This is usually a single OS-process, with mutation, time, several threads. It looks for resources and knows how to string things together inside the container.

### Component
Grouping of related functionality encapsulated behind a well-defined interface. Executes in one process - process agnostic. Requires using container to do mutation / concurrency.
JPEGLib, JSON lib. Custom component for syncing with your server.
Passive, pure.

	jpeglib.component.floyd -- declares a component, including docs and exposed API

### Code
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

### RESPONSIBILITIES:
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

A motherboard is usually its own OS process.



#### MOTHERBOARD PARTS
- Clock
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

??? Dynamic instancing. Create more HW-sockets and custom parts on demand. (*)-setting.
- Adapter, splitter, merger: expression that filters a signal


---------------

Q: External state when serveral mutable bits? One for each?

A: A motherboard has one state per clock. These states are internal to the motherboard, but external to each clock.


---------------


Q: Can you use and hide a board with clocks inside a pure function?

A: Yes, local mutation is fine but it cannot leak out of function. Unit tests are pure functions and they can make a board, test it, then dispose it.

Also: a board is similar to an iterator / generator and can be used the same way.


-----


MOTHERBOARDS

Boards behave very similar to functions except for a couple of important things:

1) they introduce time (observable by the user’s program itself) and therefore mutability. The keys to this are clocks and transformers.

2) They introduce several concurrent concepts of time = concurrency.


A board is a 2D surface that exposes pins of different types.

Inside you use chips, sub-boards, functions, constants, wires and time-transformers to implement your board.

A board always comes with a suite of tests.

You can package a board for general, shared reuse - this creates a “chip”.



---------------



?????????

CORE DESIGN QUESTION: What is dynamic data, what is static data, what is static code?

Does UI_Part support several instances of VST plugin? Is plugin instance a value? Or do we make several instances of the board? When how can design know about all boards to do communication between them, for example sample-caching?


---------------



??? CAN ONE PART HAVE SEVERAL PATHS FOR DIFFERENT CLOCKS? VST-HOST PART?

---------------




INSIGHT: We know in what context, motherboard, thread, callstack a function is called. It is in THIS context we can do tweaks. Not per static function.










---------------



Boards are statically designed, new / delete are not possible. Instead collections of objects are used

No dynamic allocation - introduce concept of fixed set of infinity instances, using indexes, like a pool / collection of instances.


Tag sections to work in “diff space”. Keyboard input etc. This is done via time-axises.

---------------




**Work process**

Can only be created declaratively.
Cannot go find resources, processes etc. These must be handed to it.

Uses a mutable process function to handle messages in inbox. It stays in loop. On return it is exited.


**Top level function**

Will find and connect all resources, runtimes and other dependencies.


** map(), fold() filter() **

Expose possible parallelism of pure function, like shaders.


** Work team **

Runs function F in parallell.



???? fan-out, fan-in, thread team. Thread slider 


---------------








# CONCURRENCY AND CLOCKS

### GOALS
- Small set of explicit and focused and restricted tools. Avoid general-purpose!
- No explicit instantiation of processes via code — they are declared statically inside a top-level process. All dynamic allocation of processes are done implicitly via pmap(), via build-in parallelism-setting on process.
- Composable
- Handle blocking, threading, concurrency, parallellism, async, distributed into OS processes, machines.
- Declarative and easy to understand.
- No DSL like nested completions -- all code should be regular code. No callback, no inversion of control or futures.


### CLOCKS
Floyd uses a simple way to model concurreny, threading and timing losely based on the Erlangs processes, called a "Clock".

A clock has a function, an input queue with values (can be big objects or primitives) an address and a previous-state value. Clients send values into a clocks input queue using its address. At some world-time, the function will be called with the first value in the queue and the clock's previous-state. Clocks consume messages one at a time.

The clock's function can either be pure or unpure.

Clocks are inexpensive, you can use 100.000-ands of clocks.
They may be run from the main thread, a thread team or cooperatively.

An unpure clock function may block on sync calls.

### PARRALLELLISM
Clocks are only used for mutable things, that tracks their own time separately from other clocks in the system. They are not for straight parallellism (like a graphics shader).
Accelerating computations (parallellism) is done using tweaks — a separate mechanism. It supports moving computations in time (lazy, eager, caching) and running work in parallell.
Often clocks are introduced in a system to expose oppotunity for parallellism.



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


# VALUES AND SIGNALS
All signals and values use Floyd's immutable types, provided by the Floyd runtime. Whenever a value, queue element or signal is mentioned, *any* of Floyd's types can be used -- even huge multi-gigabyte nested structs or collections.




### Work-Process
Concurrent lightweight process with separate address space.
Has inbox for values.
Keeps 1 state variable
Advances time.
State and inbox are owned by runtime, not process.
Has a unique adress in the world.
Is an “object”, not a dynamic ID.
Can mutate world.
Cannot find assets / ports / resources — those are handed to it.
Can call seqfunctions & core functions
DO NOT USE FOR PMAP ETC.
Cannot create other processes!

### Process-function
A function that has the correct signature to be plugged into a work-process. It handles events, can wait for different events in different parts of its callstack. (It can be a state machine using its callstack).

### Fan-in-fan-out Process
Has parallellism setting: increase > 1 to put on more threads with multiplex + merge. Built in supervision.
Declarative - only settings, no code.

### Supervisor-Process
A special type of process that mediates life and death and com of child processes.
Declarative - only settings, no code.

### Seqfunction
A function that can communicate with world, block och external calls. Non-pure.
Gets all runtimes as argument.

### Corefunc
A pure function.
Does logic, uses values only.
Cannot call blocking functions
Cannot access world
The bulk of the logic.



===============================

static clock graph | dynamic clocks vector
programmed layout | declarative layout

Like shaders

|A		|B		|C
|---	|---	|---
|yes	|yes	|Can call pure functions (runtime may throw exceptions, can use CPU, RAM, battery)
|yes	|yes	| Can call blocking functions
||			| Can have external sideffects -- file system, sockets
||			Can mutate global state
||			Can call functions that are unpure
||			Can call functions that throw exceptions (in addition to normal runtime exceptions)
||			Can manage clocks.
			
supervisor	--	Manages clocks, sets them up, reboots, allocate clocks etc. Motherboard.
clock<T, M>	--	A clock, holding a specific type of state, consuming a specific type of message.
effects_t	--	used as token to call file system, sockets etc.

clock<my_clock_state_t> tick(clock<my_clock_state_t> s, message<my_clock_message_t> m){
	... 
}




### CONCURRENCY CURRENT DESIGN

- Clock-concept. A clock is an object with a typed input state, typed input message and types output state. By returning a changed state it advances time. Only the runtime can call the clock-function.
- Clock function also gets input token with access rights to systems.
- Using its rights, a clock can call unpure functions, control outside world and send messages to other clocks.
- Supervisor: this is a function that creates and tracks and restarts clocks.
- The concept of clocks represents a top-level node that has its own state = concept of time.
- System provices clocks for timers, ui-inputs etc. When setup, these receive user input messages etc.
- A clock can send messages and block until it gets a reply.
- To make a background one-time thing: create a supervisor and set it up to spawn worker clocks, then delete them when they are done with task.
- Clocks runs as M x N threads.
- A supervisor can place clocks on different cores and processors or servers.
- A supervisor can 
- You can create clocks programmatically.
- You can create clocks and their routings using a JSON motherabord = supports interactive design by human,



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


### REFERENCES
- CSP
- Erlang
- Go routies and channels
- Clojure Core.Async



### IDEAS
- Split concept of go routine and channel / Erlang processes etc into several more specific concepts.

- Idea: Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?

- Use golang for motherboards. Visual editor/debugger. Use FloydScript for logic. Use vec/map/seq/struct for data.



### INSIGHTS

- Synchronization points between systems (state or concurrent) always breaks composition. Move to top level of product. ONE super-mediator per complete server-spanning solution.

- A system requires a master controller that may use and control sub-processes.

- A system can be statically layed out on a top-level. Some processes may be dynamically created / deleted -- this is shown using 1-2-3-many.

- Initialization often instances and connects parts together. In Floyd this is done using declaration / visual design outside of runtime.

- Sometimes we introduce concurreny to allow parallellism: multithreading a game engine is taking a non-concurrent design and making it concurrent to improve throughput. This is different to using concurrency to model real-world concurrency like UI vs background cloud com vs realtime audio processing. Maybe have different concepts for this?

### DIFFERENT APPROACHES TO CONCURRENCY

A) Use futures and promises, completions.
B) Block clock thread, hide time from program. This makes run_clock() a continuation!!!
C) clock
D) Clock result will contain list of pending operations, as completions. Now client decides what to do with them. This means do_clock() only makes one tick.

### EXAMPLE

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
	



# CLOCK & CLOCK CIRCUITS

A clock is a little virtual process that reads messages from its single input queue and acts on those inputs. To have time pass inside your system you need to have a clock, or a number of clocks. A clock is equivalent to an Erlang process.

The input queue holds message values of one specific type, as specified by the clock. This value can be an enum of different messages to handle different types of server requests or user input events etc.

A clock represents a series of discrete time increments / transformations. The output is always an immutable value. If you want concurrency you need to have several clocks, each running at their own rate.

- Each section on the board is part of exactly one clock circuit.
- A clock is not pure -- it can have side effects.
- A clock can send messages to other clocks.
- A clock has ONE internal value that represents its state aka memory. This is usually a big nested struct.
- A clock can communicate with the outside world, with file systems, sockets etc.
- A clock can call functions that block. This may cause the clock execution to be paused and some other code run.
- The clocks state-value is owned by its supervisor, not the clock itself.

Simple apps, like a basic command line apps, have only one clock that gathers input from the command line arguments, maybe calls some pure FloydScript functions on the arguments, reads and writes to the world, then finally return an integer result. A server app may have a lot more concurrency.

You can think about a handling a messages as a transaction, or even a commit to Git.

You can have several clocks running in parallell, even on separate hardware. These forms separate clock circuits that are independent of eachother.

The clock records a LOG of all generations of its value, in a forever-growing vector of states. In practice, those older generations are not kept or just kept for a short time.

??? A clock has one or more inputs and ONE output. The motherboard defines when the clock is advanced. At this moment, all its inputs are frozen and it's clock-function is called. This function is referenctial transparent. When the clock function is done it returns the result, which is stored in a log of all outputs. The previous output of a clock-function is always passed in as first argument to the clock-function the next time.


??? Replace this with a my_ui_process() that can contain a sequence of stuff and several blockings. No need to have clock.

		var my_socket = socket()
		void my_ui_process(){
			wait:
				my_socket: ui_clock_function.log += ui_clock_function(prev_clock)
				menu_selection_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
				mouse_input_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
				timeout(60.0) : ui_clock_function.log += ui_clock_function(prev_clock)
	
			do something else..
			wait:
				sdfsd
		}



# TRANSFORMER

This is how you connect wires between two different clock circuits. This is the ONLY way to send data beween clocks circuits. Use as few as possible - prefer making a composite value out of smaller values and transform that as ONE signal.

Transformers have settings specifying how it moves values from one clock to another. The default is that it samples the current input of the transformer. Alternatively you can get a vector of every change to the input since the last.

Notice that a transformer is NOT a queue. It holds ONE value and may records its history.

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


??? External clocks - these are clocks for different clients that communicate at different rates (think UI-event vs realtime output vs socket-input). Programmer uses clocks-concept to do this.


??? Channels should be built into vsthost. Also ins to vsthost for ui param requests. This mapes vsthost a highlevel chip.

State is never kept / mutated on the board - it is kept in the clock.


# PARALLELLISM

Making software faster by using distributing work to all CPUs and cores.

Future: add job-graph for parallellising more complex jobs with dependencies.

- Parallellism: runtime can chose to use many threads to accelerate the processing. This cannot be observed in any way by the user or programmer, except program is faster. Programmer controls this using tweakers. Threading problems eliminated.

??? BIG DEAL IN GAMING: MAKING DEPENDENCY GRAPH OF JOBS TO PARALLELAIZE








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


# IDEA: DIFF AND MERGE
Diff and merge are important to Motherboard code to detect what changes needs to be performed in the world.

