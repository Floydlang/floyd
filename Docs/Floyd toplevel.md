
![alt text](./floyd_logo.png "Floyd Logo")


# FLOYD SYSTEMS MANUAL

Floyd Script is the basic way to create logic. The code is isolated from the noise and troubles of the real world: everything is immutable and pure. Time does not advance. There is no concurrency or communication with other systems, no runtime errors.

Floyd Systems is how you make software that lives in the real world, where all those things happens all the time. Floyd allows you create huge software systems spanning computers and processes, handling communication and time advancing.

Floyd uses the C4 model to organize all of this. C4 model https://c4model.com/


### GOALS

1. A high-level way to organise huge code bases and systems with many threads, processes and computers, beyond functions, classes and modules and also represent those concepts through out: at the code level, in debugger, in profiler etc.
2. A simple and robust method for doing concurrency, communication etc.
3. Allow extreme performance and profiling as a separate thing done ONTOP of the correct logic.
4. Support visual design of the system.


# PERFORMANCE

Floyd is designed to make it practical to make big systems with performance better than what you get with average optimized C code.

It does this by splitting the design into two different concepts:

1. Encourage your logic and processing code to be simple and correct and to declare where there is opportunity to execute code independely of eachother.

2. At the top level profile execution and make high-level improvements that dramatically alter how the code is executed on the available hardware -- caching things, working batches, running in thread teams, running in parallell, ordering work for different localities, memory layouts and access patterns.


# C4 CONCEPTS

A complete system is organised into parts like this:


![alt text](./floyd_systems_software_system.png "Software System")


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

??? Components can be UNPURE!!


### CODE

Classes. Instance diagram. Examples.
Passive. Pure.

	jpeg_quantizer.floyd, colortab.floyd -- implementation source files for the jpeglib


# C4 DIAGRAMS

There is a set of standard diagram views for explaining and reasoning about your software:

![alt text](./floyd_systems_level1_diagram.png "Software System")
Level 1: System Context diagram


![alt text](./floyd_systems_level2_diagram.png "Software System")
Level 2: Container diagram


![alt text](./floyd_systems_level3_diagram.png "Software System")
Level 3: Component diagram

![alt text](./floyd_systems_level4_diagram.png "Software System")
Level 4: Code


Notice: a component used in several components or a piece of code that appears in several components = appears as duplicates. The perspective is: logic dependencies. There is no diagram for showing which source files or libraries that depends on eachother.


# MOTHERBOARD

??? What if you want a motherboard-like setup for each document?

The motherboard is how you implement a Floyd-based container. Other containers in your system may be implemented some other way and will be represented using a proxy in the Software System.

1. The motherboard connects all code together using wires into a product / app / executable using a declaration file.
2. It completely defines: concurrency, state, communication with ourside world and runtime errors of the container. This includes sockets, file systems, messages, screens, UI. Depndency components that are unpure are also shown.
3. Container clearly defines *all* independent *state*. All non-pure components.
4. Control performance by balancing memory, CPU and other resources. Profile control and optimize performance of system.

A motherboard is usually its own OS process. Since the motherboard is statically defined, things like new / delete of components are not possible.


### NON-GOALS

- Be reusable.
- To be composable
- Pure / free of side effects


The motherboard consists of a discrete number of components connected together with wires. The wires carries messages. A message is a Floyd value, usually an enum. Whenever a value, queue element or signal is mentioned, *any* of Floyd's types can be used -- even huge multi-gigabyte nested structs or collections.


Example components:

- Clock component: a component written in Floyd Script that has its own state & inbox.
- Multiplexer part: contains internal pool of clocks and distributes incoming messages to them to run in parallell.
- Built in local FS component: Read and write files, rename directory, swap temp files
- Built in S3 component
- Built in socket component
- Built in REST-component
- Built in UI-component
- Built in command line component: Interfaces with command line arguments / returns.
- Audio component that uses Direct X
- VST-plugin component
- A component written in C

Notice: these are all non-singletons - you can make many instances in one container

You can also connect wires, add tweakers and notes.

??? IDEA: Glue expressions, calling FLoyd functions


### CONCURRENCY: CLOCK, STATE AND THE INBOX

This it how to express time / mutation / concurrency in Floyd. For each independant state or "thread" you want in your container, you need to insert a clock-component and write its processing function.


### MOTHERBOARD FILE FORMAT

helloworld.board
This is a declarative file that describes the top-level structure of an app. Its contents looks like this:

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


# CONCURRENCY AND TIME IN DETAIL

The goal with Floyd's concurrency model is to be:

1. Simple and robust pre-made mechanisms for each concurrency needs. Avoid general-purpose!
2. Composable
3. Allow you to make a static design of your concurrency.
4. Separate out parallellism into a separate mechanism.
5. Avoid problematic constructs like threads, locks, callback hell, nested futures and await/asyc -- dead ends of concurrecy.
6. Supply means to expose execution parallellism for the separate parallisation features.
7. Let you control/tune how many threads and cores to use for what parts of the system independantly of the code.

The building block for this is the actor (similar to Erlang's or Elexir's processes) - using an inbox, a state value and a function.

The actor represents a little standalone process that listens to messages from other actors. Those actors may run in the same or other hardware threads or green threads or even synchronously.


When an actor receieves a message, its function is called (now or later) with that message and the actor's previous state. The actor does some work - something simple or a big call tree and it ends by returning its new state, which completes the message handling.

The actor function may be called synchronously when client posts it a message, or it may be run on a green thread, a hardware thread etc. It may be run the next hardware cycle or at some later time.

