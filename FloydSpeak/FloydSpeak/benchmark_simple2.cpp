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
#include <iosfwd>

using std::vector;
using std::string;

using namespace floyd;



string number_fmt(unsigned long long n, char sep = ',') {
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


static interpreter_context_t make_context3(){
	const auto t = quark::trace_context_t(true, quark::get_trace());
	interpreter_context_t context{ t };
	return context;
}


struct bench_result_t {
	std::string _name;
	std::int64_t _cpp_ns;
	std::int64_t _floyd_ns;
};

std::string format_ns(int64_t value){
	const auto s = std::string(20, ' ') + number_fmt(value, ' ');
	return s.substr(s.size() - 16, 16);
}


void show_result(const bench_result_t& result){
	const auto cpp_str = format_ns(result._cpp_ns);
	const auto floyd_str = format_ns(result._floyd_ns);

	double cpp_iteration_time = (double)result._cpp_ns;
	double floyd_iteration_time = (double)result._floyd_ns;

	double k = floyd_iteration_time / cpp_iteration_time;

	std::cout << "Test: " << result._name << std::endl;
	std::cout << "\tC++  :" << cpp_str << std::endl;
	std::cout << "\tFloyd:" << floyd_str << std::endl;
	std::cout << "\tRel  : " << k << std::endl;
}

int64_t measure_floyd_function_f(const interpreter_context_t& context, const std::string& floyd_program){
	const auto ast = program_to_ast2(context, floyd_program);
	interpreter_t vm(ast);
	const auto f = find_global_symbol2(vm, "f");
	QUARK_ASSERT(f != nullptr);

	const auto floyd_ns = measure_execution_time_ns(
		[&] {
			const auto result = call_function(vm, bc_to_value(f->_value, f->_symbol._value_type), {});
		}
	);
	return floyd_ns;
}




QUARK_UNIT_TEST_VIP("Basic performance", "Simple", "", ""){
//	interpreter_context_t context = make_test_context();
	interpreter_context_t context = make_context3();

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

		show_result(bench_result_t{ "For loop incrementing variable",
			measure_execution_time_ns(cpp_func),
			measure_floyd_function_f(context, floyd_str)
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

		show_result(bench_result_t{ "For loop with if/else",
			measure_execution_time_ns(cpp_func),
			measure_floyd_function_f(context, floyd_str)
		});
	}

	if(1){
		const auto cpp_func = [] {
			volatile int result1 = 0;
			volatile int result2 = 0;
			volatile int result3 = 0;
			for(int i = 0 ; i < 50000000 ; i++){
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
				for(i in 0 ..< 50000000){
					result1 = result1 + i * 2;
					result2 = result2 + result1 * 2;
					result3 = result3 + result1 + result1;
				}
			}
		)";

		show_result(bench_result_t{ "For loop with int math",
			measure_execution_time_ns(cpp_func),
			measure_floyd_function_f(context, floyd_str)
		});
	}


}





