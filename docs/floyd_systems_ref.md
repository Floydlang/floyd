![Floyd logo](floyd_systems_ref_floyd_logo.png)


# FLOYD SYSTEM REFERENCE

Here are all the details you need to use Floyd Systems. Every single feature. Floyd Speak is the building block for the logic.


Highest level of abstraction and describes something that delivers value to its users, whether they are human or not, can be composed of many computers working together.

There is no support for package management built into Floyd 1.0.


## SOURCE FILES

**example.floydsys** -- stores the top of the system including people, connections and so on. It also *fully* defines every component and how they are implemented with process and wires, the setup of tweakers and so on.

**example.floydcomp** -- component source file. Defines a reusable component 


# SOFTWARE SYSTEM FILE


You only have one of these in a software system. Extension is .floydsys.

There is only one dedicated keyword for software systems: **software-system**. It's contents is encoded as a JSON object and designed to be either hand-coded or processed by tools.


|Key		| Meaning
|:---	|:---	
|**name**		| name of your software system. Something short. JSON String.
|**desc**		| longer description of your software system. JSON String.
|**people**	| personas involved in using or maintaining your system. Don't go crazy. JSON object.
|**connections**	| the most important relationships between people and the containers. Be specific "user sends email using gmail" or "user plays game on device" or "mobile app pulls user account from server based on login". JSON array.
|**containers**	| named. Your iOS app, your server, the email system. Notice that you map gmail-server as a container, even though it's a gigantic software system by itself. JSON object.



### software-system - PEOPLE

This is an object where each key is the name of a persona and a short description of that persona.

```
"people": {
	"Gamer": "Plays the game on one of the mobile apps",
	"Curator": "Updates achievements, competitions, make custom on-off maps",
	"Admin": "Keeps the system running"
}
```



### software-system - CONNECTIONS

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



### software-system - CONTAINERS

Defines every container in the system. They are named.

|Key		| Meaning
|:---	|:---	
|**tech**		| short string that lists the most important technologies, languages, toolkits.
|**desc**		| short string that tells what this component is and does.
|**clocks**		| defines every clock (concurrent process) in this container and lists which processes that are synced to each of these clocks
|**connections**		| connects the processes together using virtual wires. Source-process-name, dest-process-name, interaction-description.
|**probes\_and\_tweakers**		| lists all probes and tweakers used in this container. Notice that the same function or component can have a different set of probes and tweakers per container or share them.
|**components**		| lists all imported components needed for this container


Example container:

```
			"iphone app": {
				"tech": "Swift, iOS, Xcode, Open GL",
				"desc": "Mobile shooter game for iOS.",

				"clocks": {
					"main": {
						"a": "my_gui_main",
						"b": "iphone-ux"
					},

					"com-clock": {
						"c": "server_com"
					},
					"opengl_feeder": {
						"d": "renderer"
					}
				},
				"connections": [
					{ "source": "b", "dest": "a", "interaction": "b sends messages to a", "tech": "OS call" },
					{ "source": "b", "dest": "c", "interaction": "b also sends messages to c, which is another clock", "tech": "OS call" }
				],
				"components": [
					"My Arcade Game-iphone-app",
					"My Arcade Game-logic",
					"My Arcade Game-servercom",
					"OpenGL-component",
					"Free Game Engine-component",
					"iphone-ux-component"
				]
			},
```


### software-system - PROXY CONTAINER

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


# PROCESSES AND RUNTIMES

Floyd processes are not the same as OS-processes. Floyd processes lives inside a Floyd container and are very light weight.

A process is defined by:

1. a struct for its memory / state

2. an initialisation function that instantiates needed components and returns the intial state

3. a process function that repeatedly handles messages. It can do do impure calls, send messages to other processes but ends each call by returning an updated version of its state struct.

Usually process functions are one-offs and not reusable.

Avoid having logic inside the process functions - move that logic to separate, pure functions.


Example process code:

```
struct my_gui_state_t {
	mac_logging_impure logging
	mac_files_impure files
	text_packing_impl text_packing
	image_cache_impl thumbnail_cache
	image_cache_impl fullrez_cache

	int _count
}

func my_gui_state_t my_gui__init(){
	send("a", "dec")

	return my_gui_state_t(
		mac_logging_impure(),
		mac_files_impure(),
		image_cache_impl(),
		image_cache_impl(),

		0
	);
}

func my_gui_state_t my_gui(my_gui_state_t state, json_value message){
	if(message == "inc"){
		return update(state, "_count", state._count + 1)
	}
	else if(message == "dec"){
		return update(state, "_count", state._count - 1)
	}
	else{
		assert(false)
	}
}
```


**my\_gui\_state_t**: this is a struct that holds the mutable memory of this process and any component instances needed by the container.

**my\_gui()**: this function is specified in the software-system/"containers"/"my_iphone_app"/"clocks". The message is always a json_value. You can decide how encode the message into that.

