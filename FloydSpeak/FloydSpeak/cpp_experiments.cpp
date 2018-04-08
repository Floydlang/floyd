//
//  cpp_experiments.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-07-23.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include <memory>
using namespace std;

#include "quark.h"


struct test_cpp_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_cpp_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_cpp_value_class_a", "what is needed for basic operations"){
	test_cpp_value_class_a a;
	test_cpp_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}


QUARK_UNIT_TESTQ("C++ bool", ""){
	quark::ut_compare(true, true);
	quark::ut_compare(true, !false);
	quark::ut_compare(false, false);
	quark::ut_compare(!false, true);

	const auto x = false + false;
	const auto y = false - false;

	QUARK_UT_VERIFY(x == false);
	QUARK_UT_VERIFY(y == false);
}

QUARK_UNIT_TEST("","", "", ""){
	const auto size = sizeof(std::make_shared<int>(13));
	QUARK_UT_VERIFY(size == 16);
}


