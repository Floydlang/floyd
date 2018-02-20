//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "interpretator_benchmark.h"

#include "floyd_interpreter.h"

#include <string>
#include <vector>
#include <iostream>

using std::vector;
using std::string;

using namespace floyd;

#if 1

//////////////////////////////////////////		HELPERS

interpreter_context_t make_test_context(){
	const auto t = quark::trace_context_t(false, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}


//	Returns time in milliseconds
std::int64_t measure_execution_time(std::function<void (void)> func){
	func();

	auto t0 = std::chrono::system_clock::now();
	func();
	auto t1 = std::chrono::system_clock::now();
	func();
	auto t2 = std::chrono::system_clock::now();
	func();
	auto t3 = std::chrono::system_clock::now();
	func();
	auto t4 = std::chrono::system_clock::now();
	func();
	auto t5 = std::chrono::system_clock::now();

	auto d0 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
	auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
	auto d3 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3);
	auto d4 = std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4);

	const auto average = (d0 + d1 + d2 + d3 + d4) / 5.0;
	auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(average);

	int64_t duration_ms = duration1.count();
	std::cout << "Execution time (ms): " << duration_ms << std::endl;
	return duration_ms;
}

QUARK_UNIT_TEST_VIP("", "measure_execution_time()", "", ""){
	const auto ms = measure_execution_time(
		[] { std::cout << "Hello, my Greek friends"; }
	);
	std::cout << "duration2..." << ms << std::endl;
}



//////////////////////////////////////////		TESTS



QUARK_UNIT_TEST_VIP("C++", "for-loop", "", ""){
	measure_execution_time(
		[&] {
			int count = 0;
			for (auto i = 0 ; i <= (10000 * 50000) ; i++) {
		count = count + 1;
			}
		}
	);
}
QUARK_UNIT_TEST_VIP("Basic performance", "for-loop", "", ""){
	interpreter_context_t context = make_test_context();

	measure_execution_time(
		[&] {
			const auto vm = run_global(context,
			R"(
				mutable int count = 0;
				for (index in 1...10000) {
					count = count + 1;
				}
			)");
		}
	);
}


int fibonacci(int n) {
	if (n <= 1){
		return n;
	}
	return fibonacci(n - 2) + fibonacci(n - 1);
}
QUARK_UNIT_TEST_VIP("C++", "fibonacci", "", ""){
	measure_execution_time(
		[&] {
			int sum = 0;
			for (auto i = 0 ; i <= (20 + 5) ; i++) {
				const auto a = fibonacci(i);
				sum = sum + a;
			}
		}
	);
}
QUARK_UNIT_TEST_VIP("Basic performance", "fibonacci", "", ""){
	interpreter_context_t context = make_test_context();

	measure_execution_time(
		[&] {
			const auto vm = run_global(context,

				"int fibonacci(int n) {"
				"	if (n <= 1){"
				"		return n;"
				"	}"
				"	return fibonacci(n - 2) + fibonacci(n - 1);"
				"}"

				"for (i in 0...10) {"
				"	a = fibonacci(i);"
				"}"

			);
		}
	);

}

#endif
