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

#if 1



//////////////////////////////////////////		TESTS

	const int64_t cpp_iterations = (50LL * 1000LL * 1000LL);
	const int64_t floyd_iterations = (50LL * 1000LL * 1000LL);

static void copy_test(){
			volatile int64_t count = 0;
			for (int64_t i = 0 ; i < cpp_iterations ; i++) {
				count = count + 1;
			}
			assert(count > 0);
//			std::cout << "C++ count:" << count << std::endl;
}


OFF_QUARK_UNIT_TEST_VIP("Basic performance", "for-loop", "", ""){

	const auto cpp_ns = measure_execution_time_ns(
		"C++: For-loop",
		[&] {
			copy_test();
		}
	);

	interpreter_context_t context = make_test_context();
	const auto ast = program_to_ast2(
		context,
		R"(
			int f(){
				mutable int count = 0;
				for (index in 1...50000000) {
					count = count + 1;
				}
				return 13;
			}
	//				print("Floyd count:" + to_string(count));
		)"
	);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		"Floyd: For-loop",
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		}
	);

	double cpp_iteration_time = (double)cpp_ns / (double)cpp_iterations;
	double floyd_iteration_time = (double)floyd_ns / (double)floyd_iterations;
	double k = cpp_iteration_time / floyd_iteration_time;
	std::cout << "Floyd performance: " << k << std::endl;
}




const int fib_cpp_iterations = 32;

int fibonacci(int n) {
	if (n <= 1){
		return n;
	}
	return fibonacci(n - 2) + fibonacci(n - 1);
}

void fib_test(){
	volatile int sum = 0;
	for (auto i = 0 ; i < fib_cpp_iterations ; i++) {
		const auto a = fibonacci(i);
		sum = sum + a;
	}
}

QUARK_UNIT_TEST_VIP("Basic performance", "fibonacci", "", ""){
	const auto cpp_ns = measure_execution_time_ns(
		"C++: Fibonacci",
		[&] {
			fib_test();
		}
	);

	interpreter_context_t context = make_test_context();
	const auto ast = program_to_ast2(
		context,
		R"(
			int fibonacci(int n) {
				if (n <= 1){
					return n;
				}
				return fibonacci(n - 2) + fibonacci(n - 1);
			}

			int f(){
				for (i in 0..<32) {
					a = fibonacci(i);
				}
				return 8;
			}
		)"
	);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		"Floyd: Fibonacci",
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		}
	);

	double k = (double)cpp_ns / (double)floyd_ns;
	std::cout << "Floyd performance percentage of C++: " << (k * 100.0) << std::endl;
}

#endif
