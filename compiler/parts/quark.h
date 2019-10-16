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

//	See quark.md for documentation!

#ifndef quark_h
#define quark_h

#include <algorithm>
#include <memory>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>


////////	DETECTING PLATFORM

//	Make explicit defines for each platforms. Extend with more platforms.
//	Notice that we always *DEFINE* the macros, but they are 0 or 1. This lets you do logic.
//	Use only these to detect platform.
#ifdef __APPLE__
	#define QUARK_MAC			1
	#define QUARK_WINDOWS		0
	#define QUARK_LINUX			0
#elif _MSC_VER
	#define QUARK_MAC			0
	#define QUARK_WINDOWS		1
	#define QUARK_LINUX			0
#else
	#define QUARK_MAC			0
	#define QUARK_WINDOWS		0
	#define QUARK_LINUX			1
#endif


////////////////////////////////	SETTING DEFAULTS

#ifndef QUARK_ASSERT_ON
	#define QUARK_ASSERT_ON 1
#endif

#ifndef QUARK_TRACE_ON
	#define QUARK_TRACE_ON 1
#endif

#ifndef QUARK_UNIT_TESTS_ON
	#define QUARK_UNIT_TESTS_ON 1
#endif



////////////////////////////////	NO_RETURN

//	NO_RETURN tags a function that it will never return. This avoid compiler warning that caller doesn't return a value.

#if QUARK_MAC
	#define QUARK_NO_RETURN	__dead2
#elif QUARK_WINDOWS
	#define QUARK_NO_RETURN __declspec(noreturn)
#elif QUARK_LINUX
	#define	QUARK_NO_RETURN __attribute__((__noreturn__))
#endif


////////////////////////////////	COMPILER FLAGS


//	??? This should go into your compiler settings, not be assumed by quark.

#if QUARK_MAC
#elif QUARK_WINDOWS
	#pragma warning(default:4716)

	// SB: needed to ignore warnings for unchecked vectors and deprecated functions such as strcpy()
	#define _CRT_SECURE_NO_WARNINGS
	#define _SCL_SECURE_NO_WARNINGS
#elif QUARK_LINUX
#endif



namespace quark {


////////////////////////////////	EXCEPTIONS


void throw_exception() QUARK_NO_RETURN;
inline void throw_exception(){ throw std::exception(); }

void throw_runtime_error(const std::string& s) QUARK_NO_RETURN;
inline void throw_runtime_error(const std::string& s) { throw std::runtime_error(s); }

void throw_logic_error() QUARK_NO_RETURN;
inline void throw_logic_error(){ throw std::logic_error(""); }


class format_exception : std::runtime_error {
	public: format_exception(const std::string& noun_type, const std::string& what) :
		std::runtime_error(what),
		noun_type(noun_type)
	{
	}

	public: std::string noun_type;
};

inline void throw_format(const std::string& noun_type, const std::string& what){
	throw format_exception(noun_type, what);
}


////////////////////////////////	PRIMITIVES


////////////////////////////		source_code_location

/*
	Value-object that specifies a specific line of code in a specific source file.
*/

struct source_code_location {
	source_code_location(const char source_file[], long line_number) :
		_line_number(line_number)
	{
		assert(source_file != nullptr);
		assert(std::strlen(source_file) <= 1024);
		strcpy(_source_file, source_file);
	}

	char _source_file[1024 + 1];
	long _line_number;
};


////////////////////////////		runtime_i

/*
	Interface class so client can hook-in behaviors for logging and asserts.
*/

class runtime_i {
	public: virtual ~runtime_i(){};
	public: virtual void runtime_i__on_assert(const source_code_location& location, const char expression[]) = 0;
	public: virtual void runtime_i__on_unit_test_failed(const source_code_location& location, const char s[]) = 0;
};


////////////////////////////		get_runtime() and set_runtime()

/*
	Global functions for storing the current runtime.
	Notice that only the macros use these! The implementation does NOT.
*/

runtime_i* get_runtime();
void set_runtime(runtime_i* iRuntime);


//	WORKAROUNDS
//	====================================================================================================================


/*
	Macros to generate unique names for unit test functions etc.
*/

#define QUARK_STRING(a) #a
#define QUARK_CONCAT2(a,b)  a##b
#define QUARK_CONCAT3(a, b, c) a##b##c
#define QUARK_UNIQUE_LABEL_INTERNAL(prefix, suffix) QUARK_CONCAT2(prefix, suffix)
#define QUARK_UNIQUE_LABEL(prefix) QUARK_UNIQUE_LABEL_INTERNAL(prefix, __LINE__)


//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS
inline void on_problem___put_breakpoint_here(){
}


//	ASSERT SUPPORT
//	====================================================================================================================


#if QUARK_ASSERT_ON
	inline void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
		on_problem___put_breakpoint_here();

