![Floyd logo](floyd_systems_ref_floyd_logo.png)


# FLOYD SYSTEM REFERENCE

Here are all the details you need to use Floyd Systems. Every single feature. Floyd Speak is the building block for the logic.


Highest level of abstraction and describes something that delivers value to its users, whether they are human or not, can be composed of many computers working together.



## SOURCE FILES

**example.floydsys** -- stores the top of the system including people, connections and so on. It also *fully* defines every component and how they are implemented with process and wires, the setup of tweakers and so on.

**example.fecomp** -- effect-component source file. Defines a reusable component that has *effects* -- that is can call mutating functions. It cannot keep its own state but state may be stored in OS or file system or servers.

**example.fpcomp** -- defines a pure component where every function is pure and has no side effects.

Component source files store all their functions and enums and so on. They list which other components they need.



# SOFTWARE SYSTEM FILE


You only have one of these in a software system. Extension is .floydsys.

There is only one dedicated keyword: **software-system**. Its contents is encoded as JSON object and designed to be either hand-coded or processed by tools.


|Key		| Meaning
|:---	|:---	
|**name**		| name of your software system. Something short. JSON String.
|**desc**		| longer description of your software system. JSON String.
|**people**	| personas involved in using or maintaining your system. Don't go crazy. JSON object.
|**connections**	| the most important relationships between people and the containers. Be specific "user sends email using gmail" or "user plays game on device" or "mobile app pulls user account from server based on login". JSON array.
|**containers**	| named. Your iOS app, your server, the email system. Notice that you map gmail-server as a container, even though it's a gigantic software system by itself. JSON object.



## PEOPLE

This is an object where each key is the name of a persona and a short description of that persona.

```
"people": {
	"Gamer": "Plays the game on one of the mobile apps",
	"Curator": "Updates achievements, competitions, make custom on-off maps",
	"Admin": "Keeps the system running"
}
```



## CONNECTIONS

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



# CONTAINERS

Defines every container in the system. They are named.

|Key		| Meaning
|:---	|:---	
|**tech**		| short string that lists the most important technologies, languages, toolkits.
|**desc**		| short string that tells what this component is and does.
|**clocks**		| defines every clock (concurrent process) in this container and lists which processes that are synced to each clock
|**connections**		| connects the processes together using virtual wires. Source-process-name, dest-process-name, interaction-description.
|**probes\_and\_tweakers**		| lists all probes and tweakers used in this container. Notice that the same function or component can have a different set of probes and tweakers per container or share them.
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

## PROXY CONTAINER

If you use an external component or software system, like for example gmail, you list it here so we can represent it, as a proxy.

```
"gmail mail server": {}
```

or 

```
"gmail mail server": {
	"tech": "Google tech",
	"desc": "Use gmail to store all gamer notifications.",
}
```



# PROCESSES

Floyd processes are not the same as OS-processes. Floyd processes lives inside a Floyd container and are very light weight.

A process is a function with this signature:

	x_state_t my_process_main([x_state_t] history, y_message message){
	}

It can used from "clocks" sections to instantiate an process based on that function. Usually process functions are one-offs and not reusable several time.

??? allow circular references at compile time.

You need to define an initialization function for each process. It has the same name but with "__init" at the end:

	x_state_t my_process_main__int(){
	}

It has no arguments and returns the initial state of the process.



# RUNTIMES
TBD

At the container level you can instantiate different runtimes, like memory allocators, memory pools, file system support, image caches, background loaders and so on. These can then be accessed inside the process functions and passed to the functions they call.




# FLOYD COMPONENT FILES

There is pure components and effect components. Keywords are **pure-component** and **effect-component**.


??? define external interface of component? Versions?

A pure component has no side effects, have no state and can't talk to the outside world. They can use other pure components but never effect-components.

An effect component *can* have side effects. It cannot keep its own state -- only state in the outside world. It can use both pure-components and effect-components.

Both pure-component and effect-component are followed by a JSON objects that holds their data. It's a small header.

Only one component can be defined per source file.


|Key		| Meaning
|:---	|:---	
|**version**		| array with two numbers: major and minor
|**dependencies**		| list of components needed by your component
|**desc**			| Short description what this component is about.



```
pure-component {
	"version": [ 1, 0 ],
	"dependencies": [],
	"desc": "The Song Component is basic building block to creating a music player / recorder"
}
```

The rest of the component file is normal Floyd Speak code: structs, enums, function definitions and constants and so on.





