
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
