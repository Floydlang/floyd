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



#ifndef quark_h
#define quark_h


/*
# QUARK - THE C++ SUPER GLUE

It's problematic in C++ to build code separately then compose them together into bigger programs and libraries.
Some common low-level infrastructure is missing so the library-maker must make assumptions or decisions that
really the *user* of the library should control.

How are defects are detected, how do you add and run tests etc?


Quark attempts to be the only "library" you need to use in each of your library, then help you compose libraries
together neatly. Like super glue :-)

Quark is designed to be a policy rather than actual code. These are the primitives that makes up that policy:

	QUARK_ASSERT(x)

	QUARK_TRACE(x)
	QUARK_TRACE_SS(x)
	QUARK_SCOPED_TRACE(x)

	QUARK_UNIT_TEST
	QUARK_TEST_VERIFY(x)

Using quark.h and quark.cpp is very convenient but optional.




# CONFIGURATION

Configure quark.h / quark.cpp by settings these flags in your compiler settings:

	#define QUARK_ASSERT_ON true
	#define QUARK_TRACE_ON true
	#define QUARK_UNIT_TESTS_ON true

They are independent of each other and any combination is valid! If you don't set them, they default to the above.

See "EVEN MORE INDEPENDENCE" for more advanced possibilities.




# ASSERTS - DEFECTS

Assert is used for finding runtime defects. Use QUARK_ASSERT_ON to enable / disable all asserts. You can hook what
happens when an assert fails - to exit program, throw an exception etc.

QUARK_ASSERT(x)
When expression x evaluates to false, there is a defect in the code. Compiled-out if QUARK_ASSERT_ON is false.

Examples 1 - check function arguments:

	char* my_strlen(const char* s){
		QUARK_ASSERT(s != nullptr);
		...
	}

Example 2 - detect impossible conditions:

	...
	if(a == 0){
	}
	else if (a == 3){
	}
	else {
		QUARK_ASSERT(false);
		throw std::logic_error("");	//	Remove missing-return value warning, if any.
	}



# TRACING

Quark has primitives for tracing that can be routed and enabled / disabled. Includes support for indenting the log.
Use QUARK_TRACE_ON to enable / disable tracing.

QUARK_TRACE(x)
Trace a C-string

QUARK_TRACE_SS(x)
Trace a string using stream syntax.

QUARK_SCOPED_TRACE
Trace a title and opening bracket, then indent everything afterwards until we leave the stack scope.

Examples:

	Example 1 - trace a C string

		QUARK_TRACE("abc");

	Example 2 - trace using stream style code:

		QUARK_TRACE_SS("my value" << 123);

	Example 3 - trace an indented section with a title:
		{
			QUARK_SCOPED_TRACE("File contents");
			QUARK_TRACE("inside 1 ");
			QUARK_TRACE("inside 2");
		}

		result:
			...
			...
			File contents
			{
				inside 1
				inside 2
			}
			...
			...




# UNIT TESTS

You can easily add a unit test where ever you can define a function. It's possible to interleave the functions with
the unit tests that excercise the functions. You supply 4 strings to each test like this:

	QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result){
	}

In your test you check for failure / success using QUARK_TEST_VERIFY(x).
Do not use QUARK_ASSERT to check for test failures! You want to run unit tests even when asserts are disabled.

Examples 1 - registering some unit tests

	QUARK_UNIT_TEST("std::list, "list()", "basic construction", "no exceptions"){
		std::list<int> a;
	}

	QUARK_UNIT_TEST("std::list, "find()", "empty list", "returns list.end()"){
		std::list<int> a;
		std::list<int>::const_iterator found_it = a.find(123);
		QUARK_TEST_VERIFY(found_it == a.end());
	}

Running all unit tests:
	quark::run_tests();




# EVEN MORE INDEPENDENCE

If you use (for example) QUARK_ASSERT() in all your code you can later decide in your final program if to
enable / disbale all asserts and how to handle an assert. This is great.

But an even more flexible solution is to use your own version of the Quark macros for each library, like this:


	MY_LIBRARY_ASSERT(x)

	MY_LIBRARY_TRACE(x)
	MY_LIBRARY_TRACE_SS(x)
	MY_LIBRARY_SCOPED_TRACE(x)

	MY_LIBRARY_UNIT_TEST
	MY_LIBRARY_TEST_VERIFY(x)


	TETRIS_CLONE_ASSERT(x)

	TETRIS_CLONE_TRACE(x)
	TETRIS_CLONE_TRACE_SS(x)
	TETRIS_CLONE_SCOPED_TRACE(x)

	TETRIS_CLONE_UNIT_TEST
	TETRIS_CLONE_TEST_VERIFY(x)

In your "my library" code you consistently use MY_LIBRARY_ASSERT(x), not QUARK_ASSERT(x).

This allows you to decide in clients to your library how to handle asserts, tracing etc on a per-library level! Or just:

	TETRIS_CLONE_ASSERT(x) QUARK_ASSERT(x)
	...
This becomes the chose of the *user* of your code.




# TODO - SOMEDAY

Fix so you can put your unit tests inside unnamed namespaces (to make tests internal), like in an internals-namespace.

Add basic exception classes, ala bacteria.
	steady::not_found
	steady::call_sequence_error
	steady::read_error
	steady::bad_format
	steady::timeout

Add hook for looking up text strings for easy translation.
		/	Use 7-bit english text as lookup key.
		//	Returns localized utf8 text.
		//	Basic implementations can chose to just return the lookup key.
		//	addKey is additional characters to use for lookup, but that is not part of the actual returned-text if
		//	no localization exists.
		public: virtual std::string runtime_i__lookup_text(const source_code_location& location,
			const int locale,
			const char englishLookupKey[],
			const char addKey[]) = 0;

Add support for design-by-contract:
	public: virtual void runtime_i__on_dbc_precondition_failed(const char s[]) = 0;
	public: virtual void runtime_i__on_dbc_postcondition_failed(const char s[]) = 0;
	public: virtual void runtime_i__on_dbc_invariant_failed(const char s[]) = 0;

Add mechanism for unit tests to get to test files.
	//	gives you native, absolute path to your modules test-directory.
	#define UNIT_TEST_PRIVATE_DATA_PATH(moduleUnderTest) OnGetPrivateTestDataPath(get_runtime(), moduleUnderTest, __FILE__)
	std::string OnGetPrivateTestDataPath(runtime_i* iRuntime, const char module_under_test[], const char source_file_path[]);

	public: virtual std::string icppextension_get_test_data_root(const char iModuleUnderTest[]) = 0;
*/


