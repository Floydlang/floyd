//
//  floyd_test_suite.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-21.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "interpretator_benchmark.h"

#include "floyd_interpreter.h"

#include <string>
#include <vector>
#include <iostream>

using std::vector;
using std::string;

using namespace floyd;



OFF_QUARK_UNIT_TEST_VIP("Basic performance", "for-loop", "", ""){
	auto start = std::chrono::system_clock::now();

	interpreter_context_t context{ quark::trace_context_t(false, quark::get_runtime()) };

	const auto vm = run_global(context,
	R"(
		mutable int count = 0;
		for (index in 1...100000) {
			count = count + 1;
		}
	)");

	const auto end = std::chrono::system_clock::now();
	auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//	auto duration2 = std::chrono::duration_cast<std::chrono::seconds>(end - start);
//	QUARK_TRACE_SS("duration:" << duration1.count() << "\n");
	std::cout << "duration..." << duration1.count() << std::endl;
}


QUARK_UNIT_TEST("C++", "for-loop", "", ""){
	auto start = std::chrono::system_clock::now();

	int count = 0;
	for (auto i = 0 ; i <= 100000 * 1000 ; i++) {
		count = count + 1;
	}

	std::cout << "count: " << count << std::endl;
	const auto end = std::chrono::system_clock::now();
	auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//	auto duration2 = std::chrono::duration_cast<std::chrono::seconds>(end - start);
//	QUARK_TRACE_SS("duration:" << duration1.count() << "\n");
	std::cout << "duration..." << duration1.count() << std::endl;
}



int fibonacci(int n) {
	if (n <= 1){
		return n;
	}
	return fibonacci(n - 2) + fibonacci(n - 1);
}

QUARK_UNIT_TEST("C++", "fibonacci", "", ""){
	auto start = std::chrono::system_clock::now();

	int sum = 0;
	for (auto i = 0 ; i <= (20 + 10) ; i++) {
		const auto a = fibonacci(i);
		sum = sum + a;
	}

	std::cout << "sum: " << sum << std::endl;
	const auto end = std::chrono::system_clock::now();
	auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//	auto duration2 = std::chrono::duration_cast<std::chrono::seconds>(end - start);
//	QUARK_TRACE_SS("duration:" << duration1.count() << "\n");
	std::cout << "duration..." << duration1.count() << std::endl;
}

OFF_QUARK_UNIT_TEST_VIP("Basic performance", "fibonacci", "", ""){
	auto start = std::chrono::system_clock::now();

	interpreter_context_t context{ quark::trace_context_t(false, quark::get_runtime()) };
	const auto vm = run_global(context,
		"int fibonacci(int n) {"
		"	if (n <= 1){"
		"		return n;"
		"	}"
		"	return fibonacci(n - 2) + fibonacci(n - 1);"
		"}"

		"for (i in 0...20) {"
		"	a = fibonacci(i);"
		"}"
	);

	const auto end = std::chrono::system_clock::now();
	auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//	auto duration2 = std::chrono::duration_cast<std::chrono::seconds>(end - start);
//	QUARK_TRACE_SS("duration:" << duration1.count() << "\n");
	std::cout << "duration..." << duration1.count() << std::endl;
}