		assert(runtime  != nullptr);
		assert(expression != nullptr);

		runtime->runtime_i__on_assert(location, expression);
		exit(-1);
	}

	#define QUARK_ASSERT(x) if(x){}else {::quark::on_assert_hook(::quark::get_runtime(), quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(x)); }
#else
	#define QUARK_ASSERT(x)
#endif





//	TRACE
//	====================================================================================================================


////////////////////////////		trace_i

struct trace_i {
	public: virtual ~trace_i(){};
	public: virtual void trace_i__trace(const char s[]) const = 0;
	public: virtual void trace_i__open_scope(const char s[]) const = 0;
	public: virtual void trace_i__close_scope(const char s[]) const = 0;
	public: virtual int trace_i__get_indent() const = 0;
};


//////////////////////////////////			default_tracer_t
/*
	It uses cout.
*/

struct default_tracer_t : public trace_i {
	public: default_tracer_t();

	public: virtual void trace_i__trace(const char s[]) const;
	public: virtual void trace_i__open_scope(const char s[]) const;
	public: virtual void trace_i__close_scope(const char s[]) const;
	public: virtual int trace_i__get_indent() const;


	///////////////		State.
	public: int mutable _indent;
};

inline default_tracer_t::default_tracer_t() :
	_indent(0)
{
}

//??? Take argument std::ostream to stream to.
//	Indents each line in s correctly.
inline void trace_indent(const char s[], int indent, const std::string& indent_str) {
	const auto count = strlen(s);
	if(count > 0){
		size_t pos = 0;
		while(pos != count){

			int line_indent = indent;

			//	Convert leading tabs into indents so we can indent everything using indent_str.
			while(pos < count && s[pos] == '\t'){
				line_indent++;
				pos++;
			}

			//	Indent start of line.
			for(auto i = 0 ; i < line_indent ; i++){
				std::cout << indent_str;
			}


			//	Print line until newline.
			std::string line_acc;
			while(pos < count && s[pos] != '\n'){
				const auto ch = s[pos];
				line_acc.push_back(ch);
				pos++;
			}

			std::cout << line_acc << std::endl;

			//	Skip newline.
			if(pos < count){
				pos++;
			}
		}
	}
}


//	Indents each line in s correctly.
inline void default_tracer_t::trace_i__trace(const char s[]) const{
	const auto count = strlen(s);
	if(count > 0){
		trace_indent(s, _indent, "\t");
	}
}
inline void default_tracer_t::trace_i__open_scope(const char s[]) const{
	trace_i__trace(s);
	_indent++;
}
inline void default_tracer_t::trace_i__close_scope(const char s[]) const{
	_indent--;
	trace_i__trace(s);
}
inline int default_tracer_t::trace_i__get_indent() const{
	return _indent;
}


struct trace_globals_t {
	trace_globals_t() :
		_current(nullptr)
	{
	}
	public: const default_tracer_t _default_tracer;
	public: const trace_i* _current;
};

inline trace_globals_t* get_global_data(){
	static std::shared_ptr<trace_globals_t> globals;

	if(!globals){
		globals = std::make_shared<trace_globals_t>();
		globals->_current = &globals->_default_tracer;
	}

	return globals.get();
}


inline const trace_i& get_trace(){
	const auto g = get_global_data();

	const auto result = g->_current;
	assert(result);
	return *result;
}

inline void set_trace(const trace_i* v){
	const auto g = get_global_data();
	g->_current = v;
}




