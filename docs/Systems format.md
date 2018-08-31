
??? how to enter glue code directly in component?


# FILE FORMATS

# Files

example.floydsys -- stores the top of the system including people, connections etc. It also *fully* defines every component and how they are implemented with actors and wires, the setup of tweakers and so on.

example.fecomp -- defined a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

example.fpcomp -- defines a pure component where every function is pure and has no side effects.

Components store all their functions and enums etc. They list which other components they need.


# visualiser

A visualiser is a function that takes an input value and returns a value representing the value is graph nodes, for showing in tools. You can have several visualisers for the same type of input value.

Example 1:
- file_visualiser1 returns the name of the file
- file_visualiser2 returns the name and path and meta info about the file
- file_visualiser3 returns the name + snippet of the file contents as text etc.

It's important that a visualiser thinks about the value in its entirety, even if it only returns very litte info.



# measurement_rig

This is a specification how to setup visualisation tools to observe proves over time. Think about it as setting up all those ekgs and paper plots in the hospital that helps doctors figure out what is going on. A rig can be connected directly to specific probes.

Idea: allow you to make generic rigs.
??? a rig is very similair to an actor: consumes data and has its own state.


# desc-tag

A desc is a summary of *why* you want a feature. It should be one or a few sentences long. A desc can be a text field or an *array of fields* to allow for longer texts and still be stored OK as JSON.




# FLOYD SOFTWARE SYSTEM FILE - .floydsys

Example: "my_magic_product.floydsys"

```
software-system: "My magic product"
	desc: "Helpdesk with mobile device support for hedgehog farmers."
	people:
		"Personal Banking Customer": "uses the mobile app to record music ontop of backing track",
		"Personal Banker": "mobile app with recording feature",
		"Admin": "mobile app with recording feature",

		"Voxtracker": {
			"type": "container",
			"desc": "mobile app with recording feature" 		}
	},
	"connections": [
		{ "source": "musician", "interaction": "uses", "tech": "", "dest": "voxtracker" },
	}
	
	
	containers:
		<container> iphone app
			tech: Swift, iOS, Xcode

			nodes:
				<clock> main
					struct hedgehog_gui_state_t {
						int timestamp
						[xyz_record_t] recs
					}

					struct hedgehog_gui_message_t {
						int timestamp
						int mouse_x
						int mouse_y
						case stop: struct { int duration }
						case on_mouse_move:
						case on_click: struct { int button_index }
					}
					??? specify outputs too: gui can have several outputs. They have different message-types.
					hedgehog_gui_state_t tick_gui_state_actor([hedgehog_gui_state_t] history, hedgehog_gui_message_t message){
						if(message.type: on_mouse_move){
						}
						else if(message.type: stop){
							b.send(quit_to_homescreen)
						}
						else{
						}
					}

					a = <actor> "My GUI", tick_gui_state_actor,
					b = iphone-ux

					??? allow circular references at compile time.

				<clock> com-clock
					c = <actor> "My Com", Hedgehog-servercom

			connections:
				b -> a	//	b sends messages to a
				b -> c	//	b also sends messages to c, which is another clock

			component-list:
				Hedgehog-iphone-app
				Hedgehog-engine
				Hedgehog-servercom
				jpeg-component

			probes_and_tweakers:	//	These are stored as a list inside the container definition itself.
			measurement_rigs:		//	presentations of how the systems works and uses hardware, like profiling setups.


		<container> android app
			tech: Kotlin, Javalib, Android OS

			nodes:
				<clock> main
					<actor> "My GUI", Hedgehog-Android-app
				<clock> com-clock
					<actor> "My Com", <effect-component> Hedgehog-servercom

			component-list:
				Hedgehog-Android-app
				Hedgehog-engine
				Hedgehog-servercom
				jpeg-component

		<container> server with database & admin web
			tech: Django, Pythong, Heroku, Postgres

			nodes:
				<clock> main
					<actor> "My GUI", <effect-component> Hedgehog-serverimpl

			component-list:
				Hedgehog-engine
				Hedgehog-serverimpl
				Stripe-hook component
				jpeg-component


import-custom-components:
	<effect-component> Hedgehog-iphone-app
	<effect-component> Hedgehog-Android-app
	<pure-component> Hedgehog-app-logic
	<pure-component> Hedgehog-engine
	<pure-component> Hedgehog-servercom
	<effect-component> Hedgehog-serverimpl
```

