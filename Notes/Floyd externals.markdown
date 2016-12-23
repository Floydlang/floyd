# INSIGHTS

- **PRINCIPLE**: Each top-level function introduces an implicit transactions. All pure and unpure functions runs inside this transaction. Top-level function needs to commit or rollback the transaction. A single top-level function always represents exactly _one_ transaction. Use two different top-level functions if you need two simultaneous transactions.
- PRINCIPLE: All parallell transactions are run as if they were run by separate hardware processor. It doesn't matter if they are called from different GUI event handlers (and only one actually runs at a time) or from different threads or async callbacks or other mechanisms.


# MUTATION STRATEGIES

Here I try to catalogue different approaches to allowing you to write functions that are pure but allow affecting the world in some respect. The idea is to make a toolbox of well-known strategies either for A) use in client programs or (B) building into the language so client programs don't need to use them.

A) __LOCAL AND MERGE__. Perform mutating changes in local copy, then merge to external when committing. The commit needs to be done by top-level inside a transaction. Like React. Like Git. If commit fails, rollback top-level function simulation.
	PRO: Composable.
	PRO: Main work cannot fail.
	PRO: Testable.
	PRO: No motion blur.
	PRO: No intermediate results ever observable by any client.
	CON: Must be able to simulate all results - won't work for talking to server for example.
	CON: Must have a copy of the state to start with.
	CON: Cannot load additional data.
	CON: Can cause merge conflicts if other code changes the same data.

Needs to be composed with functions higher up in callstack if part of grander transaction.

		dom_copy make_lots_of_changes(dom_copy d){
			a = d.insert_div(0);
			a = a.insert_div(a.end());
			return a;
		}
		
		my_top_level@(){
			a = make_lots_of_changes(_my_dom);
			_system_dom = merge(_system_dom, a);
		}

B) __MUTATE AND ROLLBACK__. Perform mutatation directly, like it cannot fail and roll back if something went wrong. Needs to detect if something went wrong.

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

C) __DEFER MUTATION TO TOP-LEVEL__. Return future that performs mutation later, by the top-level function. Monads.
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


### Define assumption

- 

??? nest JSON? Expand or store as string constants?
??? Can JSON be used to store and edit Floyd script?
"Floyd speak"
??? Can JSON be used to serialize all floyd data structures? De-duplication?
??? Call stack and local variables and heap are also just state, just like all other values.

# Implementation

	struct stack_frame {
		vector<value_t> _locals;
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

# Map

A map _can_ contain null values.






# Floyd Externals - how to affect the world outside the simulation

Make high-level replacements for externals. Examples:
	socket,
	file,
	file tree,
	database,
	window system output,
	window system user input,
	system events etc.

Custom widget in window system:
HIView
	Mouse input
	OnDraw()

Terminal tool:
main()
	fread
	write

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

