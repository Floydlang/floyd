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





//////////////////////////////////////////		C

//	https://medium.com/@n0mad/when-competing-with-c-fudge-the-benchmark-16d3a91b437c

static unsigned int tmetrics_hamming (unsigned int len, uint8_t *a, uint8_t *b)
{
  unsigned int acc = 0, i;
  for (i = 0; i < len; i++)
    {
      if (*(a + i) != *(b + i)) acc++;
    }
  return acc;
}



static void c_runner(unsigned int len, uint8_t *a, uint8_t *b, uint8_t *c){
	for(int i = 0 ; i < 100 ; i++){
		const auto result0 = tmetrics_hamming(len, a, b);
		const auto result1 = tmetrics_hamming(len, b, c);
		const auto result2 = tmetrics_hamming(len, c, a);

		const auto result3 = tmetrics_hamming(len, b, a);
		const auto result4 = tmetrics_hamming(len, c, b);
		const auto result5 = tmetrics_hamming(len, a, c);
		const auto result = result0 + result1 + result2 + result3 + result4 + result5;
		QUARK_ASSERT(result != 0);
	}
}




//////////////////////////////////////////		ported to Floyd -- direct-port


static const std::string gol_floyd_str = R"(
	int f(){
		mutable int count = 0;
		for (index in 1...5000) {
			count = count + 1;
		}
		return 13;
	}
)";

static const int k_len = 39 * 1000;

OFF_QUARK_UNIT_TEST_VIP("Basic performance", "Text metrics", "", ""){
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

	interpreter_context_t context = make_test_context();
	const auto ast = program_to_ast2(context, gol_floyd_str);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		"Floyd",
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


