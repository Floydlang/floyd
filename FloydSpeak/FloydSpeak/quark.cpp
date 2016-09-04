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





#if QUARK_UNIT_TESTS_ON
	unit_test_registry* unit_test_rec::_registry_instance = nullptr;
#endif


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




//	ASSERT SUPPORT
//	====================================================================================================================





#if QUARK_ASSERT_ON

//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS

void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_assert(location, expression);
	exit(-1);
}

#endif




//	TRACE
//	====================================================================================================================



#if QUARK_TRACE_ON

void on_trace_hook(runtime_i* runtime, const char s[]){
	assert(runtime != nullptr);
	assert(s != nullptr);

	runtime->runtime_i__trace(s);
}

void on_trace_hook(runtime_i* runtime, const std::string& s){
	assert(runtime != nullptr);

	runtime->runtime_i__trace(s.c_str());
}

void on_trace_hook(runtime_i* runtime, const std::stringstream& s){
	assert(runtime != nullptr);

	runtime->runtime_i__trace(s.str().c_str());
}

#endif





//	UNIT TEST SUPPORT
//	====================================================================================================================




#if QUARK_UNIT_TESTS_ON

//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS

void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_unit_test_failed(location, expression);
}

/*
std::string OnGetPrivateTestDataPath(runtime_i* iRuntime, const char iModuleUnderTest[], const char iSourceFilePath[]){
	ASSERT(iRuntime != nullptr);
	ASSERT(iModuleUnderTest != nullptr);
	ASSERT(iSourceFilePath != nullptr);

	const TAbsolutePath absPath(iSourceFilePath);
	const TAbsolutePath parent = GetParent(absPath);
	return parent.GetStringPath();
}
*/


std::string path_to_name(const std::string& path){
	size_t pos = path.size();
	while(pos > 0 && path[pos - 1] != '/'){
		pos--;
	}
	return path.substr(pos);
}

QUARK_UNIT_TESTQ("path_to_name()", ""){
	QUARK_UT_VERIFY(path_to_name("/Users/marcus/Repositories/Floyd/examples/game_of_life2.cpp") == "game_of_life2.cpp");
}
QUARK_UNIT_TESTQ("path_to_name()", ""){
	QUARK_UT_VERIFY(path_to_name("game_of_life2.cpp") == "game_of_life2.cpp");
}
QUARK_UNIT_TESTQ("path_to_name()", ""){
	QUARK_UT_VERIFY(path_to_name("") == "");
}



int run_test_list(const std::string& source_file, const std::vector<unit_test_def>& tests){
	int fail_count = 0;
	for(std::size_t i = 0 ; i < tests.size() ; i++){
		const unit_test_def& test = tests[i];

		std::stringstream testInfo;
		testInfo << source_file << ":" << std::to_string(test._source_line) << " Test #" << i
			<< " " << test._class_under_test
			<< " | " << test._function_under_test
			<< " | " << test._scenario
			<< " | " << test._expected_result;

		try{
			QUARK_SCOPED_TRACE(testInfo.str());
			test._test_f();
		}
		catch(...){
			QUARK_TRACE("FAILURE: " + testInfo.str());
			fail_count++;
		}
	}
	return fail_count;
}

/*
sort(mMyClassVector.begin(), mMyClassVector.end(), 
    [](const MyClass & a, const MyClass & b) -> bool
{ 
    return a.mProperty > b.mProperty; 
});
*/
void run_tests(const unit_test_registry& registry, const std::vector<std::string>& source_file_order){
	QUARK_TRACE_FUNCTION();

	std::size_t test_count = registry._tests.size();
	std::size_t fail_count = 0;

	QUARK_TRACE_SS("Running " << test_count << " tests...");

	std::vector<unit_test_def> tests = registry._tests;

	for(const auto f: source_file_order){

		//	Make list of all tests for this source file.
		std::vector<unit_test_def> file_tests;
		auto it = tests.begin();
		while(it != tests.end()){

			//	If this test belongs to our source file, MOVE it to file_tests.
			const std::string file = path_to_name(it->_source_file);
			if(file == f){
				file_tests.push_back(*it);
				it = tests.erase(it);
			}
			else{
				it++;
			}
		}

		QUARK_SCOPED_TRACE(f);
		fail_count += run_test_list(f, file_tests);
	}

	if(!tests.empty()){
		QUARK_SCOPED_TRACE("UNSORTED TESTS");
		fail_count += run_test_list("More...", tests);
	}

	if(fail_count == 0){
		QUARK_TRACE_SS("SUCCESS " << test_count << " tests!");
	}
	else{
		QUARK_TRACE_SS("FAILED " << fail_count << " out of " << test_count << " tests!");
		exit(-1);
	}
}

void run_tests(const std::vector<std::string>& source_file_order){
	QUARK_ASSERT(unit_test_rec::_registry_instance != nullptr);
	run_tests(*unit_test_rec::_registry_instance, source_file_order);
}

void run_tests(){
	QUARK_TRACE_FUNCTION();
	QUARK_ASSERT(unit_test_rec::_registry_instance != nullptr);

	std::size_t test_count = unit_test_rec::_registry_instance->_tests.size();
	std::size_t fail_count = 0;

	QUARK_TRACE_SS("Running " << test_count << " tests...");

	for(std::size_t i = 0 ; i < test_count ; i++){
		const unit_test_def& test = unit_test_rec::_registry_instance->_tests[i];

		std::stringstream testInfo;
		testInfo << "Test #" << i
			<< " " << test._class_under_test
			<< " | " << test._function_under_test
			<< " | " << test._scenario
			<< " | " << test._expected_result;

		try{
			QUARK_SCOPED_TRACE(testInfo.str());
			test._test_f();
		}
		catch(...){
			QUARK_TRACE("FAILURE: " + testInfo.str());
			fail_count++;
		}
	}

	if(fail_count == 0){
		QUARK_TRACE_SS("SUCCESS - " << test_count << " tests!");
	}
	else{
		QUARK_TRACE_SS("FAILED " << fail_count << " out of " << test_count << " tests!");
		exit(-1);
	}
}

#endif


//	Default implementation
//	====================================================================================================================




//////////////////////////////////			default_runtime





default_runtime::default_runtime(const std::string& test_data_root) :
	_test_data_root(test_data_root),
	_indent(0)
{
}

void default_runtime::runtime_i__trace(const char s[]){
	for(long i = 0 ; i < _indent ; i++){
		std::cout << "|\t";
	}

	std::cout << std::string(s);
	std::cout << std::endl;
}

void default_runtime::runtime_i__add_log_indent(long add){
	_indent += add;
}

int default_runtime::runtime_i__get_log_indent() const{
	return static_cast<int>(_indent);
}

void default_runtime::runtime_i__on_assert(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");
	throw std::logic_error("assert");
}

void default_runtime::runtime_i__on_unit_test_failed(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");

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
	QUARK_TRACE_FUNCTION();
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(true);
	QUARK_TEST_VERIFY(true);
}


}

