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

struct run_report_t {
	floyd::value_t result_variable;
	floyd::value_t main_result;
	std::vector<std::string> print_out;
	std::string exception_what;
};

run_report_t make_result(const value_t& result);

void ut_verify(const quark::call_context_t& context, const run_report_t& result, const run_report_t& expected);

run_report_t run_program(const compilation_unit_t& cu, const std::vector<value_t>& main_args);

std::map<std::string, value_t> test_run_container2(const std::string& program, const std::vector<std::string>& args, const std::string& container_key, const std::string& source_file);


void ut_verify_global_result(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const value_t& expected_result);

void ut_verify_global_result_as_json(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_json);
inline void ut_verify_global_result_as_json_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_json){
	ut_verify_global_result_as_json(context, program, compilation_unit_mode::k_no_core_lib, expected_json);
}


void ut_verify_printout(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<std::string>& printout);

//	Has no output value: only compilation errors or floyd-asserts.
void ut_run_closed(const std::string& program, compilation_unit_mode mode);

void ut_verify_mainfunc_return(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<floyd::value_t>& args, const value_t& expected_return);
inline void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<floyd::value_t>& args, const value_t& expected_return){
	ut_verify_mainfunc_return(context, program, compilation_unit_mode::k_no_core_lib, args, expected_return);
}

void ut_verify_exception(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_what);


}

#endif /* test_helpers_hpp */
