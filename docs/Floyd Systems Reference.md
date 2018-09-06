
# FLOYD SYSTEM REFERENCE

Here are all the details you need to use Floyd Systems. Every single feature. Floyd Speak is the building block for the logic.


Highest level of abstraction and describes something that delivers value to its users, whether they are human or not, can be composed of many computers working together.




# SOFTWARE SYSTEM FILE - example.floydsys


You only have one of these in a software system. Extension is .floydsys.

There is only one dedicated keyword, **software-system**. Its contents is encoded as JSON object and designed to be either handcoded or processed by tools.



|Key		| Meaning
|:---	|:---	
|**name**		| name of your software system. Something short. JSON String.
|**desc**		| longer description of your software system. JSON String.
|**people**	| personas involved in using or maintainging your system. Don't go crazy. JSON object.
|**connections**	| the most important relationships between people and the containers. Be specific "user sends email using gmail" or "user plays game on device" or "mobile app pulls user account from server based on login". JSON array.
|**containers**	| named. Your iOS app, your server, the email system. Notice that you map gmail-server as a container, even though its a gigantic software system by itself. JSON object.



# PEOPLE (SOFTWARE SYSTEM)

This is an object where each key is the name of a persona and a short description of that persona.

```
	"people": {
		"Gamer": "Plays the game on one of the mobile apps",
		"Curator": "Updates achievements, competitions, make custom on-off maps",
		"Admin": "Keeps the system running"
	}
```

# CONNECTIONS (SOFTWARE SYSTEM)

```
	"connections": [
		{
			"source": "Game",
			"dest": "iphone app",
			"interaction": "plays",
			"tech": ""
		}
	]
```


|Key		| Meaning
|:---	|:---	
|**source**		| the name of the container or user, as listed in the software-system
|**dest**		| the name of the container or user, as listed in the software-system
|**interaction**		| "user plays game on device" or "server sends order notification"
|**tech**		| "webhook", "REST command"


# CONTAINERS (SOFTWARE SYSTEM)
Defines every container in the system. They are named.

|Key		| Meaning
|:---	|:---	
|**tech**		| short string that lists the most important technologies, languages, toolkits.
|**desc**		| short string that tells what this component is and does.
|**clocks**		| defines every clock (concurrent process) in this container and lists which actors are synced to each clock
|**connections**		| connects the actors together using virtual wires. Source-actor-name, dest-actor-name, interaction-description.
|**probes_and_tweakers**		| lists all probes and tweakers used in this container. Notice that the same function or component can have a different set of probes and tweakers per container or share them.
|**components**		| lists all imported components needed for this container


Example container:

```
	"iphone app": {
		"tech": "Swift, iOS, Xcode, Open GL",
		"desc": "Mobile shooter game for iOS.",

		"clocks": {
			"main": [ "my_gui_main", "iphone-ux" ],
			"com-clock": [ "server_com" ],
			"opengl_feeder": [ "renderer" ]
		},
		"connections": [
			[ "iphone-ux", "my_gui_main",	"Handle requests from iOS for our app" ],
			[ "my_gui_main", "server_com",	"Our app queues server commands" ]
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
	}
```


If you use an external component or software system, like for example gmail, you list it here so we can represent it, as a proxy.

You use the

"gmail mail server": {}