////////////////////////////		scoped_trace
/*
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/


#if QUARK_TRACE_ON
	struct scoped_trace {
		scoped_trace(const std::string& s, const trace_i& tracer) :
			_tracer(tracer),
			_trace_brackets(true)
		{
			_tracer.trace_i__open_scope((s + " {").c_str());
		}

		~scoped_trace(){
			if(_trace_brackets){
				_tracer.trace_i__close_scope("}");
			}
			else{
				_tracer.trace_i__close_scope("");
			}
		}

		private: const trace_i& _tracer;
		private: bool _trace_brackets = true;
	};

	////////////////////////////		Hook functions.
	/*
		These functions are called by the macros and they in turn call the runtime_i.
		TODO: Use only trace_context_t, runtime_i should contain a tracer.
	*/
	inline void on_trace_hook(const char s[], const trace_i& tracer){
		tracer.trace_i__trace(s);
	}

	//	Overloaded for char[] and std::string.
	inline void quark_trace_func(const char s[], const trace_i& tracer){
		::quark::on_trace_hook(s, tracer);
	}
	inline void quark_trace_func(const std::string& s, const trace_i& tracer){
		quark_trace_func(s.c_str(), tracer);
	}
	inline void quark_trace_func(const std::stringstream& s, const trace_i& tracer){
		quark_trace_func(s.str().c_str(), tracer);
	}

	#define QUARK_TRACE(s) ::quark::quark_trace_func(s, quark::get_trace())
	#define QUARK_TRACE_SS(x) {std::stringstream ss; ss << x; ::quark::quark_trace_func(ss, quark::get_trace());}
	#define QUARK_SCOPED_TRACE(s) ::quark::scoped_trace QUARK_UNIQUE_LABEL(scoped_trace) (s, quark::get_trace())
#else
	struct scoped_trace {
		scoped_trace(const std::string& s, const trace_i& tracer){
		}

		~scoped_trace(){
		}
	};

	#define QUARK_TRACE(s)
	#define QUARK_TRACE_SS(s)
	#define QUARK_SCOPED_TRACE(s)
#endif



	inline int get_log_indent(){
		const trace_i& tracer = get_trace();
		return tracer.trace_i__get_indent();
	}


//	UNIT TEST SUPPORT
//	====================================================================================================================


struct call_context_t {
	runtime_i* runtime;
	source_code_location location;
};

	typedef void (*unit_test_function)();


	////////////////////////////		unit_test_def
	/*
		The defintion of a single unit test, including the function itself.
	*/
	struct unit_test_def {
		unit_test_def(const std::string& source_file, int source_line, const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f, bool vip)
		:
			_source_file(source_file),
			_source_line(source_line),
			_class_under_test(p1),
			_function_under_test(p2),
			_scenario(p3),
			_expected_result(p4),
			_test_f(f),
			_vip(vip)
		{
		}

		////////////////		State.
			std::string _source_file;
			int _source_line;
			std::string _class_under_test;

			std::string _function_under_test;
			std::string _scenario;
			std::string _expected_result;

			unit_test_function _test_f;
			bool _vip;
	};


	////////////////////////////		unit_test_registry
	/*
		Stores all unit tests registered for the entire executable.
	*/
	struct unit_test_registry {
		public: std::vector<unit_test_def> _tests;
	};


	////////////////////////////		unit_test_rec
	/*
		This is part of an RAII-mechansim to register and unregister unit-tests.
	*/
	struct unit_test_rec {
		unit_test_rec(const std::string& source_file, int source_line, const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f, bool vip){
			unit_test_def test(source_file, source_line, p1, p2, p3, p4, f, vip);
			if(!registry_instance){
				registry_instance = new unit_test_registry();
			}
			registry_instance->_tests.push_back(test);
		}


		////////////////		State.

		//	!!! Singleton. ### lose this.
		static unit_test_registry* registry_instance;
	};


#if QUARK_UNIT_TESTS_ON


	////////////////////////////		Hooks


	void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]);


	////////////////////////////		Macros used by client code


	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_TEST(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(quark_test_f_), false); \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)()

	//	When one or more of these exists, no non_VIP tests are run. Let's you iterate on a broken test quickly and set breakpoints in etc.
	#define QUARK_TEST_VIP(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(quark_test_f_), true); \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)()

	#define QUARK_TESTQ(function_under_test, scenario) \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, "", function_under_test, scenario, "", QUARK_UNIQUE_LABEL(quark_test_f_), false); \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)()


	//### Add argument to unit-test functions that can be used / checked in UT_VERIFY().
	#define QUARK_UT_VERIFY(exp) if(exp){}else{ ::quark::on_unit_test_failed_hook(::quark::get_runtime(), ::quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(exp)); }
	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY

	//??? Use only this in the future! Deprecate QUARK_UT_VERIFY() and QUARK_TEST_VERIFY()
	#define QUARK_VERIFY QUARK_UT_VERIFY



inline void fail_test(const call_context_t& context){
	::quark::on_unit_test_failed_hook(
		context.runtime,
		context.location,
		""
	);
}

template <typename T> void ut_verify_auto(const quark::call_context_t& context, const T& result, const T& expected){
	if(result == expected){
	}
	else{
		QUARK_TRACE_SS("  result: " << result);
		QUARK_TRACE_SS("expected: " << expected);

		fail_test(context);
	}
}

