
# GOALS

- One well-defined and composable method of async.
- No DSL like nested completions.
- No concept of threads
- No callback hell or inversion of control


Modelling concurrent processess are done using clocks. Accelerating computations (parallellism) is done using tweaks — a separate mechanism.

- Will have the same state until it advances to next generation, by incrementing clock.

# CLOCK & CLOCK CIRCUITS

A clock is an object that controls the advancement of time. This is how you mutate values in an otherwise immutable process. You can also think about a clock step as a transaction, or even a commit to Git.

You can have several clocks running in parallell. These forms separate clock circuits that are independent of eachother.

**You cannot send data between clock circuits -- only inside of them -- you need to use a clock-transformer.**

Each section on the board is part of exactly one clock circuit.

Use a transformer to send data between clock circuits.

A clock holds ONE value - the complete state of the clock circuit. When advancing time, this value is sent into the clock-circuit and a new value is generated and stored in the clock.

No simple mutation is done to the value in the clock, instead the clock records a LOG of all generations of the boards state, in a forever-growing vector of states. In practice, those older generations are not kept for just kept for a short time.


# TRANSFORMER

This is how you connect wires between two different clock circuits. This is the ONLY way to send data beween clocks circuits. Use as few as possible - prefer making a composite value out of smaller values and transform that as one signal.

Transformers have settings specifying how it moves values from one clock to another. The default is that it samples the current input of the transformer. Alternatively you can get a vector of every change to the input since the last.

Notice that a transformer is NOT a queue. It holds ONE value and may records its history.

These are setup to one of these:

1. sample-value: Make snapshot of current value. If no current value, returns none.

2. get-new-writes: Get a vector with every value that has been written (including double-writes) since last read of the transformer.

3. get-all-writes: Get a vector of EVERY value EVER written to the transformer. This includes double-writes. Doesn't reset the history.

4. block-until-value: Block execution until a value exists in the transformer. Pops the value. If value already exists: returns immediately.






- External clocks - these are clocks for different clients that communicate at different rates (think UI-event vs realtime output vs socket-input). Programmer uses clocks-concept to do this.

- Parallellism: runtime can chose to use many threads to accelerate the processing. This cannot be observed in any way by the user or programmer, except program is faster. Programmer controls this using tweakers. Threading problems eliminated.



??? Channels should be built into vsthost. Also ins to vsthost for ui param requests. This mapes vsthost a highlevel chip.

State is never kept / mutated on the board - it is kept in the clock.



??? BIG DEAL IN GAMING: MAKING DEPENDENCY GRAPH OF JOBS TO PARALLELAIZE



# BLOCKING AND ASYNC


def_clock magic_state_t {
	magic_state_t init(){
	}


	magic_state_t on_select(){

	}
}





Wait on timer
Wait on message from socket
Send message, await reply
Make sequence of calls, block on each









??????????????????????????????


# CLOCKS AND CHANNELS
A motherboard has one to many **clock**-parts. A clock-part is a little virtual process that perform a sequence of operations, call FloydScript, read and write to channels and other i/o. The clock-part introduces *time* and *mutation* to an otherwise pure and timeless program.

Clocks are modelled after Golang's Goroutines, complete with the **select**-statement. A clock-value is a first-class type but can only be used from the motherboard, never in Floyd Script.

Simple apps, like a basic command line apps, have only one clock-part that gathers input from the command line arguments, maybe calls some pure FloydScript functions on the arguments, reads and writes to the world, then finally return an integer result. A server app may have a lot more concurrency.

The clocks cannot share any non-const data, only const data. They can be connected together using channels.

A **channel** is a first-class type that channels a specific value. It is modelled after Golang's channels. Channels can only be used from the motherboard, not from FloydScript, since they have sideeffects. This is the only way to communicate between clocks.

Clocks and channels can be created dynamically in the Motherboard scripts too.




# INSIGHTS

- Synchronization points between systems (state or concurrent) always breaks composition. Move to top of product. ONE super-mediator per complete server-spanning solution.

- Differentiate between _parts_ (node instances of channels and coroutines that you position and wire together) with values of channels and corutines, that are created programatially and dynamically.



??? make select() expression based, no statements.
??? make ALL code in motherboard expressions-only.
??? How are we sure process() never blocks + context switches becasue control.pop() is running?
??? Are pin function calls, callback?


midi_in could be ??? which?
A) a callback - you put code there that reacts vs queues into a channel.
B) an output pin that wants to write to an input pin.

Can you connect an input pin directory to an output pin, no channel involved?


??? Split channel into several concepts:
	A: block-both1,
	B: sampleN -- Channel supports 1 or other fixed number of elements. Read when empty = nil. 
	C: blockN -- Like B but blocks reader.
	


