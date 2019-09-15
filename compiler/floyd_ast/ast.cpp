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
		const auto fd = function_def_to_ast_json(e);
		fds.push_back(fd);
	}

	const auto function_defs_json = json_t::make_array(fds);


	std::vector<json_t> types;
	for(auto i = 0 ; i < ast._interned_types.interned.size() ; i++){
		const auto& e = ast._interned_types.interned[i];
		const auto x = typeid_to_ast_json(e, json_tags::k_tag_resolve_state);
		types.push_back(x);
	}


	return json_t::make_object(
		{
			{ "globals", body_to_json(ast._globals) },
			{ "function_defs", function_defs_json },
			{ "types", json_t::make_array(types) }
		}
	);

	//??? add software-system and containerdef.
}

general_purpose_ast_t json_to_gp_ast(const json_t& json){
	const auto globals0 = json.get_object_element("globals");
	const auto function_defs = json.get_object_element("function_defs");
	const auto types0 = json.get_object_element("types");

	body_t globals1 = json_to_body(globals0);

	std::vector<floyd::function_definition_t> function_defs1;
	for(const auto& f: function_defs.get_array()){
		const auto f1 = json_to_function_def(f);
		function_defs1.push_back(f1);
	}


	std::vector<typeid_t> types;
	for(const auto& t: types0.get_array()){
		const auto t2 = typeid_from_ast_json(t);
		types.push_back(t2);
	}
	type_interner_t types2;
	types2.interned = types;

	return general_purpose_ast_t {
		globals1,
		function_defs1,
		types2,
		software_system_t{},
		container_t{}
	};
}



} //	floyd
