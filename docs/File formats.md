

# SYNTAX


```
STATEMENT "software-system" JSON_BODY
STATEMENT "effect-component" JSON_BODY
STATEMENT "pure-component" JSON_BODY
```


# SOURCE FILES

**example.floydsys** -- stores the top of the system including people, connections etc. It also *fully* defines every component and how they are implemented with actors and wires, the setup of tweakers and so on.

**example.fecomp** -- defined a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

**example.fpcomp** -- defines a pure component where every function is pure and has no side effects.

Components store all their functions and enums etc. They list which other components they need.



# RUNTIMES
TBD

At the container level you can instantiate different runtimes, like memory allocators, memory pools, file system support, image caches, background loaders etc. These can then be accessed inside the actor functions and passed to the functions they call.



# ACTOR

An actor is a function with this signature:

	x_state_t my_actor_main([x_state_t] history, y_message message){
	}

It can used from "clocks" sections to instantiate an actor based on that function. Ususally actor functions are one-offs and not reusable several time.



# EXAMPLE FLOYD SOFTWARE SYSTEM FILE - test.floydsys

. ??? allow circular references at compile time.

```
software-system: {
	"name": "My Arcade Game",
	"desc": "Space shooter for mobile devices, with connection to a server.",
	"people": {
		"Gamer": "Plays the game on one of the mobile apps",
		"Curator": "Updates achievements, competitions, make custom on-off maps",
		"Admin": "Keeps the system running"
	},
	"connections": [
		{ "source": "Game", "dest": "iphone app", "interaction": "plays", "tech": "" }
	],
	"containers": {
		"iphone app": {
			"tech": "Swift, iOS, Xcode, Open GL",
			"desc": "Mobile shooter game for iOS.",

			"clocks": {
				"main": [
					"a": "my_gui_main",
					"b": "iphone-ux"
				],

				"com-clock": [
					"c": "server_com"
				],
				"opengl_feeder": [
					"d": "renderer"
				]
			},
			"connections": [
				{ "b", "a",	"b sends messages to a" },
				{ "b", "c",	"b also sends messages to c, which is another clock" }
			],
			"probes_and_tweakers": [],
			"components": [
				"My Arcade Game-iphone-app",
				"My Arcade Game-logic",
				"My Arcade Game-servercom",
				"OpenGL-component",
				"Free Game Engine-component",
				"iphone-ux-component"
			]
		},

		"Android app": {
			"tech": "Kotlin, Javalib, Android OS, OpenGL",
			"desc": "Mobile shooter game for Android OS.",
	
			"clocks": {
				"main": [
					"a": "my_gui_main",
					"b": "iphone-ux"
				],
				"com-clock": [
					"c": "server_com"
				],
				"opengl_feeder": [
					"d": "renderer"
				]
			},
			"probes_and_tweakers": [],
			"components": [
				"My Arcade Game-android-app",
				"My Arcade Game-logic",
				"My Arcade Game-servercom",
				"OpenGL-component",
				"Free Game Engine-component",
				"Android-ux-component"
			]
		},
		"Game Server with players & admin web": {
			"tech": "Django, Pythong, Heroku, Postgres",
			"desc": "The database that stores all user accounts, levels and talks to the mobile apps and handles admin tasks.",

			"clocks": {
				"main": [
				]
			},
			"probes_and_tweakers": [],
			"components": [
				"My Arcade Game-logic",
				"My Arcade Game server logic"
			]
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

//////////////////		My GUI actor code

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

hedgehog_gui_state_t my_gui_main([hedgehog_gui_state_t] history, hedgehog_gui_message_t message){
	if(message.type: on_mouse_move){
	}
	else if(message.type: stop){
		b.send(quit_to_homescreen)
	}
	else{
	}
}


```



# FLOYD COMPONENT


??? define external interface of component? Versions?

```
component {
	"version": [ 1, 0 ],
	"dependencies": [],
	"desc": [
		"The Song Component is basic building block to ",
		"creating a music player / recorder"
	]
}

// A song has a beginning and an end and a number of tracks of music.
struct song_t {
	// Where song starts, relative to the world-ppq
	int start_pos

	// Where ends starts, relative to the world-ppq,
	int end_pos

	// Any number of tracks are OK. No need to check the tracks themselves, they are guaranteed to have correct invariants.
	[track_t] tracks
}


// An empty song with no tracks. Range is zero too.
let empty0 = song_t(expression make_def_song(0, 0))

// A one-track song. Range is zero.
let one_track_song = song_t(expression make_def_song(0, 0))

let big_song = make_song(12345)
let zigzag_song = scale_song(make_song(1235), 2.0)


// Delete the track from the song, as specified by track index
func
	// The new song but where the track track_index has been removed.
	song_t
	scale_song(
		// The original song. The track must exists.
		song_t original

		// The track to delete, specified as a track index inside the original song.
		int track_index
	)
{
	assert(original.count_tracks() > 0)
	assert(track_index >= 0 && track_index < original.count_tracks())

	int temp = 3
	probe(temp, "Intermediate result", "key-1")

	assert(original.count_tracks() == result.count_tracks() - 1)
	return x
}

```
