//
//  compiler_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef compiler_helpers_hpp
#define compiler_helpers_hpp

#include "quark.h"


#include "ast_value.h"

#include <string>
#include <vector>
#include <map>

namespace floyd {
struct semantic_ast_t;
struct compilation_unit_t;
struct unchecked_ast_t;

namespace parser {
	struct parse_tree_t;
}


////////////////////////////////		run_output_t

//??? Use enum to tell if output is for main/processes/none
struct run_output_t {
	run_output_t() :
		main_result(-1)
	{}

	run_output_t(int64_t main_result, std::map<std::string, value_t> process_results) :
		main_result(main_result),
		process_results(process_results)
	{}


	////////////////////////////////		STATE

	int64_t main_result;
	std::map<std::string, value_t> process_results;
};

bool operator==(const run_output_t& lhs, const run_output_t& rhs);

void ut_verify_run_output(const quark::call_context_t& context, const run_output_t& result, const run_output_t& expected);



////////////////////////////////		ERRORS



parser::parse_tree_t parse_program__errors(const compilation_unit_t& cu);
semantic_ast_t run_semantic_analysis__errors(const unchecked_ast_t& ast, const compilation_unit_t& cu);


enum class compilation_unit_mode {
	k_include_core_lib,
	k_no_core_lib
};

compilation_unit_t make_compilation_unit_nolib(const std::string& source_code, const std::string& source_path);
compilation_unit_t make_compilation_unit_lib(const std::string& source_code, const std::string& source_path);
compilation_unit_t make_compilation_unit(const std::string& source_code, const std::string& source_path, compilation_unit_mode mode);


semantic_ast_t compile_to_sematic_ast__errors(const compilation_unit_t& cu);

}

#endif /* compiler_helpers_hpp */
