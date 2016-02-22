#GOAL: NOT MATH! MORE LIKE ELECTRICIAN!


#SNIPPET (internal in Floyd runtime)
Snippet = short linjear-memory array of values in a single memory block. Immutable, RC, shareable between any types and instances of collections. Size depends on hw cache size AND inline size of value. Cache friendly!

GOAL: Allow sharing of state between mutations of data type or collection. Speed and memory. Allow sharing them between completely different types of collections at runtime. No need to copy values.


# Instances of the same data type (or collection of composte data types) can have different memory layout / representation behind the scenes.
If the object is used as a member of composite type or collection, those may be physically merged together into one, bigger layout.

An instance of a vector of GameObjects may internally be stored as
			vector<GameObject>
			OR
				vector_game_object {
					vector<bool> enabled
					vector<Vector3> positions
					vector<GameObjectRest> rest
				}

Compiler generates all different structs needed at rumtime.
OR memory layout is completely handled at runtime and using std::vector<uint8> only.

Collections may sometimes be inline expanded into parent composite.

Research: F#

# Arrays
		Arrays = 32-item snippets + array listing those.
		pvector = the exact same 32-item snippets, stored in a ptree
		Avoid copying snippets.

			Linear vector-backend:
				One large array refering to 32-item snippets.
			Pvector array: ptree referes to 32-item snippets

			32-item append cache

#Data type examples
When defining a data type (composite) you need to list 4 example instances. Can use functions to build themselves. These are used in example docs, example code and for unit testing this data types and *other* data types. You cannot change examples without breaking client tests higher up physical dependncy graph.

# You list priorities on clocks in order:
	low-latency results
	keep in hw cache
	minimize memory usage
	fast appending
	fast random writes
	fast linear reads
	fast composite mutations

#Value types
Value:
	- one value
	- collection of values
	- function returning a value

	- optional additional read cache
	- optional additional write cache


	- format can be

Sometimes normalized state isn't kept at all, only optimized states.


#OLD
MUTATOR void OnClick(){

		let song2 = DeleteSelected(song1);
		let updateRects = CalcUpdateRects(song1, song2, clipRect)
}


APPROACH: Make lib that lets you write core things functionally and use them for threading etc. UI-part of program is written old-style. Wait with hardware boards, threading model etc.



# EXAMPLE STACK
MY GAME
	User input
		{User input event with timestamp}

		CLOCK: advances everytime an event is insert into the program. All events are stored in queue. Queue is polled by game simulation.

	Screen output
		{Screen object + game simulation state}
		Game simulation advances game state to the next step, using previous game state and user input queue. All simulation steps are stored.
		Renderer1 samples latest game state and creates a diarama-object with information needed to render the view.
		Renderer2 reads latest diarama-object and pushes it to OpenGL.

	Audio output
		{Audio buffer + audio simulation state}
		When audio card requests a new audio buffer, the audio engine samples the speakers in the latest game state, makes those speakers generate 3D audio buffers (with culling). Mixer reads all imortant speakers and does mixing and effects before storing in audio buffer. All audio buffers and simulation states are stored.
		
	(Network io)

	
	
# CONCURRENCY / THREADING
- External clocks - these are clocks for different clients that communicate at different rates (think UI vs realtime output vs sockets). Programmer uses clocks-concept to do this.

- Internal threading: runtime can chose to use many threads to accellerate the processing. This cannot be observed in any way by the user or programmer, except program is faster. Programmer controls this using tweakers. Threading problems eliminated.

