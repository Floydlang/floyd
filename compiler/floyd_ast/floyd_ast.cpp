//
//  parser_ast.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_ast.h"


#include "statement.h"
#include "utils.h"
#include "json_support.h"
#include "parser_primitives.h"


namespace floyd {



//////////////////////////////////////////////////		general_purpose_ast_t



bool general_purpose_ast_t::check_invariant() const{
	QUARK_ASSERT(_globals.check_invariant());
	for(const auto& e: _function_defs){
		QUARK_ASSERT(e.check_invariant());
	}
	QUARK_ASSERT(_types.check_invariant());

	return true;
}


general_purpose_ast_t make_test_gp_ast(){
	return general_purpose_ast_t {
		{},
		{},
		types_t(),
		software_system_t {},
		container_t {}
	};
}




QUARK_TEST("", "make_test_gp_ast()", "", ""){
	auto b = make_test_gp_ast();
	QUARK_VERIFY(b.check_invariant());
}




json_t gp_ast_to_json(const general_purpose_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	std::vector<json_t> fds;
	for(const auto& e: ast._function_defs){
		const auto fd = function_def_to_ast_json(ast._types, e);
		fds.push_back(fd);
	}

	const auto function_defs_json = json_t::make_array(fds);
	const auto types = types_to_json(ast._types);

	return json_t::make_object(
		{
			{ "globals", scope_to_json(ast._types, ast._globals) },
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
	auto types = types_from_json(types0);

	lexical_scope_t globals1 = json_to_scope(types, globals0);

	std::vector<floyd::function_definition_t> function_defs1;
	for(const auto& f: function_defs.get_array()){
		const auto f1 = json_to_function_def(types, f);
		function_defs1.push_back(f1);
	}

	return general_purpose_ast_t {
		globals1,
		function_defs1,
		types,
		software_system_t{},
		container_t{}
	};
}



} //	floyd
