//
//  ast_utils.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 17/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_utils_hpp
#define ast_utils_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "parser_ast.h"

namespace floyd_parser {

	//////////////////////		Traversal of AST


	struct resolved_path_t {
		public: scope_ref_t get_leaf() const;
		public: bool check_invariant() const;


		public: std::vector<const scope_ref_t> _scopes;
	};

	resolved_path_t go_down(const resolved_path_t& path, const scope_ref_t& child);

	//////////////////////		Traversal of parse tree

	struct parser_path_t {
		//	Returns a scope_def in json format.
		public: json_value_t get_leaf() const;

		public: bool check_invariant() const;


		public: std::vector<json_value_t> _scopes;
	};

	resolved_path_t go_down(const resolved_path_t& path, const scope_ref_t& child);


	//////////////////////		Finding stuff in AST graph


	/*
		Searches for a symbol in the scope, traversing to parent scopes if needed.
		Returns the scope where the symbol can be found, or empty if not found. It tells which member index.
	*/
//	std::pair<scope_ref_t, int> resolve_scoped_variable(const floyd_parser::resolved_path_t& path, const std::string& s);


}	//	floyd_parser

#endif /* ast_utils_hpp */