# IDEAS
- Split concept of go routine and channel into several more specific concepts.

- Use golang for motherboards. Visual editor/debugger. Use FloydScript for logic. Use vec/map/seq/struct for data.



# Use-cases for GO routines & channels
- move data between clocks (instead of atomic operation / queue / mutex)
- do blocking call, like REST request.
- start non-blocking lambda operations - referential transparent - like calculating high-quality image in the background.
- start non-blocking unpure background calculation (auto save doc)
- start lengthy operation, non-blocking using passive future
- run process concurrently, like analyze game world to prefetch assets
- handle requests from OS quickly, like call to audio buffer switch process()
- implement a generator / seq / iterator for lazy calculation during foreach
- enable parallelization by hiding function behind channel (allows it to be parallellized)
- Coroutines let you time slice heavy computation so that it runs a little bit in each frame.
- Concept: State-less lambda. Takes time but is referential transparent. These can be callable from pure code. Not true!! This introduces cow time.
- Model a process, like Erlang. UI process, realtime audio process etc.
- Supply a service that runs all the time, like GO netpoller. Hmm. Maybe and audio thread works like this too?



-- Parallellize-tweak


- Ex: communicate with a read-only server using REST. No side effects, just time.

A) Use futures and promises, completions.
B) Block clock thread, hide time from program. This makes run_clock() a continuation!!!
C) clock
D) Clock result will contain list of pending operations, as completions. Now client decides what to do with them. This means do_clock() only makes one tick.


Can pure function create a socket? Token socket is OK. Or create socket with clock at top level?! This allows simple asynchronous programming: blocking-style-only.

		### Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?




You can have modules / chips that you instantiate and that internally has channels and processes.



•• NOTICE: Floyd coroutines ARE using real threads = needs to be thread safe. Call them vthreads to goroutines ("coroutines" means cooperative multitasking)

Each process is-a vthread. You can spawn vthreads values -- owned by calling context

Composable.
Make chip and dynamically instantiate chips with internal processes, channels and vthreads
- low-priority, long running background thread


!!! Make chips with common patterns: fan-in etc.
- prefetch chunks from channel


??? It's possible to have a pure function that "sends" a REST-request, then has a separate function that is called with response. Open loop. Bad idea? Think about these as separate go-channels.
		on_rest_reply(reply)



# Goroutines
A goroutine is a lightweight thread managed by the Go runtime.

go f(x, y, z)
starts a new goroutine running

f(x, y, z)
The evaluation of f, x, y, and z happens in the current goroutine and the execution of f happens in the new goroutine.

Goroutines run in the same address space, so access to shared memory must be synchronized. The sync package provides useful primitives, although you won't need them much in Go as there are other primitives. (See the next slide.)

< 1/11 > goroutines.go Syntax

select {
	case v: <-ch1
}

??? seq vs channels? Is there a way to make pure code that reads from channels? Iterator / generator vs channel? Use for

Find good Coroutine library!!!

- Copy Go channels for our optocoupler? Clojure Core.Async? Allocate "channel" live?




# CLOCK
A clock represents a series of discrete time increments / transformations. The output is always an immutable value. If you want concurrency you need to have several clocks, each running at their own rate.

Clocks is how mutatation / memory is handled in the motherboard.

A clock has one or more inputs and ONE output. The motherboard defines when the clock is advanced. At this moment, all its inputs are frozen and it's clock-function is called. This function is referenctial transparent. When the clock function is done it returns the result, which is stored in a log of all outputs. The previous output of a clock-function is always passed in as first argument to the clock-function the next time.

		"clock": {
			"name": "ui_clock",
			"inputs": [
			]
			"clock-function": "ui_clock_function"
			"layout": {		"x": 48, "y": 16 }
		}

??? add info about blocking on external event (blocking on reply from socket?).


Your motherboard uses a list of expressions to trigger a clock advancement, like

		ui_clock_function:
			menu_selection_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
			mouse_input_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
			timeout(60.0) : ui_clock_function.log += ui_clock_function(prev_clock)

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


# OPTOCOUPLERS

"optocoupler": {
	"writer": "socket_clock",
	"reader": "command_line_clock"
}
There is no way for a clock to access that from another clock-function - they run in their own sandbox.

Optocouplers lets you send a value between two clocks, even though they run in different speed. An optocoupler is similar to a Golang channel.

READER modes:
- Default: reader blocks until new value is written be writer.
- Sample the latest value in the optocoupler, don't block.
- Sample queue of all values since last read.
Notice that the value can be a huge struct with all app state or just a message or a UI event etc.

Notice: pure functions cannot access optocouplers. They would not be referential transparent.
