//
//  compiler_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_helpers.h"

#include "parser_primitives.h"
#include "floyd_parser.h"

#include "pass3.h"
#include "host_functions.h"
#include "bytecode_generator.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"

#include <thread>
#include <deque>
#include <future>

#include <pthread.h>
#include <condition_variable>

namespace floyd {

parse_tree_json_t parse_program__errors(const compilation_unit_t& cu){
	try {
		const auto parse_tree = parse_program2(cu.prefix_source + cu.program_text);
		const parse_tree_json_t p2(parse_tree._value);
		return p2;
	}
	catch(const compiler_error& e){
		const auto refined = refine_compiler_error_with_loc2(cu, e);
		throw_compiler_error(refined.first, refined.second);
	}
	catch(const std::exception& e){
		throw;
//		const auto location = find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc){
	}
}


compilation_unit_t make_compilation_unit_nolib(const std::string& source_code, const std::string& source_path){
	return compilation_unit_t{
		.prefix_source = "",
		.program_text = source_code,
		.source_file_path = source_path
	};
}

compilation_unit_t make_compilation_unit_lib(const std::string& source_code, const std::string& source_path){
	return compilation_unit_t{
		.prefix_source = k_builtin_types_and_constants + "\n",
		.program_text = source_code,
		.source_file_path = source_path
	};
}

compilation_unit_t make_compilation_unit(const std::string& source_code, bool corelib){
	if(corelib){
		return make_compilation_unit_lib(source_code, "");
	}
	else{
		return make_compilation_unit_nolib(source_code, "");
	}
}



semantic_ast_t run_semantic_analysis__errors(const pass2_ast_t& pass2, const compilation_unit_t& cu){
	try {
		const auto pass3 = run_semantic_analysis(pass2);
		return pass3;
	}
	catch(const compiler_error& e){
		const auto refined = refine_compiler_error_with_loc2(cu, e);
		throw_compiler_error(refined.first, refined.second);
	}
	catch(const std::exception& e){
		throw;
//		const auto location = find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc){
	}
}

semantic_ast_t compile_to_sematic_ast__errors(const compilation_unit_t& cu){

//	QUARK_CONTEXT_TRACE(context._tracer, json_to_pretty_string(statements_pos.first._value));
	const auto parse_tree = parse_program__errors(cu);
	const auto pass2 = parse_tree_to_pass2_ast(parse_tree);
	const auto pass3 = run_semantic_analysis__errors(pass2, cu);
	return pass3;
}


}	//	floyd
