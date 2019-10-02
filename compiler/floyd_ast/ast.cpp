//
//  parser_ast.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast.h"


#include "statement.h"
#include "utils.h"
#include "json_support.h"
#include "parser_primitives.h"


namespace floyd {



//////////////////////////////////////////////////		general_purpose_ast_t



json_t gp_ast_to_json(const general_purpose_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	std::vector<json_t> fds;
	for(const auto& e: ast._function_defs){
		const auto fd = function_def_to_ast_json(ast._interned_types, e);
		fds.push_back(fd);
	}

	const auto function_defs_json = json_t::make_array(fds);
	const auto types = type_interner_to_json(ast._interned_types);

	return json_t::make_object(
		{
			{ "globals", body_to_json(ast._interned_types, ast._globals) },
			{ "function_defs", function_defs_json },
			{ "types", types }
		}
	);

	//??? add software-system and containerdef.
}

general_purpose_ast_t json_to_gp_ast(const json_t& json){
	QUARK_ASSERT(json.check_invariant());

	const auto globals0 = json.get_object_element("globals");
	const auto function_defs = json.get_object_element("function_defs");
	const auto types0 = json.get_object_element("types");


	//	Fix types first, before globals and functions.
	auto interner = type_interner_from_json(types0);

	body_t globals1 = json_to_body(interner, globals0);

	std::vector<floyd::function_definition_t> function_defs1;
	for(const auto& f: function_defs.get_array()){
		const auto f1 = json_to_function_def(interner, f);
		function_defs1.push_back(f1);
	}

	return general_purpose_ast_t {
		globals1,
		function_defs1,
		interner,
		software_system_t{},
		container_t{}
	};
}



} //	floyd
