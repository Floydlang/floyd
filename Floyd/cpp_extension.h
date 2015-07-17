//
//  cpp_extension.h
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-16.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef steady_cpp_extension_h
#define steady_cpp_extension_h

#include <cassert>
#include <vector>
#include <string>
#include <sstream>


////////////////////////////		Function prototyp for client unit test.


typedef void (*unit_test_function)();


////////////////////////////		Macros


#define PP_STRING(a) #a
#define PP_CONCAT2(a,b)  a##b
#define PP_CONCAT3(a, b, c) a##b##c
#define PP_UNIQUE_LABEL_INTERNAL(prefix, suffix) PP_CONCAT2(prefix, suffix)
#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_INTERNAL(prefix, __LINE__)
//#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_INTERNAL(PP_UNIQUE_LABEL_INTERNAL(prefix, __LINE__), __FILE__)
//#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_INTERNAL(prefix, __COUNTER__)
//#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_INTERNAL(PP_UNIQUE_LABEL_INTERNAL(prefix, __LINE__), PP_UNIQUE_LABEL_INTERNAL(abcd, HashString(__FILE__)))

/*
// boiler-plate
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)

// per-transform type
#define GL_TRANSLATE_DETAIL(n, x, y, z) GlTranslate n(x, y, z)
#define GL_TRANSLATE(x, y, z) GL_TRANSLATE_DETAIL(MAKE_UNIQUE(_trans_), x, y, z)
*/


////////////////////////////		TUnitTestDefinition


/**
	The defintion of a single unit test, including the function itself.
*/

struct TUnitTestDefinition {
	TUnitTestDefinition(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f)
	:
		_class_under_test(p1),
		_function_under_test(p2),
		_scenario(p3),
		_expected_result(p4),
		_test_f(f)
	{
	}


	////////////////		State.
		std::string _class_under_test;

		std::string _function_under_test;
		std::string _scenario;
		std::string _expected_result;

		unit_test_function _test_f;
};


////////////////////////////		TUnitTestDefinition


/**
	Stores all unit tests registered for the entire executable.
*/

struct TUniTestRegistry {
	public: std::vector<const TUnitTestDefinition> fTests;
};


////////////////////////////		TUnitTestDefinition


/**
	This is part of an RAII-mechansim to register and unregister unit-tests.
*/

struct TUnitTestReg {
	static TUniTestRegistry* gRegistry;

	TUnitTestReg(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f){
		TUnitTestDefinition test(p1, p2, p3, p4, f);
		if(!gRegistry){
			gRegistry = new TUniTestRegistry();
		}
		gRegistry->fTests.push_back(test);
	}
};


////////////////////////////		TSourceLocation


/**
	Value-object that specifies a specific line of code in a specific source file.
*/



struct TSourceLocation {
	TSourceLocation(const char iSourceFile[], long iLineNumber) :
		fLineNumber(iLineNumber)
	{
		assert(iSourceFile != nullptr);
		assert(std::strlen(iSourceFile) <= 1024);
		strcpy(fSourceFile, iSourceFile);
	}

	char fSourceFile[1024 + 1];
	long fLineNumber;
};


////////////////////////////		icppextension_runtime


/**
	Interface class so client executable can hook-in behaviors for basics like logging and asserts.
*/


class icppextension_runtime {
	public: virtual ~icppextension_runtime(){};
	public: virtual void icppextension_runtime__trace(const char s[]) = 0;
	public: virtual void icppextension_runtime__add_log_indent(long iAdd) = 0;
	public: virtual void icppextension_runtime__on_assert(const TSourceLocation& iLocation, const char iExpression[]) = 0;
	public: virtual void icppextension_runtime__on_unit_test_failed(const TSourceLocation& iLocation, const char s[]) = 0;

	/**
		Use 7-bit english text as lookup key.
		Returns localized utf8 text.
		Basic implementations can chose to just return the lookup key.
		addKey is additional characters to use for lookup, but that is not part of the actual returned-text if no localization exists.
	*/
/*
	public: virtual std::string icppextension_runtime__lookup_text(const TSourceLocation& iLocation,
		const int locale,
		const char englishLookupKey[],
		const char addKey[]) = 0;

*/
//	public: virtual std::string icppextension_get_test_data_root(const char iModuleUnderTest[]) = 0;
//	public: virtual void icppextension_runtime__on_dbc_precondition_failed(const char s[]) = 0;
//	public: virtual void icppextension_runtime__on_dbc_postcondition_failed(const char s[]) = 0;
//	public: virtual void icppextension_runtime__on_dbc_invariant_failed(const char s[]) = 0;
};


////////////////////////////		GetRuntime() and SetRuntime()

/**
	Global functions for storing the current runtime.
	Notice that only the macros use these! The implementation does NOT.
*/

