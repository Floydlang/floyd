	### IDEA: Use JSON as outer wrapper for source file. Entries can be code, markdown or whatever. Roundtrip. Hmm. 





# This module provides advance collision detection between 2D objects.

# pixel_t
Describes an integer RGB pixel, where each component is *0 - 255*. Features are
- Non-lossy
- Easy to use to do math.

	struct pixel_t {
		int red;
		int green;
		int blue;
	}

	struct image_t {
		[pixel_t] pixels;
		int width;
		int height;
	}

	[pixel_t] make_image(int width, int height){
		p = pixel_t(width, height);
		return p;
	}

	proof {
		"Make sure we can make empty images"
		assert()
	}



# COMPACT code

		struct pixel_t
		int red
		int green
		int blue
		
		struct image_t
		[pixel_t] pixels;
		int width;
		int height;
		



# CODE META DATA

- Probes
- (Coverage)
- Performance O(3)
- Tweaks


.DOT graphs

Generate excel-data
Generate JSON values

# EXAMPLE PROGRAM 1

		delete_track()
		Delete the track from the song, as specified by track index.
		
		
		- Notice that the track cannot be the edit-track.
		- The track will be removed, and so will all other state that is related to the track. If only this track references a clip, then the clip will be deleted too.
		??? Support markdown in docs?
		
		song r: r != nil && r.count_tracks() == original.count_tracks()
		song original: original != nil && original.track_to_index(original.edit_track) != track_index
		int track_index: _ >= 0 && _ < original.count_tracks()
		
		{
			mut temp = copy(original);
			temp.length = temp.length / 2.0f;
			@p1 = temp.length;
			temp.last_pos = temp.last_pos / 2.0f;
			temp.internal_play_pos = internal_play_pos / 2.0f;
		}
		
		"examples": [
		]
		
		”proofs": [
			{
				"scenario": "Search empty comtainer => nothing found",	"inputs": [ example_song1, -1 ],	"expected_result": {
				}
			},
			{
				"scenario": "Search 1-entry container => correct entry found",	"inputs": [ example_song1, -1 ],	"expected_result": {
				}
			},
			{
				"scenario": "Search empty comtainer => nothing found",	"inputs": [ example_song1, -1 ],	"expected_result": {
				}
			},
		]
