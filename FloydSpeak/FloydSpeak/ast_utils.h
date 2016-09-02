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


	//////////////////////		Finding stuff in AST graph


	/*
		Searches for a symbol in the scope, traversing to parent scopes if needed.
		Returns the scope where the symbol can be found, or empty if not found. It tells which member index.
	*/
	std::pair<scope_ref_t, int> resolve_scoped_variable(const floyd_parser::resolved_path_t& path, const std::string& s);


	/*
		Resolves the type, starting at scope_def, moving via parents up towards to global space. This is a compile-time operation.
		Returns the scope where the type is found + the type_def for the type.
		If the type is already resolved, it's simply returned.
	*/
	std::shared_ptr<const floyd_parser::type_def_t> resolve_type_to_def(const resolved_path_t& path, const type_identifier_t& type);

	//	Attempts to resolve type (if not already resolved). If it fails, the input type is returned unresolved.
	floyd_parser::type_identifier_t resolve_type_to_id(const resolved_path_t& path,
		const floyd_parser::type_identifier_t& type);


	floyd_parser::value_t make_default_value(const resolved_path_t& path, const type_identifier_t& type);
	floyd_parser::value_t make_default_value(const resolved_path_t& path, const floyd_parser::type_def_t& type_def);
	floyd_parser::value_t make_default_struct_value(const resolved_path_t& path, scope_ref_t struct_def);

	member_t find_struct_member_throw(const scope_ref_t& struct_ref, const std::string& member_name);
	type_identifier_t resolve_type_throw(const resolved_path_t& path, const type_identifier_t& s);



}	//	floyd_parser

#endif /* ast_utils_hpp */
