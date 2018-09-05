# COMMENTS AND DOCUMENTATION

// and /* */ are used for comments.

All comments before a statement are collapsed and becomes docs for that statement. First line of comment becomes the *summary*. Any additional lines of comments becomes the *details*.

A desc is a summary of *why* you want a feature. It should be one or a few sentences long. A desc can be a text field or an *array of fields* to allow for longer texts and still be stored OK as JSON.



# STRUCT INVARIANT
TBD

Invariants are defined per data member of a struct. They are used to verify input argument to automatic constructor (int start_pos, int end_pos). There is no way to *ever* create an instance of song_t that breaks the invariant. If you add special maker-functions, those can have other invariants but the song_t variants are always there.

All aggregate values will automatically have their invariants checked deeply.

```
struct song_t {
	// Where song starts, relative to the world-ppq
	!! start_pos >= 0 && start_pos <= end_pos
	int start_pos

	// Where ends starts, relative to the world-ppq,
	!! end_pos >= start_pos && end_pos <= max_ppq
	int end_pos

	// Any number of tracks are OK. No need to check the tracks themselves, they are guaranteed to have correct invariants.
	[track_t] tracks
}
```

	STATEMENT "struct" INDENTIFIER ((DOC||INVARIANT)* MEMBER)*


# FUNCTION CONTRACTS
TBD

A function specifies possible input arguments, ranges etc. These are validated at runtime and used for documentation.
A function also specifies the possible values of its result. You can define the results using the original inputs values.

```
// Delete the track from the song, as specified by track index
func
	// The new song but where the track track_index has been removed.
	song_t !! original.count_tracks() == result.count_tracks() - 1

	scale_song(
		// The original song. The track must exists.
		song_t original !! original.count_tracks() > 0

		// The track to delete, specified as a track index inside the original song.
		int track_index !! track_index >= 0 && track_index < original.count_tracks()
	){
		int temp = 3
		probe(temp, "Intermediate result", "key-1")
		return x
	}

??? Have return last instead?
```


# PRESENTER & PRESOS
TBD

??? presenter == convention for a function, or use special keyword? "preso" instead of "func"?

A presenter is a function that takes an input value and returns a value representing the value is graph nodes, for showing in tools. You can have several presenters for the same type of input value.

Example 1:
- file_presenter1 returns the name of the file
- file_presenter2 returns the name and path and meta info about the file
- file_presenter3 returns the name + snippet of the file contents as text etc.

It's important that a presenter thinks about the value in its entirety, even if it only returns very litte info.


Presenters are used extensively by the IDE and the runtime and debuggers etc to let you explore your system. They can also be used to quickly generate user interfaces or configuration files.

```
// One-line textual summary of song
// presenter = function that returns a preso and is used to show humans the value
presenter textual_summary(song_t value){
	return make_song_text_summary(true, true)
}

// Show song and contents as diagram, including tracks with notes.
presenter song_presentor_summary(song_t value){
	return make_dot_diagram(_.tracks.count)
}

```

##### preso

This is a value that lists the contents of a value of a specific type.


This is a concerete type, designed to hold a string, a list of strings, a tree of nodes etc. Like a high-level SVG but without layout information, only the data.

??? what type is the presentation? JSON-value?

	STATEMENT "presenter" INDENTIFIER FUNCTION_ARGUMENTS FUNCTION_BODY


# DEMO
TBD

This is an object that can be used to setup an interactive demonstration of a function. It has desc, can define some sliders and settings, defines expressions to execute. Can recommend what type of information in preso:s to show and how. Show a time-axis and use a slider to zoom?

```
demo demo1 {
	init {
		"scenario": "Scale different tracks"

		"parameters": [
			{ "ui": "slider", "min": 0, "max": 100, "title": "Number of tracks", "id": "count" },
			{ "ui": "slider", "min": 0, "max": 100, "title": "Track to scale", "id": "track_index" }
		]
	}

	code {
		test_song = make_test_song(count)
		test_song = scale_song(test_song, track_index)
		(test_song, track_index) => _
	}
}
```

	STATEMENT "demo" IDENTIFIER DEMO_BODY
	DEMO_BODY "init" JSON_VALUE "code" FUNCTION_BODY


# PROOF
TBD

When you write a function you also supply a number of proofs that shows that the function works as advertised. These are unit tests and will always run at startup and other times.

```
// Delete the only track in song => empty song
proof scale_song (song1track, 0) => empty0

// Delete one of 2 tracks => 1-track song
proof scale_song (song2track, 0) => song1track
```

	STATEMENT "proof" INDENTIFIER ARGUMENT_LIST "=>" EXPRESSION


# EXAMPLE_VALUE
TBD

These are used to define good example values for a struct to be used for demoing, making tests for this struct, functions working on the struct values and for all clients.

```
	example_value one_track_song = song_t(1)

	// An empty song with no tracks. Range is zero too.
	example_value empty0 = song_t(expression make_def_song(0, 0))
	
	// A one-track song. Range is zero.
	example_value one_track_song = song_t(expression make_def_song(0, 0))
```

	STATEMENT "example_value" INDENTIFIER "=" EXPRESSION


Example values are automatically available in other modules.


# MEASUREMENT_RIG
TBD

This is a specification how to setup visualisation tools to observe proves over time. Think about it as setting up all those ekgs and paper plots in the hospital that helps doctors figure out what is going on. A rig can be connected directly to specific probes.

Idea: allow you to make generic rigs.
??? a rig is very similair to an actor: consumes data and has its own state.



