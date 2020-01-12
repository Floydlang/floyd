//
//  doc_examples.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-12.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"

using namespace floyd;



//######################################################################################################################
//	QUICK REFERENCE SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################




//#define QUARK_TEST QUARK_TEST_VIP

QUARK_TEST("QUICK REFERENCE SNIPPETS", "TERNARY OPERATOR", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

//	Snippets setup
let b = ""



let a = b == "true" ? true : false

	)");
}

QUARK_TEST("QUICK REFERENCE SNIPPETS", "COMMENTS", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

/* Comment can span lines. */

let a = 10; // To end of line

	)");
}



QUARK_TEST("QUICK REFERENCE SNIPPETS", "LOCALS", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = 10
mutable b = 10
b = 11

	)");
}


#if 0
QUARK_TEST("QUICK REFERENCE SNIPPETS", "WHILE", "", ""){
	//	Just make sure it compiles, don't run it!
	ut_run_closed(R"(

//	Snippets setup
let expression = true


while(expression){
	print("body")
}

	)",
	compilation_unit_mode::k_no_core_lib
	);
}
#endif


QUARK_TEST("QUICK REFERENCE SNIPPETS", "FOR", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

for (index in 1 ... 5) {
	print(index)
}
for (index in 1  ..< 5) {
	print(index)
}

	)");
}


QUARK_TEST("QUICK REFERENCE SNIPPETS", "IF ELSE", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

//	Snippets setup
let a = 1000



if(a > 10){
	print("bigger")
}
else if(a > 0){
	print("small positive")
}
else{
	print("zero or negative")
}

	)");
}


QUARK_TEST("QUICK REFERENCE SNIPPETS", "BOOL", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = true
if(a){
}

	)");
}

QUARK_TEST("QUICK REFERENCE SNIPPETS", "STRING", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let s1 = "Hello, world!"
let s2 = "Title: " + s1
assert(s2 == "Title: Hello, world!")
assert(s1[0] == 72) // ASCII for 'H'
assert(size(s1) == 13)
assert(subset(s1, 1, 4) == "ell")
let s4 = to_string(12003)

	)");
}




QUARK_TEST("QUICK REFERENCE SNIPPETS", "FUNCTION", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

func string f(double a, string s){
	return to_string(a) + ":" + s
}

let a = f(3.14, "km")

	)");
}

QUARK_TEST("QUICK REFERENCE SNIPPETS", "IMPURE FUNCTION", "", ""){
	ut_verify_mainfunc_return_nolib(
	QUARK_POS,
	R"(

func int main([string] args) impure {
	return 1
}

	)", {}, 1);
}



QUARK_TEST("QUICK REFERENCE SNIPPETS", "STRUCT", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

struct rect {
	double width
	double height
}

let a = rect(0.0, 3.0)
assert(a.width == 0.0
	&& a.height == 3.0)

assert(a == a)
assert(a == rect(0.0, 3.0))
assert(a != rect(1.0, 1.0))
assert(a < rect(0.0, 4.0))

let b = update(a, width, 100.0)
assert(a.width == 0.0)
assert(b.width == 100.0)

	)");
}


QUARK_TEST("QUICK REFERENCE SNIPPETS", "VECTOR", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = [ 1, 2, 3 ]
assert(size(a) == 3)
assert(a[0] == 1)

let b = update(a, 1, 6)
assert(a == [ 1, 2, 3 ])
assert(b == [ 1, 6, 3 ])

let c = a + b
let d = push_back(a, 7)

let g = subset([ 4, 5, 6, 7, 8 ], 2, 4)
assert(g == [ 6, 7 ])

let h = replace([ 1, 2, 3, 4, 5 ], 1, 4, [ 8, 9 ])
assert(h == [ 1, 8, 9, 5 ])

for(i in 0 ..< size(a)){
	print(a[i])
}

	)");
}


QUARK_TEST("QUICK REFERENCE SNIPPETS", "DICTIONARY", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let a = { "one": 1, "two": 2 }
assert(a["one"] == 1)

let b = update(a, "one", 10)
assert(a == { "one": 1, "two": 2 })
assert(b == { "one": 10, "two": 2 })

	)");
}


QUARK_TEST("QUICK REFERENCE SNIPPETS", "json", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

let json a = {
	"one": 1,
	"three": "three",
	"five": { "A": 10, "B": 20 }
}

	)");
}




QUARK_TEST("QUICK REFERENCE SNIPPETS", "MAP", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		func int f(int v, int c){
			return 1000 + v
		}
		let r = map([ 10, 11, 12 ], f, 0)
		assert(r == [ 1010, 1011, 1012 ])

	)");
}




//######################################################################################################################
//	MANUAL SNIPPETS -- VERIFY THEY WORK
//######################################################################################################################