/* SB: needed to ignore warnings for unchecked vectors and deprecated functions such as strcpy() */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif



#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>


#ifndef QUARK_ASSERT_ON
	#define QUARK_ASSERT_ON 1
#endif

#ifndef QUARK_TRACE_ON
	#define QUARK_TRACE_ON 1
#endif

#ifndef QUARK_UNIT_TESTS_ON
	#define QUARK_UNIT_TESTS_ON 1
#endif



namespace quark {




//	PRIMITIVES
//	====================================================================================================================



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
	Interface class so client can hook-in behaviors for basics like logging and asserts.
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



//	ASSERT SUPPORT
//	====================================================================================================================



#if QUARK_ASSERT_ON

	#if _MSC_VER
		void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]);
	#else
		void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]) __dead2;
	#endif

	#define QUARK_ASSERT(x) if(x){}else {::quark::on_assert_hook(::quark::get_runtime(), quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(x)); }

#else

	#define QUARK_ASSERT(x)

#endif




inline void on_problem___put_breakpoint_here(){
}

#if QUARK_ASSERT_ON

//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS

inline void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	on_problem___put_breakpoint_here();

	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_assert(location, expression);
	exit(-1);
}

#endif








//	TRACE
//	====================================================================================================================




////////////////////////////		trace_i


struct trace_i {
	public: virtual ~trace_i(){};
	public: virtual void trace_i__trace(const char s[]) const = 0;
	public: virtual void trace_i__open_scope(const char s[]) const = 0;
	public: virtual void trace_i__close_scope(const char s[]) const = 0;
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


	///////////////		State.
	public: long mutable _indent;
};

inline default_tracer_t::default_tracer_t() :
	_indent(0)
{
}

