//
//  cpp_extension.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-22.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <sstream>

//#include "Paths.h"


TUniTestRegistry* TUnitTestReg::gRegistry = nullptr;


namespace {
	icppextension_runtime* gRuntimePtr = nullptr;
}


icppextension_runtime* GetRuntime(){
	return gRuntimePtr;
}

void SetRuntime(icppextension_runtime* iRuntime){
	gRuntimePtr = iRuntime;
}




void OnTraceHook(icppextension_runtime* iRuntime, const char iS[]){
	assert(iRuntime != nullptr);
	assert(iS != nullptr);

	iRuntime->icppextension_runtime__trace(iS);
}

void OnTraceHook(icppextension_runtime* iRuntime, const std::string& iS){
	assert(iRuntime != nullptr);

	iRuntime->icppextension_runtime__trace(iS.c_str());
}

void OnTraceHook(icppextension_runtime* iRuntime, const std::stringstream& iS){
	assert(iRuntime != nullptr);

	iRuntime->icppextension_runtime__trace(iS.str().c_str());
}

void OnAssertHook(icppextension_runtime* iRuntime, const TSourceLocation& iLocation, const char iExpression[]){
	assert(iRuntime != nullptr);
	assert(iExpression != nullptr);

	iRuntime->icppextension_runtime__on_assert(iLocation, iExpression);
	exit(-1);
}

void OnUnitTestFailedHook(icppextension_runtime* iRuntime, const TSourceLocation& iLocation, const char iExpression[]){
	assert(iRuntime != nullptr);
	assert(iExpression != nullptr);

	iRuntime->icppextension_runtime__on_unit_test_failed(iLocation, iExpression);
}

/*
std::string OnGetPrivateTestDataPath(icppextension_runtime* iRuntime, const char iModuleUnderTest[], const char iSourceFilePath[]){
	ASSERT(iRuntime != nullptr);
	ASSERT(iModuleUnderTest != nullptr);
	ASSERT(iSourceFilePath != nullptr);

	const TAbsolutePath absPath(iSourceFilePath);
	const TAbsolutePath parent = GetParent(absPath);
	return parent.GetStringPath();
}
*/




void run_tests(){
	TRACE_FUNCTION();
	ASSERT(TUnitTestReg::gRegistry != nullptr);

	std::size_t test_count = TUnitTestReg::gRegistry->fTests.size();
//	std::size_t fail_count = 0;

	TRACE_SS("Running " << test_count << " tests...");

	for(std::size_t i = 0 ; i < test_count ; i++){
		const TUnitTestDefinition& test = TUnitTestReg::gRegistry->fTests[i];

		std::stringstream testInfo;
		testInfo << "Test #" << i
			<< " " << test._class_under_test
			<< " | " << test._function_under_test
			<< " | " << test._scenario
			<< " | " << test._expected_result;

		try{
			SCOPED_TRACE(testInfo.str());
			test._test_f();
		}
		catch(...){
			TRACE("FAILURE: " + testInfo.str());
//			fail_count++;
			exit(-1);
		}
	}

//	if(fail_count == 0){
		TRACE_SS("Success - " << test_count << " tests!");
//	}
//	else{
//		TRACE_SS("FAILED " << fail_count << " out of " << test_count << " tests!");
//		exit(-1);
//	}
}



//////////////////////////////////			TDefaultRuntime





TDefaultRuntime::TDefaultRuntime(const std::string& iTestDataRoot) :
	fTestDataRoot(iTestDataRoot),
	fIndent(0)
{
}

void TDefaultRuntime::icppextension_runtime__trace(const char s[]){
//		for (auto &i: items){
//		}
	for(long i = 0 ; i < fIndent ; i++){
		std::cout << "|\t";
	}

	std::cout << std::string(s);
	std::cout << std::endl;
}

void TDefaultRuntime::icppextension_runtime__add_log_indent(long iAdd){
	fIndent += iAdd;
}

void TDefaultRuntime::icppextension_runtime__on_assert(const TSourceLocation& iLocation, const char iExpression[]){
	TRACE_SS(std::string("Assertion failed ") << iLocation.fSourceFile << ", " << iLocation.fLineNumber << " \"" << iExpression << "\"");
	perror("perror() says");
	throw std::logic_error("assert");
}

void TDefaultRuntime::icppextension_runtime__on_unit_test_failed(const TSourceLocation& iLocation, const char iExpression[]){
	TRACE_SS("Unit test failed " << iLocation.fSourceFile << ", " << iLocation.fLineNumber << " \"" << iExpression << "\"");
	perror("perror() says");

	throw std::logic_error("Unit test failed");
}




