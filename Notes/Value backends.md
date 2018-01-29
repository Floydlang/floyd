Value backends
How values are stored in RAM has a major impact on performance: RAM and cache usage, CPU clock cycles.

Floyd decides how to store each value to get best performance. Where "performance" is an APP-level quality, not always CPU clocks. NOTICE: Floyd can have *different* packings for the same datatype. Hot-path can have a different packing than cold path and STILL share the same functions.

- How compact is value in RAM? Caches and memory bandwidth?
- How fast is it to access the parts of the value? CPU (indirection cache patterns)
- How fast is it to MODIFY part of the value? Must it be copied in entirety?


1) Normal mode: struct and vectors as in source code. Padding.
2) JSON format.
3) vector-flattened: vector<T> is stored as { vector<T.1> vector><T.2> } where .1 and .2 are members.
4) Max-pack: flattens and encodes as packed binary.
5) Interning: replace value with a 32-bit ID and define constant value that is shared.
6) ZIP: performs max-pack, then ZIP.
7)

- Floyd can decide to keep a value in several different representations at the same time. Ex: a large value (game's 3D-level) is stored as JSON, as highly optimized for rendering and for collision detection. It's the same value, but different code wants different representation for max speed.

- Read-caching values. If a value is slow to read, store a representation that is fast for your purpose.
- Write caching values: a temporary write-buffer is stored in parallell with the original value. Reads looks in write-buffer first, then original value. This makes value bigger and slower, but fast to modify.


- cache line primitive

- callstack vs closure vs RC



struct pixel_t {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
}
??? Allow you to manually code alternative representations?

representation<pixel_t>
	struct pixel_t:word {
		uint32_t pixel_data;
	}
	pixel_data = (value.red << 24) | (value.green << 16) | (value.blue << 8) | (value.alpha << 0)
	pixel_t = pixel_t(red: (pixel_data && 0xff000000) >> 24, green: (pixel_data && 0x00ff0000) >> 16, blue: (pixel_data && 0xff000000) >> 24, red: (pixel_data && 0xff000000) >> 24
	pixel_data = (value.red << 24) | (value.green << 16) | (value.blue << 8) | (value.alpha << 0)
	

struct image_t {
	int width;
	int height;
	vector<pixel> pixels;
}


struct image_t: write-cache {
	struct area_t{
		int x;
		int y;
		image_t image;
	}

	vector<area_t> writes
	image_t original_value;
}


main(){
	image = load_jpeg()


}


------------------------------





??? make alternaitve RAM layouts: compactness, small values, modification.
??? Pack together with its parent aggregation. If this struct sits in parent struct -- merge them for optimization potential.



struct product_t {
	bool purchaseable;
	int<0,1000> price;
	string name;
	vector<string> colors;
}

a = product_t(
	true,
	29,
	"Pencil",
	["cool-black", "white", "invisible"]
);

blank = product_t(
	false,
	0,
	"",
	"",
	[]
);


a in JSON format (text):
{
	"purchaseable": true,
	"price": 29.0,
	"name": "Pencil",
	"colors": ["cool-black", "white", "invisible"]
}




vector<product_t>

	Simple:
		vector<product_t>

	Flattened:
		vector<bool> purchasable;
		vector<int<0,1000>> price;
		vector<string> name;
		vector<vector<string>> colors;


	Flattened-max-pack-elements32, 33 values
		int32 count = 33
		uint32[?] = {
			//	purchaseable__0_31, purchaseable__32-63
			0x12345678,
			0x00000003,

			//	[10bits] * 33 = 330 bits = 10 int32
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111,
			0x11111111

			//	0x00 = blank, 0x01 = 1 char, 0xff = 32bit length follows.
			0x06, P, E, N, C, I, L, 0x00, 0x00, 0x07, ......

			colors

		}



