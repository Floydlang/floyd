//
//  utils.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 11/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "utils.h"
#include "quark.h"
#include <string>


//	template <typename T> bool compare_shared_values(const T& ptr_a, const T& ptr_b){


QUARK_UNIT_TESTQ("compare_shared_values()", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("a");
	QUARK_TEST_VERIFY(compare_shared_values(a, b));
}

QUARK_UNIT_TESTQ("compare_shared_values()", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = a;
	QUARK_TEST_VERIFY(compare_shared_values(a, b));
}


QUARK_UNIT_TESTQ("compare_shared_values()", ""){
	const auto a = std::make_shared<std::string>("a");
	const auto b = std::make_shared<std::string>("b");
	QUARK_TEST_VERIFY(!compare_shared_values(a, b));
}





auto lambda_echo = [](int i ) { QUARK_TRACE_SS(i); };

std::vector<int> test_collection{20,24,37,42,23,45,37};

QUARK_UNIT_TEST("", "lambda_echo()", "", "") {
	for_each(test_collection,lambda_echo);
}

auto addOne = [](int i) { return i+1;};

QUARK_UNIT_TEST("", "mapf()", "", "") {
	auto returnCol = mapf(test_collection, addOne);
	for_each(returnCol,lambda_echo);
}



QUARK_UNIT_TEST("", "filter()", "", "") {
	auto filteredCol = filter(test_collection,[](int value){ return value > 30;});
	for_each(filteredCol,lambda_echo);
}







#if 0
std::string float_to_string_no_trailing_zeros(float v){
    std::stringstream ss;
    ss << v;
    return ss.str();
/*
	char temp[200 + 1];//### Use C++ function instead.
	sprintf(temp, "%f", _float);
	return std::string(temp);
*/
}


QUARK_UNIT_TEST("", "", "", ""){
	QUARK_ASSERT(float_to_string_no_trailing_zeros(0) == "0");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_ASSERT(float_to_string_no_trailing_zeros(123) == "123");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_ASSERT(float_to_string_no_trailing_zeros(1.123) == "1.123");
}
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_ASSERT(float_to_string_no_trailing_zeros(1.5) == "1.5");
}
#endif



#include <sstream>
