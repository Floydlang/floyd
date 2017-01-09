"Motherboard" - better names: app, solution, finalizer, wrapper, product, application, top-level

"WORLD" - Better name needed for tech concept: "Snapshot", "Checkout", Git term: "staging area".



??? It's possible to have a pure function that "sends" a REST-request, then has a separate function that is called with response. Open loop. Bad idea? Think about these as separate go-channels.
		on_rest_reply(reply)


# Goroutines
A goroutine is a lightweight thread managed by the Go runtime.

go f(x, y, z)
starts a new goroutine running

f(x, y, z)
The evaluation of f, x, y, and z happens in the current goroutine and the execution of f happens in the new goroutine.

Goroutines run in the same address space, so access to shared memory must be synchronized. The sync package provides useful primitives, although you won't need them much in Go as there are other primitives. (See the next slide.)

< 1/11 > goroutines.go Syntax

select {
	case v: <-ch1
}

??? seq vs channels? Is there a way to make pure code that reads from channels? Iterator / generator vs channel? Use for

Find good Coroutine library!!!

- Copy Go channels for our optocoupler? Clojure Core.Async? Allocate "channel" live?


# INSIGHTS
- Insight: synchronization points between systems (state or concurrent) always breaks composition. Move to top of product. ONE super-mediator per complete server-spanning solution.

- Concept: State-less lambda. Takes time but is referential transparent. These can be callable from pure code.




# CLOCK
A clock represents a series of discrete time increments / transformations. The output is always an immutable value. If you want concurrency you need to have several clocks, each running at their own rate.

Clocks is how mutatation / memory is handled in the motherboard.

A clock has one or more inputs and ONE output. The motherboard defines when the clock is advanced. At this moment, all its inputs are frozen and it's clock-function is called. This function is referenctial transparent. When the clock function is done it returns the result, which is stored in a log of all outputs. The previous output of a clock-function is always passed in as first argument to the clock-function the next time.

		"clock": {
			"name": "ui_clock",
			"inputs": [
			]
			"clock-function": "ui_clock_function"
			"layout": {		"x": 48, "y": 16 }
		}

??? add info about blocking on external event (blocking on reply from socket?).


Your motherboard uses a list of expressions to trigger a clock advancement, like

		ui_clock_function:
			menu_selection_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
			mouse_input_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
			timeout(60.0) : ui_clock_function.log += ui_clock_function(prev_clock)

??? Replace this with a my_ui_process() that can contain a sequence of stuff and several blockings. No need to have clock.

		var my_socket = socket()
		void my_ui_process(){
			wait:
				my_socket: ui_clock_function.log += ui_clock_function(prev_clock)
				menu_selection_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
				mouse_input_optocoupler: ui_clock_function.log += ui_clock_function(prev_clock)
				timeout(60.0) : ui_clock_function.log += ui_clock_function(prev_clock)
	
			do something else..
			wait:
				sdfsd
		}


# OPTOCOUPLERS

"optocoupler": {
	"writer": "socket_clock",
	"reader": "command_line_clock"
}
There is no way for a clock to access that from another clock-function - they run in their own sandbox.

Optocouplers lets you send a value between two clocks, even though they run in different speed. An optocoupler is similar to a Golang channel.

READER modes:
- Default: reader blocks until new value is written be writer.
- Sample the latest value in the optocoupler, don't block.
- Sample queue of all values since last read.
Notice that the value can be a huge struct with all app state or just a message or a UI event etc.

Notice: pure functions cannot access optocouplers. They would not be referential transparent.




# PROBLEM 2 - DIFF AND MERGE EFFICIENTLY
Solve externals support by calculating a new snapshot of its state, then diff. Diffing becomes central.

??? todo
- diff - merge


# PROBLEM 3 - mutation, clocks, externals
Top level, no statements allowed


# PROBLEM 4 - Monad problem
Practical Pure code vs top-level. The bulk of the code should be pure. How to make it simple to write pure code that in the end affects the world. No monads. Let pure code do more work directly = easier to code.


# PROBLEM 4 - SOLUTION Z - MONADS

Sucks.

