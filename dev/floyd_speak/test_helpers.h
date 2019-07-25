//
//  test_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef test_helpers_hpp
#define test_helpers_hpp

#include "ast_value.h"
#include <string>
#include <vector>

#include "quark.h"
#include "compiler_helpers.h"

namespace floyd {

struct compilation_unit_t;

struct test_report_t {
	floyd::value_t result_variable;
	run_output_t output;
	std::vector<std::string> print_out;
	std::string exception_what;
};

inline bool compare(const test_report_t& lhs, const test_report_t& rhs, bool check_printout){
	if(check_printout){
		return
			lhs.result_variable == rhs.result_variable
			&& lhs.output == rhs.output
			&& lhs.print_out == rhs.print_out
			&& lhs.exception_what == rhs.exception_what;
	}
	else{
		return
			lhs.result_variable == rhs.result_variable
			&& lhs.output == rhs.output
			&& lhs.exception_what == rhs.exception_what;
	}
}
inline bool operator==(const test_report_t& lhs, const test_report_t& rhs){
	return compare(lhs, rhs, true);
}

inline test_report_t check_nothing(){
	return test_report_t { value_t::make_undefined(), { -1, {} }, {}, "" };
}

inline test_report_t check_result(const value_t& expected){
	return test_report_t { expected, { -1, {} }, {}, "" };
}
inline test_report_t check_printout(const std::vector<std::string>& print_out){
	return test_report_t { value_t::make_undefined(), { -1, {} }, print_out, "" };
}
inline test_report_t check_main_return(int64_t main_return){
	return test_report_t { value_t::make_undefined(), { main_return, {} }, {}, "" };
}
inline test_report_t check_exception(const std::string& exception_what){
	return test_report_t { value_t::make_undefined(), { -1, {} }, {}, exception_what };
}


void ut_verify_report(const quark::call_context_t& context, const test_report_t& result, const test_report_t& expected);


void test_floyd(const quark::call_context_t& context, const compilation_unit_t& cu, const std::vector<std::string>& main_args, const test_report_t& expected, bool check_printout);


void ut_verify_global_result_as_json(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_json);
void ut_verify_global_result_as_json_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_json);

void ut_verify_global_result_lib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result);
void ut_verify_global_result_nolib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result);


void ut_verify_printout_lib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout);
void ut_verify_printout_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout);

//	Has no output value: only compilation errors or floyd-asserts.
void ut_run_closed_nolib(const std::string& program);
void ut_run_closed_lib(const std::string& program);

void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& args, int64_t expected_return);

void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what);


} // floyd

#endif /* test_helpers_hpp */
