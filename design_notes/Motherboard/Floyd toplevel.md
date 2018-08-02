# SYSTEM STRUCTURE
C4 model
https://c4model.com/

### Person
various human users of your software system

### Software System
highest level of abstraction and describes something that delivers value to its users, whether they are human or not. 

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


# DIAGRAMS
Level 1: System Context diagram
Level 2: Container diagram
Level 3: Component diagram
Level 4: Code

Notice: a component used in several components or a piece of code that appears in several components = appears as duplicates. The perspective is: logic dependencies. There is no diagram for showing which source files or libraries that depends on eachother.



# MOTHERBOARD = CONTAINER
The motherboard is the top-level design that connects all code together into a product / app / executable.

WORLD: The exposition between client code and the outside world. This includes sockets, file systems, messages, screens, UI.

### Responsibilities:
- Instantiate all parts of the final product and connect them together.
- Introduce time and mutation into the system.
- Use Floyd-script modules
- Call C functions
- Connect to the outside world, communicating with sockets, reading / writing files etc.
- Profile control and optimize performance of system.
- Limits to what you can do in motherboard -- force programmer to do advanced coding in Floyd Script.

### Non-goal
- Be reusable.
- To be composable
- Pure / free of side effects

The motherboard is a declarative system, based on JSON. Real-world performance decisions, profiling, optimizations, caching etc.

Most logic is done using Floyd Script, which is a pure, referential transparent language.

- **pin** (alt port)	-- where you can attach a wire
- **wire** -- a connection between to pins
- **channel** -- an object where producers can store elements and consumer can read them. Optionally non-blocking, 1...many elements big
- **part** -- a node in the diagram. There are channel-parts, custom-parts etc.
- **pure-function**
- **unpure-function**
- **value** -- an immutable value, either a primitive like a float or a collection or a struct. Can be big data.


# VALUES AND SIGNALS
All signals and values use Floyd's immutable types, provided by the Floyd runtime. Whenever a value, queue element or signal is mentioned, *any* of Floyd's types can be used -- even huge nested structs or collections.

# MOTHERBOARD PARTS
- Clock
- Channel
- Adapter, splitter, merger: expression that filters a signal
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









# FILE SYSTEM-part
??? Simple file API or use channels?

# REST-part
Channels?

# LOG-probe
- Pulse everytime a function is called
- Pulse everytime a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log

# Profiler-probe

# Command-line-part

# Tweaks - Optimizations
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

	
=============================================================================



This design solves how Floyd handles time, mutation, concurrency on the boards. Pure functions cannot do this stuff.




- You can package a board into a *chip*. This is a first class value that can be instantiate using code or using part nodes.
- 
