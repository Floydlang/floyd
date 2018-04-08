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




static interpreter_context_t make_context2(){
	const auto t = quark::trace_context_t(true, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}


//////////////////////////////////////////		C


static void c_runner(unsigned int len, uint8_t *a, uint8_t *b, uint8_t *c){
	for(int i = 0 ; i < 3 ; i++){
		int result = 3;
		QUARK_ASSERT(result != 0);
	}
}




//////////////////////////////////////////		x


static const std::string gol_floyd_str = R"(

	int f(){
		struct pixel_t { int red; int green; int blue;}

		mutable [pixel_t] a = [];
		for(i in 0 ..< 32){
			a = push_back(a, pixel_t(100, 100, 100));
			a = push_back(a, pixel_t(100, 0, 0));
			a = push_back(a, pixel_t(0, 0, 0));
			a = push_back(a, pixel_t(0, 0, 0));
		}

		mutable [pixel_t] b = [];
		for(i in 0 ..< size(a)){
			n = pixel_t(a[i].red * 2, a[i].green + 1, a[i].blue * 3);
			b = push_back(b, n);
		}
//		print(a);
//		print(b);
		return 0;
	}

)";

static const int k_len = 39 * 1000;

OFF_QUARK_UNIT_TEST_VIP("Basic performance", "Simple", "", ""){
/*
	std::vector<uint8_t> a;
	std::vector<uint8_t> b;
	std::vector<uint8_t> c;

	for(int i = 0 ; i < k_len ; i++){
		a.push_back(i / 7);
		b.push_back(i / 13);
		c.push_back(100 + (i / 13));
	}

	const auto cpp_ns = measure_execution_time_ns(
		"C++",
		[&] {
			c_runner(k_len, &a[0], &b[0], &c[0]);
		}
	);
*/

//	interpreter_context_t context = make_test_context();
	interpreter_context_t context = make_context2();
	const auto ast = program_to_ast2(context, gol_floyd_str);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		}
	);

/*
	double cpp_iteration_time = (double)cpp_ns / (double)cpp_iterations;
	double floyd_iteration_time = (double)floyd_ns / (double)floyd_iterations;
	double k = cpp_iteration_time / floyd_iteration_time;
	std::cout << "Floyd performance: " << k << std::endl;
*/

}


