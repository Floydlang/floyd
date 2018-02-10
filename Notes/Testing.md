
# FLOYD TEST PRINCIPLES

Tests without clear policies grows into a jungle is a burden. Test code must be extremely simple and stupid. Do no use fancy framework that let's you do setups or other things. A unit test should be 100% standalone, not depend on any other code running before or after it. There is no point in running and failing lots of tests for level 2 and 3 code when the level 0 has failing tests.

1. One test is used to prove one aspect of a specific function works as promised, the function's contract.
- Use one test for each aspect.
- The order of tests is critical. Prove the basics features works, after this you prove level 1 functions work etc. Build on that. your test can use other functions, but their test must have passed first.
- NEVER check the same thing twice.
- On test failure, halt all tests.
- All tests are always run. Must take < 3 seconds. Only by setting special optimzation-mode are tests disabled.
- Test serve as examples on how to use function.
- Tests are written with the tested function and lives next to it in source file, not some other place.
- Each test can be run on its own, not requiring the world to be in any special state.


# FUNCTION TESTS

You list your tests and examples above the function definition in your source file.

Tag examplex with "ex1", "ex2" etc. Tests you tag with "test1" etc.

Examples and tests will all be run together in order specified in the source file. Examples will also be put in documentation when generating that from source, while tests are purely technical edge cases and too much noise for docs.

You can provide a human-text that tells what this test proves.

# FUNCTION CONTRACTS

You state any input requirements your function has. Breaking the contract is a defect of the caller, never a runtime error.

You also verify the output of your function. While doing this you can both access the function arguments and the result of the function.


# EXAMPLE 1

	/*
		- Handles Lukes & Han Solos
		- Only used during setup of game, after that we always use sha1s
	*/
	tests = [
		ex1: to_hex_color(color(255, 255, 255)) == "#ffffff", "White color works, 255 is max value"
		ex1: to_hex_color(color(0, 0, 0)) == "#000000", "Black works"
		ex1: to_hex_color(color(1, 2, 3)) == "#010203", "Color coefs in right order"
	]
	pre = [
	]
	post = [
		$0.size == 7
		$0[0] == "#"
	]
	string to_hex_color(color c){
		...
	}


# EXAMPLE VALUES

When defining a data type (composite) you need to list 4 example values. Can use functions to build them or just fill-in manually or a mix. These are used in example docs, example code and for unit testing this data types and *other* data types. You cannot change examples without breaking client tests higher up physical dependency graph. Part of contract. sha1.

	
	white = pixel_t(255, 255, 255);
	gray50 = pixel_t(128, 128, 128);
	black = pixel_t(0, 0, 0);
	red = pixel_t(255, 0, 0);
	
	empty = image_t(0, 0, []);
	gradient = make_gradient(640, 480);

A common thing is to copy a Floyd value from a log, as JSON, and paste it into the code to create a new example value. Both good to make tests, but perfect to reproduce problems in the wild, make tests for the value, then keep it as a regression test.



# EXAMPLE 2

	struct game_object {
		point pos;
		gotype type;
		gomode mode;
	}
	game_object make_hero_tank()
	game_object make_burning_hero_tank()
	game_object make_simple_enemy()


	struct game_world {
		[game_object] gos
		bool pause
		int ticks
	}
	game_world make_2_vehicle_world()
	game_world make_empty_world()
	game_world make_exploding_ship_world()
	game_world make_paused_world_2_vehicles()

	game_world make_scenario_game_world(){
		return unpack_game_world(
			{"gos":[{"orientation":"portrait","idiom":"iphone","extent":"full-screen","minimum-system-version":"7.0","scale":"2x"},{"orientation":"portrait","idiom":"iphone","subtype":"retina4","extent":"full-screen","minimum-system-version":"7.0","scale":"2x"}]}
		)
	}

	SOLO ex1: { fire_laser(make_paused_world_2_vehicles(), make_hero_tank(), 0), diff(w, $0.0) }
	ex2: { fire_laser(make_paused_world_2_vehicles(), make_hero_tank) }
	test1: { fire_laser("World 3") == "" }
	DISABLE test2: { fire_laser("World") == "" }
	test3: { fire_laser() => throws }

	pre {
		w.pause == false
		go.type == gotype::vehicle
		go.mode != pause
		energy >= 0.0
	}
	post {
		$0.log.count > 0
		if(energy == 0){
			$0 == w
		}
	}

	(game_world, log) fire_laser(game_world w, game_object go, float energy){
		...
	}


# TEST RUNNER

Tests show visibly that they run and complete. On success, just print 37 test succeeded. On failure, dump a lot. Callstack, stop in debugger etc.

	Running test 3 of 686...

	All 686 test succeeded, warning: 3 tests disabled.

	Failure test 118 of 686
		file hello.floyd, line 1201
		Expected:
			123
		Result:
			124
	...


# SOLO & DISABLE

SOLO: While working on getting a test to succeed you want quick and easy iterations and leave breakpoints breakpoints etc: because of this, there is a SOLO feature that runs ONLY that one test.

DISABLE: you can disable tests. This results in warnings during test running.


# FIXTURES, SUITES, TEST CASES, SCENARIOS
There are no special features for this in Floyd. If a test needs to do work in preparation, make that a separate normal function.

You usually useexample values provided by a type to setup your tests.




# IDEA: FUNCTION CONTRACT

A function signature + its example becomes a dependable unit. Make sha1. Function implementor uses examples to prove the function works. It is also used to make sure it KEEPS working as promised. If not, the function needs a new name.

When client package imports a function, it keeps sha1 to tell it depends on that function. All other functions in importat package can change without affecting client.


# TESTING EXCEPTIONS

exception
