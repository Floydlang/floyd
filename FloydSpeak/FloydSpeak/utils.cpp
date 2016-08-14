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



std::string quote(const std::string& s){
	return std::string("\"") + s + "\"";
}

QUARK_UNIT_TESTQ("quote()", ""){
	QUARK_UT_VERIFY(quote("") == "\"\"");
}

QUARK_UNIT_TESTQ("quote()", ""){
	QUARK_UT_VERIFY(quote("abc") == "\"abc\"");
}



std::string float_to_string(float value){
	std::stringstream s;
	s << value;
	const auto result = s.str();
	return result;
}

QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(0.0f), "0");
}
QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(13.0f), "13");
}
QUARK_UNIT_TESTQ("float_to_string()", ""){
	quark::ut_compare(float_to_string(13.5f), "13.5");
}



std::string double_to_string(double value){
	std::stringstream s;
	s << value;
	const auto result = s.str();
	return result;
}

QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(0.0), "0");
}
QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(13.0), "13");
}
QUARK_UNIT_TESTQ("double_to_string()", ""){
	quark::ut_compare(float_to_string(13.5), "13.5");
}