This is the only way to keep state in Floyd.


The inbox has two purposes:
	
1. Allow component to *wait* for external messages using the select() call.
	
2. Move messages between different clock-bases -- they are thread safe and atomic.

You can always post a message to *any* clock-component, even when it runs on another clock.


A clock is statically instantiated in a motherboard -- you cannot allocate them at runtime.

The actor function CAN chose to have several selects() which makes it work as a small state machine.

The actors cannot change any other state than its own, excep sending messages to other actors or call unpure functions.
A clock can send messages to other clocks, optionally blocking on a reply, handling replies via its inbox or not use replies.

The clock's function is unpure. The function can call OS-function, block on writes to disk, use sockets etc. Clocks needs to take all runtimes they require as arguments. Clock function also gets input token with access rights to systems.

The runtime can place clocks on different cores and processors or servers. You can have several clocks running in parallell, even on separate hardware. These forms separate clock circuits that are independent of eachother.

Clocks are inexpensive, you can use 100.000-ands of clocks. Clocks typically runs as M x N threads. They may be run from the main thread, a thread team or cooperatively.

Who decides when to advances the clocks? Runtime advances a clock when it has at least one message in its inbox. Runtime settings controls how runtime prioritizes the clocks when many have waiting messages.

An actor can be synced to another actor's clock. All posts to the inbox will then be synchrnous and blocking calls. These types of clock still have their own state and can be used as controllers / mediators -- even when it doesnt need its own thread.


### LIMITATIONS

Cannot find assets / ports / resources — those are handed to it.
Cannot go find resources, processes etc. These must be handed to it.
Clocks cannot be created or deleted at runtime.
Cannot create other clocks!

- ??? IDEA: Supervisors: this is a function that creates and tracks and restarts clocks.
- ??? System provices clocks for timers, ui-inputs etc. When setup, these receive user input messages etc.



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



### Ex: Simple app

This is a basic command line app, have only one clock that gathers ONE input value from the command line arguments, calls some pure Floyd Script functions on the arguments, reads and writes to the world, then finally return an integer result. A server app may have a lot more concurrency.



# THE FILE SYSTEM PART

All file system functions are blocking. If you want to do something else while waiting, run the file system calls in a separate clock. There are no futures, callbacks, async / await etc. It is very easy to write a function that does lots of processing and conditional file system calls etc and run it concurrently with other things.

??? Simple file API
	file_handle open_fileread(string path) unpure
	file_handle make_file(string path) unpure
	void close_file(file_handle h) unpure

	vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) unpure
	delete_fsnode(string path) unpure

	make_dir(string path, string name) unpure
	make_file(string path, string name) unpure


# THE REST-API PART

??? TBD



# PROBES AND TWEAKERS


### LOG-probe

- Pulse everytime a function is called
- Pulse everytime a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log


### Profiler-probe


### Command-line-part
		int on_commandline_input(string args) unpure
		void print(string text) unpure
		string readline() unpure


### CACHE TWEAKER

A cache will store the result of a previous computation and if a future computation uses the same function and same inputs, the execution is skipped and the previous value is returned directly.

A cache is always a shortcut for a (pure) function. You can decide if the cache works for *every* invocation of a function or limit the cache to invocations of the function within specified parent part.


### EAGER TWEAKER

Like a cache, but calculates its values *before* clients call the function. It can be used to create a static lookup table at app startup.


### BATCH TWEAKER

When a function is called, this part calls the function with similar parameters while the functions instructions and its data sits in the CPU caches.

You supply a function that takes the parameters and make variations of them.


### LAZY TWEAKER

Make the function return a future and don't calculate the real value until client accesses it.

### Watchdog probe

### Breakpoint probe

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



# MORE EXAMPLES


### Ex: VST-plugin

??? example Software System diagram.

![alt text](./floyd_systems_vst.png "VST-plugin")


Source file: my_vst_plug.board

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



## Destiny game
A video game may have several clocks:

- UI event loop clock
- Prefetch assets clock
- World-simulation / physics clock
- Rendering pass 1 clock
- Commit to OpenGL clock
- Audio streaming clock


# Basic console app
- main() one clock only.


# Instagram app
- main ui thread()
- rendering / scaling clock
- server comm clock






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



# NOTES AND IDEAS FOR FUTURE






### IDEA: FAN-IN-FAN-OUT PART

Like a clock but distributes messages on a number of paralell threads, then records the (out of order) results as they happen.

Has parallelism setting: increase > 1 to put on more threads with multiplex + merge. Built in supervision. Declarative - only settings, no code.


### IDEA: PIPELINE PART
??? No, supermap() is a better solution.

This allows you to configure a number of steps with queues between them. You supply a function for each step. All settings can be altered via UI or programatically.

This setting allows you to process more than one message in the inbox at once. If you set it to 100, up to 100 messages can be processed at the same time. Each message will be stamped 


### IDEA: TASK DISPATCHER

This part lets you queue up tasks that have dependencies between them, and it will r
??? This only increases performance = part of optimization. Make this something you declare to expose potential for paralell task processing. Supports a stream of jobs, like a game engine does.


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



# ??? OPEN ISSUES

	??? All OS-services are implemented as clock: you send them messages and they execute asynchronously.??? Nah, better to have blocking calls.


	- ??? Does UI_Part support several instances of VST plugin? Is plugin instance a value? Or do we make several instances of the board? When how can design know about all boards to do communication between them, for example sample-caching?

	- ??? CAN ONE PART HAVE SEVERAL PATHS FOR DIFFERENT CLOCKS? VST-HOST PART?

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
