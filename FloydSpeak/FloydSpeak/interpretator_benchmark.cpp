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

using std::string;

using namespace floyd;


static int k_repeats = 1;



//??unused
static const std::string pixel_floyd_str = R"(

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




#if 0

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
		[&] {
			c_runner(k_len, &a[0], &b[0], &c[0]);
		},
		k_repeats
	);

	interpreter_context_t context = make_test_context();
	const auto ast = program_to_ast2(context, gol_floyd_str);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		},
		k_repeats
	);

#endif





QUARK_UNIT_TEST_VIP("Basic performance", "", "", ""){
//	interpreter_context_t context = make_test_context();
	interpreter_context_t context = make_verbose_context();

	if(1){
		const auto cpp_func = [] {
			volatile int result = 0;
			for(int i = 0 ; i < 50000000 ; i++){
				result = result + 1;
			}
		};

		const std::string floyd_str = R"(
			void f(){
				mutable result = 0;
				for(i in 0 ..< 50000000){
					result = result + 1;
				}
			}
		)";

		trace_result(bench_result_t{ "For loop incrementing variable",
			measure_execution_time_ns(cpp_func, k_repeats),
			measure_floyd_function_f(context, floyd_str, k_repeats)
		});
	}

	if(1){
		const auto cpp_func = [] {
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
		};

		const std::string floyd_str = R"(
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
			}
		)";

		trace_result(bench_result_t{ "For loop with if/else",
			measure_execution_time_ns(cpp_func, k_repeats),
			measure_floyd_function_f(context, floyd_str, k_repeats)
		});
	}

	if(1){
		const auto cpp_func = [] {
			volatile int result1 = 0;
			volatile int result2 = 0;
			volatile int result3 = 0;
			for(int i = 0 ; i < 20000000 ; i++){
				result1 = result1 + i * 2;
				result2 = result2 + result1 * 2;
				result3 = result3 + result1 + result1;
			}
		};

		const std::string floyd_str = R"(
			void f(){
				mutable int result1 = 0;
				mutable int result2 = 0;
				mutable int result3 = 0;
				for(i in 0 ..< 20000000){
					result1 = result1 + i * 2;
					result2 = result2 + result1 * 2;
					result3 = result3 + result1 + result1;
				}
			}
		)";

		trace_result(bench_result_t{ "For loop with int math",
			measure_execution_time_ns(cpp_func, k_repeats),
			measure_floyd_function_f(context, floyd_str, k_repeats)
		});
	}

	if(1){
		struct dummy_y {
			static int fibonacci(int n) {
				if (n <= 1){
					return n;
				}
				return fibonacci(n - 2) + fibonacci(n - 1);
			}
		};

		const auto cpp_func = [] {
			volatile int sum = 0;
			for (auto i = 0 ; i < 32 ; i++) {
				const auto a = dummy_y::fibonacci(i);
				sum = sum + a;
			}
		};

		const std::string floyd_str = R"(
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
		)";

		trace_result(bench_result_t{ "Fibonacci",
			measure_execution_time_ns(cpp_func, k_repeats),
			measure_floyd_function_f(context, floyd_str, k_repeats)
		});

	}

}



