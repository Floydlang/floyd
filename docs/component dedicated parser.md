

```

# FLOYD COMPONENT -- DEDICATED PARSER


STATEMENT "component" JSON_BODY
STATEMENT "struct" INDENTIFIER ((DOC||INVARIANT)* MEMBER)*
STATEMENT "example_value" INDENTIFIER "=" EXPRESSION
STATEMENT "presenter" INDENTIFIER FUNCTION_ARGUMENTS FUNCTION_BODY
STATEMENT "proof" INDENTIFIER ARGUMENT_LIST "=>" EXPRESSION
STATEMENT "demo" IDENTIFIER DEMO_BODY
DEMO_BODY "init" JSON_VALUE "code" FUNCTION_BODY



// component = keyword, contents is a JSON
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
	!! start_pos >= 0 && start_pos <= end_pos
	int start_pos

	// Where ends starts, relative to the world-ppq,
	!! end_pos >= start_pos && end_pos <= max_ppq
	int end_pos

	// Any number of tracks are OK. No need to check the tracks themselves, they are guaranteed to have correct invariants.
	[track_t] tracks
}


// An empty song with no tracks. Range is zero too.
example_value empty0 = song_t(expression make_def_song(0, 0))

// A one-track song. Range is zero.
example_value one_track_song = song_t(expression make_def_song(0, 0))


// One-line textual summary of song
// presenter = function that returns a preso and is used to show humans the value
presenter textual_summary(song_t value){
	return make_song_text_summary(true, true)
}


example_value big_song = make_song(12345)
example_value zigzag_song = scale_song(make_song(1235), 2.0)


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

// Delete the only track in song => empty song
proof scale_song (song1track, 0) => empty0

// Delete one of 2 tracks => 1-track song
proof scale_song (song2track, 0) => song1track


// Show song and contents as diagram, including tracks with notes.
presenter song_presentor_summary(song_t value){
	return make_dot_diagram(_.tracks.count)
}

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