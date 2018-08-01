
# GOALS
- Small set of explicit and focused and restricted tools. Avoid general-purpose!
- No explicit instantiation of processes via code — they are declared statically inside a top-level process. All dynamic allocation of processes are done implicitly via pmap(), via build-in parallelism-setting on process.

- Composable
- Handle blocking, threading, concurrency, parallellism, async, distributed into OS processes, machines.
- Declarative and easy to understand.
- No DSL like nested completions -- all code should be regular code. No callback, no inversion of control or futures.



# SCENARIOS

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

Modelling concurrent processess are done using clocks. Accelerating computations (parallellism) is done using tweaks — a separate mechanism.

Processes are only used for mutable things, not for straight pure parallellism (like a shader).

# Toplevel
Process that CAN find resources and assets and configures other processes.

# Work-Process
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

# Process-function
A function that has the correct signature to be plugged into a work-process. It handles events, can wait for different events in different parts of its callstack. (It can be a state machine using its callstack).

# Fan-in-fan-out Process
Has parallellism setting: increase > 1 to put on more threads with multiplex + merge. Built in supervision.
Declarative - only settings, no code.

# Supervisor-Process
A special type of process that mediates life and death and com of child processes.
Declarative - only settings, no code.

# Seqfunction
A function that can communicate with world, block och external calls. Non-pure.
Gets all runtimes as argument.

# Corefunc
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




# CURRENT DESIGN

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


# REFERENCES
- CSP
- Erlang
- Go routies and channels
- Clojure Core.Async



# IDEAS
- Split concept of go routine and channel / Erlang processes etc into several more specific concepts.

- Idea: Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?

- Use golang for motherboards. Visual editor/debugger. Use FloydScript for logic. Use vec/map/seq/struct for data.



# INSIGHTS

- Synchronization points between systems (state or concurrent) always breaks composition. Move to top level of product. ONE super-mediator per complete server-spanning solution.

- A system requires a master controller that may use and control sub-processes.

- A system can be statically layed out on a top-level. Some processes may be dynamically created / deleted -- this is shown using 1-2-3-many.

- Initialization often instances and connects parts together. In Floyd this is done using declaration / visual design outside of runtime.

- Sometimes we introduce concurreny to allow parallellism: multithreading a game engine is taking a non-concurrent design and making it concurrent to improve throughput. This is different to using concurrency to model real-world concurrency like UI vs background cloud com vs realtime audio processing. Maybe have different concepts for this?

# DIFERENT APPROACHES TO CONCURRENCY

A) Use futures and promises, completions.
B) Block clock thread, hide time from program. This makes run_clock() a continuation!!!
C) clock
D) Clock result will contain list of pending operations, as completions. Now client decides what to do with them. This means do_clock() only makes one tick.

# EXAMPLE

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



# GOLANG
	
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
