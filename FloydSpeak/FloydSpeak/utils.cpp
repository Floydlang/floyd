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