QUARK_TEST("MANUAL SNIPPETS", "subset()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		assert(subset("hello", 2, 4) == "ll")
		assert(subset([ 10, 20, 30, 40 ], 1, 3 ) == [ 20, 30 ])

	)");
}





#if 0
Crashes at runtime when k_max_samples_count = 100000
QUARK_TEST("Floyd test suite", "", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"___(

				mutable [benchmark_result_t] results = []
//				let instances = [ 0, 1, 2, 3, 4, 10, 20, 100, 1000 ]
				let instances = [ 0, 1, 2, 3, 4, 10, 20 ]
//				let instances = [ 20 ]
//				let instances = [ 10, 20 ]
				for(instance in 0 ..< size(instances)){
					let count = instances[instance]
					let [int] start = []

					let r = benchmark {
						//	The work to measure
						for(a in 0 ..< count){
							let b = push_back(start, 0)
						}
					}
					results = push_back(results, benchmark_result_t(r, json( { "Count": count, "Per Element": count > 0 ? r / count : -1 } )))
				}

		)___"
	);
}
#endif

#if 0
QUARK_TEST("Floyd test suite", "dict hamt performance()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"___(

			benchmark-def "dict hamt push_back()" {
				mutable [benchmark_result_t] results = []
				let instances = [ 0, 1, 2, 3, 4, 10, 20, 100, 1000 ]
				for(instance in 0 ..< size(instances)){
					let count = instances[instance]
					let [int] start = []

					let r = benchmark {
						//	The work to measure
						for(a in 0 ..< count){
							let b = push_back(start, 0)
						}
					}
					results = push_back(results, benchmark_result_t(r, json( { "Count": count, "Per Element": count > 0 ? r / count : -1 } )))
				}
				return results
			}

			print(make_benchmark_report(run_benchmarks(get_benchmarks())))

		)___"
	);
}
#endif










/*
QUARK_TEST("Floyd test suite", "OPTIMZATION SETTINGS" "Fibonacci 10", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"(

			func int fibonacci(int n) {
				if (n <= 1){
					return n
				}
				return fibonacci(n - 2) + fibonacci(n - 1)
			}

			for (i in 0...10) {
				print(fibonacci(i))
			}

		)"
	);
}
*/







//######################################################################################################################
//	DEMOS
//######################################################################################################################






FLOYD_LANG_PROOF("", "Demo DEEP BY VALUE", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(


			struct vec2d_t { double x; double y }
			func vec2d_t make_zero_vec2d(){ return vec2d_t(0.0, 0.0) }

			struct game_object_t {
				vec2d_t pos
				vec2d_t velocity
				string key
				[game_object_t] children
			}

			let a = game_object_t(make_zero_vec2d(), vec2d_t(0.0, 1.0), "player", [])
			let b = game_object_t(make_zero_vec2d(), make_zero_vec2d(), "world", [ a ])

			assert(a != b)
			assert(a > b)

			let [game_object_t] c = [ a, b ]
			let [game_object_t] d = push_back(c, a)
			assert(c != d)


		)___"
	);
}

FLOYD_LANG_PROOF("", "Demo processes", "", ""){
	ut_run_closed_nolib(
		QUARK_POS,
		R"___(


			container-def {
				"name": "iphone app",
				"tech": "Swift, iOS, Xcode, Open GL",
				"desc": "Mobile shooter game for iOS.",
				"clocks": {
					"main_clock": {
						"gui": "my_gui"
					},
					"audio_clock": {
						"audio": "my_audio"
					}
				}
			}

			struct doc_t { [double] audio }

			struct gui_message_t { string type }

			struct audio_engine_t { int audio ; doc_t doc }
			struct audio_message_t { string type ; doc_t doc }

			func audio_engine_t my_audio__init() impure {
				return audio_engine_t(0, doc_t([]))
			}

			func audio_engine_t my_audio__msg(audio_engine_t state, audio_message_t m) impure {
				if(m.type == "start_note"){
					return update(state, audio, state.audio + 1)
				}
				else if(m.type == "buffer_switch"){
					return update(state, audio, state.audio + 1)
				}
				else if(m.type == "exit"){
					exit()
				}
				else{
					assert(false)
				}
				return state
			}


			struct gui_t { int count ; doc_t doc }

			func gui_t my_gui__init() impure {
				send("gui", gui_message_t("key_press"))
				send("gui", gui_message_t("key_press"))
				send("gui", gui_message_t("quit"))
				return gui_t(1000, doc_t([ 0.0, 1.0, 2.0 ]))
			}

			func gui_t my_gui__msg(gui_t state, gui_message_t m) impure {
				if(m.type == "key_press"){
					send("audio", audio_message_t("start_note", state.doc))
					return update(state, count, state.count + 1)
				}
				else if(m.type == "quit"){
					send("audio", audio_message_t("exit", state.doc))
					exit()
				}
				else{
					assert(false)
				}
				return state
			}


		)___"
	);
}
