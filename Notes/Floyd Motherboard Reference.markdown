
# MOTHERBOARD

The motherboard defines:
- Input / outputs
- Clocks
- Optocouplers bewteen clock dimensions
- WHich pure functions to call and when
- Glue expressions
- Memories
- Connections to servers, local storage, preferences
- Attaches to user interface
- Interfaces with command line arguments / returns.
- Profiling and tweaks


my first desgin.mboard


{
	"major_version": 1,
	"minior_version": 0,

	"nodes": {
	}
}



# CLOCK

A clock represents a series of discrete time increments / transformations. The output is always an immutable value. If you want concurrency you need to have several clocks, each running at their own rate.

Clocks is how mutatation / memory is handled in the motherboard.

A clock has one or more inputs and ONE output. The motherboard defines when the clock is advanced. At this moment, all its inputs are frozen and it's clock-function is called. This function is referenctial transparent. When the clock function is done it returns the result, which is stored in a log of all outputs. The previous output of a clock-function is always passed in as first argument to the clock-function the next time.


"clock": {
	"name": "command_line_clock",
	"inputs": [
	]
	"clock-function": "my_clock_function"
	"layout": {		"x": 48, "y": 16 }
}

??? add info about blocking on external event (blocking on reply from socket?).


# OPTOCOUPLERS

"optocoupler": {
	"writer": "socket_clock",
	"reader": "command_line_clock"
}
There is no way for a clock to access that from another clock-function - they run in their own sandbox.

Optocouplers lets you send a value between two clocks, even though they run in different speed.
An optocoupler is similar to a Golang channel. You can make your clock block / wait on a new value appearing.

Also
- Block until next time writer store value.
- Sample the latest value in the optocoupler, don't block.
- Sample queue of all values since last read.
Notice that the value can be a huge struct with all app state or just a message or a UI event etc.

Notice: pure functions cannot access optocouplers themselves. They would not be referential transparent.
