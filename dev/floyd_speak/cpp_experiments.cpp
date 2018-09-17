//
//  cpp_experiments.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-07-23.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include <memory>
#include <string>

#include "parts/xcode-libcxx-xcode9/variant"	//	https://github.com/youknowone/xcode-libcxx

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




QUARK_UNIT_TEST("C++17 variant", "", "", ""){
    std::variant<int,std::string> foo;
	
    //Assign a value to the integer member
    foo = 69;
    //Extract the last assigned value:
    if( std::get<int>(foo) == 69 ) {
        std::cout << "Let's do it!\n";
    }
	
    //Assign a value to the std::string member
    foo = "Ciao!";
	
    std::cout << std::get<std::string>(foo) << std::endl;

	const auto x = std::get<std::string>(foo);
	QUARK_UT_VERIFY(x == "Ciao!");
}

/*
struct no_op_equal {
	float x;
	private: bool operator==(const no_op_equal& other){return false;};
};

QUARK_UNIT_TEST("C++17 variant", "operator==", "", ""){
    std::variant<int, std::string, no_op_equal> a = no_op_equal{ 3.14f };
    std::variant<int, std::string, no_op_equal> b = no_op_equal{ 3.14f };
	QUARK_UT_VERIFY(a == b);
}
*/




struct expression_t {
	std::string _source_code;
};

struct return_t {
	expression_t _expression;
};
struct store_t {
	std::string _variable_name;
	expression_t _expression;
};
struct bind_t {
	std::string _variable_name;
	expression_t _expression;
};
struct expression_statement_t {
	expression_t _expression;
};

struct test_statement_t {
    std::variant<return_t, store_t, bind_t, expression_statement_t> _contents;

	std::string _debug_str;
};


QUARK_UNIT_TEST("C++17 variant", "", "", ""){
	test_statement_t a{bind_t{"myval", {"1+3"}}, "debug myval = 1+3"};

	const auto s = sizeof(a);

	const auto bind = std::get<bind_t>(a._contents);
	QUARK_UT_VERIFY(bind._variable_name == "myval");
	QUARK_UT_VERIFY(bind._expression._source_code == "1+3");


	QUARK_UT_VERIFY(a._contents.index() == 2);
	const auto bind2 = std::get<2>(a._contents);
	QUARK_UT_VERIFY(bind2._variable_name == "myval");
	QUARK_UT_VERIFY(bind2._expression._source_code == "1+3");



	QUARK_UT_VERIFY(holds_alternative<bind_t>(a._contents) == true);
	QUARK_UT_VERIFY(holds_alternative<return_t>(a._contents) == false);


// 	const auto b = std::get_if<int>(&a._contents);
//	QUARK_UT_VERIFY(b != nullptr);

 	const auto c = std::get_if<bind_t>(&a._contents);
	QUARK_UT_VERIFY(c != nullptr);
}





