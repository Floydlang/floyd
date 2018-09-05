
??? how to enter glue code directly in container?


# FILE FORMATS


# Files

example.floydsys -- stores the top of the system including people, connections etc. It also *fully* defines every component and how they are implemented with actors and wires, the setup of tweakers and so on.

example.fecomp -- defined a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

example.fpcomp -- defines a pure component where every function is pure and has no side effects.

Components store all their functions and enums etc. They list which other components they need.



# Documentation and comments

// and /* */ are used for comments.

All comments before a statement are collapsed and becomes docs for that statement. First line of comment becomes the *summary*. Any additional lines of comments becomes the *details*.

A desc is a summary of *why* you want a feature. It should be one or a few sentences long. A desc can be a text field or an *array of fields* to allow for longer texts and still be stored OK as JSON.



# struct invariant

Invariants are defined per data member of a struct. They are used to verify input argument to automatic constructor (int start_pos, int end_pos). There is no way to *ever* create an instance of song_t that breaks the invariant. If you add special maker-functions, those can have other invariants but the song_t variants are always there.

All aggregate values will automatically have their invariants checked deeply.


# function contracts

A function specifies possible input arguments, ranges etc. These are validated at runtime and used for documentation.
A function also specifies the possible values of its result. You can define the results using the original inputs values.



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

This is a value that lists the contents of a value of a specific type.


This is a concerete type, designed to hold a string, a list of strings, a tree of nodes etc. Like a high-level SVG but without layout information, only the data.

??? what type is the presentation? JSON-value?


# measurement_rig

This is a specification how to setup visualisation tools to observe proves over time. Think about it as setting up all those ekgs and paper plots in the hospital that helps doctors figure out what is going on. A rig can be connected directly to specific probes.

Idea: allow you to make generic rigs.
??? a rig is very similair to an actor: consumes data and has its own state.



# RUNTIMES

At the container level you can instantiate different runtimes, like memory allocators, memory pools, file system support, image caches, background loaders etc. These can then be accessed inside the actor functions and passed to the functions they call.


# PROOF

When you write a function you also supply a number of proofs that shows that the function works as advertised. These are unit tests and will always run at startup and other times.


# EXAMPLE_VALUE

These are used to define good example values for a struct to be used for demoing, making tests for this struct, functions working on the struct values and for all clients.

	example_value one_track_song = song_t(1)

Example values are automatically available in other modules.



# DEMO

This is an object that can be used to setup an interactive demonstration of a function. It has desc, can define some sliders and settings, defines expressions to execute. Can recommend what type of information in preso:s to show and how. Show a time-axis and use a slider to zoom?



# FLOYD SOFTWARE SYSTEM FILE - .floydsys

Example: "my_magic_product.floydsys"

```
"software-system": {
	"name": "My magic product",
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
					"a": "my_gui",
					"b": "iphone-ux",
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



actor my_gui {
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

	//??? specify outputs too: gui can have several outputs. They have different message-types.

	hedgehog_gui_state_t tick_gui_state_actor([hedgehog_gui_state_t] history, hedgehog_gui_message_t essage){
		if(message.type: on_mouse_move){
		}
		else if(message.type: stop){
			b.send(quit_to_homescreen)
		}
		else{
		}
	}

	state: hedgehog_gui_state_t
	message: hedgehog_gui_message_t
	function: tick_gui_state_actor
}


```


