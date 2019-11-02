//
//  test_helpers.hpp
//  Floyd
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
#include "compiler_basics.h"

namespace floyd {

struct compilation_unit_t;
struct config_t;



struct test_report_t {
	json_t result_variable;
	run_output_t output;
	std::vector<std::string> print_out;
	std::string exception_what;
};

bool compare(const test_report_t& lhs, const test_report_t& rhs, bool check_printout);
bool operator==(const test_report_t& lhs, const test_report_t& rhs);


void ut_verify_report(const quark::call_context_t& context, const test_report_t& result, const test_report_t& expected);

void test_floyd(const quark::call_context_t& context, const compilation_unit_t& cu, const compiler_settings_t& settings, const std::vector<std::string>& main_args, const test_report_t& expected, bool check_printout);




//////////////////////////////////////////		SHORTCUTS


const int k_default_main_result = 0;


inline test_report_t check_nothing(){
	const types_t types;
	return test_report_t { json_t(), { k_default_main_result, {} }, {}, "" };
}

inline test_report_t check_result(const json_t& expected){
	return test_report_t { expected, { k_default_main_result, {} }, {}, "" };
}
inline test_report_t check_printout(const std::vector<std::string>& print_out){
	const types_t types;
	return test_report_t { json_t(), { k_default_main_result, {} }, print_out, "" };
}
inline test_report_t check_main_return(int64_t main_return){
	const types_t types;
	return test_report_t { json_t(), { main_return, {} }, {}, "" };
}
inline test_report_t check_exception(const std::string& exception_what){
//	return test_report_t { value_t::make_undefined(), { k_default_main_result, {} }, {}, exception_what };

	const types_t types;
	return test_report_t { json_t(), { k_default_main_result, {} }, {}, exception_what };
}


void ut_verify_global_result_lib(const quark::call_context_t& context, const std::string& program, const json_t& expected_result);
void ut_verify_global_result_nolib(const quark::call_context_t& context, const std::string& program, const json_t& expected_result);

void ut_verify_printout_lib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout);
void ut_verify_printout_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout);

//	Has no output value: only compilation errors or floyd-asserts.
void ut_run_closed_nolib(const quark::call_context_t& context, const std::string& program);
void ut_run_closed_lib(const quark::call_context_t& context, const std::string& program);

void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& args, int64_t expected_return);

void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what);


} // floyd

#endif /* test_helpers_hpp */
