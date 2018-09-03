
??? how to enter glue code directly in container?


# FILE FORMATS

# Files

example.floydsys -- stores the top of the system including people, connections etc. It also *fully* defines every component and how they are implemented with actors and wires, the setup of tweakers and so on.

example.fecomp -- defined a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

example.fpcomp -- defines a pure component where every function is pure and has no side effects.

Components store all their functions and enums etc. They list which other components they need.


# presenter
??? presenter == convention for a function, or use special keyword? "preso" instead of "func"?

A presenter is a function that takes an input value and returns a value representing the value is graph nodes, for showing in tools. You can have several presenters for the same type of input value.

Example 1:
- file_presenter1 returns the name of the file
- file_presenter2 returns the name and path and meta info about the file
- file_presenter3 returns the name + snippet of the file contents as text etc.

It's important that a presenter thinks about the value in its entirety, even if it only returns very litte info.


Presenters are used extensively by the IDE and the runtime and debuggers etc to let you explore your system. They can also be used to quickly generate user interfaces or configuration files.


# preso

This is a value that lists the contenst of a value of a specific type.


This is a concerete type, designed to hold a string, a list of strings, a tree of nodes etc. Like a high-level SVG but without layout information, only the data.

??? what type is the presentation? JSON-value?


# measurement_rig

This is a specification how to setup visualisation tools to observe proves over time. Think about it as setting up all those ekgs and paper plots in the hospital that helps doctors figure out what is going on. A rig can be connected directly to specific probes.

Idea: allow you to make generic rigs.
??? a rig is very similair to an actor: consumes data and has its own state.


# desc-tag

A desc is a summary of *why* you want a feature. It should be one or a few sentences long. A desc can be a text field or an *array of fields* to allow for longer texts and still be stored OK as JSON.


# RUNTIMES

At the container level you can instantiate different runtimes, like memory allocators, memory pools, file system support, image caches, background loaders etc. These can then be accessed inside the actor functions and passed to the functions they call.


# PROOF

When you write a function you also supply a number of proofs that shows that the function works as advertised. These are unit tests and will always run at startup and other times.



# DEMO

THis is an object that can be used to setup an interactive demonstration of a function. It has desc, can define some sliders and settings, defines expressions to execute. Can recommend what type of information in preso:s to show and how. Show a time-axis and use a slider to zoom?




# FLOYD SOFTWARE SYSTEM FILE - .floydsys

Example: "my_magic_product.floydsys"

```
{
	"software-system": "My magic product",
	"desc": "Helpdesk with mobile device support for hedgehog farmers.",
	"people": {
		"Personal Banking Customer": "uses the mobile app to record music ontop of backing track",
		"Personal Banker": "mobile app with recording feature",
		"Admin": "mobile app with recording feature"
	},
	"connections": [
		{ "source": "musician", "interaction": "uses", "tech": "", "dest": "voxtracker" }
	],
	"containers": {
		"iphone app": {
			"tech": "Swift, iOS, Xcode",
			"desc": "Mobile app with buttons for hedgehogs.",

			"clocks": {
				"main": [
					"struct hedgehog_gui_state_t {",
					"\tint timestamp",
					"\t[xyz_record_t] recs",
					"}",
					"",
					"struct hedgehog_gui_message_t {",
					"\tint timestamp",
					"\tint mouse_x",
					"\tint mouse_y",
					"\tcase stop: struct { int duration }",
					"\tcase on_mouse_move:",
					"case on_click: struct { int button_index }",
					"}",
					"??? specify outputs too: gui can have several outputs. They have different message-types.",
					"hedgehog_gui_state_t tick_gui_state_actor([hedgehog_gui_state_t] history, hedgehog_gui_message_t message){",
					"\tif(message.type: on_mouse_move){",
					"\t}",
					"\telse if(message.type: stop){",
					"\t\tb.send(quit_to_homescreen)",
					"\t}",
					"\telse{",
					"\t}",
					"}",
					"",

					"a = <actor> My GUI, tick_gui_state_actor",
					"b = iphone-ux",
					"??? allow circular references at compile time."
				],

				"com-clock": [
					"c = <actor> My Com, Hedgehog-servercom"
				]
			},
			"connections": [
				"b -> a",	"b sends messages to a",
				"b -> c",	"b also sends messages to c, which is another clock"
			],
			"probes_and_tweakers": [],
			"measurement_rigs": [],
			"components": [
				"Hedgehog-iphone-app", 	"Hedgehog-engine", "Hedgehog-servercom", "jpeg-component"
			]
		},

		"Android app": {
			"tech": "Kotlin, Javalib, Android OS",
			"desc": "Android app with google integration.",
	
			"clocks": {
				"main": [
					"<actor> My GUI, Hedgehog-Android-app"
				],
				"com-clock": [
					"<actor> My Com, <effect-component> Hedgehog-servercom"
				]
			},
			"components": [ "Hedgehog-Android-app", "Hedgehog-engine", "Hedgehog-servercom", "jpeg-component" ],
			"probes_and_tweakers": [],
			"measurement_rigs": []
		},
		"Server with database & admin web": {
			"tech": "Django, Pythong, Heroku, Postgres",
			"desc": "The database that stores all user accounts, talks to the mobile apps and handles admin tasks.",

			"clocks": {
				"main": [
					"<actor> My GUI, <effect-component> Hedgehog-serverimpl"
				]
			},
			"components": [ "Hedgehog-engine", "Hedgehog-serverimpl", "Stripe-hook component", "jpeg-component" ],
			"probes_and_tweakers": [],
			"measurement_rigs": []
		}
	},
	"import-custom-components": [
		"<effect-component> Hedgehog-iphone-app",
		"<effect-component> Hedgehog-Android-app",
		"<pure-component> Hedgehog-app-logic",
		"<pure-component> Hedgehog-engine",
		"<pure-component> Hedgehog-servercom",
		"<effect-component> Hedgehog-serverimpl"
	]
}
```


### EXAMPLE ACTOR

```
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
		}
	}
```