# PROBLEM 4 - SOLUTION A - RUNTIME AND MERGE
Perform mutating changes in local copy, then merge to external when committing. The commit needs to be done by top-level inside a transaction. Like React. Like Git. If commit comflicts, top-level can chose to merge, rollback or ignore error, overwrite etc.
	PRO: Composable.
	PRO: Main work cannot fail.
	PRO: Testable.
	PRO: No motion blur.
	PRO: No intermediate results ever observable by any client.

	CON: Must be able to simulate all results - won't work for talking to server for example.
	CON: Must have a copy of the state to start with.
	CON: Cannot load additional data.
	CON: Can cause merge conflicts if other code changes the same data.

		dom_copy make_lots_of_changes(dom_copy d){
			a = d.insert_div(0);
			a = a.insert_div(a.end());
			return a;
		}
		
		my_top_level@(){
			a = make_lots_of_changes(_my_dom);
			_system_dom = merge(_system_dom, a);
		}


# PROBLEM 4 - SOLUTION A2 - RUNTIME AND MERGE, MOTION BLUR
Pulls in data from the world on-demand. This makes huge potential worlds possible but makes start non-atomic.


# PROBLEM 4 - SOLUTION B - REAL MUTATE AND ROLLBACK
We perform mutatation directly in the wrold, assuming it cannot fail. In case this illusions blows up, we rollback it all and try again..
Concepts: Clean room vs airlock vs real-world.
Spurious changes to externals.
Time capsule illusion. XTimeCapsuleBroken

	PRO: Simple to implement and understand for programmer.
	CON: Motion blur problem: either you need to start top-level transaction by opening/locking all external assets for the transaction, or client code will open/lock them one by one (causing motion blur).
	CON: Requires detection / guessing race conditions with other clients modifying / observering the same data.

		void store_preferences@(my_prefs p, string path){
			store_file(path + "/preferences.json", p.to_json());
			store_file(
		}
	
		my_top_level@(){
			store_preferences@(_prefs, "~/Application Support/Preferences/My App");
		}

	**CON: NOT REFERENTIAL TRANSPARENT!!**


# PROBLEM 4 - SOLUTION C - DEFER MUTATION TO TOP-LEVEL
Return future that performs mutation later, by the top-level function. Monads.

	1) You want to perform as much work as possible outside the functor.
	2) The function can contain complexity and moving parts but to client appear as pure + rollback. Asynk.

	PRO: Your unpure code can do synchronous calls (it will block its thread).
	PRO: Unpure code can do bidrectional communication.
	PRO: Composable. You block on these functions. Entire top-level function becomes a future!???
	CON: Unpredictable execution time.
	CON: How does top-level function chain these? Ugly chained callback style?
	CON: Fails needs to be handled asynk.

D) __OUTSIDE OF SIMULATION__. Define some mutation as a sub-layer in the runtime stack. This means that the side effects are completely hidden from the simulation, like memory allocation heap can or JIT-loading of read-only resources ec. To the simulation it appears to be guaranteed features. If the guarantee cannot be held, the simulation is rolled back.

Use for more stuff?

	CON: Only works when no other client can observe what's going on in the runtime.
	CON: Only works for sync operations.

	auto f1(){
		//	Lazy-loads text string from resource file. Caches. Throws exception if it cannot be found or OOM.
		text a = lookup_text("Save button");
		return a;
	}

E) _REFACTOR-CODE-TO-RETURN-ABSOLUTE-STATE_ Return the contents of the file you want to write, let top-level actually explicitly write it to FS.


F) _RETURN-OBJECT-DESCRIBING-UNPURE-OPERATIONS_.

		RESTGetRequest download_image(uri a, text image_name){
			return RESTGetRequest{
				uri = a + "?=23" + image_name,
				
			}
		}

# Examples

- Using REST GET can be pure.


{my_document} demo_local_and_merge@[#transaction](my_document d){
	r = d.replace_layers(inverse_layers(d.layers))
	return r
}

