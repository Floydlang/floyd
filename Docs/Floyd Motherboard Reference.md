

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

??? make select() expression based, no statements.
??? make ALL code in motherboard expressions-only.
??? How are we sure process() never blocks + context switches becasue control.pop() is running?
??? Are pin function calls, callback?

midi_in could be
A) a callback - you put code there that reacts vs queues into a channel.
B) an output pin that wants to write to an input pin.

Can you connect an input pin directory to an output pin, no channel involved?


??? Split channel into several concepts:
	A: block-both1,
	B: sampleN -- Channel supports 1 or other fixed number of elements. Read when empty = nil. 
	C: blockN -- Like B but blocks reader.