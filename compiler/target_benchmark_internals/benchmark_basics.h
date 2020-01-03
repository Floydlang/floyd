//
//  floyd_test_suite.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef benchmark_basics_h
#define benchmark_basics_h

#include <string>
#include <vector>

#include "bytecode_execution_engine.h"



//	Returns time in nanoseconds
//	Runs warmup, 4 measuring calls. count is a multiplier that causes entire set to run more times.
std::int64_t measure_execution_time_ns(std::function<void (void)> func, int count = 1);


std::string number_fmt(unsigned long long n, char sep = ',');
std::string format_ns(int64_t value);


struct bench_result_t {
	std::string _name;
	std::int64_t _cpp_ns;
	std::int64_t _floyd_ns;
};


void trace_result(const bench_result_t& result);

#if 0
int64_t measure_floyd_function_f(const std::string& floyd_program, int count);
#endif

#endif
