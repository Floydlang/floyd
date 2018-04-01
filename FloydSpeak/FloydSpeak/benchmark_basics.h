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
std::int64_t measure_execution_time_ns(const std::string& test_name, std::function<void (void)> func);

#endif
