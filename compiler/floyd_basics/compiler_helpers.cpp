//
//  compiler_helpers.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-03-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_helpers.h"

#include "parser_primitives.h"
#include "floyd_parser.h"

#include "parse_tree_to_ast_conv.h"
#include "desugar_pass.h"
#include "semantic_analyser.h"
#include "semantic_ast.h"

#include "compiler_helpers.h"
#include "compiler_basics.h"
#include "floyd_corelib.h"

namespace floyd {



bool operator==(const run_output_t& lhs, const run_output_t& rhs){
	return lhs.main_result == rhs.main_result && lhs.process_results == rhs.process_results;
}

void ut_verify_run_output(const quark::call_context_t& context, const run_output_t& result, const run_output_t& expected){
	if(result == expected){
	}
	else{
		types_t interner;
		{
			QUARK_SCOPED_TRACE("  result: ");
			QUARK_TRACE_SS("main_result: " << result.main_result);
			QUARK_SCOPED_TRACE("process_results");
			for(const auto& e: result.process_results){
				QUARK_TRACE_SS(e.first << ":\t" << value_and_type_to_string(interner, e.second));
			}
		}

		{
			QUARK_SCOPED_TRACE("expected:");
			QUARK_TRACE_SS("main_result: " << expected.main_result);
			QUARK_SCOPED_TRACE("process_results");
			for(const auto& e: expected.process_results){
				QUARK_TRACE_SS(e.first << ":\t" << value_and_type_to_string(interner, e.second));
			}
		}

		fail_test(context);
	}
}




parser::parse_tree_t parse_program__errors(const compilation_unit_t& cu){
	try {
		const auto parse_tree = parser::parse_program2(cu.prefix_source + cu.program_text);
		return parse_tree;
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
		.prefix_source = k_corelib_builtin_types_and_constants + "\n",
		.program_text = source_code,
		.source_file_path = source_path
	};
}

compilation_unit_t make_compilation_unit(const std::string& source_code, const std::string& source_path, compilation_unit_mode mode){
	if(mode == compilation_unit_mode::k_include_core_lib){
		return make_compilation_unit_lib(source_code, source_path);
	}
	else if(mode == compilation_unit_mode::k_no_core_lib){
		return make_compilation_unit_nolib(source_code, source_path);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



semantic_ast_t run_semantic_analysis__errors(const unchecked_ast_t& unchecked_ast, const compilation_unit_t& cu){
	try {
		const auto sem_ast = run_semantic_analysis(unchecked_ast);
		return sem_ast;
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
	const auto unchecked_ast = parse_tree_to_ast(parse_tree);
	const auto sem_ast = run_semantic_analysis__errors(unchecked_ast, cu);
	return sem_ast;
}


}	//	floyd