inline void ut_verify(const quark::call_context_t& context, const std::string& result, const std::string& expected){
	if(result == expected){
	}
	else{
		QUARK_TRACE_SS("  result: " << result);
		QUARK_TRACE_SS("expected: " << expected);

		//	Show exactly where mismatch is.
		for(size_t pos = 0 ; pos < std::max(result.size(), expected.size()) ; pos++){
			const int result_int = pos < result.size() ? result[pos] : -1;
			const int expected_int = pos < expected.size() ? expected[pos] : -1;

			std::stringstream s;
			s << pos;
			s << "\t result:\t";
			if(result_int == -1){
				s << "<OUT OF BOUNDS>";
			}
			else{
				s << "\'" << std::string(1, result_int) << "\'";
				s << "\t" << result_int;
				s << "\t" << "0x" << std::hex << result_int << std::dec;
			}

			s << "\t expected:\t";
			if(expected_int == -1){
				s << "<OUT OF BOUNDS>";
			}
			else{
				s << "\'" << std::string(1, expected_int) << "\'";
				s << "\t" << expected_int;
				s << "\t" << "0x" << std::hex << expected_int << std::dec;
			}

			s << (result_int == expected_int ? "" : "\t=> DIFFERENT");
			QUARK_TRACE_SS(s.str());
		}
		fail_test(context);
	}
}
inline void ut_verify(const quark::call_context_t& context, const std::vector<std::string>& result, const std::vector<std::string>& expected){
	if(result != expected){
		if(result.size() != expected.size()){
			QUARK_TRACE("Vector are different sizes!");
		}
		const auto count = std::max(result.size(), expected.size());
		for(int i = 0 ; i < count ; i++){
			const auto result_str = (i < result.size()) ? result[i] : "---";
			const auto expected_str = (i < expected.size()) ? expected[i] : "---";
			QUARK_TRACE_SS(std::to_string(i) << ": \"" << result_str << "\" != \"" << expected_str << "\"");
		}

		quark::fail_test(context);
	}
}


inline void ut_verify(const call_context_t& context, const char* result, const char* expected){
	ut_verify(context, std::string(result), std::string(expected));
}
inline void ut_verify(const call_context_t& context, const std::string& result, const char* expected){
	ut_verify(context, result, std::string(expected));
}
inline void ut_verify(const call_context_t& context, const char* result, const std::string& expected){
	ut_verify(context, std::string(result), expected);
}

#else

	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_TEST(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)()

	#define QUARK_TESTQ(function_under_test, scenario) \
		static void QUARK_UNIQUE_LABEL(quark_test_f_)()

	#define QUARK_UT_VERIFY(exp)
	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY


inline void fail_test(const call_context_t& context){
}

template <typename T> void ut_verify_auto(const quark::call_context_t& context, const T& result, const T& expected){
}

inline void ut_verify(const quark::call_context_t& context, const std::string& result, const std::string& expected){
}
inline void ut_verify(const quark::call_context_t& context, const std::vector<std::string>& result, const std::vector<std::string>& expected){
}


inline void ut_verify(const call_context_t& context, const char* result, const char* expected){
}
inline void ut_verify(const call_context_t& context, const std::string& result, const char* expected){
}
inline void ut_verify(const call_context_t& context, const char* result, const std::string& expected){
}

#endif

	#define QUARK_POS quark::call_context_t{::quark::get_runtime(), ::quark::source_code_location(__FILE__, __LINE__)}


#if QUARK_UNIT_TESTS_ON


inline void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	on_problem___put_breakpoint_here();

	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_unit_test_failed(location, expression);
}

inline std::string path_to_name(const std::string& path){
	size_t pos = path.size();
	while(pos > 0 && path[pos - 1] != '/'){
		pos--;
	}
	return path.substr(pos);
}

inline bool run_test(const unit_test_def& test, bool oneline){
	std::stringstream testInfo;
	testInfo << test._source_file << ":" << std::to_string(test._source_line)
		<< " | " << test._class_under_test
		<< " | " << test._function_under_test
		<< " | " << test._scenario
		<< " | " << test._expected_result;

	try{
		if(oneline){
			std::cout << testInfo.str() << "...";
			test._test_f();
			std::cout << "OK" << std::endl;
			return true;
		}
		else{
			::quark::scoped_trace tracer(testInfo.str(), quark::get_trace());
			test._test_f();
			return true;
		}
	}
	catch(const std::exception& e){
		std::cout << "FAILURE: " + testInfo.str() << std::endl;
		std::cout << typeid(e).name() << std::endl;
		std::cout << std::string(e.what()) << std::endl;
		on_problem___put_breakpoint_here();
		return false;
	}
	catch(...){
		std::cout << "FAILURE: " + testInfo.str() << std::endl;
		on_problem___put_breakpoint_here();
		return false;
	}
}


