TODO: Make more meaty examples, with structs and actual data and "objects".

![](floyd_logo_banner.png)

# INTRODUCTION

Floyd is programming language as an alternative to Java and C++, JavaScript and Python. When you write Floyd programs you write functions and complex data structures. You setup your program's concurrency and parallelism and how it communicate with the world around it.



# CONTENTS


1. BASIC CONCEPTS
2. THE LANGUAGE
3. CORE FUNCTIONS
4. COMMAND LINE TOOL
5. REFERENCE



# QUICK REFERENCE


 ![](floyd_quick_reference.png)



# 1. BASIC CONCEPTS


## FUNCTIONS (PURE AND IMPURE)

Functions in Floyd are by default *pure*, or *referential transparent*. This means they can only read their input arguments and constants, never read or modify anything: not global variables, not by calling another, impure function.

It's not possible to call a pure function with a set of arguments and later call it again with the same arguments and get a different result.

A function like get_time() is impure.

While a function executes, it perceives the outside world to stand still.

Functions always return exactly one value. Use a struct to return more values.

Example definitions of functions:

```
func int f1(string x){
	return 3
}

func int f2(int a, int b){
	return 5
}

func int f3(){	
	return 100
}

func string f4(string x, bool y){
	return "<" + x + ">"
}
```

Function types:

```
func bool (string, double)
```


This is a function that takes a function value as argument:

```
func int f5(func bool (string, string))
```

This is a function that returns a function value:

```
func bool (string, string) f5(int x)
```

All arguments to a function are read-only -- you cannot modify the arguments or anything they refer too. There are no output parameters in Floyd.


### IMPURE FUNCTIONS

You can tag a function to be impure using the "impure" keyword.

```
func int f1(string x) impure {
	return 3
}
```

This mean the function has side effects or gets information somewhere that can change over time.

- A pure function cannot call any impure functions!
- An impure function can call both pure and impure functions.

Limit the amount of impure code!


### GRAY PURE FUNCTIONS

This is a type of function that *has side effects or state* -- but the calling functions cannot observe this so from their perspective it is pure. Examples are memory allocators and memory pools and logging program execution.

Why is the OK? Well to be picky there are no pure functions, since calling a pure function makes your CPU emit more heat and consumes real-world time, makes other programs run slower, consumes memory bandwidth. But a pure function cannot observe those side effects either.


## VALUES, VARIABLES

### IMMUTABLE VALUES VS VARIABLES

All values in Floyd are immutable -- you make new values based on previous values, but you don't directly modify old values. Internally Floyd uses clever mechanisms to make this fast and avoids copying data too much. It's perfectly good to replace a character in a 3 GB long string and get a new 3 GB string as a result. Almost all of the characters will be stored only once.

The only exception is local variables that can be forced to be mutable.

(Also, each green-process has one mutable value too.)


- Function arguments
- Function local variables
- Member variables of structs.

When defining a variable, you can often skip telling which type it is, since the type can be inferred by the Floyd compiler.

Explicit

```
let int x = 10
```

Implicit

```
let y = 11
```

You can even leave out "let" to define a new variable, if that local variable does not yet exist:

```
y = 12
```

Example:

```
int main(){
	let a = "hello"
	a = "goodbye"	//	Runtime error - you cannot change variable a.
	return 3
}
```

You can use "mutable" to make a local variable changeable.

```
int main(){
	mutable a = "hello"
	a = "goodbye"	//	Changes variable a to "goodbye".
	return 3
}
```


### DEEP BY VALUE

All values and aggregated members values are always considered in operations in any type of nesting of structs and values and collections. This includes equality checks or assignment for example.

The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

These are features built into every type: integer, string, struct, dictionary:

|EXPRESSION		| EXPLANATION
|:---				|:---	
|__a = b__ 		| This true-deep copies the value b to the new name a.
|__a == b__		| a exactly the same as b
|__a != b__		| a different to b
|__a < b__		| a smaller than b
|__a <= b__ 		| a smaller or equal to b
|__a > b__ 		| a larger than b
|__a >= b__ 		| a larger or equal to b

This also goes for print(), map(), to\_string(), value\_to\_jsonvalue(), send() etc.

Example: your application's entire state may be stored in *one* value in a struct containing other structs and vectors and so on. This value can still be copied around quickly, it is automatically sortable, convertible to JSON or whatever.


### NO POINTERS

There are no pointers or references in Floyd. You copy values around deeply instead. Even a big value like your game's entire world or your word processor's entire document. Behind the curtains Floyd uses pointers extensively to make this fast.

Removing the concept of pointers makes programming easier. There are no dangling pointers, aliasing problems or defensive copying and other classic problems. It also makes it simpler for the runtime and compiler to generate extremely fast code.

### STATIC TYPING, INFERRED

Every value and variable and identifier have a static type: a type that is defined at compile time, before the program runs. This is how Java, C++ and Swift works. Javascript, Python and Ruby does not use static typing.

You can often leave out the actual type from the code, when the compiler already knows the type.




## DATA TYPES

These are the primitive data types built into the language itself. The building blocks of all values and data in Floyd.

The goal is that all the basics you need are already there in the language and the core library. This makes it easy to start making meaningful programs.

It also promotes composability since all Floyd code can rely on these types and communicate between themselves using these types. This greatly reduces the need to write glue code that converts between different library's string classes and logging and so on.

|TYPE		  	| USE
|:---				|:---	
|__bool__			|__true__ or __false__
|__int__			| Signed 64 bit integer
|__double__		| 64-bit floating point number
|__string__		| Built-in string type. 8-bit pure (supports embedded zeros). Use for machine strings, basic UI. Not localizable. Typically used for Windows Latin1, UTF-8 or ASCII.
|__typeid__		| Describes the *type* of a value.
|__function__	| A function value. Functions can be Floyd functions or C functions. They are callable.
|__struct__		| Like C struct or classes or tuples. A value object.
|__vector__		| A continuous array of elements addressed via indexes.
|__dictionary__	| Lookup values using string keys.
|__json__	| A value that holds a JSON-compatible value, can be a big JSON tree.
|TODO: __protocol__	| A value that hold a number of callable functions. Also called interface or abstract base class.
|TODO: __sum-type__		| Tagged union.
|TODO: __float__		| 32-bit floating point number
|TODO: __int8__		| 8-bit signed integer
|TODO: __int16__		| 16-bit signed integer
|TODO: __int32__		| 32-bit signed integer

Notice that string has many qualities of an array of characters. You can ask for its size, access characters via [], etc.







## SOFTWARE SYSTEM - C4

Floyd uses the C4 model to navigate and present your code. It's completely optional to use this feature. The C4 features gives you a very fast and lightweight way method to think about your system and to automatically generate a few great diagrams that helps you reason about it.

Read more here: https://c4model.com/

The C4 diagrams lets you have a complete overview over your entire system and how users interact with it, then drill down to individual containers and further down to components and the code itself.

In Floyd you describe your system using the keywords **software-system-def** and **container-def**, which use C4 terminology.

![Software Systems](floyd_systems_software_system.png)


### PERSON

Represents various human users of your software system. They access the system through some sort of user interface. For example, a UI on an iPhone.


### LEVEL 1 - SYSTEM CONTEXT DIAGRAM

This describes the entire software system you are building -- something that delivers value to its users, whether they are human or not. It can be composed of many computers, servers and apps working together -- or just one, like an iPhone app.

![Level1](floyd_systems_level1_diagram.png)


### LEVEL 2 - CONTAINER DIAGRAM

Zooms into your software system and shows the different containers (apps) that makes up the system.

You can use a mix of your own custom containers and proxy-containers for things like Amazon S3 or an email server. They are all part of the diagram.

![Level2](floyd_systems_level2_diagram.png)


### CONTAINERS

A container is a thing that needs to be running in order for the overall software system to work. A mobile app, a server-side web application, a client-side web application, a micro service.

A container is usually a single OS-process. It looks for resources and knows how to string things together inside the container, does its own processing of events and so forth, has its own state and handles runtimer errors. Sockets, file systems, concurrency, messages and screens are handled here.

A container's design is usually a one-off and not reusable.

![](floyd_container_example.png)

