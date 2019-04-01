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

#include "bytecode_interpreter.h"
#include <string>
#include <vector>

namespace floyd {
struct value_t;
struct semantic_ast_t;
struct compilation_unit_t;
struct parse_tree_json_t;
struct pass2_ast_t;

parse_tree_json_t parse_program__errors(const compilation_unit_t& cu);
semantic_ast_t run_semantic_analysis__errors(const pass2_ast_t& pass2, const compilation_unit_t& cu);

enum class compilation_unit_mode {
	k_include_core_lib,
	k_no_core_lib
};

semantic_ast_t compile_to_sematic_ast__errors(const std::string& program, const std::string& file, compilation_unit_mode mode);

}

#endif /* compiler_helpers_hpp */
