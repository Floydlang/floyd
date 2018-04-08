//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef benchmark_basics_h
#define benchmark_basics_h

#include <string>
#include <vector>

#include "floyd_interpreter.h"


floyd::interpreter_context_t make_test_context();

//	Returns time in nanoseconds
std::int64_t measure_execution_time_ns(std::function<void (void)> func);

floyd::interpreter_context_t make_verbose_context();


std::string number_fmt(unsigned long long n, char sep = ',');
std::string format_ns(int64_t value);




struct bench_result_t {
	std::string _name;
	std::int64_t _cpp_ns;
	std::int64_t _floyd_ns;
};


void trace_result(const bench_result_t& result);

int64_t measure_floyd_function_f(const floyd::interpreter_context_t& context, const std::string& floyd_program);

#endif
