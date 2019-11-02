
/*
	Copyright 2015 Marcus Zetterquist

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	quark is a minimal library for super-gluing C++ code together
	steady::vector<> is a persistent vector class for C++
*/

#include "quark.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <sstream>


namespace quark {


//	Force compiler error if these flags has not been setup.
bool assert_on = QUARK_ASSERT_ON;
bool trace_on = QUARK_TRACE_ON;
bool unit_tests_on = QUARK_UNIT_TESTS_ON;


unit_test_registry* unit_test_rec::registry_instance = nullptr;


namespace {
	quark::default_runtime default_runtime("");
	runtime_i* g_runtime = &default_runtime;
}


runtime_i* get_runtime(){
	return g_runtime;
}

void set_runtime(runtime_i* iRuntime){
	g_runtime = iRuntime;
}


//	UNIT TEST SUPPORT
//	====================================================================================================================


#if QUARK_UNIT_TESTS_ON


QUARK_TESTQ("path_to_name()", ""){
	QUARK_VERIFY(path_to_name("/Users/marcus/Repositories/Floyd/examples/game_of_life2.cpp") == "game_of_life2.cpp");
}
QUARK_TESTQ("path_to_name()", ""){
	QUARK_VERIFY(path_to_name("game_of_life2.cpp") == "game_of_life2.cpp");
}
QUARK_TESTQ("path_to_name()", ""){
	QUARK_VERIFY(path_to_name("") == "");
}

//??? test mixes of string vs char*

QUARK_TEST("Quark", "ut_verify()", "", ""){
	ut_verify(QUARK_POS, std::string("xyz123"), std::string("xyz123"));
}
QUARK_TEST("Quark", "ut_verify()", "", ""){
	ut_verify(QUARK_POS, "xyz", "xyz");
}
/*
QUARK_TEST("Quark", "ut_verify()", "", ""){
	try{
		ut_verify(QUARK_POS, "xyzabc", "xyztbcd");
		fail_test(QUARK_POS);
	}
	catch(...){
		//	We should land here.
	}
}
*/

struct custom_type_t {
	int a;
	std::string s;
};

void ut_verify(const call_context_t& context, const custom_type_t& result, const custom_type_t& expected){
	if(result.a == expected.a && result.s == expected.s){
	}
	else{
		QUARK_TRACE_SS("  result: " << result.a << " " << result.s);
		QUARK_TRACE_SS("expected: " << expected.a << " " << expected.s);
		quark::fail_test(context);
	}
}

#endif


//	Default implementation
//	====================================================================================================================


//////////////////////////////////			default_runtime


default_runtime::default_runtime(const std::string& test_data_root) :
	_test_data_root(test_data_root)
{
}

void default_runtime::runtime_i__on_assert(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");

	int error = errno;
	if(error != 0){
		perror("perror() says");
	}

	throw std::logic_error("assert");
}

void default_runtime::runtime_i__on_unit_test_failed(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");

	int error = errno;
	if(error != 0){
		perror("perror() says");
	}

	throw std::logic_error("Unit test failed");
}


//	TESTS
//	====================================================================================================================


/*
	This function uses all macros so we know they compile.
*/
void test_macros(){
	QUARK_ASSERT(true);

	QUARK_TRACE("hello");
	QUARK_TRACE_SS("hello" << 1234);
	QUARK_SCOPED_TRACE("");
	QUARK_SCOPED_TRACE(std::string("scoped trace") + "std::string version");
}

QUARK_TEST("", "", "", ""){
	QUARK_VERIFY(true);
	QUARK_VERIFY(true);
}

}