/*
...used components
	<pure-component> Hedgehog-app-logic
	<pure-component> Hedgehog-engine
	<effect-component> Hedgehog-servercom
	<effect-component> iphone-ux
	<pure-component> jpeg-component
*/




### EXAMPLE ACTOR

??? TBD


	defclock mytype1_t myclock1(mytype1_t prev, read_transformer<mytype2_t> transform_a, write_transformer<mytype2_t> transform_b){
		... init stuff.
		...	state is local variable.

		let a = readfile()	//	blocking

		while(true){	
			let b = transform_a.read()	//	Will sample / block,  depending on transformer mode.
			let c = a * b
			transform_b.store(c)	//	Will push on tranformer / block, depending on transformer mode.

			//	Runs each task in parallel and returns when all are complete. Returns a vector of values of type T.
			//	Use return to finish a task with a value. Use break to finish and abort any other non-complete tasks.
			//??? Split to parallel_tasks_race() and parallel_tasks_all().
			let d = parallel_tasks<T>(timeout: 10000) {
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
			let e = map_parallel(seq(i: 0..<1000)){ return $0 * 2 }
			let f = fold_parallel(0, seq(i: 0..<1000)){ return $0 * 2 }
			let g = filter_parallel(seq(i: 0..<1000)){ return $0 == 1 }
		}
	}



### EXAMPLE VST-PLUG CONTAINER

Source file: *my_vst\_plug.container*


```
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
```


# FLOYD COMPONENT FILE FORMATS .fecomp and .fpcomp



```
{
	"component": {
		"dependencies": [],
		"desc": [
			"The Song Component is basic building block to ",
			"creating a music player / recorder"
		]
	},

	//	Ordered list of global things: constants, function definitions, struct definitions and enum definitions etc.
	"nodes": [
		{
			"type": "struct",	"name": "song_t",
			"def": {
				"desc": "A song has a beginning and an end and a number of tracks of music",
				"members": [
					{
						"type": "int",
						"name": "start_pos",
						"desc": "Where song starts, relative to the world-ppq",
						"invariant": "start_pos >= 0 && start_pos <= end_pos"
					},
					{
						"type": "int",
						"name": "end_pos",
						"desc": "Where ends starts, relative to the world-ppq",
						"invariant": "end_pos >= start_pos && end_pos <= max_ppq"
					}
				],
				"invariants": [
					"start_pos >= 0 && start_pos <= end_pos",
					"start_pos >= 0 && start_pos <= end_pos"
				]
			},
			"example_values": [
				{
					"name": "empty0",
					"desc": "An empty song with no tracks. Range is zero too.",
					"expression": "make_def_song(0, 0)"
				},
				{
					"name": "one_track_song",
					"desc": "A one-track song. Range is zero.",
					"expression": "make_def_song(0, 0)"
				}
			],
			"visualizers": [
				{
					"name": "Textal summary",
					"expression": "dot make_song_text_summary(true, true)"
				}
			]
		},
		{
			"type": "example_value",
			"name": "big_song",
			"expression": "make_song(12345)"
		},
		{
			"type": "example_value",
			"name": "zigzag_song",
			"expression": "make_song(12345)"
		},
		{
			"type": "function",
			"name": "scale_song",
			"def": {
				"desc": "Delete the track from the song, as specified by track index",
				"inputs": [
					{
						"type": "song",
						"name": "original",
						"desc": "The original song. The track must exists.",
						"contract": "original.count_tracks() > 0"
					},
					{
						"type": "int",
						"name": "track_index",
						"desc": "The track to delete, specified as a track index inside the original song.",
						"contract": "track_index >= 0 && track_index < original.count_tracks()"
					},
				],
				"result": {
					"type": "song",
					"desc": "The new song, where the track track_index has been removed.",
					"contract": "original.count_tracks() == result.count_tracks() - 1"
				},
				"implementation": [
					"song scale_song(song original, int track_index) {",
					"probe(original, \"The new song, where the track track_index has been removed.\", \"key2\")",
					"\tint temp = 3",
					"}"
				],
				"proofs": [
					{
						"scenario": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					},
					{
						"scenario": "delete one of 2 tracks",
						"expected": "1-track song",
						"result": "song1track",
						"inputs": [
							"song2track",
							0
						]
					}
				],
				"demos": [
					{
						"name": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					}
				]
			}
		},
		{
			"type": "visualiser",
			"name": "song_presentor_summary",
			"expression": "make_dot_diagram(_.tracks.count)"
		}
	]
}
```


##### COMPONENT AS JSON

```
{
	"component": {
		"version": {
			"major": 1,
			"minor": 0
		},
		"dependencies": [],
		"desc": [
			"The Song Component is basic building block to ",
			"creating a music player / recorder"
		]
	},
	"nodes": [
		{
			"type": "struct",
			"name": "song_t",
			"def": {
				"desc": "A song has a beginning and an end and a number of tracks of music",
				"members": [
					{
						"type": "int",
						"name": "start_pos",
						"desc": "Where song starts, relative to the world-ppq",
						"invariant": "start_pos >= 0 && start_pos <= end_pos"
					},
					{
						"type": "int",
						"name": "end_pos",
						"desc": "Where ends starts, relative to the world-ppq",
						"invariant": "end_pos >= start_pos && end_pos <= max_ppq"
					}
				],
				"invariants": [
					"start_pos >= 0 && start_pos <= end_pos",
					"start_pos >= 0 && start_pos <= end_pos"
				]
			},
			"example_values": [
				{
					"name": "empty0",
					"desc": "An empty song with no tracks. Range is zero too.",
					"expression": "make_def_song(0, 0)"
				},
				{
					"name": "one_track_song",
					"desc": "A one-track song. Range is zero.",
					"expression": "make_def_song(0, 0)"
				}
			],
			"visualizers": [
				{
					"name": "Textal summary",
					"expression": "dot make_song_text_summary(true, true)"
				}
			]
		},
		{
			"type": "example_value",
			"name": "big_song",
			"expression": "make_song(12345)"
		},
		{
			"type": "example_value",
			"name": "zigzag_song",
			"expression": "make_song(12345)"
		},
		{
			"type": "function",
			"name": "scale_song",
			"def": {
				"desc": "Delete the track from the song, as specified by track index",
				"inputs": [
					{
						"type": "song",
						"name": "original",
						"desc": "The original song. The track must exists.",
						"contract": "original.count_tracks() > 0"
					},
					{
						"type": "int",
						"name": "track_index",
						"desc": "The track to delete, specified as a track index inside the original song.",
						"contract": "track_index >= 0 && track_index < original.count_tracks()"
					},
					{
						"type": "int",
						"name": "probe_log_level",
						"desc": "debug probe"
					}
				],
				"result": {
					"type": "song",
					"desc": "The new song, where the track track_index has been removed.",
					"contract": "original.count_tracks() == result.count_tracks() - 1"
				},
				"probe_results": [
					{
						"type": "song",
						"desc": "The new song, where the track track_index has been removed.",
						"contract": "original.count_tracks() == result.count_tracks() - 1"
					}
				],
				"implementation": [
					"song scale_song(song original, int track_index) {",
					"\tint temp = 3",
					"}"
				],
				"probes": [
					{
						"line": 3,
						"type": "collection-access-probe"
					}
				],
				"proofs": [
					{
						"scenario": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					},
					{
						"scenario": "delete one of 2 tracks",
						"expected": "1-track song",
						"result": "song1track",
						"inputs": [
							"song2track",
							0
						]
					}
				],
				"demos": [
					{
						"name": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					}
				]
			}
		},
		{
			"type": "presentor",
			"name": "song_presentor_summary",
			"expression": "make_dot_diagram(_.tracks.count)"
		}
	]
}
```

##### VARIATION ON FUNC

```
desc "Delete the track from the song, as specified by track index"
	
func scale_song {
	result {
		type song
		desc "The new song, where the track track_index has been removed."
		contract: original.count_tracks() == result.count_tracks() - 1
	},
	inputs [
		{
			song original
			desc "The original song. The track must exists."
			contract original.count_tracks() > 0
		},
		{
			int track_index
			desc "The track to delete, specified as a track index inside the original song."
			contract track_index >= 0 && track_index < original.count_tracks()
		}
	],
	
	
	int temp = 3
	probe(temp, "Intermediate result", "key-1")
	
	
	proofs [
		{
			"Delete only track in song" => "empty song"
			(song1track, 0) => empty0
		},
		{
			"Delete one of 2 tracks" => "1-track song"
			(song2track, 0) => song1track
		}
	]
},

demos [
	{
		"scenario": "Scale different tracks"

		count = slider(0, 20), "Number of tracks"
		track_index = slider(0, 20), "Track to scale"

		test_song = make_test_song(count)
		test_song = scale_song(test_song, track_index)
		(test_song, track_index) => _
	}
]
```


##############################################	COMPONENT

```
component = json {
	version: {
		major: 1,
		minor: 0
	},
	dependencies: []
	desc: [
		"The Song Component is basic building block to ",
		"creating a music player / recorder"
	]
}
	
	
//	song_t	============================================================================================
	
	
struct song_t {
	desc "A song has a beginning and an end and a number of tracks of music"
	
	
	desc "Where song starts, relative to the world-ppq"
	invariant start_pos >= 0 && start_pos <= end_pos
	int start_pos
	
	desc "Where ends starts, relative to the world-ppq",
	invariant end_pos >= start_pos && end_pos <= max_ppq
	int end_pos
	
	invariant: [
		start_pos >= 0 && start_pos <= end_pos
		end_pos >= start_pos && end_pos <= max_ppq
	]
}
	
example_value empty0 {
	desc "An empty song with no tracks. Range is zero too."
	expression make_def_song(0, 0)
}
	
example_value one_track_song {
	desc "A one-track song. Range is zero."
	expression make_def_song(0, 0)
}
	
visualizer textual_summary {
	expression dot make_song_text_summary(true, true)
}
	
example_value big_song {
	expression make_song(12345)
}
	
example_value zigzag_song {
	expression make_song(12345)
}
	
	
desc "Delete the track from the song, as specified by track index"
song func scale_song(song original, int track_index){
	
	"inputs": [
		{
			"type": "song",
			"name": "original",
			desc "The original song. The track must exists.",
			"contract": "original.count_tracks() > 0"
		},
		{
			"type": "int",
			"name": "track_index",
			desc "The track to delete, specified as a track index inside the original song.",
			"contract": "track_index >= 0 && track_index < original.count_tracks()"
		},
		{
			"type": "int",
			"name": "probe_log_level",
			desc "debug probe"
		}
	],
	
	"result": {
		"type": "song",
		desc "The new song, where the track track_index has been removed.",
		"contract": "original.count_tracks() == result.count_tracks() - 1"
	},
	"probe_results": [
		{
			"type": "song",
			#desc: "The new song, where the track track_index has been removed.",
			"contract": "original.count_tracks() == result.count_tracks() - 1"
		}
	],
	"implementation": [
		"song scale_song(song original, int track_index) {",
		"\tint temp = 3",
		"}"
	],
	"probes": [
		{
			"line": 3,
			"type": "collection-access-probe"
		}
	],
	
	"proofs": [
		{
			"scenario": "delete only track in song",
			"expected": "empty song",
			"result": "empty0",
			"inputs": [
				"song1track",
				0
			]
		},
		{
			"scenario": "delete one of 2 tracks",
			"expected": "1-track song",
			"result": "song1track",
			"inputs": [
				"song2track",
				0
			]
		}
	],
	"demos": [
		{
			"name": "delete only track in song",
			"expected": "empty song",
			"result": "empty0",
			"inputs": [
				"song1track",
				0
			]
		}
	]
}
	
	
	
presentor song_presentor_summary {
	"expression": "make_dot_diagram(_.tracks.count)"
}
```
