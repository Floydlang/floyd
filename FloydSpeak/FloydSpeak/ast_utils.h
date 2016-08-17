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

//	std::shared_ptr<type_def_t> resolve_type(const scope_ref_t scope_def, const type_identifier_t& s);

	/*
		Searches for a symbol in the scope, traversing to parent scopes if needed.
		Returns the scope where the symbol can be found, or empty if not found. Int tells which member.
	*/
	std::pair<floyd_parser::scope_ref_t, int> resolve_scoped_variable(floyd_parser::scope_ref_t scope_def, const std::string& s);

	/*
		Resolves the type, starting at scope_def, moving via parents up towards to global space. This is a compile-time operation.
		Returns the scope where the type is found + the type_def for the type.
	*/
	std::shared_ptr<floyd_parser::type_def_t> resolve_type(const floyd_parser::scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);
	floyd_parser::type_identifier_t resolve_type2(const floyd_parser::scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);


	floyd_parser::value_t make_default_value(const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);

	floyd_parser::value_t make_default_value(const floyd_parser::type_def_t& t);
	floyd_parser::value_t make_default_value(scope_ref_t t);

}	//	floyd_parser


#endif /* ast_utils_hpp */