```
software-system {
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
		"gmail mail server": {},

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
	}
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



### CONTAINER REFERENCE


This is a declarative file that describes the top-level structure of an app. Its contents looks like this:


This is th	e container's start function. It will find, create and and connect all resources, runtimes and other dependencies to boot up the container and to do executibe decisions and balancing for the container.





# FLOYD COMPONENT


??? define external interface of component? Versions?

```
pure-component {
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



### CODE REFERENCE






# SYNTAX


```
STATEMENT "software-system" JSON_BODY
STATEMENT "effect-component" JSON_BODY
STATEMENT "pure-component" JSON_BODY
```


### SOURCE FILES

**example.floydsys** -- stores the top of the system including people, connections etc. It also *fully* defines every component and how they are implemented with actors and wires, the setup of tweakers and so on.

**example.fecomp** -- defined a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

**example.fpcomp** -- defines a pure component where every function is pure and has no side effects.

Components store all their functions and enums etc. They list which other components they need.



### RUNTIMES
TBD

At the container level you can instantiate different runtimes, like memory allocators, memory pools, file system support, image caches, background loaders etc. These can then be accessed inside the actor functions and passed to the functions they call.



### ACTOR

An actor is a function with this signature:

	x_state_t my_actor_main([x_state_t] history, y_message message){
	}

It can used from "clocks" sections to instantiate an actor based on that function. Ususally actor functions are one-offs and not reusable several time.

. ??? allow circular references at compile time.































### BUILT-IN COMPONENTS - REFERENCE


##### COMMAND LINE COMPONENT REFERENCE

??? TBD

	int on_commandline_input(string args) unpure
	void print(string text) unpure
	string readline() unpure


##### MULTIPLEXER COMPONENT

Contains internal pool of actors and distributes incoming messages to them to run messages in parallel.



### PROBES REFERENCE


##### LOG PROBE REFERENCE

??? TBD

- Pulse everytime a function is called
- Pulse everytime a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log


##### PROFILE PROBE REFERENCE


##### WATCHDOG PROBE REFERENCE

??? TBD


##### BREAKPOINT PROBE REFERENCE

??? TBD


##### ASSERT PROBE REFERENCE


### TWEAKERS REFERECE


##### COLLECTION SELECTOR TWEAKER - REFERENCE

??? TBD

Defines rules for which collection backend to use when.


##### CACHE TWEAKER REFERENCE

??? TBD

A cache will store the result of a previous computation and if a future computation uses the same function and same inputs, the execution is skipped and the previous value is returned directly.

A cache is always a shortcut for a (pure) function. You can decide if the cache works for *every* invocation of a function or limit the cache to invocations of the function within specified parent part.

- Insert read cache
- Insert write cache
- Increase random access speed for dictionary
- Increase forward read speed, stride
- Increase backward read speed, stride


##### EAGER TWEAKER REFERENCE

??? TBD

Like a cache, but calculates its values *before* clients call the function. It can be used to create a static lookup table at app startup.


##### BATCH TWEAKER REFERENCE

??? TBD

When a function is called, this part calls the function with similar parameters while the functions instructions and its data sits in the CPU caches.

You supply a function that takes the parameters and make variations of them.

- Batching: make 64 value each time?
- Speculative batching with rewind.


##### LAZY TWEAKER REFERENCE

??? TBD

Make the function return a future and don't calculate the real value until client accesses it.
- Lazy-buffer


##### PRECALC TWEAKER REFERENCE

Prefaclucate at compile time and store statically in executable.
Precalculate at container startup time, store in FS.
- Precalculate / prefetch, eager


##### CONTENT DE-DUPLICATION TWEAKER REFERENCE

- Content de-duplication
- Cache in local file system
- Cache on Amazon S3


##### TRANSFORM MEMORY LAYOUT TWEAKER REFERENCE

??? TBD

Turn array of structs to struct of arrays etc.
- Rearrange nested composite (turn vec<pixel> to struct{ vec<red>, vec<green>, vec<blue> }


##### SET THREAD COUNT & PRIO TWEAKER REFERENCE
- Parallelize pure function



### THE FILE SYSTEM PART

??? TBD
All file system functions are blocking. If you want to do something else while waiting, run the file system calls in a separate clock. There are no futures, callbacks, async / await etc. It is very easy to write a function that does lots of processing and conditional file system calls etc and run it concurrently with other things.

??? Simple file API
	file_handle open_fileread(string path) unpure
	file_handle make_file(string path) unpure
	void close_file(file_handle h) unpure

	vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) unpure
	delete_fsnode(string path) unpure

	make_dir(string path, string name) unpure
	make_file(string path, string name) unpure


### THE REST-API PART

??? TBD


### THE S3 BLOB PART

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistance. Uses SHA1 and content deduplication.


### THE LOCAL FS BLOB PART

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistance. Uses SHA1 and content deduplication.



### THE ABI PART

??? TBD

This is a way to create a component from the C language, using the C ABI.