//	Returns a mutating function that performs its work later. Only top-level functions can call @-functions.
{my_document@[#transaction]()} demo_defer_mutation_to_top_level[#transaction](my_document d){
	return my_document @[#transaction](my_document d){
		r = d.replace_layers(inverse_layers(d.layers))
		return r
	}
}


# Floyd Runtimes

- PRINCIPLE: Define "side effect": layers of illusions of side-effect free: malloc, using CPU resources, consuming COW time to execute, recording stats / profiling etc. Side effects? No: there are mutable mechanisms hidden in the runtime that the simulation cannot observe = OK and considered side-effect free. Reading from internet server is a pure operation, even if server records log files.

- Runtimes: with every function is a secret argument specifying the runtime. It use used for allocating new values and other runtime things. It has a type. You can make your own runtime-types and add capabilities. A function can only be called if you have the proper typ of runtime object.

	<simulator_runtime>
	core_runtime: memory allocation (for values), debug, log, exceptions.
	file_system_runtime
	Default is *core_runtime*.
	*file_runtime* provides (core_support and file_system_support).
	You can make your own combinations. Example of custom runtime could be renderer_runtime for game.

	//	This means this function requires a renderer_runtime to be used.
	auto my_function[renderer_runtime](int a)

# Clients

An observer looks at an aspect of the outside world and needs it to be consistent.
It defines what part of world it observers and how consistent it needs to be.
You make observers in your Floyd program to represent other programs or services.
Without these observers, Floyd could optimize-away a lot of the 

observer {
	dir_spec = "/Volumes/Macintosh HD/Users/Marcus/Pictures/";
}


# Floyd Externals - how to affect the world outside the simulation

Make high-level replacements for externals, on a per case bases. File system, REST etc.

- PRINCIPLE: Only top-level functions can commit transactions / advance time. Top level functions cannot be nestable and you write them explicitly.
Top level function has one clock.

- PRINCIPLE: Floyd code mutating external state can be hidden by simulating the external object, then make one transaction that diffs new state to external. Throw exception if things could not be integrated. Only top-level code performs transaction. Problem is when two-way communication is needed = external object cannot be simulated. Like communicating with webserver via sockets.

- Snapshot of read files can be made gradually while floyd accesses the files. BUT: this causes “motion blur” of read data.

- Use temporary renaming of directories and files to random names to hide them while used by transaction. Only when all have been hidden does the transaction start.

- “@“  = clock symbol / mutation

- # = tag symbol

- _ and _1 and _2 are placehold names for lambdas.

- TECHNIQUE: client has copy of entire state and modifies it in private. Later integrate state with external state. React, Git etc.

	f = readfile@(“c:/test/bild.jpeg”);
	header = f.first(4)
	if(header == “text”){
	}

- PRINCIPLE: Each external client observing the Floyd program must be defined to Floyd. This way floyd knows if and how it needs to propagate changes to the external. Example: if your command line tool writes two JSON files to the file system you can setup a client that requires the two files to appear all ready as soon as possible in an atomic way. Or you can define the client to want the files done when your tool exits and won’t care if intermediate states are visible in the file system.

	t0: no files
	t1: both files complete

# Top level functions

string diff(string a, string b)
{string, string} split_main_args(string args)

int main(string[] args){
	if(args.count() == 4){
		auto source_path1 = args[1];
		auto source_path2 = args[2];
		auto dest_file_path = args[3];
		auto params = split_main_args(args[0]);

		@{
			var source1 = load_file@(source_path1);
			var source2 = load_file@(source_path2);
			var result = diff(source1, source2);
			store_file@(dest_file_path);
		)
	}
	else{
		return -1;
	}
}

int main(string[] args){
	if(args.count() == 4){
		auto source_path1 = args[1];
		auto source_path2 = args[2];
		auto dest_file_path = args[3];
		auto params = split_main_args(args[0]);

		@{
			auto s = open_socket@("https:\\www.trolldom.se");
			var r1 = s.rest_get("xyc").unpack_json();
			var r2 = r1.sha1 != null ? s.rest_get("xyc").unpack_json() : null;
			var r3 = s.rest_delete("xyc" + r1.sha1).unpack_json();
			if(r3 == "OK"){
				return 0;
			}
			else{
				return -1;
			}
		)
	}
	else{
		return -1;
	}
}


# Files

INPUT
	filea.txt = “hello”
	fileb.txt = “world”
OUTPUT
	filec.txt = “kitty”

# Sockets
	/*
		This function does not modify the server state. (Except stats and any cloud code).
		Calling it with the same inputs gives the same reply every time.
		It consumes COW time because of communication.

		NOTICE: This function itself cannot observe time since it is sync.
	*/
	auto read_my_sequence@(socket s, string base_url, credentials c, string troll_sha1){
		a_raw = s.get_sync@(base_url + “/trolls/“ + troll_sha1;
		a = unpack_json_to_map(a_raw);
		switch(t : a.type){
			case(t == “v1”){
				v1 = s.get_sync@(base_url + "/old_trolls/" + troll_sha1 + "?convert").unpack_json_to_map();
				v2 = s.get_sync@(base_url + "/toplist/").unpack_json_to_map();
				if(find(v1.popular, v2.type){
					resp = s.delete_sync@(base_url + "/trolls/").unpack_json_to_map();
				}
			}
		}
	}

#insert

Do we pass around the runtime as a value, that can be replaced / mutated? If it represents file system:

a) No changes clients perform updates the runtime at current clock tick.Those changes happens next tick.

B) Changes happend synchronously from client’s perspective - reading back from runtime is possible. Return new runtime state.

Clients CANNOT create runtimes, they are stacked together on motherboard.






# PURE FILE SYSTEM RUNTIME

struct info {
	bool is_dir;
	string name
	int file_size
	bytes meta_data
}


/*
	No changes performed to file system.
	All reads will be performed on exteran FS just-in-time: this means we get motion-blur on read.

	??? If order is important *between* different runtimes, we may need to perform a system-wide submit where different external-writes are interleaved.
*/
runtime fs_runtime {
	uri get_parent(uri item)
	string get_node_name(uri path)
	uri get_file_path(uri path, string filename)

	info get_info(uri path)

	/*
		Reads entire file into memory.
	*/
	seq<byte> read_file(uri path)

	/*
		Stores a file to file system, replacing any previous version of it.

	*/
	fs_runtime store_file(uri path, seq<byte> data)


	fs_runtime delete_file(uri path)
	fs_runtime delete_dir(uri path)
	fs_runtime make_dir(uri parent, string dirname)


	///////////////////////////		STATE
	//	Describe changes to file system, compared to current world state.
	map<uri, >
}

/*
	Merges the fs_runtime into the real world, applying changes to file system.
*/
commit_to_world(fs_runtime r)



==============================================


defworld mainworld {
	//	We need to access the file system.
	fs_runtime fs;
	logentru log;

	/*
		Describes where the virtual program counter is when _tick() returns.
		It is ususally nil.
			nil: function executed completely and returned a new world-state.
			blocking: execution paused because it needs to communicate with outside world = unpure.
			blocking {
				continuate* current_pc;
				waiting for file to load / waiting for REST response / waiting for several things.
			}
	*/
	pc pc;
}

clock mailbox

defclock main {

	/*
		Called to by runtime to advance the world ONE clock tick.
		_tick() is a pure function.
	*/
	mainworld _tick(log log, mainworld world0){
		world1 = world.counter++;
		return world1;
	}
}




/*

 A: --- read --- read --- write --- read --- write ---
B does:

 B: ---- write --- write --- read --- read --- write ---
read() ->
    receive
        Pattern1 -> 	
           ...
        Pattern2 -> 
           ...
    end.
*/


process mailbox






# FLOYD TOKENS AND RUNTIMES


A token is a value object that is immutable. It can represents a lifetime in one runtime. You can resolve it and it gives you a value. This value is the same all the time.

Similar to reference counted file handles.

Clients that observer tokens cannot see them change.

But next clock, tokens can resolve to new values.

Tokens can represent physical side effects, like files, data in sockets, concurrency, hardware.

Tokens own their runtime, cannot exists without their runtime.



### Filesystem Runtime

JIT freeze the world while reading?
All changes goes to RAM first

All times is hidden from program.

ReadFile()
Futures resolved between clock ticks.
Modifications create uncommitted generations of the file system.

Support completion-lambdas? No?
No completion chains — do this automatically instead!

Result of run_clocktick() = a future. It cannot return a value since clock needs to advance (mutate) in several generations first.



# Problem 1 - Async, future, completions

Scenario: communicate with a read-only server using REST. No side effects, just time.
GOAL: One well-defined and composable method of async. No DSL like nested completions.

A) Use futures and promises, completions.
B) Block clock thread, hide time from program. This makes run_clock() a continuation!!!
C) clock
D) Clock result will contain list of pending operations, as completions. Now client decides what to do with them. This means do_clock() only makes one tick.

Can pure function create a socket? Token socket is OK. Or create socket with clock at top level?! This allows simple asynchronous programming: blocking-style-only.

		### Channels: pure code can communicate with sockets using channels. When waiting for a reply, the pure code's coroutine is paused and returned to motherboard level. Motherboard resumes coroutine when socket responds. This hides time from the pure code - sockets appear to be instantantoues + no inversion of control. ??? Still breaks rule about referenctial transparency. ### Use rules for REST-com that requires REST-commands to be referential transparent = it's enough we only hide time. ??? more?




# Problem 2 - Side-effects in the world, while reading back

Scenario: reading and writing to local file system, no races with other processes.

A) Mutate directly, propagate to world
B) Work in stable copy then top-level code merges with world. Possible 
C) Top-level code use normal mutation.
D) Top-level mutatation happens usng HW-metaphore
E) 