icppextension_runtime* GetRuntime();
void SetRuntime(icppextension_runtime* iRuntime);


////////////////////////////		CScopedTrace

/**
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/

struct CScopedTrace {
	CScopedTrace(const char s[]){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__trace(s);
		r->icppextension_runtime__trace("{");
		r->icppextension_runtime__add_log_indent(1);
	}
	CScopedTrace(const std::string& s){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__trace(s.c_str());
		r->icppextension_runtime__trace("{");
		r->icppextension_runtime__add_log_indent(1);
	}
	CScopedTrace(const std::stringstream& s){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__trace(s.str().c_str());
		r->icppextension_runtime__trace("{");
		r->icppextension_runtime__add_log_indent(1);
	}
	~CScopedTrace(){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__add_log_indent(-1);
		r->icppextension_runtime__trace("}");
	}
};


////////////////////////////		CScopedIndent


/**
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/

struct CScopedIndent {
	CScopedIndent(){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__add_log_indent(1);
	}
	~CScopedIndent(){
		icppextension_runtime* r = GetRuntime();
		r->icppextension_runtime__add_log_indent(-1);
	}
};

////////////////////////////		Hook functions.

/**
	These functions are called by the macros and they in turn call the icppextension_runtime.
*/


void OnTraceHook(icppextension_runtime* iRuntime, const char iS[]);
void OnTraceHook(icppextension_runtime* iRuntime, const std::string& iS);
void OnTraceHook(icppextension_runtime* iRuntime, const std::stringstream& iS);
void OnAssertHook(icppextension_runtime* iRuntime, const TSourceLocation& iLocation, const char iExpression[]) __dead2;
void OnUnitTestFailedHook(icppextension_runtime* iRuntime, const TSourceLocation& iLocation, const char iExpression[]);

std::string OnGetPrivateTestDataPath(icppextension_runtime* iRuntime, const char iModuleUnderTest[], const char iSourceFilePath[]);


////////////////////////////		Hook functions.

/**
	Client application calls this function run all unit tests.
	It will handle tracing and exceptions etc.
	On unit-test failure this function exits the executable.
*/
void run_tests();



////////////////////////////		Client-API - macros




//	### Use runtime as arg.
#define TRACE(s) ::OnTraceHook(::GetRuntime(), s)
#define TRACE_SS(x) {std::stringstream ss; ss << x; ::OnTraceHook(::GetRuntime(), ss);}

/**
	Works with:
		char[]
		std::string
*/
#define SCOPED_TRACE(s) CScopedTrace PP_UNIQUE_LABEL(scoped_trace) (s)
#define SCOPED_INDENT() CScopedIndent PP_UNIQUE_LABEL(scoped_indent)

#define TRACE_FUNCTION() CScopedTrace PP_UNIQUE_LABEL(trace_function) (__FUNCTION__)


#define ASSERT(x) if(x){}else {OnAssertHook(::GetRuntime(), TSourceLocation(__FILE__, __LINE__), PP_STRING(x)); }


#define UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
	static void PP_UNIQUE_LABEL(fun_)(); \
	static TUnitTestReg PP_UNIQUE_LABEL(rec)(class_under_test, function_under_test, scenario, expected_result, PP_UNIQUE_LABEL(fun_)); \
	static void PP_UNIQUE_LABEL(fun_)()

//### Add argument to unit-test functions that can be used / checked in UT_VERIFY().
#define UT_VERIFY(exp) if(exp){}else{ OnUnitTestFailedHook(::GetRuntime(), TSourceLocation(__FILE__, __LINE__), PP_STRING(exp)); }


/**
	gives you native, absolute path to your modules test-directory.
*/
//#define UNIT_TEST_PRIVATE_DATA_PATH(moduleUnderTest) OnGetPrivateTestDataPath(GetRuntime(), moduleUnderTest, __FILE__)




//////////////////////////////////			TDefaultRuntime

/**
	This is a default implementation that client can chose to instantiate and plug-in using SetRuntime().

	It uses cout.
*/

struct TDefaultRuntime : public icppextension_runtime {
	TDefaultRuntime(const std::string& iTestDataRoot);

	public: virtual void icppextension_runtime__trace(const char s[]);
	public: virtual void icppextension_runtime__add_log_indent(long iAdd);
	public: virtual void icppextension_runtime__on_assert(const TSourceLocation& iLocation, const char iExpression[]);
	public: virtual void icppextension_runtime__on_unit_test_failed(const TSourceLocation& iLocation, const char iExpression[]);
//	public: virtual std::string icppextension_get_test_data_root(const char iModuleUnderTest[]);


	///////////////		State.
		const std::string fTestDataRoot;
		long fIndent;
};


#endif