inline void default_tracer_t::trace_i__trace(const char s[]) const{
	if(strlen(s) > 0){
		for(long i = 0 ; i < _indent ; i++){
			std::cout << "|\t";
		}

		std::cout << std::string(s);
		std::cout << std::endl;
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


struct trace_globals_t {
	trace_globals_t() :
		_current(nullptr)
	{
	}
	public: const default_tracer_t _default_tracer;
	public: const trace_i* _current;
};

inline trace_globals_t* get_global_data(){
	static trace_globals_t globals;
	globals._current = & globals._default_tracer;

	return &globals;
}


inline const trace_i* get_trace(){
	const auto g = get_global_data();

	const auto result = g->_current;
	assert(result);
	return result;
}

inline void set_trace(const trace_i* v){
	const auto g = get_global_data();
	g->_current = v;
}



////////////////////////////		trace_context_t

//	Notice: this is CONST but has hidden side effects. I decided this is OK.


struct trace_context_t {
	public: bool _verbose;
	public: const trace_i* _tracer;


	public: trace_context_t(bool verbose, const trace_i* tracer) :
		_verbose(verbose),
		_tracer(tracer)
	{
	}

	public: void trace(const char s[]) const{
		if(_verbose && strlen(s) > 0){
			_tracer->trace_i__trace(s);
		}
	}
	public: void open_scope(const char s[]) const{
		_tracer->trace_i__open_scope(_verbose ? s : "");
	}
	public: void close_scope(const char s[]) const{
		_tracer->trace_i__close_scope(_verbose ? s : "");
	}
};



inline trace_context_t make_default_tracer(){
	return trace_context_t(true, get_trace());
}


////////////////////////////		scoped_trace
/*
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/


#if QUARK_TRACE_ON
	struct scoped_trace {
		scoped_trace(const std::string& s, const trace_context_t* tracer) :
			_tracer(tracer == nullptr ? make_default_tracer() : *tracer),
			_trace_brackets(true)
		{
			_tracer.open_scope((s + " {").c_str());
		}

		~scoped_trace(){
			if(_trace_brackets){
				_tracer.close_scope("}");
			}
			else{
				_tracer.close_scope("");
			}
		}

		private: const trace_context_t _tracer;
		private: bool _trace_brackets = true;
	};
#else
	struct scoped_trace {
		scoped_trace(const std::string& s, const trace_context_t* tracer){
		}

		~scoped_trace(){
		}
	};
#endif



#if QUARK_TRACE_ON

	////////////////////////////		Hook functions.
	/*
		These functions are called by the macros and they in turn call the runtime_i.
		TODO: Use only trace_context_t, runtime_i should contain a tracer.
	*/
	inline void on_trace_hook(const char s[], const trace_context_t& tracer){
		tracer.trace(s);
	}

	//	Overloaded for char[] and std::string.
	inline void quark_trace_func(const char s[], const trace_context_t* tracer){
		if(tracer == nullptr){
			const auto def = make_default_tracer();
			return quark_trace_func(s, &def);
		}
		else{
			::quark::on_trace_hook(s, *tracer);
		}
	}
	inline void quark_trace_func(const std::string& s, const trace_context_t* tracer){
		quark_trace_func(s.c_str(), tracer);
	}
	inline void quark_trace_func(const std::stringstream& s, const trace_context_t* tracer){
		quark_trace_func(s.str().c_str(), tracer);
	}

	#define QUARK_TRACE(s) ::quark::quark_trace_func(s, nullptr)
	#define QUARK_TRACE_SS(x) {std::stringstream ss; ss << x; ::quark::quark_trace_func(ss, nullptr);}
	#define QUARK_SCOPED_TRACE(s) ::quark::scoped_trace QUARK_UNIQUE_LABEL(scoped_trace) (s, nullptr)

	#define QUARK_CONTEXT_TRACE(context, s) ::quark::quark_trace_func(s, &context)
	#define QUARK_CONTEXT_TRACE_SS(context, x) {std::stringstream ss; ss << x; ::quark::quark_trace_func(ss, &context);}
	#define QUARK_CONTEXT_SCOPED_TRACE(context, s) ::quark::scoped_trace QUARK_UNIQUE_LABEL(scoped_trace) (s, &context)
#else
	#define QUARK_TRACE(s)
	#define QUARK_TRACE_SS(s)
	#define QUARK_SCOPED_TRACE(s)

	#define QUARK_CONTEXT_TRACE(context, s)
	#define QUARK_CONTEXT_TRACE_SS(context, s)
	#define QUARK_CONTEXT_SCOPED_TRACE(context, s)
#endif




	inline int get_log_indent(){
		return 0;
	}








//	UNIT TEST SUPPORT
//	====================================================================================================================





#if QUARK_UNIT_TESTS_ON

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
			if(!_registry_instance){
				_registry_instance = new unit_test_registry();
			}
			_registry_instance->_tests.push_back(test);
		}


		////////////////		State.

		//	!!! Singleton. ### lose this.
		static unit_test_registry* _registry_instance;
	};


	////////////////////////////		Hooks


	void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]);


	////////////////////////////		run_tests()
	/*
		Client application calls this function run all unit tests.
		It will handle tracing and exceptions etc.
		On unit-test failure this function exits the executable.
	*/
	void run_tests(const std::vector<std::string>& source_file_order);


	////////////////////////////		Macros used by client code


	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(cppext_unit_test_), false); \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	//	When one or more of these exists, no non_VIP tests are run. Let's you iterate on a broken test quickly and let breakpoints in etc.
	#define QUARK_UNIT_TEST_VIP(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(cppext_unit_test_), true); \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	#define QUARK_UNIT_TESTQ(function_under_test, scenario) \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, "", function_under_test, scenario, "", QUARK_UNIQUE_LABEL(cppext_unit_test_), false); \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	#define QUARK_UNIT_1(function_under_test, scenario, test_expression) \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(__FILE__, __LINE__, "", function_under_test, scenario, "", QUARK_UNIQUE_LABEL(cppext_unit_test_), false); \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(){ if(test_expression){}else{ ::quark::on_unit_test_failed_hook(::quark::get_runtime(), ::quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(exp)); } }


	//### Add argument to unit-test functions that can be used / checked in UT_VERIFY().
	#define QUARK_UT_VERIFY(exp) if(exp){}else{ ::quark::on_unit_test_failed_hook(::quark::get_runtime(), ::quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(exp)); }

	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY


	#define QUARK_UT_VERIFY2(result, expected) ut_compare2((result), (expected), ::quark::source_code_location(__FILE__, __LINE__))


