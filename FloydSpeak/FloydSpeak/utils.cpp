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
