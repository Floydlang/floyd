//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "interpretator_benchmark.h"

#include "benchmark_basics.h"

#include <string>
#include <vector>
#include <iostream>

using std::vector;
using std::string;

using namespace floyd;



static interpreter_context_t make_context3(){
	const auto t = quark::trace_context_t(true, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}

#if 0
static void c_runner(){
		volatile int result = 0;
		for(int i = 0 ; i < 10000000 ; i++){
			int a = result + i * i + 2 * i - result;
			if(a > 0){
				result = -a;
			}
			else{
				result = a;
			}
		}
}

static const std::string floyd_str = R"(
	void f(){
		mutable result = 0;
		for(i in 0 ..< 10000000){
			a = result + i * i + 2 * i - result;
			if(a > 0){
				result = -a;
			}
			else{
				result = a;
			}
		}

		print(result);
	}
)";
#endif


static void cpp_runner(){
	volatile int result = 0;
	for(int i = 0 ; i < 1000000000 ; i++){
		result = result + i * 2;
	}
}

static const std::string floyd_str = R"(
	void f(){
		mutable result = 0;
		for(i in 0 ..< 1000000000){
			result = result + i * 2;
		}
	}
)";



OFF_QUARK_UNIT_TEST_VIP("Basic performance", "Simple", "", ""){
	const auto cpp_ns = measure_execution_time_ns(
		"C++",
		[&] {
//			cpp_runner();
		}
	);

	interpreter_context_t test_context = make_test_context();
	interpreter_context_t context = make_context3();
	const auto ast = program_to_ast2(context, floyd_str);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		"Floyd",
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		}
	);

	double cpp_iteration_time = (double)cpp_ns;
	double floyd_iteration_time = (double)floyd_ns;
	double k = cpp_iteration_time / floyd_iteration_time;
	std::cout << "Floyd performance: " << k << std::endl;
}


