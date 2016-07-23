
### Example

{
	External_OpenGL openGL;
	External_KeyboardInput keyboard;
	External_COWClock worldClock(100hz);
	External_COWClock videoClock(60hz);

	//	Every time world is stored to, a new generation is created.
	//	“All” old generations are still available and referenced existing objects. 
	GENERATIONS WorldSimulation worldGenerations;
	WorldDrawerFunction worldDrawer;
	WorldAdvancerFunction worldAdvancer;
	Latcher<World> latcher;

	//	Setup so world is updated at 100 Hz.
	worldAdvancer.inputWorldPin = world;
	worldAdvancer.clocksPin = worldClock.clockOutPin;
	worldGenerations.nextPin = worldAdvancer;

	//	Setup so world is painted at 60 Hz.
	latcher.inputPin = world;
	latcher.clockPin = videoClock.clockOutPin;
	worldDrawer.inputWorldPin = latcher;

	OpenGL.commandsPin = worldDrawer.outputPin;
}


	External_AudioStream audioStream;
	AudioGenerator audioGenerator;
	//	Audio stream requests buffers of 64 audio frames at a time from audio generator.
	audioStream.bufferInputPin = audioGenerator.bufferOutputPin;


# ISSUES

### Requires trim pots
### Requires measurements
### How to solve mutability
### Requires language to describe dependencies between chips.
### Requires efficient reference counting, across threads, cores and network.
### Requires efficient persistent vector and map.
### Use sha1 internally to do de-duplication.

### Make JSON format for entire simulation. Normalise format so it can be generated, editted in GUI.

Content-addressable objects?

??? Hash objects?

??? Always do de-duplication aka "interning".

??? How to do fast, distributed reference counting? When to copy? Trim settings.


### Example programs

Example: Hello, world
Example: Copy a file.
Example: Game of life
Example: space invaders
Example program: read a text file and generate JSON file.



# Why circuit boards?

* Calculations: functions, data structures, collections - no time.
* Time and externals: mutability, clocks, concurrency, transformers  - circuit boards
* Optimizations: measure / profile / tweak runtime.


int function f1(int a) is a value that is equivalent to a int myVector[int]

Runtime optimization: Only MUTABLE<SONG> a = x;  will use atomic operations on internal RC for the value X, all other access to value X will use normal inc/dec.



### SIMULATION OVERVIEW

Simulation is a controlled environment managed by an active runtime.

0 to many functions
0 to many packages
0 to many types
0 to many objects
0 to many external artefacts
0 to many time bases
0 to many chips


PACKAGE
0 to many functions
0 to many CHIPs
	0 to many objects
	0 to many wirings
0 to many types

0 to many external artefacts





# ROADMAP


### IDEAS
port
bus
rising and falling edges of the clock signal

	edge-detection triggers new generation
testbench

transitivity of immutability
http://lua-users.org/wiki/ImmutableObjects

state
combinational logic
sequential logic
https://en.wikipedia.org/wiki/State_(computer_science)
