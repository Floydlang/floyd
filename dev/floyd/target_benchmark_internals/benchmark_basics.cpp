//
//  floyd_test_suite.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "interpretator_benchmark.h"

#include "ast_value.h"
#include "benchmark_basics.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"
#include "bytecode_helpers.h"

#include <string>
#include <vector>
#include <iostream>
#include <chrono>

using std::vector;
using std::string;

using namespace floyd;



string number_fmt(unsigned long long n, char sep) {
    std::stringstream fmt;
    fmt << n;
    std::string s = fmt.str();
    s.reserve(s.length() + s.length() / 3);

    // loop until the end of the string and use j to keep track of every
    // third loop starting taking into account the leading x digits (this probably
    // can be rewritten in terms of just i, but it seems more clear when you use
    // a seperate variable)
    for (int i = 0, j = 3 - s.length() % 3; i < s.length(); ++i, ++j)
        if (i != 0 && j % 3 == 0)
            s.insert(i++, 1, sep);

    return s;
}


std::string format_ns(int64_t value){
	const auto s = std::string(20, ' ') + number_fmt(value, ' ');
	return s.substr(s.size() - 16, 16);
}


void trace_result(const bench_result_t& result){
	const auto cpp_str = format_ns(result._cpp_ns);
	const auto floyd_str = format_ns(result._floyd_ns);

	double cpp_iteration_time = (double)result._cpp_ns;
	double floyd_iteration_time = (double)result._floyd_ns;

	double k = floyd_iteration_time / cpp_iteration_time;
	double p = (100.0 * cpp_iteration_time) / floyd_iteration_time;

	std::cout << "Test: " << result._name << std::endl;
	std::cout << "\tC++  :" << cpp_str << " ns" <<std::endl;
	std::cout << "\tFloyd:" << floyd_str << " ns"  << std::endl;
	std::cout << "\tRel  : " << k << std::endl;
	std::cout << "\t%    : " << p << std::endl;
}

int64_t measure_floyd_function_f(const std::string& floyd_program, int count){
	const auto cu = make_compilation_unit_lib(floyd_program, "");
	const auto program = compile_to_bytecode(cu);


	const auto s = json_to_pretty_string(bcprogram_to_json(program));
	std::cout << s << std::endl;

	interpreter_t vm(program);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value), {});
		},
		count
	);
	return floyd_ns;
}


//	Returns time in nanoseconds
std::int64_t measure_execution_time_ns(std::function<void (void)> func, int count){
	vector<std::chrono::nanoseconds> results;
	for(int i = 0 ; i < count ; i++){
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

		auto d0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
		auto d1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
		auto d2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2);
		auto d3 = std::chrono::duration_cast<std::chrono::nanoseconds>(t4 - t3);

		results.push_back(d0);
		results.push_back(d1);
		results.push_back(d2);
		results.push_back(d3);
	}

	auto d0 = results[results.size() - 4 + 0];
	auto d1 = results[results.size() - 4 + 1];
	auto d2 = results[results.size() - 4 + 2];
	auto d3 = results[results.size() - 4 + 3];

//	const auto average = (d0 + d1 + d2 + d3) / 4.0;
	const auto min = std::min(std::min(d0, d1), std::min(d2, d3));
	auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(min);

	int64_t duration = duration1.count();
	return duration;
}

QUARK_UNIT_TEST("quark", "measure_execution_time_ns()", "", ""){
	const auto t = measure_execution_time_ns(
		[] { std::cout << "Hello, my Greek friends"; }
	);
	std::cout << "duration2..." << t << std::endl;
}