template <typename T> void ut_compare(const T& result, const T& expected){
	if(!(result == expected)){
		::quark::on_unit_test_failed_hook(
			::quark::get_runtime(),
			::quark::source_code_location(__FILE__, __LINE__),
			QUARK_STRING(result) " != " QUARK_STRING(expect)
		);
	}
}

template <typename T> std::string to_debug_str(const T& v);

template <typename T> void ut_compare2(const T& result, const T& expected, const source_code_location& code_location){
	if(!(result == expected)){
		const std::string result_str = to_debug_str(result);
		const std::string expected_str = to_debug_str(expected);

		QUARK_SCOPED_TRACE("Test failed");

		QUARK_TRACE("  Result:");
		QUARK_TRACE(result_str);
		QUARK_TRACE("Expected:");
		QUARK_TRACE(expected_str);

		::quark::on_unit_test_failed_hook(
			::quark::get_runtime(),
			code_location,
			(result_str + " != " + expected_str).c_str()
		);
	}
}

inline void ut_compare_strings(const std::string& result, const std::string& expected){
	if(result != expected){
		QUARK_TRACE_SS("  result:" << result);
		QUARK_TRACE_SS("expected:" << expected);

		for(size_t pos = 0 ; pos < std::max(result.size(), expected.size()) ; pos++){
			const int ach = pos < result.size() ? result[pos] : 1000;
			const int bch = pos < expected.size() ? expected[pos] : 1000;
			QUARK_TRACE_SS("" << pos << ": " << ach << " : " << bch << " " << (ach == bch ? "" : "DIFFERENT"))
		}

		::quark::on_unit_test_failed_hook(
			::quark::get_runtime(),
			::quark::source_code_location(__FILE__, __LINE__),
			(result + " != " + expected).c_str()
		);
	}
}


#else

	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
		void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	#define QUARK_UT_VERIFY(exp)
	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY

#endif


	#define OFF_QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
		void QUARK_UNIQUE_LABEL(cppext_unit_test_)()
	#define OFF_QUARK_UNIT_TEST_VIP(class_under_test, function_under_test, scenario, expected_result) \
		void QUARK_UNIQUE_LABEL(cppext_unit_test_)()



#if QUARK_UNIT_TESTS_ON

//	!!!! SET A BREAK POINT HERE USING YOUR DEBUGGER TO INSPECT ANY ASSERTS

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

inline int run_test_list(const std::vector<unit_test_def>& tests){
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

inline std::vector<unit_test_def> prepare_test_list(const std::vector<unit_test_def>& tests, const std::vector<std::string>& source_file_order){
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


inline void run_tests(const unit_test_registry& registry, const std::vector<std::string>& source_file_order){
	QUARK_SCOPED_TRACE(__FUNCTION__);

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

inline void run_tests(const std::vector<std::string>& source_file_order){
	QUARK_ASSERT(unit_test_rec::_registry_instance != nullptr);
	run_tests(*unit_test_rec::_registry_instance, source_file_order);
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