**my\_gui__init()**: this is the init function -- it has the same name with "__init" at the end. It has no arguments and returns the initial state of the process.


In the init function you instantiate all components (aka libraries) you need to use in this container.

**No code in the container can access any other libraries or API:s but those specified here.**

[//]: # (??? add names socket as destinations for send())


### Context feature

```
func a(): b("hello")
func b(string message): context.trace(message)
```

[//]: # (???)

Contexts don't need to be an actual argument passed between all functions. It is mostly static. It can sit on separate stack - only push/pop when changed. Or be a parameter in the interpreter. Go all the way to Lua environment?

Function context: All functions have access to small set of basic infrastructure. A built-in context is automatically passed as argument to every function implicitly. It has features like logging, memory and error handling (like Quark). A function can add more protocols to it or replace protocol implementations when calling a child function, at any position in the callstack (??? or just at top level?). This is a way to add new infrastructure without introducing globals. Top-level function can add a special pool-feature and a low-level function can pick it up.


Protocol member functions can be tagged "impure" which allows it to be implemented so it saves or uses state, modifies the world. There is no way to do these things in the implementation of pure protocol function memembers.





# FLOYD COMPONENT FILES

There is a keyword for defining components, called "component-api". It specify the exported features of this component.

Floyd wants to make it explicit what is the API of a component. Functions, structs, version, tests, docs. Every library needs this to export API.

It's also important to make the component-API syntax compact - no need to duplicate or reformat like headers in C++. No implementation possible in "header".

API: defines one version of a component's exported interface. Functions, protocols, structs, tests and docs. Similar to a C++ namespace.


Tag every element with "export" that shall be part of component-api.

Keyword "impure" is used to tag functions that can have side effects. You cannot call these functions from a pure function.

[//]: # (??? What is a component instance?)

[//]: # (??? Future: add contracts to API.)

[//]: # (??? Future: add tests to API.)

```
component-api {
	"name": "PNGLib",
	"minor_version": 4,
	"dependencies": [],
	"desc": "Library for packing and unpacking PNGs of all 3 modes: gray scale, RGB and RGBA"
}
```

Only one component can be defined per source file and a component needs to export all its features from that source file. It can call functions in other source files.


|Key		| Meaning
|:---	|:---	
|**name**		| Short name of the component API
|**minor_version**		| Minor version as a number
|**dependencies**		| List of components needed by your component
|**desc**			| Short description what this component is about.


The rest of the source file is normal Floyd Speak, but tags functions and other elements that should be exported via API.

```
export struct png_lib_t {
}

export png_lib_t init()
export void deinit(png_lib_t)

/*
	Describes any PNG as loading into RAM. It holds its pixel data.
	There are three types, depending on how many color channels are used in image.
	It is possible to convert any PNG into rgba-mode, but that wastes memory.
*/
export struct png_t {
	int width
	int height
	enum {
		grayscale: [[byte]] gray_lines
		rgb: [[struct { byte red, byte green, byte blue } ]] rgb_lines
		rgba: [[struct { byte alpha, byte red, byte green, byte blue } ]] rgba_lines
	}
}

//	Will use the PNG mode that is most economical depending on the fill-color.
export png_t make_fill(int width, int height, byte red, byte green, byte blue, byte alpha){
	...
	...
	...
}

export png_t make_example1_black_32x32(){
	return make_fill(32, 32, 0, 0, 0, 255)
}

export png_t make_example2_white_32x32(){
	return make_fill(32, 32, 0, 0, 0, 255)
}

export png_t make_example3_checker_alpha_32x32(){
	return make_fill(32, 32, 0, 0, 0, 255)
}

export png_t unpack_png([byte] binary_png){
	...
	...
	...
}
export [byte] pack_png(png_t a){
	...
	...
	...
}
export png_t scale(png_t a, float scale){
	...
	...
	...
}
```



Functions take protocols or component APIs as arguments.

Future: syntax to thunk protocol packs easily -- where protocols aren't 1:1 or have different types.






### COMPONENT VERSIONS

You cannot change an API so it break existing clients.

[//]: # (??? Future: add unit tests to API.)

If you need to do breaking changes, make a completely new API but call it something similar: "Pixelib" becomes "Pixelib 2". This makes sure a new version of a component doesn't break existing clients. A client always needs to explicitly migrate to a new version of the API.

But there is a number of things you *can* change to your API without breaking clients.
- Fix defects (that don't change API).
- Add more tests to API.
- Add more documentation to API.
- Add new functions or types to an API.

It is OK and even preferable to keep the same component API and just add new version of functions, like this:

```
func void draw_text(float x, float y, string text)
```

Later on you can add a better draw-text function that does antialiasing:

```
func void draw_text_antialized(float x, float y, string text)
```

This does not break old code either and a client can chose to migrate calls one by one.
























# BUILT-IN COMPONENTS

A bunch of standard components come with Floyd.


## LOCAL FILE SYSTEM COMPONENT

[//]: # (???)

TBD
All file system functions are blocking. If you want to do something else while waiting, run the file system calls in a separate process. There are no futures, callbacks, async / await or equivalents. It is very easy to write a function that does lots of processing and conditional file system calls etc and run it concurrently with other things.

[//]: # (???)

Simple file API
	file_handle open_fileread(string path) impure
	file_handle make_file(string path) impure
	void close_file(file_handle h) impure

	vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) impure
	delete_fsnode(string path) impure

	make_dir(string path, string name) impure
	make_file(string path, string name) impure



## COMMAND LINE COMPONENT

[//]: # (???)

TBD

	int on_commandline_input(string args) impure
	void print(string text) impure
	string readline() impure



## THE C-ABI COMPONENT

[//]: # (???)

TBD

This is a way to create a component from the C language, using the C ABI.



## THE REST-API COMPONENT

[//]: # (???)

TBD



## THE S3 BLOB COMPONENT

[//]: # (???)

TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistence. Uses SHA1 and content deduplication.



## THE LOCAL FS BLOB COMPONENT

[//]: # (???)

TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistence. Uses SHA1 and content deduplication.





# PROBES

You add probes to wires, processes and individual functions and expressions. They gather intel on how your program runs on the hardware, let's you explore your running code and profile its hardware use.


## TRACE PROBE


## LOG PROBE

[//]: # (???)

TBD

- Pulse every time a function is called
- Pulse every time a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log



## WATCHDOG PROBE

[//]: # (???)

TBD


## BREAKPOINT PROBE

[//]: # (???)

TBD


## ASSERT PROBE






# TWEAKERS

Tweakers are inserted onto the wires and clocks and functions and expressions of the code and affect how the runtime and language executes that code, without changing its logic. Caching, batching, pre-calculation, parallelization, hardware allocation, collection-type selection are examples of what's possible.



## SET THREAD COUNT & PRIO TWEAKER

- Parallelize pure function
- Increase thread priority for an process or clock.


## COLLECTION SELECTOR TWEAKER

[//]: # (???)

TBD

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

[//]: # (???)

TBD

A cache will store the result of a previous computation and if a future computation uses the same function and same inputs, the execution is skipped and the previous value is returned directly.

A cache is always a shortcut for a (pure) function. You can decide if the cache works for *every* invocation of a function or limit the cache to invocations of the function within specified parent part.

- Insert read cache
- Insert write cache
- Increase random access speed for dictionary
- Increase forward read speed, stride
- Increase backward read speed, stride


## EAGER TWEAKER

[//]: # (???)

TBD

Like a cache, but calculates its values *before* clients call the function. It can be used to create a static lookup table at app startup.



## BATCH TWEAKER

[//]: # (???)

TBD

When a function is called, this part calls the function with similar parameters while the functions instructions and its data sits in the CPU caches.

You supply a function that takes the parameters and make variations of them.

- Batching: make 64 value each time?
- Speculative batching with rewind.



## LAZY TWEAKER

[//]: # (???)

TBD

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

[//]: # (???)

TBD

Turn array of structs to struct of arrays etc.
- Rearrange nested composite (turn vec<pixel> to struct{ vec<red>, vec<green>, vec<blue> }




# SYNTAX

```
STATEMENT "software-system" JSON_BODY
STATEMENT "component-API" JSON_BODY
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
				"main": {
					"a": "my_gui_main",
					"b": "iphone-ux"
				},

				"com-clock": {
					"c": "server_com"
				},
				"opengl_feeder": {
					"d": "renderer"
				}
			},
			"connections": [
				{ "source": "b", "dest": "a", "interaction": "b sends messages to a", "tech": "OS call" },
				{ "source": "b", "dest": "c", "interaction": "b also sends messages to c, which is another clock", "tech": "OS call" }
			],
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
				"main": {
					"a": "my_gui_main",
					"b": "iphone-ux"
				},
				"com-clock": {
					"c": "server_com"
				},
				"opengl_feeder": {
					"d": "renderer"
				}
			},
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
				"main": {}
			},
			"components": [
				"My Arcade Game-logic",
				"My Arcade Game server logic"
			]
		}
	}
}

////////////////////////////////	my_gui -- process

struct my_gui_state_t {
	int _count
}

func my_gui_state_t my_gui__init(){
	send("a", "dec")
	send("a", "dec")
	send("a", "dec")
	send("a", "stop")
	return my_gui_state_t(1000)
}

func my_gui_state_t my_gui(my_gui_state_t state, json_value message){
	if(message == "inc"){
		return update(state, "_count", state._count + 1)
	}
	else if(message == "dec"){
		return update(state, "_count", state._count - 1)
	}
	else{
		assert(false)
	}
}


```