# BUILT-IN COMPONENTS

A bunch of standard components come with Floyd.


## LOCAL FILE SYSTEM COMPONENT

??? TBD
All file system functions are blocking. If you want to do something else while waiting, run the file system calls in a separate process. There are no futures, callbacks, async / await or equivalents. It is very easy to write a function that does lots of processing and conditional file system calls etc and run it concurrently with other things.

??? Simple file API
	file_handle open_fileread(string path) impure
	file_handle make_file(string path) impure
	void close_file(file_handle h) impure

	vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) impure
	delete_fsnode(string path) impure

	make_dir(string path, string name) impure
	make_file(string path, string name) impure



## COMMAND LINE COMPONENT

??? TBD

	int on_commandline_input(string args) impure
	void print(string text) impure
	string readline() impure



## THE C-ABI COMPONENT

??? TBD

This is a way to create a component from the C language, using the C ABI.



## THE REST-API COMPONENT

??? TBD



## THE S3 BLOB COMPONENT

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistence. Uses SHA1 and content deduplication.



## THE LOCAL FS BLOB COMPONENT

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistence. Uses SHA1 and content deduplication.





# PROBES

You add probes to wires, processes and individual functions and expressions. They gather intel on how your program runs on the hardware, let's you explore your running code and profile its hardware use.


## TRACE PROBE


## LOG PROBE

??? TBD

- Pulse every time a function is called
- Pulse every time a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log



## WATCHDOG PROBE

??? TBD


## BREAKPOINT PROBE

??? TBD


## ASSERT PROBE






# TWEAKERS

Tweakers are inserted onto the wires and clocks and functions and expressions of the code and affect how the runtime and language executes that code, without changing its logic. Caching, batching, pre-calculation, parallelization, hardware allocation, collection-type selection are examples of what's possible.



## SET THREAD COUNT & PRIO TWEAKER

- Parallelize pure function
- Increase thread priority for an process or clock.


## COLLECTION SELECTOR TWEAKER

??? TBD

Control which collection to use on a per-value instance basis.

vector
	c-array
	HAMT
	linear-write cache
	linear-read cache
	scatter-gather cache

dictionary
	c-array with binary search
	HAMT
	binary tree
	hash
	unsorted vector with searching

Defines rules for which collection backend to use when. How large must collection be to use different backends? When is it worthwhile to create alternative backend for existing collection = double memory.


## CACHE TWEAKER

??? TBD

A cache will store the result of a previous computation and if a future computation uses the same function and same inputs, the execution is skipped and the previous value is returned directly.

A cache is always a shortcut for a (pure) function. You can decide if the cache works for *every* invocation of a function or limit the cache to invocations of the function within specified parent part.

- Insert read cache
- Insert write cache
- Increase random access speed for dictionary
- Increase forward read speed, stride
- Increase backward read speed, stride







## EAGER TWEAKER

??? TBD

Like a cache, but calculates its values *before* clients call the function. It can be used to create a static lookup table at app startup.



## BATCH TWEAKER

??? TBD

When a function is called, this part calls the function with similar parameters while the functions instructions and its data sits in the CPU caches.

You supply a function that takes the parameters and make variations of them.

- Batching: make 64 value each time?
- Speculative batching with rewind.



## LAZY TWEAKER

??? TBD

Make the function return a future and don't calculate the real value until client accesses it.
- Lazy-buffer



## PRECALC TWEAKER

Pre-calculate at compile time and store statically in executable.
Pre-calculate at container startup time, store in FS.
- Pre-calculate / prefetch, eager



## CONTENT DE-DUPLICATION TWEAKER

Uses built-in hash-mechanism that exists for all values in Floyd Speak

- Content de-duplication
- Cache in local file system
- Cache on Amazon S3



## TRANSFORM MEMORY LAYOUT TWEAKER

??? TBD

Turn array of structs to struct of arrays etc.
- Rearrange nested composite (turn vec<pixel> to struct{ vec<red>, vec<green>, vec<blue> }




# SYNTAX


```
STATEMENT "software-system" JSON_BODY
STATEMENT "effect-component" JSON_BODY
STATEMENT "pure-component" JSON_BODY
```







# EXAMPLE SOFTWARE SYSTEM FILE


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

//////////////////		My GUI process code

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

hedgehog_gui_state_t my_gui_main__int(){
	return hedgehog_gui_state_t()
}

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
