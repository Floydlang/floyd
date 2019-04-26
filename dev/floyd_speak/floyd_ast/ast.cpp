//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast.h"


#include "ast_json.h"
#include "statement.h"



#include "statement.h"
#include "utils.h"
#include "json_support.h"
#include "ast.h"
#include "ast_typeid_helpers.h"
#include "parser_primitives.h"


namespace floyd {

using namespace std;



itype_t make_new_itype(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		type_interner_t& interner;
		const typeid_t& type;

		int32_t operator()(const typeid_t::internal_undefined_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::any_t& e) const{
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::void_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::bool_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::int_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::double_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::string_t& e) const{
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::json_type_t& e) const{
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::typeid_type_t& e) const{
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::struct_t& e) const{
			return interner.struct_next_id++;
		}
		int32_t operator()(const typeid_t::vector_t& e) const{
			return interner.vector_next_id++;
		}
		int32_t operator()(const typeid_t::dict_t& e) const{
			return interner.dict_next_id++;
		}
		int32_t operator()(const typeid_t::function_t& e) const{
			return interner.function_next_id++;
		}
		int32_t operator()(const typeid_t::internal_unresolved_type_identifier_t& e) const{
			return interner.simple_next_id++;
		}
	};
	const auto new_id = std::visit(visitor_t{ interner, type }, type._contents);
	return { new_id };
}

std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
	if(it != interner.interned.end()){
		return *it;
	}
	else{
		const auto itype = make_new_itype(interner, type);
		const auto p = std::pair<itype_t, typeid_t>{ itype, type };
		interner.interned.push_back(p);
		return p;
	}
}

itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type){
	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
	if(it != interner.interned.end()){
		return it->first;
	}
	throw std::exception();
}

typeid_t lookup_type(const type_interner_t& interner, const itype_t& type){
	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.first.itype == type.itype; });
	if(it != interner.interned.end()){
		return it->second;
	}
	throw std::exception();
}



json_t gp_ast_to_json(const general_purpose_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	std::vector<json_t> fds;
	for(const auto& e: ast._function_defs){
		const auto fd = function_def_to_ast_json(*e);
		fds.push_back(fd);
	}

	const auto function_defs_json = json_t::make_array(fds);
	return json_t::make_object(
		{
			{ "globals", body_to_json(ast._globals) },
			{ "function_defs", function_defs_json }
		}
	);

	//??? add software-system and containerdef.
}

general_purpose_ast_t json_to_gp_ast(const json_t& json){

	//	This is a pass2 AST: it contains an array of statements, with hierachical functions and types.
	if(json.is_array()){
		const auto program_body = astjson_to_statements(json);
		return general_purpose_ast_t{
			body_t{ program_body },
			{},
			{},
			{},
			{}
		};
	}

	//	Pass 3 AST aka SAST: contains globals + function-defs.
	else{
		const auto globals0 = json.get_object_element("globals");
		const auto function_defs = json.get_object_element("function_defs");

		body_t globals1 = json_to_body(globals0);

		std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs1;
		for(const auto& f: function_defs.get_array()){
			const auto f1 = json_to_function_def(f);
			const auto f2 = std::make_shared<const floyd::function_definition_t>(f1);
			function_defs1.push_back(f2);
		}

		return general_purpose_ast_t {
			globals1,
			function_defs1,
			{},
			software_system_t{},
			container_t{}
		};
	}
}




} //	floyd
