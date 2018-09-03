


# FLOYD COMPONENT .fecomp and .fpcomp -- JSON ENCODING


```
{
	"component": {
		"version": [ 1, 0 ],
		"dependencies": [],
		"desc": [
			"The Song Component is basic building block to ",
			"creating a music player / recorder"
		]
	},

	//	Ordered list of global things: constants, function definitions, struct definitions and enum definitions etc.
	"nodes": [
		{
			"type": "struct",	"name": "song_t",
			"def": {
				"desc": "A song has a beginning and an end and a number of tracks of music",
				"members": [
					{
						"type": "int",
						"name": "start_pos",
						"desc": "Where song starts, relative to the world-ppq",
						"invariant": "start_pos >= 0 && start_pos <= end_pos"
					},
					{
						"type": "int",
						"name": "end_pos",
						"desc": "Where ends starts, relative to the world-ppq",
						"invariant": "end_pos >= start_pos && end_pos <= max_ppq"
					}
				],
				"invariants": [
					"start_pos >= 0 && start_pos <= end_pos",
					"start_pos >= 0 && start_pos <= end_pos"
				]
			},
			"example_values": [
				{
					"name": "empty0",
					"desc": "An empty song with no tracks. Range is zero too.",
					"expression": "make_def_song(0, 0)"
				},
				{
					"name": "one_track_song",
					"desc": "A one-track song. Range is zero.",
					"expression": "make_def_song(0, 0)"
				}
			],
			"presenters": [
				{
					"name": "Textal summary",
					"expression": "preso make_song_text_summary(true, true)"
				}
			]
		},
		{
			"type": "example_value",
			"name": "big_song",
			"expression": "make_song(12345)"
		},
		{
			"type": "example_value",
			"name": "zigzag_song",
			"expression": "make_song(12345)"
		},

		{
			"type": "function",
			"name": "scale_song",
			"def": {
				"desc": "Delete the track from the song, as specified by track index",
				"inputs": [
					{
						"type": "song",
						"name": "original",
						"desc": "The original song. The track must exists.",
						"contract": "original.count_tracks() > 0"
					},
					{
						"type": "int",
						"name": "track_index",
						"desc": "The track to delete, specified as a track index inside the original song.",
						"contract": "track_index >= 0 && track_index < original.count_tracks()"
					}
				],
				"result": {
					"type": "song",
					"desc": "The new song, where the track track_index has been removed.",
					"contract": "original.count_tracks() == result.count_tracks() - 1"
				},
				"implementation": [
					"song scale_song(song original, int track_index) {",
					"probe(original, \"The new song, where the track track_index has been removed.\", \"key2\")",
					"\tint temp = 3",
					"}"
				],
				"proofs": [
					{
						"scenario": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					},
					{
						"scenario": "delete one of 2 tracks",
						"expected": "1-track song",
						"result": "song1track",
						"inputs": [
							"song2track",
							0
						]
					}
				],
				"demos": [
					{
						"name": "delete only track in song",
						"expected": "empty song",
						"result": "empty0",
						"inputs": [
							"song1track",
							0
						]
					}
				]
			}
		},
		{
			"type": "presenter",
			"name": "song_presentor_summary",
			"expression": "make_dot_diagram(_.tracks.count)"
		}
	]
}
```



```
	
# FLOYD COMPONENT -- DEDICATED PARSER

	
struct song_t {
	desc "A song has a beginning and an end and a number of tracks of music"

	desc "Where song starts, relative to the world-ppq"
	invariant start_pos >= 0 && start_pos <= end_pos
	int start_pos
	
	desc "Where ends starts, relative to the world-ppq",
	invariant end_pos >= start_pos && end_pos <= max_ppq
	int end_pos
	
	invariant: [
		start_pos >= 0 && start_pos <= end_pos
		end_pos >= start_pos && end_pos <= max_ppq
	]
}
	
example_value empty0 {
	desc "An empty song with no tracks. Range is zero too."
	expression make_def_song(0, 0)
}
	
example_value one_track_song {
	desc "A one-track song. Range is zero."
	expression make_def_song(0, 0)
}
	
func preso presenter__textual_summary(song_t value){
	return make_song_text_summary(true, true)
}
}
	
example_value big_song {
	expression make_song(12345)
}
	
example_value zigzag_song {
	expression make_song(12345)
}
	



desc "Delete the track from the song, as specified by track index"
func scale_song {
	result {
		type song
		desc "The new song, where the track track_index has been removed."
		contract: original.count_tracks() == result.count_tracks() - 1
	},
	inputs [
		{
			song original
			desc "The original song. The track must exists."
			contract original.count_tracks() > 0
		},
		{
			int track_index
			desc "The track to delete, specified as a track index inside the original song."
			contract track_index >= 0 && track_index < original.count_tracks()
		}
	],
	
	int temp = 3
	probe(temp, "Intermediate result", "key-1")
	
	
	proofs [
		{
			"Delete only track in song" => "empty song"
			(song1track, 0) => empty0
		},
		{
			"Delete one of 2 tracks" => "1-track song"
			(song2track, 0) => song1track
		}
	]
},


preso presenter__song_presentor_summary {
	"expression": "make_dot_diagram(_.tracks.count)"
}

demos [
	{
		"scenario": "Scale different tracks"

		count = slider(0, 20), "Number of tracks"
		track_index = slider(0, 20), "Track to scale"

		test_song = make_test_song(count)
		test_song = scale_song(test_song, track_index)
		(test_song, track_index) => _
	}
]

```