inline std::vector<unit_test_def> sort_tests(const std::vector<unit_test_def>& tests, const std::vector<std::string>& source_file_order){
	std::vector<unit_test_def> ordered_tests;
	std::vector<unit_test_def> remaining_tests = tests;

	for(const auto& f: source_file_order){

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

inline int count_vip_tests(const std::vector<unit_test_def>& tests){
	int count = 0;
	for(const auto& e: tests){
		if(e._vip){
			count++;
		}
	}
	return count;
}

enum class test_result {
	k_run_failed,
	k_run_succeeded,
	k_not_run
};

inline void trace_failures(const std::vector<unit_test_def>& tests, const std::vector<test_result>& test_results){
	int index = 0;
	for(const auto& test: tests){
		if(test_results[index] == test_result::k_run_failed){

			std::stringstream testInfo;
			testInfo
				<< test._source_file << ":" << std::to_string(test._source_line)
				<< " | " << test._class_under_test
				<< " | " << test._function_under_test
				<< " | " << test._scenario
				<< " | " << test._expected_result;

			std::cout << testInfo.str() << std::endl;
		}
		index++;
	}
}

////////////////////////////		run_tests()
/*
	Client application calls this function run all unit tests.
	It will handle tracing and exceptions etc.
	On unit-test failure this function exits the executable.
*/
inline void run_tests(const unit_test_registry& registry, const std::vector<std::string>& source_file_order, bool oneline){
	const auto sorted_tests = sort_tests(registry._tests, source_file_order);
	const auto vip_count = count_vip_tests(sorted_tests);

	const auto total_test_count = registry._tests.size();

	if(vip_count > 0){
		std::vector<test_result> test_results;
		std::cout << "Running VIP tests: " << vip_count << " / " << total_test_count << std::endl;

		int fail_count = 0;
		for(const auto& test: sorted_tests){
			if(test._vip){
				bool success = run_test(test, oneline);
				if(success == false){
					fail_count++;
					test_results.push_back(test_result::k_run_failed);
				}
				else{
					test_results.push_back(test_result::k_not_run);
				}
			}
			else{
				test_results.push_back(test_result::k_not_run);
			}
		}

		std::cout << "================================================================================" << std::endl;
		if(fail_count == 0){
			std::cout << "VIP mode: Success all " << vip_count << " passed" << std::endl;
		}
		else{
			std::cout << "VIP mode: Failure " << fail_count << " / " << vip_count << std::endl;
			trace_failures(sorted_tests, test_results);
			exit(-1);
		}
	}
	else{
		std::vector<test_result> test_results;
		std::cout << "Running tests: " << total_test_count << std::endl;

		int fail_count = 0;
		for(const auto& test: sorted_tests){
			bool success = run_test(test, oneline);
			if(success == false){
				fail_count++;
				test_results.push_back(test_result::k_run_failed);
			}
			else{
				test_results.push_back(test_result::k_run_succeeded);
			}
		}

		std::cout << "================================================================================" << std::endl;
		if(fail_count == 0){
			std::cout << "Success all " << sorted_tests.size() << " tests" <<std::endl;
		}
		else{
			std::cout << "Failure " << fail_count << " of all (" << total_test_count << ")" << std::endl;
			trace_failures(sorted_tests, test_results);
			exit(-1);
		}
	}
}

inline void run_tests(const std::vector<std::string>& source_file_order, bool oneline){
	QUARK_ASSERT(unit_test_rec::registry_instance != nullptr);
	run_tests(*unit_test_rec::registry_instance, source_file_order, oneline);
}

#endif


//	Default implementation
//	====================================================================================================================


//////////////////////////////////			default_runtime
/*
	This is a default implementation that client can chose to instantiate and plug-in using set_runtime().

	It uses cout.
*/
struct default_runtime : public runtime_i {
	default_runtime(const std::string& test_data_root);

	public: virtual void runtime_i__on_assert(const source_code_location& location, const char expression[]);
	public: virtual void runtime_i__on_unit_test_failed(const source_code_location& location, const char expression[]);


	///////////////		State.
		const std::string _test_data_root;
};


}	//	quark

#endif
