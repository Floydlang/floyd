
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


void on_problem___put_breakpoint_here(){
}



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
	on_problem___put_breakpoint_here();

	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_assert(location, expression);
	exit(-1);
}

#endif




//	TRACE
//	====================================================================================================================

	

#if QUARK_TRACE_ON

void on_trace_hook(runtime_i* runtime, const char s[], trace_context_t* tracer){
	//Make sure runtime OR tracer is provided.
	assert((runtime == nullptr || tracer == nullptr) && (runtime != nullptr || tracer != nullptr));

	if(tracer != nullptr){
		tracer->trace_i__trace(s);
	}
	else{
		assert(runtime != nullptr);
		assert(s != nullptr);
		runtime->runtime_i__trace(s);
	}
}

void on_trace_hook(runtime_i* runtime, const std::string& s, trace_context_t* tracer){
	//Make sure runtime OR tracer is provided.
	assert((runtime == nullptr || tracer == nullptr) && (runtime != nullptr || tracer != nullptr));

	if(tracer != nullptr){
		tracer->trace_i__trace(s.c_str());
	}
	else{
		assert(runtime != nullptr);
		runtime->runtime_i__trace(s.c_str());
	}
}

void on_trace_hook(runtime_i* runtime, const std::stringstream& s, trace_context_t* tracer){
	//Make sure runtime OR tracer is provided.
	assert((runtime == nullptr || tracer == nullptr) && (runtime != nullptr || tracer != nullptr));

	if(tracer != nullptr){
		QUARK_ASSERT(runtime == nullptr);
		tracer->trace_i__trace(s.str().c_str());
	}
	else{
		assert(runtime != nullptr);

		runtime->runtime_i__trace(s.str().c_str());
	}
}

#endif





//	UNIT TEST SUPPORT
//	====================================================================================================================




#if QUARK_UNIT_TESTS_ON

//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS

void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	on_problem___put_breakpoint_here();

	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_unit_test_failed(location, expression);
}



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


int run_test_list(const std::vector<unit_test_def>& tests){
	int fail_count = 0;
	for(std::size_t i = 0 ; i < tests.size() ; i++){
		const unit_test_def& test = tests[i];

		std::stringstream testInfo;
		testInfo << test._source_file << ":" << std::to_string(test._source_line) << " Test #" << i
			<< " " << test._class_under_test
			<< " | " << test._function_under_test
			<< " | " << test._scenario
			<< " | " << test._expected_result;

		try{
			QUARK_SCOPED_TRACE(testInfo.str());
			test._test_f();
		}
		catch(const std::exception& e){
			QUARK_TRACE("FAILURE: " + testInfo.str());
			QUARK_TRACE(typeid(e).name());
			QUARK_TRACE(e.what());
			fail_count++;
			on_problem___put_breakpoint_here();
		}
		catch(...){
			QUARK_TRACE("FAILURE: " + testInfo.str());
			fail_count++;
			on_problem___put_breakpoint_here();
		}
	}
	return fail_count;
}



std::vector<unit_test_def> prepare_test_list(const std::vector<unit_test_def>& tests, const std::vector<std::string>& source_file_order){
	std::vector<unit_test_def> vip_tests;
	for(const auto e: tests){
		if(e._vip){
			vip_tests.push_back(e);
		}
		else{
		}
	}
	if(vip_tests.empty() == false){
		return vip_tests;
	}

	else{
		std::vector<unit_test_def> ordered_tests;
		std::vector<unit_test_def> remaining_tests = tests;
		
		for(const auto f: source_file_order){

			//	Make list of all tests for this source file.
			auto it = remaining_tests.begin();
			while(it != remaining_tests.end()){

				//	If this test belongs to our source file, MOVE it to file_tests.
				const std::string file = path_to_name(it->_source_file);
				if(file == f){
					ordered_tests.push_back(*it);
					it = remaining_tests.erase(it);
				}
				else{
					it++;
				}
			}
		}
		auto result = ordered_tests;
		result.insert(result.end(), remaining_tests.begin(), remaining_tests.end());
		return result;
	}
}


void run_tests(const unit_test_registry& registry, const std::vector<std::string>& source_file_order){
	QUARK_TRACE_FUNCTION();

	const auto filtered_tests = prepare_test_list(registry._tests, source_file_order);

	std::size_t test_count = registry._tests.size();
	std::size_t fail_count = 0;
	QUARK_TRACE_SS("Running " << test_count << " tests...");

	fail_count += run_test_list(filtered_tests);

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
