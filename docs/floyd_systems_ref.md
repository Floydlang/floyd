![Floyd logo](floyd_systems_ref_floyd_logo.png)


# FLOYD SYSTEM REFERENCE

Here are all the details you need to use Floyd Systems. Every single feature. Floyd Speak is the building block for the logic used by the Floyd System.

Highest level of abstraction and describes something that delivers value to its users, whether they are human or not, can be composed of many computers working together.

There is no support for package management built into Floyd 1.0.

Floyd System defines what people use the system, which servers and programs and applications that are involved, how each program does its internal processing, which components its uses and so on. It's a lot. But the bulk of code is not included here - the program logic. The program logic is written in Floyd Speak and stored inside one or many components.




# SOFTWARE-SYSTEM

This is a dedicated keyword for defining software systems: **software-system**. It's contents is encoded as a JSON object and designed to be either hand-coded or processed by tools. You only have one of these in a software system.

|Key		| Meaning
|:---	|:---	
|**name**		| name of your software system. Something short. JSON String.
|**desc**		| longer description of your software system. JSON String.
|**people**	| personas involved in using or maintaining your system. Don't go crazy. JSON object.
|**connections**	| the most important relationships between people and the containers. Be specific "user sends email using gmail" or "user plays game on device" or "mobile app pulls user account from server based on login". JSON array.
|**containers**	| Your iOS app, your server, the email system. Notice that you map gmail-server as a container, even though it's a gigantic software system by itself. JSON array with container names as strings. These strings are used as keys to identify containers.



### SOFTWARE SYSTEM - PEOPLE

This is an object where each key is the name of a persona and a short description of that persona.

```
"people": {
	"Gamer": "Plays the game on one of the mobile apps",
	"Curator": "Updates achievements, competitions, make custom on-off maps",
	"Admin": "Keeps the system running"
}
```



### SOFTWARE SYSTEM - CONNECTIONS

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



# CONTAINER-DEF
This is a dedicated keyword. It defines *one* container, it's name, its internal processes and how they interact.

|Key		| Meaning
|:---	|:---	
|**name**		| Needs to match the name listed in software-systems, containers.
|**tech**		| short string that lists the most important technologies, languages, toolkits.
|**desc**		| short string that tells what this component is and does.
|**clocks**		| defines every clock (concurrent process) in this container and lists which processes that are synced to each of these clocks
|**connections**		| connects the processes together using virtual wires. Source-process-name, dest-process-name, interaction-description.
|**probes\_and\_tweakers**		| lists all probes and tweakers used in this container. Notice that the same function or component can have a different set of probes and tweakers per container or share them.
|**components**		| lists all imported components needed for this container


You should keep this statement close to process-code that makes up the container. That handles messages, stores their mutable state, does all communication with the real world. Keep the logic code out of here as much as possible, the Floyd processes are about communication and state and time only.


Example container:

```
container-def {
	"name": "iphone app",
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
}
```



##### PROXY CONTAINER

If you use an external component or software system, like for example gmail, you list it here so we can represent it, as a proxy.

```
container-def {
	"name": "gmail mail server"
}
```

or 

```
container-def {
	"name": "gmail mail server"
	"tech": "Google tech",
	"desc": "Use gmail to store all gamer notifications."
}
```



# PROCESSES

Floyd processes are not the same as OS-processes. Floyd processes lives inside a Floyd container and are very light weight.

A process is defined by:

1. a struct for its memory / state

2. an initialisation function that instantiates needed components and returns the intial state

3. a process function that repeatedly handles messages. It can make impure calls, send messages to other processes and block for a long time. The process function ends each call by returning an updated version of its state - this is the only mutable memory in Floyd.

Usually process functions are one-offs and not reusable, they are the glue that binds your program together.

Avoid having logic inside the process functions - move that logic to separate, pure functions.


Example process code:

```
struct my_gui_state_t {
	int _count
}

func my_gui_state_t my_gui__init(){
	send("a", "dec")

	return my_gui_state_t(0	);
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

|Part		| Details
|:---	|:---	
|**my\_gui\_state_t**		| this is a struct that holds the mutable memory of this process and any component instances needed by the container.
|**my\_gui()**				| this function is specified in the software-system/"containers"/"my_iphone_app"/"clocks". The message is always a json_value. You can decide how encode the message into that.
|**my\_gui__init()**		| this is the init function -- it has the same name with "__init" at the end. It has no arguments and returns the initial state of the process.





# FLOYD COMPONENTS

TBD: COMING SOON

There is a keyword for defining components, called "component-api". It specify the exported features of this component.

Floyd wants to make it explicit what is the API of a component. Functions, structs, version, tests, docs. Every library needs this to export API.

It's also important to make the component-API syntax compact - no need to duplicate or reformat like headers in C++. No implementation possible in "header".

API: defines one version of a component's exported interface. Functions, protocols, structs, tests and docs. Similar to a C++ namespace.





# BUILT-IN COMPONENTS

TBD: COMING SOON
A bunch of standard components come with Floyd.







# PROBES

TBD: COMING SOON


You add probes to wires, processes and individual functions and expressions. They gather intel on how your program runs on the hardware, let's you explore your running code and profile its hardware use.




# TWEAKERS

TBD: COMING SOON

Tweakers are inserted onto the wires and clocks and functions and expressions of the code and affect how the runtime and language executes that code, without changing its logic. Caching, batching, pre-calculation, parallelization, hardware allocation, collection-type selection are examples of what's possible.




# SYNTAX

```
STATEMENT "software-system" JSON_BODY
STATEMENT "component-def" JSON_BODY
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
	"containers": [
		"gmail mail server",
		"iphone app",
		"Android app"
	]
}
result = 123

container-def {
	"name": "iphone app",
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
}

func string my_gui_main__init() impure {
	print("HELLO")
	send("a", "stop")
	send("b", "stop")
	send("c", "stop")
	send("d", "stop")
	return "a is done"
}

```
