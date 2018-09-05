
	// and /* */ are used for comments. All comments before a statement are collapsed and becomes docs for that statement.



```
	
# FLOYD COMPONENT -- DEDICATED PARSER

// component = keyword, contents is a JSON
component {
	"version": [ 1, 0 ],
	"dependencies": [],
	"desc": [
		"The Song Component is basic building block to ",
		"creating a music player / recorder"
	]
}


// A song has a beginning and an end and a number of tracks of music
struct song_t {

	// Where song starts, relative to the world-ppq
	!! start_pos >= 0 && start_pos <= end_pos
	int start_pos

	// Where ends starts, relative to the world-ppq,
	!! invariant end_pos >= start_pos && end_pos <= max_ppq
	int end_pos
}

// Invariants are defined per data member. They are used to verify input argument to automatic constructor (int start_pos, int end_pos). There is no way to *ever* create an instance of song_t that breaks the invariant. If you add special maker-functions, those can have other invariants but the song_t variants are always there.


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
	!! original.count_tracks() == result.count_tracks() - 1
	type song

	scale_song(
		song original

		// The original song. The track must exists.
		contract original.count_tracks() > 0,
		int track_index
		// The track to delete, specified as a track index inside the original song.
		contract track_index >= 0 && track_index < original.count_tracks()
	)
	{
		int temp = 3
		probe(temp, "Intermediate result", "key-1")
		return x
	}	


???
proof(scale_song) 
	Delete only track in song => empty song
	(song1track, 0) => empty0

proof(scale_song) 
	Delete one of 2 tracks => 1-track song
	(song2track, 0) => song1track



func preso presenter__song_presentor_summary(song_t value){
	return make_dot_diagram(_.tracks.count)
}
	// Show song and contents as diagram



demo demo1 {
	"scenario": "Scale different tracks"

	count = slider(0, 20), "Number of tracks"
	track_index = slider(0, 20), "Track to scale"

	test_song = make_test_song(count)
	test_song = scale_song(test_song, track_index)
	(test_song, track_index) => _
}

```