In Floyd, containers are defined using container-def, which lists the components it needs, which internal Floyd processes it runs and which impure Floyd functions that run those processes.


### LEVEL 3 - COMPONENT DIAGRAM

This diagram zooms into an individual container to show the components inside.

![Level3](floyd_systems_level3_diagram.png)


### COMPONENTS

A component is the same as a library, package or module. It's a group of related features encapsulated behind a well-defined API. You can make your own components, use built-in component and get 3rd party components.

Examples: JPEG library, JSON lib. Custom component for syncing with your server. Amazon S3 library, socket library.


### LEVEL 4 - CODE DIAGRAM

Classes. Instance diagram. Examples. Passive. You often use a simple UML diagram here.

![Level3](floyd_systems_level4_diagram.png)


TODO: Make script that generates diagrams from software-system-def JSON.


### EXAMPLE SOFTWARE SYSTEM STATEMENT


```
software-system-def {
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
```

TODO: support proxy software-systems
TODO: support connections between components inside containers.




## FLOYD PROCESSES - CONCURRENCY, TIME, STATE, COMMUNICATION

Floyd processes lives inside a Floyd container and are very light weight. They are not the same as OS-processes. They *are* sandboxed from their sibbling processes. They can be run in their own OS-threads or share OS-thread with other Floyd processes.

A process is defined by:

1. a value **M** for its memory / state -- usually a struct

2. an initialisation function that instantiates needed components and returns the intial state

3. a process function **f** that repeatedly handles messages. It can make impure calls, send messages to other processes and block for a long time. The process function ends each call by returning an updated version of its state - this is the only mutable memory in Floyd.

4. A message inbox.


![Floyd Process](floyd_process.png)

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

func my_gui_state_t my_gui(my_gui_state_t state, json message){
	if(message == "inc"){
		return update(state, _count, state._count + 1)
	}
	else if(message == "dec"){
		return update(state, _count, state._count - 1)
	}
	else{
		assert(false)
	}
}
```

|Part		| Details
|:---	|:---	
|**my\_gui\_state_t**		| this is a struct that holds the mutable memory of this process and any component instances needed by the container.
|**my\_gui()**				| this function is specified in the software-system/"containers"/"my_iphone_app"/"clocks". The message is always a json. You can decide how to encode the message into that.
|**my\_gui__init()**		| this is the init function -- it has the same name with "__init" at the end. It has no arguments and returns the initial state of the process.


This is how you express time / mutation / concurrency in Floyd. These concepts are related, and they are all setup at the top level of a container. In fact, this is the main **purpose** of a container.

The goal with Floyd's concurrency model is:

1. Simple and robust pre-made mechanisms for real-world concurrency need. Avoid general-purpose primitives, instead have a ready-made solution that works.
2. Composable.
3. Allow you to make a *static design* of your container and its concurrency and state.
4. Separate out parallelism into a separate mechanism.
5. Avoid problematic constructs like threads, locks, callback hell, nested futures and await/async -- dead ends of concurrency.
6. Let you control/tune how many threads and cores to use for what parts of the system, independently of the actual code.

Inspirations for Floyd's concurrency model are CSP, Erlang, Go routines and channels and Clojure Core.Async.


##### PROCESSES: INBOX, STATE, PROCESSING FUNCTION

For each independent mutable state and/or "thread" you want in your container, you need to insert a process. Processes are statically instantiated in a container -- you cannot allocate them at runtime.

The process represents a little standalone program with its own call stack that listens to messages from other processes. When a process receives a message in its inbox, its function is called (now or some time later) with the message and the process's previous state. The process does some work - something simple or a maybe call a big call tree or do blocking calls to the file system, and then it ends by returning its new state, which completes the message handling.

**The process feature is the only way to keep state in Floyd.**

Use a process if:

1. You want to be able to run work concurrently, like loading data in the background
2. You want a mutable state / memory
3. You want to model a system where things happen concurrently, like audio streaming vs main thread


The inbox is thread safe and it's THE way to communicate across processes. The inbox has these purposes:
	
1. Allow process to *wait* for external messages using the select() call
2. Transform messages between different clock-bases -- the inbox is thread safe
3. Allow buffering of messages, that is moving them in time

You need to implement your process's processing function and define its mutable state. The processing function is impure. It can call OS-functions, block on writes to disk, use sockets etc. Each API you want to use needs to be passed as an argument into the processing function, it cannot go find them - or anything else.

Processes cannot change any other state than its own, they run in their own virtual address space.

When you send messages to other process you can block until you get a reply, get replies via your inbox or just don't use replies.

The process function CAN chose to have several select()-statements which makes it work as a small state machine.

Processes are very inexpensive.


**Synchronization points between systems (state or concurrent) always breaks all attempts to composition. That's why Floyd has moved these to top level of container.**


The runtime can choose to execute processes on different cores or servers. You have control over this via tweakers. Tweakers also controls the priority of processes vs hardware.


Floyd process limitations:

- Cannot find assets / ports / resources — those are handed to it via the container's wiring
- Cannot be created or deleted at runtime
- Cannot access any global state or other processes


##### SYNCHRONOUS PROCESSES

If the processes are running on the same clock, the sequence of:

- process A posts message to process B
- process B receives the message
- process B processes the message
- process B completes and updates its state
- process A continues

...is done synchronously without any scheduling or OS-level context switching - just like a function call from A to B.

You synchronise processes when it's important that the receiving process handles the messages *right away*. 

Synced processes still have their own state and can be used as controllers / mediators.



### GAIN PERFORMANCE VIA CONCURRENCY

Sometimes we introduce concurrency to make more parallelism possible: multithreading a game engine is taking a non-concurrent design and making it concurrent to be able to improve throughput by running many tasks in parallel. This is different to using concurrency to model real-world concurrency like UI vs background cloud com vs real-time audio processing.



### CONCURRENCY SCENARIOS

|#	|Need		|Traditional	|Floyd
|---	|---			|---			|---
|1	| Make a REST request	| Block entire thread / nested callbacks / futures / async-await | Just block. Make call from process to keep caller running
|2	| Make a sequence of back and forth communication with a REST server | Make separate thread and block on each step then notify main thread on completion / nested futures or callbacks / await-async | Make an process that makes blocking calls
|3	| Perform non-blocking impure background calculation (auto save doc) | Copy document, create worker thread | Use process, use data directly
|4	| Run process concurrently, like analyze game world to prefetch assets | Manually synchronize all shared data, use separate thread | Use process -- data is immutable
|5	| Handle requests from OS quickly, like call to audio buffer switch process() | Use callback function | Use process and set its clock to sync to clock of buffer switch
|6	| Improve performance using concurrency + parallelism / fan-in-fan-out / processing pipeline | Split work into small tasks that are independent, queue them to a thread team, resolve dependencies somehow, use end-fence with competition notification | call map() or map_dag() from a process.
|7	| Spread heavy work across time (do some processing each game frame) | Use coroutine or thread that sleeps after doing some work. Wake it next frame. | Process does work. It calls select() inside a loop to wait on next trigger to continue work.
|8	| Do work regularly, independent of other threads (like a timer interrupt) | Call timer with callback / make thread that sleeps on event | Use process that calls post_at_time(now() + 100) to itself
|9	| Small server | Write loop that listens to socket | Use process that waits for messages




### EXAMPLE: SIMPLE CONSOLE PROGRAM

This is a basic command line app. It doesn't need a Floyd process at all, it's just a function. It gets its input values from the command line arguments, calls some pure Floyd functions on the arguments, reads and writes to the world, then finally return an integer result. 



### EXAMPLE: PACMAN IPHONE

![Pacman](floyd_pacman.png)

TODO: make example of *all* the diagrams, including Software System diagram.

A complex gamewill have many clocks, both to model concurreny and to allow parallelism. When simulation has been run, it can hand of a copy of the game world for renderer to work on in parallel.

- UI event loop clock
- Prefetch assets clock
- World-simulation / physics clock
- Rendering pass 1 clock
- Commit to OpenGL clock
- Audio streaming clock

https://www.youtube.com/watch?v=v2Q_zHG3vqg


## TODO: EXCEPTIONS

Throw exception. Built in types, free noun. Refine, final.




## ABOUT PERFORMANCE

Chandler Carruth:

- EFFICIENCY: HOW MUCH WORK IS REQUIRED BY A TASK
	- Improve by doing less work.
	- Better algorithms.
	
- PERFORMANCE: HOW QUICKLY A PROGRAM DOES ITS WORK.
	- Do work faster. 
	- Lighting up all the transistors.
	- Data structures


**EFFICIENCY WITH ALGORITHMS, PERFORMANCE WITH DATA STRUCTURES**



Floyd is designed to make it simple and practical to make big systems with performance better than what you get with average optimized C code.

It does this by splitting the design into two different concepts:

1. Encourage your logic and processing code to be simple and correct and to declare where there is opportunity to execute code independently of each other. This type of code is ideal to run in parallel or cache etc, like a shader in a graphics API.

2. At the top level, profile execution and make high-level improvements that dramatically alter how the code is *generated* and executed to run on the available hardware. Caching, collection type selection, batching, parallelization, ordering work for different localities, memory layouts and access patterns.

It is also simple to introduce more concurrency to create more opportunities to run computations in parallel.



![](cpu_overview.png)

TODO: Also draw computing power.
TOD: Fact check stats.





## TODO: PROBES

You add probes to wires, processes and individual functions and expressions. They gather intel on how your program runs on the hardware, let's you explore your running code and profile its hardware use.




## TODO: TWEAKERS

Tweakers are inserted onto the wires and clocks and functions and expressions of the code and affect how the runtime and language executes that code, without changing its logic. Caching, batching, pre-calculation, parallelization, hardware allocation, collection-type selection are examples of what's possible.




## ABOUT PARALLELISM

In Floyd you accelerate the performance of your code by making it expose where there are dependencies between computations and where there are not. Then you can orchestrate how to best execute your container from the top level -- using tweak probes and profiling probes, affecting how the hardware is mapped to your logic.

Easy ways to expose parallelism is by writing pure functions (their results can be cached or precomputed) and by using functions like map(), fold(), filter() and map_dag(). These function work on individual elements of a collection and each computation is independent of the others. This lets the runtime process the different elements on parallel hardware.

The functions map() and map_dag() replaces FAN-IN-FAN-OUT-mechanisms.

You can inspect in code and visually how the elements are distributed as tasks.

map_dag() works like map(), but each element also has dependencies to other elements in the collection.

Accelerating computations (parallelism) is done using tweaks — a separate mechanism. It supports moving computations in time (lazy, eager, caching) and running work in parallel.


Often processes and concurrency is introduced into a system to *expose opportunity* for parallelism.

The optimizations using tweaks in no way affect the logic of your program, only the timing and order where those don't matter.

To make something like a software graphics shaders, you would do

let image2 = map(image1, my_pixel_shader) and the pixels can be processed in parallel.


???
**Task** - this is a work item that takes usually approximately 0.5 - 10 ms to execute and has an end. The runtime generates these when it wants to run map() elements in parallel. All tasks in the entire container are scheduled together.

Notice: map() and map_dag() shares threads with other mechanisms in the Floyd runtime. This mean that even if your tasks cannot be distributed to all execution units, other things going on can fill those execution gaps with other work.










## JSON LITERALS

You can directly embed JSON inside Floyd source code file. This is extremely simple - no escaping needed - just paste a snippet into the Floyd source code. Use this for test values. Round trip. The Floyd parser will create Floyd strings, dictionaries and so on for the JSON data. Then it will create a json\_value from that data. This will validate that this indeed is correct JSON data or an exception is thrown.

Example JSON:

```
let json a = 13
let json b = "Hello!"
let json c = { "hello": 1, "bye": 3 }
let json d = { "pigcount": 3, "pigcolor": "pink" }

