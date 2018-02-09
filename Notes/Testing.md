



You always run them, there is no way to skip them, except as a special optimzation-mode.
The show visibly they run and complete. On success, just print 37 test succeeded. On failure, dump a lot.

	Running test 3 of 686...
	All 686 test succeeded, warning: 3 tests disabled.
	Failure test 118 of 686
		file hello.floyd, line 1201
		Expected:
		Result:

A unit = one function.

A unit test proves that one possible scenario gives the expected result.
Prove that function X, in scenario xyz, result is 123.
A unit tests is to prove the function works, not necessarily any other code that is called during test.

The order of tests is critical. Prove the basics works, after this you prove level 1 functions work etc. Build on that. NEVER check the same thing twice.

Tests without clear policies grows into a jungle is a burden.

Each test has ONE assert.

Test code must be extremely simple and stupid. Do no use fancy framework that let's you do setups or other things. A unit test should be 100% standalone, not depend on any other code running before or after it.

There is no point in running and failing lots of tests for level 2 and 3 code when the level 0 has failing tests.

On test failure, halt all tests.

While working on getting a test to succeed you want quick and easy iterations and leave breakpoints breakpoints etc: because of this, there is a SOLO feature that runs ONLY that one test.

DISABLE: you can disable test. This results in warnings during test running.

Test are also excellent examples of the code and documentation of the function's contract.


/*
# split_actor_name
- Handles Lukes & Han Solos
- Only used during setup of game, after that we always use sha1s
*/
split_actor_name(), example 1: split_actor_name("World 3/actor Luke") == "Luke", When there is only one actor, always use that as-is.
split_actor_name(), example 2: split_actor_name("World 3/") == "", When there is no actor name, result is ""
split_actor_name(), example 3: split_actor_name("World 3") == ""

string split_actor_name(string actor_name){
	...
}



FIXTURES


string my_func(game_world w, string label){
}


example: 



exception

example vs proof
fixture








# OLD 2




Base64



========	markdown



========	examples values

white = pixel_t(255, 255, 255);
gray50 = pixel_t(128, 128, 128);
black = pixel_t(0, 0, 0);
red = pixel_t(255, 0, 0);

empty = image_t(0, 0, []);
gradient = make_gradient(640, 480);

//	Uses default JSON representation of image_t
dog = unpack_json(
	image_t,
	{
		"width": 4, "height": 3, "pixels": [
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0}
		]
	}
)

cat = unpack_json(unpack_base64(
	"UmVwdWJsaWthbmVybmFzIGxlZGFyZSBpIHNlbmF0ZW4sIE1pdGNoIE1jQ29ubmVsbCwgdmlsbGUg
	dGlkaWdhcmVs5GdnYSBvbXL2c3RuaW5nZW4gdGlsbCBzZW50IHDlIHP2bmRhZ3NrduRsbGVuLCBt
	ZW4gZGV0IHNhdHRlIGRlbW9rcmF0aXNrZSBtaW5vcml0ZXRzbGVkYXJlbiBDaHVjayBTY2h1bWVy
	IHN0b3BwIGb2ci4gU2NodW1lciBz5GdlciBhdHQgdHJvdHMgZvZyaGFuZGxpbmdhcm5hIJRoYXIg
	dmkg5G5udSBhdHQgbuUgZW4g9nZlcmVuc2tvbW1lbHNlIHDlIGVuIHbkZyBzb20g5HIgYWNjZXB0
	YWJlbCBm9nIgYuVkYSBzaWRvcpQuIJRIYW4ga2FsbGFyIGzkZ2V0IGb2ciBUcnVtcCBzaHV0ZG93
	bpQgb2NoIFZpdGEgaHVzZXQga2FsbGFyIGRldCCUU2NodW1lciBzaHV0ZG93bpQuDQoNCg=="
))

========	scenarios

{
	function_under_test: make_filled()
	scenario: Make even-width, even-height image
	expected: Resulting image has exact dimensions
	expression: make_filled(4, 2, pixel_t(255,255,0))
	correct_result: image_t(4, 4, [pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0)])
},
{
	function_under_test: make_filled()
	scenario: Make even-width, even-height image
	expression: make_filled(4, 2, pixel_t(255,255,0))
	expected: image_t(4, 4, [pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0), pixel_t(255,255,0)])
}

========	implementation

struct pixel_t {
	int red;
	int green;
	int blue;
};

struct image_t {
	int width;
	int height;
	[pixel_t] pixels;
};


pixel_t make_filled(int width, int height, pixel_t fill){
	...
}