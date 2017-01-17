This design solves how Floyd handles time, mutation, concurrency on the boards. Pure functions cannot do this stuff.



# IDEA - Split concept of go routine and channel into several more specific concepts.

# IDEA: Use golang for motherboards. Visual editor/debugger. Use FloydScript for logic. Use vec/map/seq/struct for data.

# Use cases for GO routines & channels
- move data between clocks
- do blocking call, like REST request
- do async lambda operations - referential transparent. Or unpure (auto save doc)
- start lengthy operation, non-blocking using passive future
- run process concurrently, like analyze game world to prefetch assets
- handle requests from OS quickly, like audio buffer switch
- implement a generator / seq
- enable parallelization 

- Coroutines let you time slice heavy computation so that it runs a little bit in each frame.

•• NOTICE: Floyd coroutines ARE using real threads = needs to be thread safe. Call them vthreads to goroutines ("coroutines" means cooperative multitasking)

Each process is-a vthread
You can spawn vthreads -- owned by calling context

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




# INSIGHTS

- Insight: synchronization points between systems (state or concurrent) always breaks composition. Move to top of product. ONE super-mediator per complete server-spanning solution.






# Problem 1 - Async, future, completions

Scenario: communicate with a read-only server using REST. No side effects, just time.
GOAL: One well-defined and composable method of async. No DSL like nested completions.

A) Use futures and promises, completions.
B) Block clock thread, hide time from program. This makes run_clock() a continuation!!!
C) clock
D) Clock result will contain list of pending operations, as completions. Now client decides what to do with them. This means do_clock() only makes one tick.

Can pure function create a socket? Token socket is OK. Or create socket with clock at top level?! This allows simple asynchronous programming: blocking-style-only.

		### Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?