assert(a == 13)
assert(b == "Hello!")
assert(c["hello"] == 1)
assert(c["bye"] == 3)
assert(size(c) == 2)

let test_json2 = json(
	{
		"one": 1,
		"two": 2,
		"three": "three",
		"four": [ 1, 2, 3, 4 ],
		"five": { "alpha": 1000, "beta": 2000 },
		"six": true,
		"seven": false,
	}
)
```

Notice that JSON objects are laxer than other Floyd values: you can mix different types of values in the same object or array. Floyd is stricter: a vector can only hold one type of element, same with dictionaries.



## GLOBAL SCOPE

Here you normally define functions, structs and global constants. The global scope can have almost any statement and they execute at program start. Simple programs can do without defining any functions at all.



## COMMENTS AND DOCUMENTATION

Use comments to write documentation, notes or explanations in the code. Comments are not executed or compiled -- they are only for humans. You often use the comment features to disable / hide code from the compiler.

Two types of comments:


You can wrap many lines with "/\*" and "\*/" to make a big section of documentation or to disable many lines of code. You can nest comments, for example wrap a lot of code that already contains comments using /* ... */.

```
/*	This is a comment */
```


Everything between // and newline is a comment:

```
//	This is an end-of line comment
let a = "hello" //	This is an end of line comment.
```





## AUTOMATIC SERIALIZATION

Serializing any Floyd value is a built-in mechanism. It is always true-deep.

**This is very central to Floyd -- values are core and they can easily be passed around, sent as messages, stored in files, copy-pasted from log or debugger into the source code.**


##### JSON DATA SHAPES, ESCAPING

These are the different shapes a JSON can have in Floyd:

1. Floyd value: a normal Floyd value - struct or a vector or a number etc. Stored in RAM.

2. json\_value: data is JSON compatible, stored in RAM the 7 different value types supported by json\_value. It holds one JSON value or a JSON object or a JSON array, that in turn can hold other json\_value:s.

3. JSON-script string: JSON data encoded as a string of characters, as stored in a text file.

	Example string: {"name":"John", "age":31, "city":"New York"}

	It contains quotation characters, question mark characters, new lines etc.

4. Escaped string: the string is simplified to be stuffed as a string snippet into some restricted format, like inside a parameter in a URL, as a string-literal inside another JSON or inside a REST command.

	Example string: {\\"name\":\\"John\", \\"age\\":31, \\"city\\":\\"New York\\"}

Different destinations have different limitations and escape mechanisms and will need different escape functions. This is not really a JSON-related issue, more a URL, REST question.


#### FUNCTIONS
Converting a floyd json\_value to a JSON string and back. The JSON-string can be directly read or written to a text file, sent via a protocol and so on.

```
string generate_json_script(json v)
json parse_json_script(string s)
```

Converts any Floyd value, (including any type of nesting of custom structs, collections and primitives) into a json\_value, storing enough info so the original Floyd value can be reconstructed at a later time from the json\_value, using from_json().

```
json to_json(any v)
any from_json(json v)
```

- __generate\_json\_script()__
- __parse\_json\_script()__
- __to\_json()__
- __from\_json()__




## MAIN()

You can implement a function called "main" that will called by the Floyd runtime after all global statements have been executed. The main() function is optional, but the only way to get command line arguments and return a command line error code. Main can have two different forms:
- int main(){} -- which uses no input arguments or
- int main([string] args){} -- that receives command line input.


```
func int main([string] args){
	print(args)
	assert(args == ["-a", "output.txt"])

	return 42
}
```

>floyd run my_program.floyd -a output.txt

...will call your main() function with ["-a", "output.txt"]


## EXECUTING CODE: MAIN(), PROCESS, GLOBAL STATEMENTS AND COMPILE TIME


These are the steps used by the Floyd runtime to executing a Floyd program that has a main() function

1. Main thread initialises all globals and constants
2. Main thread executes all global statements 
3. Main thread calls main()
4. When main() returns, the runtime is taken down and the OS executable is exited

These are the steps used by the Floyd runtime to executing a Floyd program that has no main() function

1. Main thread initialises all globals and constants
2. Main thread executes all global statements 
3. Main thread starts all floyd processes in separate threads. How they execute is undefined here but under your control
4. When all floyd processs have exited, the runtime is taken down and the OS executable is exited.

A Floyd program either has a main() function or processes.


# 2. THE LANGUAGE REFERENCE





## SOURCE CODE FILES

Floyd files are always utf-8 files with no BOM. Their extension is ".floyd".



## EXPRESSIONS

An expression is how you calculate new values. The output of an expression is always another value.

Comparisons are deep: for a composite value they consider all members values and their member values. This goes for struct members and collections.



### LITERALS

This is a value that is fully defined directly in the code. Like the number 3.


|CODE		| EXPLANATION
|:---			|:---	
| 0				| Integer literal
| 0.3			| Double literal
| "Hello, world!	| String literal
| [ "one", "two", "three" ] | Vector-of-strings literal
| { "a": 100, "b": 200 } | Dictionary of string-integer literal.
| ...			| Any literal that is compatible with json_t can be a JSON literal
| 'A'			| Character literal. ASCII. This is equivalent to the number 65 (A as ASCII). Can be one character or an escape character (see string literals).

Floyd supports decimal literals, as above, but also hexadecimal and binary literals.





|CODE		| EXPLANATION
|:---											|:---	
| 0x0											| Hexadecimal number 0, decimal 0
| 0xff											| Hexadecimal number FF, decimal 255
| 0xabcd										| Hexadecimal number ABCD, decimal 43981
| 0xffff1111										| Hexadecimal number FFFF1111, decimal 4294906129
| 0b00000000										| Binary number 00000000, decimal 0
| 0b00000001										| Binary number 00000001, decimal 1
| 0b10000000										| Binary number 10000000, decimal 128
| 0b11111111										| Binary number 11111111, decimal 255
| 0b11111111000000000000000011111111				| Binary number 11111111000000000000000011111111, decimal 255

You can use the ' character as a dividier between groups of 8 number for both hex and binary literals. But not for decima literals. This makes longer sequences of numbers easier to read.

|CODE		| EXPLANATION
|:---											|:---	
| 0xffff1111'00000000'00000fff'fff00000				| Hexadecimal number with separators
| 0b11111111000000000000000011111111				| Binary number with separators


### VECTOR-CONSTRUCTOR

This lets you create a new vector value anywhere an expression can be put. This expression supports non-constant elements of the constructor.
TODO: More text.


### DICTIONARY-CONSTRUCTOR

This lets you create a new dictionary value anywhere an expression can be typed. This expression supports non-constant elements of the constructor.
TODO: More text.

### FUNCTION CALL

```
	let a = make_friendly_message("Lana")
