//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "interpretator_benchmark.h"

#include "benchmark_basics.h"

#include <string>
#include <vector>
#include <iostream>

using std::vector;
using std::string;

using namespace floyd;


interpreter_context_t make_test_context(){
	const auto t = quark::trace_context_t(false, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}


//	Returns time in nanoseconds
std::int64_t measure_execution_time_ns(const std::string& test_name, std::function<void (void)> func){
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

	auto d0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
	auto d1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
	auto d2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2);
	auto d3 = std::chrono::duration_cast<std::chrono::nanoseconds>(t4 - t3);
	auto d4 = std::chrono::duration_cast<std::chrono::nanoseconds>(t5 - t4);

	const auto average = (d0 + d1 + d2 + d3 + d4) / 5.0;
	auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(average);

	int64_t duration = duration1.count();
	std::cout << test_name << " execution time (ns): " << duration << std::endl;
	return duration;
}

OFF_QUARK_UNIT_TEST("", "measure_execution_time_ns()", "", ""){
	const auto t = measure_execution_time_ns(
		"test",
		[] { std::cout << "Hello, my Greek friends"; }
	);
	std::cout << "duration2..." << t << std::endl;
}



