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
		public: bool check_invariant() const {
			for(const auto i: _scopes){
				QUARK_ASSERT(i && i->check_invariant());
			}
			return true;
		};
		std::vector<scope_ref_t> _scopes;
	};

	// Normally you build this while traversing down tree, adding new scopes as we go down.
	resolved_path_t resolve_path(const ast_t& ast, const floyd_parser::ast_path_t& path);

	floyd_parser::ast_path_t unresolve_path(const resolved_path_t& path);


	floyd_parser::ast_path_t go_down(const floyd_parser::ast_path_t& path, const std::string& sub_scope_name);
	floyd_parser::ast_path_t go_up(const floyd_parser::ast_path_t& path);

	/*
		Searches the entire AST for the specified scope_def, returns path to it or empty path.
		Name and type of scope is used for search.
	*/
	resolved_path_t find_path_slow(const ast_t& ast, const scope_ref_t& scope_def);



	//////////////////////		Finding stuff in AST graph


	/*
		Searches for a symbol in the scope, traversing to parent scopes if needed.
		Returns the scope where the symbol can be found, or empty if not found. Int tells which member index.
	*/
	std::pair<scope_ref_t, int> resolve_scoped_variable(const ast_t& ast, const floyd_parser::ast_path_t& path, const std::string& s);


	/*
		Resolves the type, starting at scope_def, moving via parents up towards to global space. This is a compile-time operation.
		Returns the scope where the type is found + the type_def for the type.
		If the type is already resolved, it's simply returned.
	*/
	std::shared_ptr<const floyd_parser::type_def_t> resolve_type(const ast_t& ast, const floyd_parser::ast_path_t& path, const floyd_parser::scope_ref_t scope_def, const type_identifier_t& type);

	//	Attempts to resolve type (if not already resolved). If it fails, the input type is returned unresolved.
	floyd_parser::type_identifier_t resolve_type2(const ast_t& ast, const ast_path_t& path, const floyd_parser::scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);


	floyd_parser::value_t make_default_value(const ast_t& ast, const ast_path_t& path, const scope_ref_t scope_def, const type_identifier_t& type);
	floyd_parser::value_t make_default_value(const ast_t& ast, const ast_path_t& path, const floyd_parser::type_def_t& t);
	floyd_parser::value_t make_default_struct_value(const ast_t& ast, const ast_path_t& path, scope_ref_t t);


	member_t find_struct_member_throw(const scope_ref_t& struct_ref, const std::string& member_name);
	type_identifier_t resolve_type_throw(const ast_t& ast, const ast_path_t& path, const scope_ref_t& scope_def, const type_identifier_t& s);




}	//	floyd_parser


#endif /* ast_utils_hpp */