```

Anywhere an expression can be put, so can a function call be put. Notice that the function value itself can also be an expression, so can each of its arguments.



### ARITHMETIC OPERATORS

How to add and combine values:

|OPERATOR		| EXPLANATION
|:---			|:---	
|+	|Addition - adds two operands: "a = b + c", "a = b + c + d"
|−	|Subtracts second operand from the first. "a = b - c", "a = b - c - d"
|*	|Multiplies both operands: "a = b * c", "a = b * c * d"
|/	|Divides numerator by de-numerator: "a = b / c", "a = b / c / d"
|%	|Modulus Operator and remainder of after an integer division: "a = b / c", "a = b / c / d"


### RELATIONAL OPERATORS

Used to compare two values. The result is true or false:

|OPERATOR		| EXPLANATION
|:---			|:---	
|	a == b	|	true if a and b have the same value
|	a != b	|	true if a and b have different values
|	a > b	|	true if the value of a is greater than the value of b
|	a < b	|	true if the value of a is smaller than the value of b
|	a >= b	|	true if a is greater than or equal to b
|	a <= b	|	true if a is less than or equal to b


### LOGICAL OPERATORS

Used to compare two values. The result is true or false:

|OPERATOR		| EXPLANATION
|:---			|:---	
| a && b		|	true if a is true and b is true
| a \|\| b		|	true if a is true or b is true


### CONDITIONAL OPERATOR

When the condition is true, this entire expression has the value of a. Else it has the value of b. Also called ternary operator, because it has three parts.

```
condition ? a : b
```

Condition, a and b can all be complex expressions, with function calls, etc.

```
func bool is_polite(string x){
	return x == "hello" ? "polite" : "rude"
}
assert(is_polite("hiya!") == false)
assert(is_polite("hello") == true)
```

### BITWISE OPERATORS

Floyd uses explicit names for all bitwise operators, not special language operators like C does. C uses "&" for AND and "<<" for shift left. This is to make evaluation order clear and to avoid accidental mixup between logical operators and bitwise operators.

|OPERATOR		| EXPLANATION
|:---	|:---
| int bw_not(int v)		| inverts all bits in the integer v.
| int bw_and(int a, int b)		| ands each bit in a with the corresponding bit in b
| int bw_or(int a, int b)		| ors each bit in a with the corresponding bit in b
| int bw_xor(int a, int b)		| xors each bit in a with the corresponding bit in b
| int bw_shift_left(int v, int count)		| shifts the bits in v left, the number of bits specified by count. New bits are set to 0.
| int bw_shift_right(int v, int count)		| shifts the bits in v right, the number of bits specified by count. New bits are set to 0.
| int bw_shift_right_arithmetic(int v, int count)		| shifts the bits in v right, the number of bits specified by count. New bits are copied from bit 63, which sign-extends the number		| it doesn't lose its negativeness.

When doing bitwise manipulation it is often convenient to use binary literals or hexadecimal literals, rather than normal decimal literals.


### EXAMPLE EXPRESSIONS

|SOURCE		| MEANING
|:---	|:---	
| 0											|
| 3											|
| 3.5										|
| (3)										|
| 3 + 4										|
| (1 + 2) * 3									|
| x											|
| x + y										|
| hello + 3									|
| "test"										|
| "test number"								|
| f()										|
| f(10, 122)									|
| print(3)									|
| print (3) 									|
| print ("Hello, World!")						|
| print("Hello, World!" + f(3) == 2)				|
| (my\_fun1("hello, 3) + 4) * my_fun2(10))		|	
| hello[\"troll\"].kitty[10].cat					|
| condition_expr ? yesexpr : noexpr				|
| condition_expr ? yesexpr : noexpr				|
| a == 1 ? "one" : ”some other number"			|



## STATEMENTS


### LET STATEMENT

Makes a new constant with a name and a value. The value cannot be changed.

```
let hello = "Greeting message."
```

|SOURCE		| MEANING
|:---		|:---	
| let int b = 11				| Allocate an immutable local int "b" and initialize it with 11
| let c = 11				| Allocate an immutable local "b" and initialize it with 11. Type will be inferred to int.
| let d = 8.5				| Allocate an immutable local "d" and initialize it with 8.5. Type will be inferred to double.
| let e = "hello"				| Allocate an immutable local "e" and initialize it with "hello". Type will be inferred to string.
| let f = f(3) == 2		| Allocate an immutable local "f" and initialize it true/false. Type will be bool.
| let int x = {"a": 1, "b": 2} | Allocate a new dictionary.




### MUTABLE STATEMENT

Makes a new local variable with a name, calculated from the expression. The variable can be changed to hold another value of the same type.


```
mutable hello = "Greeting message."
```

|SOURCE		| MEANING
|:---		|:---	
| mutable int a = 10				| Allocate a mutable local int "a" and initialize it with 10
| a = 12						| Assign 12 to local mutable "a".
| mutable int x = 10 |




## FUNC (FUNCTION) DEFINITION STATEMENT

```
func int hello(string s){
	...
}
```
This defines a new function value and gives it a name in the current scope.

|SOURCE		| MEANING
|:---	|:---	
| func int f(string name){ return 13 |
| func int print(float a) { ... }			|
| func int print (float a) { ... }			|
| func int f(string name)					|




## STRUCT DEFINITION STATEMENTS

This defines a new struct-type and gives it a name in the current scope.

```
struct my_struct_t {
	int a
	string b
}
```

|SOURCE		| MEANING
|:---	|:---	
| struct mytype_t { float a float b } | 
| struct a {} 						|
| struct b { int a }					|
| struct c { int a = 13 }				|
| struct pixel { int red int green int blue }		|
| struct pixel { int red = 255 int green = 255 int blue = 255 }|





### IF - THEN - ELSE STATEMENT

This is a normal if-elseif-else feature, like in most languages. Brackets are required always.

```
if (s == "one"){
	return 1
}
```

You can add an else body like this:

```
if(s == "one"){
	return 1
}
else{
	return -1
}
```

Else-if lets you avoid big nested if-else statements and do this:

```
if(s == "one"){
	return 1
}
else if(s == "two"){
	return 2
}
else{
	return -1
}
```

In each body you can write any statements. There is no "break" keyword.

##### EXAMPLE IF STATEMENTS

|SOURCE		| MEANING
|:---	|:---	
| if(true){ return 1000 } else { return 1001 } [





### FOR LOOP STATEMENT

For loops are used to execute a body of statements many times. The number of times is calculated *before* the first time the body is called. Many other languages evaluate the condition for each loop iteration. In Floyd you use a while-loop for that.

Example: Closed range that starts with 1 and ends with 5:

```
for (index in 1 ... 5) {
	print(index)
}
```

Example: Open range that starts with 1 and ends with 59:

```
for (tickMark in 0 ..< 60) {
}
```

You can use expressions to define the range:, not only constants:

```
for (tickMark in a ..< string.size()) {
}
```

- ..< defines an *open range*. Up to the end value but *excluding the end value*.
- ..- defines a *closed range*. Up to and *including* the end.

Notice: prefer to use map(), filter() and reduce() instead of for-loops whenever possible. This makes cleaner code and more optimisation opportunities.

### WHILE LOOP STATEMENT

Perform the loop body while the expression is true.

```
while (my_array[a] != 3){
}
```

The condition is executed each time before body is executed. If the condition is false initially, then zero loops will run. If you can calculate the number of loop iteration beforehand, then prefer to use for-loop since it expresses that better and also can give better performance.



### RETURN STATEMENT

The return statement aborts the execution of the current function as the function will have the return statement's expression as its return value.


|SOURCE		| MEANING
|:---	|:---	
| return 3						|
| return myfunc(myfunc() + 3) |





## SOFTWARE-SYSTEM-DEF STATEMENT

This is a dedicated keyword for defining software systems: **software-system-def**. Its contents is encoded as a JSON object and designed to be either hand-coded or processed by tools. You only have one of these in a software system.

|Key		| Meaning
|:---	|:---	
|**name**		| name of your software system. Something short. JSON String.
|**desc**		| longer description of your software system. JSON String.
|**people**	| personas involved in using or maintaining your system. Don't go crazy. JSON object.
|**connections**	| the most important relationships between people and the containers. Be specific "user sends email using gmail" or "user plays game on device" or "mobile app pulls user account from server based on login". JSON array.
|**containers**	| Your iOS app, your server, the email system. Notice that you gmail-server is a container, even though it's a gigantic software system by itself. JSON array with container names as strings. These strings are used as keys to identify containers.



##### PEOPLE

This is an object where each key is the name of a persona and a short description of that persona.

```
"people": {
	"Gamer": "Plays the game on one of the mobile apps",
	"Curator": "Updates achievements, competitions, make custom on-off maps",
	"Admin": "Keeps the system running"
}
```


##### CONNECTIONS

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
|**source**		| the name of the container or user, as listed in the software-system-def
|**dest**		| the name of the container or user, as listed in the software-system-def
|**interaction**		| "user plays game on device" or "server sends order notification"
|**tech**		| "webhook", "REST command"





## CONTAINER-DEF STATEMENT

This is a dedicated keyword. It defines *one* container, its name, its internal processes and how they interact.

|Key		| Meaning
|:---	|:---	
|**name**		| Needs to match the name listed in software-system-defs, containers.
|**tech**		| short string that lists the most important technologies, languages, toolkits.
|**desc**		| short string that tells what this component is and does.
|**clocks**		| defines every clock (concurrent process) in this container and lists which processes that are synced to each of these clocks
|**connections**		| connects the processes together using virtual wires. Source-process-name, dest-process-name, interaction-description.
|**probes\_and\_tweakers**		| lists all probes and tweakers used in this container. Notice that the same function or component can have a different set of probes and tweakers per container or share them.
|**components**		| lists all imported components needed for this container


You should keep this statement close to process-code that makes up the container. That handles messages, stores their mutable state, does all communication with the real world. Keep the logic code out of here as much as possible, the Floyd processes are about communication and state and time only.

##### EXAMPLE CONTAINER

```
container-def {
	"name": "iphone app",
	"tech": "Swift, iOS, Xcode, Open GL",
	"desc": "Mobile shooter game for iOS.",

	"clocks": {
		"main": {
			"a": "my_gui",
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

##### CONTAINER CODE

For each floyd process you've listed under "clocks", ("my_gui", "iphone-ux", "server_com" and "renderer" in example above) you need to implement two functions. The init-function and the message handler. These functions are named based on the process.

The init function is called x__init() where x is a placeholder. It takes no arguments, is impure and returns a value of type of your choice. This type is your process' memory slot -- the only mutable state your process has access to.

The message handler is named like your process, takes two arguments: the current state of your memory, of type T and a message to process. It's am impure function. It returns the next state of it's memory. The memory type must be the same between the init and message handler functions. It's usually a struct that represents the top level of your process' entire state.

The message is represented as a json for now. This is a workaround until there is a proper sumtype feature in Floyd.


```
func my_gui_state_t my_gui__init() impure {
	send("a", "dec")
	send("a", "dec")
	send("a", "dec")
	send("a", "dec")
	send("a", "stop")
	return my_gui_state_t(1000)
}

func my_gui_state_t my_gui(my_gui_state_t state, json message) impure{
}
```


### TODO: SWITCH STATEMENT

TODO POC



# DATA TYPES

## EXAMPLE TYPE DECLARATIONS

|SOURCE		| MEANING
|:---	|:---	
| bool								| Bool type
| int								| Int type
| string								| String type
| [int]								| Vector of ints
| [[int]]								| Vector of int-vectors
| [string:int]						| Dictionary of ints
| int ()								| Function returning int, no arguments
| int (double a, string b)				| Function returning int, arguments are double and string
| [int (double a, string b)]			| vector of functions, were function returns int and takes double and string arg.
| int (double a) ()					| Function A with no arguments, that returns a function B. B returns int and has a double argument.
| my_global							| name of custom type
| [my_global]							| vector of custom type
| mything (mything a, mything b)			| function returning mything-type, with two mything arguments
| bool (int (double a) b)				| function returns bool and takes argument of type: function that returns in and take double-argument.






## STRING DATA TYPE

This is a pure 8-bit string type. It is immutable. You can compare with other strings, copy it using = and so on. There is a small kit of functions for changing and processing strings.

The encoding of the characters in the string is undefined. You can put 7-bit ASCII in them or UTF-8 or something else. You can also use them as fast arrays of bytes.

You can make string literals directly in the source code like this:

```
let a = "Hello, world!"
```

All comparison expressions work, like a == b, a < b, a >= b, a != b and so on.

You can access a random character in the string, using its integer position, where element 0 is the first character, 1 the second etc.

```
let a = "Hello"[1]
assert(a == "e")
```

Notice 1: You cannot modify the string using [], only read. Use update() to change a character.
Notice 2: Floyd returns the character as an int, which is 64 bit signed.

You can append two strings together using the + operation.

```
let a = "Hello" + ", world!"
assert(a == "Hello, world!")
```

### ESCAPE SEQUENCES

String literals in Floyd code cannot contain any character, because that would make it impossible for the compiler to understand what is part of the string and what is the program code. Some special characters cannot be entered directly into string literals and you need to use a special trick, an escape sequence.

You can use these escape characters in string literals by entering \n or \' or \" etc.

|ESCAPE SEQUENCE	| RESULT CHAR, AS HEX		| ASCII MEANING | JSON
|:---				|:---						|:---			|:---
| \0				| 0x00	| ZERO	| NO
| \a				| 0x07	| BEL, bell, alarm, \a	| NO
| \b				| 0x08	| BS, backspace, \b	| YES
| \f				| 0x0c	| FF, NP, form feed, \f	| NO
| \n				| 0x0a	| Newline (Line Feed)	| YES
| \r				| 0x0d	| Carriage Return		| YES
| \t				| 0x09	| Horizontal Tab		| YES
| \v				| 0x0b	| Vertical Tab	| NO
| \\\\				| 0x5c	| Backslash		| YES
| \\'				| 0x27	| Single quotation mark		| NO
| \\"				| 0x22	| Double quotation mark		| YES
| \\/				| 0x2f	| Forward slash		| YES
| \uABCD			| 0xabcd	| 4 hex characters => 16 bit character	| YES
##### EXAMPLES

|CODE		| OUTPUT | NOTE
|:---			|:--- |:---
| print("hello") | hello
| print("What does \\"blob\\" mean?") | What does "blob" mean? | Allows you to insert a double quotation mark into your string literal without ending the string literal itself.


Floyd string literals do not support insert hex sequences or Unicode code points.


##### CORE FUNCTIONS

- __print()__: prints a string to the default output of the app.
- __update()__: changes one character of the string and returns a new string.
- __size()__: returns the number of characters in the string, as an integer.
- __find()__: searches from left to right after a substring and returns its index or -1
- __push_back()__: appends a character or string to the right side of the string. The character is stored in an int.
- __subset()__: extracts a range of characters from the string, as specified by start and end indexes. aka substr()
- __replace()__: replaces a range of a string with another string. Can also be used to erase or insert.



## VECTOR DATA TYPE

A vector is a collection of values where you look up the values using an index between 0 and (vector size - 1). The items in a vector are called "elements". The elements are ordered. Finding an element at an index uses constant time. In other languages vectors are called "arrays" or even "lists".


You can make a new vector and specify its elements directly, like this:

```
let a = [ 1, 2, 3]
```

You can also calculate its elements, they don't need to be constants:

```
let a = [ calc_pi(4), 2.1, calc_bounds() ]
```

You can put ANY type of value into a vector: integers, doubles, strings, structs, other vectors and so on. But all elements must be the same type inside a specific vector.

You can copy vectors using =. All comparison expressions work, like a == b, a < b, a >= b, a != b and similar. Comparisons will compare each element of the two vectors.

This lets you access a random element in the vector, using its integer position.

```
let a = [10, 20, 30][1]
assert(a == 20)
```

Notice: You cannot modify the vector using [], only read. Use update() to change an element.

You can append two vectors together using the + operation.

```
let a = [ 10, 20, 30 ] + [ 40, 50 ]
assert(a == [ 10, 20, 30, 40, 50 ])
```

##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one element of the vector and returns a new vector.
- __size()__: returns the number of elements in the vector, as an integer.
- __find()__: searches from left to right after a element and returns its index or -1
- __push_back()__: appends an element to the right side of the vector.
- __subset()__: extracts a range of elements from the vector, as specified by start and end indexes.
- __replace()__: replaces a range of a vector with another vector. Can also be used to erase or insert.



## DICTIONARY DATA TYPE

A collection of values where you identify the values using string keys. It is not sorted. In C++ you would use a std::map. 

You make a new dictionary and specify its values like this:

```
let [string: int] a = {"red": 0, "blue": 100,"green": 255}
```

or shorter:

```
b = {"red": 0, "blue": 100,"green": 255}
```

Dictionaries always use string-keys. When you specify the type of dictionary you must always include "string". In the future other types of keys may be supported.

```
struct test {
	[string: int] _my_dict
}
```

You can put any type of value into the dictionary (but not mix inside the same dictionary).

Use [] to look up values using a key. It throws an exception is the key not found. If you want to avoid that. check with exists() first.

You copy dictionaries using = and all comparison expressions work, just like with strings and vectors.


##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one value of the dictionary and returns a new dictionary
- __size()__: returns the number of values in the dictionary, as an integer.
- __exists()__: checks to see if the dictionary holds a specific key
- __erase()__: erase a specific key from the dictionary and returns a new dictionary



## STRUCT DATA TYPE

Structs are the central building block for composing data in Floyd. They are used in place of classes in other programming languages. Structs are always values and immutable. They are very fast and compact: behind the curtains copied structs shares state between them, even when partially modified.

##### AUTOMATIC FEATURES OF EVERY STRUCT

- constructor -- this is the only function that can create a value of the struct. It always requires every struct member, in the order they are listed in the struct definition. Usually you create some functions that makes instancing a struct convenient, like make_black_color(), make_empty() etc.
- destructor -- will destroy the value including member values, when no longer needed. There are no custom destructors.
- Comparison operators: == != < > <= >= (this allows sorting too).
- Reading member values.
- Modify member values.

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects caused by several clients modifying the same struct.

This all makes simple structs extremely simple to create and use.

##### NOT POSSIBLE

- You cannot make constructors. There is only *one* way to initialize the members, via the constructor, which always takes *all* members
- There is no way to directly initialize a member when defining the struct.
- There is no way to have several different constructors, instead create explicit functions like make_square().
- If you want a default constructor, implement one yourself: ```rect make_zero_rect(){ return rect(0, 0) }```.
- There is no aliasing of structs -- changing a struct is always invisible to all other code that has copies of that struct.
- Unlink struct in the C language, Floyd's struct has an undefined layout so you cannot use them to do byte-level mapping like you often do in C. Floyd is free to store members in any way it wants too - pack or reorder in any way.


Example:

```
//	Make simple, ready-for use struct.
struct point {
	double x
	double y
}

//	Try the new struct:

let a = point(0, 3)
assert(a.x == 0)
assert(a.y == 3)

let b = point(0, 3)
let c = point(1, 3)

assert(a == a)
assert(a == b)
assert(a != c)
assert(c > a)
```

A simple struct works almost like a collection with fixed number of named elements. It is only possible to make new instances by specifying every member or copying / modifying an existing one.


##### UPDATE()

let b = update(a, member, value)

```
//	Make simple, ready-for use struct.
struct point {
	double x
	double y
}

let a = point(0, 3)

//	Nothing happens! Setting width to 100 returns us a new point but we don't keep it.
update(a, x, 100)
assert(a.x == 0)

//	Modifying a member creates a new instance, we assign it to b
let b = update(a, x, 100)

//	Now we have the original, unmodified a and the new, updated b.
assert(a.x == 0)
assert(b.x == 100)
```

This works with nested values too:


```
//	Define an image-struct that holds some stuff, including a pixel struct.
struct image { string name; point size }

let a = image("Cat image.png", point(512, 256))

assert(a.size.x == 512)

//	Update the width-member inside the image's size-member. The result is a brand-new image, b!
let b = update(a, size.x, 100)
assert(a.size.x == 512)
assert(b.size.x == 100)
```


## TYPEID DATA TYPE

A typeid is tells the type of a value.

When you reference one of the built-in primitive types by name, you are accessing a variable of type typeid.

- bool
- int
- double
- string

```
assert(typeid("hello") == string)
assert(to_string(typeid([1,2,3])) == "[int]")
```

A typeid is a proper Floyd value: you can copy it, compare it, convert it to strings, store it in dictionaries or whatever. Since to_string() supports typeid, you can easily print out the exact layout of any type, including complex ones with nested structs and vectors etc.



## JSON DATA TYPE

Why JSON? JSON is very central to Floyd. Floyd is based on values (simple ones or entire data models as one value) JSON is a simple and standard way to store composite values in a tree shape in a simple and standardized way. It also makes it easy to serializing any Floyd value to text and back. JSON is built directly into the language as the default serialized format for Floyd values.

It can be used for custom file format and protocol and to interface with other JSON-based systems. All structs also automatically are serializable to and from JSON automatically.

JSON format is also used by the compiler and language itself to store intermediate Floyd program code, for all logging and for debugging features.

- Floyd has built in support for JSON in the language. It has a JSON type called __json\_value__ and functions to pack & unpack strings / JSON files into the JSON type.

- Floyd has support for JSON literals: you can put JSON data directly into a Floyd file. Great for copy-pasting snippets for tests.

Read more about JSON here: www.json.org


This value can contain any of the 7 JSON-compatible types:

- **string**, **number**, **object**, **array**, **true**, **false**, **null**

Notice: This is the only situation were Floyd supports null. Floyd things null is a concept to avoid when possible.

Notice that Floyd stores true/false as a bool type with the values true / false, not as two types called "true" and "false".

__json\_value__: 	This is an immutable value containing any JSON. You can query it for its contents and alter it (you get new values).

Notice that json\_value can contain an entire huge JSON file, with a big tree of JSON objects and arrays and so on. A json\_value can also contain just a string or a number or a single JSON array of strings. The json\_value is used for every node in the json\_value tree.


##### __get\_json\_type()__:

Returns the actual type of this value stores inside the json\_value. It can be one of the types supported by JSON.

	typeid get_json_type(json v)

This needs to be queried at runtime since JSON is dynamically typed.

##### CORE FUNCTIONS

Many of the core functions work with json\_value, but it often depends on the actual type of json\_value. Example: size() works for strings, arrays and object only.

- __get\_json\_type()__
- __size()__
- __generate\_json\_script()__
- __parse\_json\_script()__
- __to\_json()__
- __from\_json()__





## TODO: PROTOCOL DATA TYPE

TODO 1.0

This defines a set of functions that several clients can implement differently. This introduced polymorphism into Floyd. Equivalent to interface classes in other languages.

Protocol member functions can be tagged "impure" which allows it to be implemented so it saves or uses state, modifies the world. There is no way to do these things in the implementation of pure protocol function members.






# 3. CORE FUNCTIONS

These functions are built into the language itself and are always available to your code. They are all pure unless explicitly specified to be impure.


### print() -- IMPURE

** IMPURE FUNCTION **
This outputs one line of text to the default output of the application. It can print any type of value. If you want to compose output of many parts you need to convert them to strings and add them. Also works with types, like a struct-type.

	print(any)


| Example										| Result |
|---											| ---
| print(3)										| 3
| print("shark")								| shark
| print("Number four: " + to_string(4))			| Number four: 4
| print(int)									| int
| print([int])									| [int]
| print({string: double})						| {string:double}
| print([7, 8, 9])								| [7, 8, 9]
| print({"a": 1})								| {"a": 1}
| print(json("b"))						| b
| print(json(5.3))						| 5.3
| print(json({"x": 0, "y": -1}))			| {"a": 0, "b": -1}
| print(json(["q", "u", "v"]))			| ["q", "u", "v"]
| print(json(true))						| true
| print(json(false))						| false
| print(json(null))						| null




### assert()

Used this to check your code for programming errors, and check the inputs of your function for misuse by its callers.

	assert(bool)

If the expression evaluates to false, the program will log to the output, then be aborted via an exception.

Example:

```
func f(int x){
	assert(x >= 0)
}
```


### to_string()

Converts its input to a string. This works with any type of values. It also works with types themselves, which is useful for debugging.

	string to_string(any)

You often use this function to convert numbers to strings.



### to\_pretty\_string()

Converts its input to a string of JSON data that is formatted nicely with indentations. It works with any Floyd value.



### typeof()

Return the type of its input value. The returned typeid-value is a complete Floyd type and can be stored, compared and so on. The type is resolved at compile time so the same source code line will always return the same type. 

```
typeid typeof(any)
```



### send() -- IMPURE

Sends a message to the inbox of a Floyd process, possibly your own process.

The process may run on a different OS thread but send() is guaranteed to be thread safe.

	send(string process_key, json message) impure

The send function returns immediately.





## WORKING WITH COLLECTIONS

### update()

This is how you modify a field of a struct, an element in a vector or string or a dictionary. It replaces the value of the specified key and returns a completely new object. The original object (struct, vector etc) is unchanged.

	let obj_b = update(obj_a, key, new_value)


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| update("hello", 3, 120)		| "helxo"
| vector		| update([1,2,3,4], 2, 33)		| [1,2,33,4]
| dictionary	| update({"a": 1, "b": 2, "c": 3}, "a", 11) | {"a":11,"b":2,"c":3}
| struct		| update(pixel, red, 123)		| pixel(123,---,---)
| json:array		| 
| json:object		| 

For dictionaries it can be used to add completely new elements too.

|TYPE		  	| EXAMPLE						| RESULT
|:---			|:---							|:---
| dictionary	| update({ "a": 1 }, "b", 2] | { "a": 1, "b": 2 }



TODO 1.0 - Update nested collections and structs.



### size()

Returns the size of a collection -- the number of elements.

```
int size(obj)
```

|TYPE		  		| EXAMPLE					| RESULT
|:---				|:---						|:---
| string			| size("hello")				| 5
| vector			| size([1,2,3,4])			| 4
| dictionary		| size({"a": 1, "b": })		| 2
| struct			|							|
| json:array	| size(json([1,2,3])	| 3
| json:object	| size(json({"a": 9})	| 1





### push_back()

Appends an element to the end of a collection. A new collection is returned, the original unaffected.

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| push_back("hello", 120)		| hellox
| vector		| push_back([1,2,3], 7)			| [1,2,3,7]
| dictionary	| 								|
| struct		|								|
| json:array	|							|
| json:object	| 							|





### subset()

This returns a range of elements from the collection.

```
string subset(string a, int start, int end)
vector subset(vector a, int start, int end)
```

start: 0 or larger. If it is larger than the collection, it will be clipped to the size.
end: 0 or larger. If it is larger than the collection, it will be clipped to the size.


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| subset("hello", 2, 4)			| ll
| vector		| subset([10,20,30,40, 1, 3)	| [20,30]
| dictionary	| 								|
| struct		|								|
| json:array	|							|
| json:object	| 							|



### replace()

Replaces a range of a collection with the contents of another collection.

```
string replace(string a, int start, int end, string new)
vector replace(vector a, int start, int end, vector new)
```


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		|replace("hello", 0, 2, "bori")	| borillo
| vector		|replace([1,2,3,4,5], 1, 4, [8, 9])	| [1,8,9,5]
| dictionary	| 								|
| struct		|								|
| json:array	|							|
| json:object	| 							|


Notice: if you use an empty collection for *new*, you will actually erase the range.
Notice: by specifying the same index in *start* and *length* you will __insert__ the new collection into the existing collection.



### find()

Searches for a value in a collection and returns its index or -1.

```
int find(obj, value)
```

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| find("hello", "o")			| 4
| string		| find("hello", "x")			| -1
| vector		| find([10,20,30,40], 30)		| 2
| dictionary	| 								|
| struct		|								|
| json:array	|							|
| json:object	| 							|



### exists()

Checks if the dictionary has an element with this key. Returns true or false.

```
bool exists(dict, string key)
```

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| 								|
| vector		| 								|
| dictionary	| exists({"a":1,"b":2,"c":3), "b")	| true
| dictionary	| exists({"a":1,"b":2,"c":3), "f")	| false
| struct		|								|
| json:array	|							|
| json:object	| 							|




### erase()

Erase an element in a dictionary, as specified using its key.

```
dict erase(dict, string key)
```


### get_keys()

Use this with a dictionary to get a vector with all the dictionary's keys. Then you can loop through the keys to get to all the values of the dictionary.

```
vector get_keys(dict)
```

Example
```
let a = { "a": 1, "b": 2, "c" : 3 }
let b = get_keys(a)
assert(b == [ "a", "b", "c"])
```

Notice that the *order* of the keys is undefined and may change depending on the what backend is used for the dictionary.





## FUNCTIONAL-STYLE COLLECTION FUNCTIONS

Also called "Higher-order functions".

IMPORTANT: These functions replace custom loops but *also* expose parallelism opportunities that allows the Floyd runtime to process each element on a separate hardware core, like shaders works in a graphics card. The supplied function must be pure.

### map()

Processes a vector of values one by one using function f. It returns a vector of the same size, but with values of type R.

```
[R] map([E] elements, func R (E e, C context) f, C context)
```

Supports mapping over
- vectors
- characters in a string



### map_dag()

```
[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
```

This function runs a bunch of tasks with dependencies between them. When map_dag() returns, all tasks have been executed.

- Tasks can call blocking functions or impure functions. This makes the map_dag() call impure too.
- Tasks cannot generate new tasks.
- A task *can* call map_dag.
- Task will not start until all its dependencies have been finished.
- There is no way for any code to observe partially executed map_dag(). It's done or not.

- **values**: a vector of tasks and their dependencies. A task is a value of type T. T can be an enum to allow mixing different types of tasks. Each task also has a vector of integers tell which other tasks it depends upon. The indexes are the indexes into the tasks-vector. Use index -1 to mean *depends on nothing*.

- **f**: this is your function that processes one T and returns a result R. The function must not depend on the order in which tasks execute. When f is called, all the tasks dependency tasks have already been executed and you get their results in [R].

- **result**: a vector with one element for each element in the tasks-argument. The order of the elements is the same as the input vector.


Notice: your function f can send messages to a clock — this means another clock can start consuming results while map_dag() is still running.

Notice: using this function exposes potential for parallelism.



### filter()

Processes a vector of values and returns each that function f decides to include.

```
[E] filter([E] elements, func bool (E e, C context) f, C context)
```


### reduce()

Processes a vector or values using the supplied function. Result is *one* value.

```
R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
```


### stable_sort()

Sort a vector and return a new sorted vector. The existing vector is unchanged.

```
[T] stable_sort([T] elements, func bool (T left, T right, C context) less, C context)
```





## WORKING WITH JSON


### get\_json_type()

If you have a json, this is how you find out if it's a string or a number and so on. It can be one of the types supported by JSON.

```
typeid get_json_type(json v)
```

This is how you check the type of JSON value and reads their different values.

|INPUT						| RESULT 		| INT
|:---						|:---			|:---
| json({"a": 1})		| json_object	| 1
| json([1, 2, 3])		| json_array	| 2
| json("hi")			| json_string	| 3
| json(13)			| json_number	| 4
| json(true)			| json_true		| 5
| json(false)			| json_false	| 6
| 							| json_null		| 7


Demo snippet, that checks type of a json\_value:

```
func string get_name(json value){
	let t = get_json_type(value)
	if(t == json_object){
		return "json_object"
	}
	else if(t == json_array){
		return "json_array"
	}
	else if(t == json_string){
		return "json_string"
	}
	else if(t == json_number){
		return "json_number"
	}
	else if(t == json_true){
		return "json_true"
	}
	else if(t == json_false){
		return "json_false"
	}
	else if(t == json_null){
		return "json_null"
	}
	else {
		assert(false)
	}
}
	
assert(get_name(json({"a": 1, "b": 2})) == "json_object")
assert(get_name(json([1,2,3])) == "json_array")
assert(get_name(json("crash")) == "json_string")
assert(get_name(json(0.125)) == "json_number")
assert(get_name(json(true)) == "json_true")
assert(get_name(json(false)) == "json_false")
```


### generate\_json\_script()

Pack a JSON value to a JSON script string, ready to write to a file, send via protocol etc. The string is unescaped.

	string generate_json_script(json v)

The result is a valid JSON script string that can be handed to another system to be unpacked.


### parse\_json\_script()

Make a new Floyd JSON value from a JSON-script string. If the string is malformed, exceptions will be thrown. The string is unescaped.
 
	json parse_json_script(string s)



### to\_json()

	json to_json(any v)



### from\_json()

	any from_json(json v)






# 4. COMMAND LINE TOOL

|COMMAND		  	| MEANING
|:---				|:---	
| floyd run mygame.floyd | compile and run the floyd program "mygame.floyd"
| floyd compile mygame.floyd | compile the floyd program "mygame.floyd" to an AST, in JSON format
| floyd help		| Show built in help for command line tool
| floyd runtests	| Runs Floyds internal unit tests
| floyd benchmark 		| Runs Floyd built in suite of benchmark tests and prints the results.
| floyd -t mygame.floyd		| the -t turns on tracing, which shows Floyd compilation steps and internal states





# 5. REFERENCE



### EXAMPLE SOFTWARE SYSTEM FILE


```
software-system-def {
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
			"a": "my_gui",
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

func string my_gui__init() impure {
	print("HELLO")
	send("a", "stop")
	send("b", "stop")
	send("c", "stop")
	send("d", "stop")
	return "a is done"
}

```
## FLOYD SYNTAX

Here is the DAG for the complete syntax of Floyd.

	IDENTIFIER_CHARS: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
	WHITESPACE: " \n\t"

	LITERAL:
		BOOL_LITERAL: "true" or "false"
		INTEGER_LITERAL: [0123456789]
		DOUBLE_LITERAL: [0123456789.]
		STRING_LITERAL: "?"
	IDENTIFIER: IDENTIFIER_CHARS*

	TYPE:
		NULL							"null"
		BOOL							"bool"
		INT							"int"
		DOUBLE						"double"
		STRING						"string"
		TYPEID						???
		VECTOR						"[" TYPE "]"
		DICTIONARY					"{" TYPE ":" TYPE" "}"
		FUNCTION-TYPE					TYPE "(" ARGUMENT ","* ")" "{" BODY "}"
			ARGUMENT					TYPE identifier
		STRUCT-DEF					"struct" IDENTIFIER "{" MEMBER* "}"
			MEMBER					 TYPE IDENTIFIER
		UNNAMED-STRUCT				"struct" "{" TYPE* "}"

		UNRESOLVED-TYPE				IDENTIFIER-CHARS, like "hello"

	EXPRESSION:
		EXPRESSION-COMMA-LIST			EXPRESSION | EXPRESSION "," EXPRESSION-COMMA-LIST
		NAME-EXPRESSION				IDENTIFIER ":" EXPRESSION
		NAME-COLON-EXPRESSION-LIST		NAME-EXPRESSION | NAME-EXPRESSION "," NAME-COLON-EXPRESSION-LIST

		TYPE-NAME						TYPE IDENTIFIER
		TYPE-NAME-COMMA-LIST			TYPE-NAME | TYPE-NAME "," TYPE-NAME-COMMA-LIST
		TYPE-NAME-SEMICOLON-LIST		TYPE-NAME ";" | TYPE-NAME ";" TYPE-NAME-SEMICOLON-LIST

		NUMERIC-LITERAL				"0123456789" +++
		RESOLVE-IDENTIFIER				IDENTIFIER +++

		CONSTRUCTOR-CALL				TYPE "(" EXPRESSION-COMMA-LIST ")" +++

		VECTOR-DEFINITION				"[" EXPRESSION-COMMA-LIST "]" +++
		DICT-DEFINITION				"[" NAME-COLON-EXPRESSION-LIST "]" +++
		ADD							EXPRESSION "+" EXPRESSION +++
		SUB							EXPRESSION "-" EXPRESSION +++
									EXPRESSION "&&" EXPRESSION +++
		RESOLVE-MEMBER				EXPRESSION "." EXPRESSION +++
		GROUP						"(" EXPRESSION ")"" +++
		LOOKUP						EXPRESSION "[" EXPRESSION "]"" +++
		CALL							EXPRESSION "(" EXPRESSION-COMMA-LIST ")" +++
		UNARY-MINUS					"-" EXPRESSION
		CONDITIONAL-OPERATOR			EXPRESSION ? EXPRESSION : EXPRESSION +++

	STATEMENT:
		BODY							"{" STATEMENT* "}"

		RETURN						"return" EXPRESSION
		DEFINE-STRUCT					"struct" IDENTIFIER "{" TYPE-NAME-SEMICOLON-LIST "}"
		DEFINE-FUNCTION				"func" TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" BODY
		DEFINE-IMPURE-FUNCTION			"func" TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" "impure" BODY
		IF							"if" "(" EXPRESSION ")" BODY "else" BODY
		IF-ELSE						"if" "(" EXPRESSION ")" BODY "else" "if"(EXPRESSION) BODY "else" BODY
		FOR							"for" "(" IDENTIFIER "in" EXPRESSION "..." EXPRESSION ")" BODY
		FOR							"for" "(" IDENTIFIER "in" EXPRESSION "..<" EXPRESSION ")" BODY
		WHILE 						"while" "(" EXPRESSION ")" BODY

 		BIND-IMMUTABLE-TYPED			"let" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-IMMUTABLE-INFERETYPE		"let" IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-TYPED				"mutable" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-INFERCETYPE			"mutable" IDENTIFIER "=" EXPRESSION
		EXPRESSION-STATEMENT 			EXPRESSION
 		ASSIGNMENT	 				IDENTIFIER "=" EXPRESSION
		SOFTWARE-SYSTEM-DEF				"software-system-def" JSON_BODY
		COMPONENT-DEF					"component-def" JSON_BODY


