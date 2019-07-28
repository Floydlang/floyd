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
#include "parser_primitives.h"


namespace floyd {

using namespace std;




struct itype_lookup {
};


//////////////////////////////////////////////////		type_interner_t



type_interner_t::type_interner_t() :
	simple_next_id(0),
	struct_next_id(100000000),
	vector_next_id(200000000),
	dict_next_id(300000000),
	function_next_id(400000000)
{
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(0), typeid_t::make_undefined() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(1), typeid_t::make_any() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(2), typeid_t::make_void() });

	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(3), typeid_t::make_bool() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(4), typeid_t::make_int() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(5), typeid_t::make_double() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(6), typeid_t::make_string() });
	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(7), typeid_t::make_json() });

	interned.push_back(std::pair<itype_t, typeid_t>{ itype_t(8), typeid_t::make_typeid() });
	simple_next_id = static_cast<int32_t>(interned.size());

	QUARK_ASSERT(check_invariant());
}


bool check_basetype(const type_interner_t& interner, base_type type){
	const int index = static_cast<int>(type);
	const auto& i = interner.interned[index];

	QUARK_ASSERT(i.first.itype == index);
	QUARK_ASSERT(i.second.get_base_type() == type);
	return true;
}

bool type_interner_t::check_invariant() const {
	QUARK_ASSERT(check_basetype(*this, base_type::k_undefined));
	QUARK_ASSERT(check_basetype(*this, base_type::k_any));
	QUARK_ASSERT(check_basetype(*this, base_type::k_void));

	QUARK_ASSERT(check_basetype(*this, base_type::k_bool));
	QUARK_ASSERT(check_basetype(*this, base_type::k_int));
	QUARK_ASSERT(check_basetype(*this, base_type::k_double));
	QUARK_ASSERT(check_basetype(*this, base_type::k_string));
	QUARK_ASSERT(check_basetype(*this, base_type::k_json));

	QUARK_ASSERT(check_basetype(*this, base_type::k_typeid));

	//!!! We don't register struct, vector, dict and function, since those get explicit types.


	//	Make sure each itype

	return true;
}



static itype_t make_new_itype_recursive(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		type_interner_t& interner;
		const typeid_t& type;

		int32_t operator()(const typeid_t::undefined_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::any_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::void_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::bool_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::int_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::double_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::string_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::json_type_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}
		int32_t operator()(const typeid_t::typeid_type_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
			return interner.simple_next_id++;
		}

		int32_t operator()(const typeid_t::struct_t& e) const{
			//	Warning: Need to remember struct_next_id since members can add intern other structs and bump struct_next_id.
			interner.interned.push_back({ interner.struct_next_id, type });
			const auto result = interner.struct_next_id++;

			for(const auto& m: e._struct_def->_members){
				intern_type(interner, m._type);
			}
			return result;
		}
		int32_t operator()(const typeid_t::vector_t& e) const{
			//	Warning: Need to remember vector_next_id since members can add intern other structs and bump vector_next_id.
			QUARK_ASSERT(e._parts.size() == 1);

			interner.interned.push_back({ interner.vector_next_id, type });
			const auto result = interner.vector_next_id++;

			intern_type(interner, e._parts[0]);
			return result;
		}
		int32_t operator()(const typeid_t::dict_t& e) const{
			//	Warning: Need to remember dict_next_id since members can add intern other structs and bump dict_next_id.
			QUARK_ASSERT(e._parts.size() == 1);

			interner.interned.push_back({ interner.dict_next_id, type });
			const auto result = interner.dict_next_id++;

			intern_type(interner, e._parts[0]);
			return result;
		}
		int32_t operator()(const typeid_t::function_t& e) const{
			//	Warning: Need to remember function_next_id since members can add intern other structs and bump function_next_id.
			interner.interned.push_back({ interner.function_next_id, type });
			const auto result = interner.function_next_id++;

			for(const auto& m: e._parts){
				intern_type(interner, m);
			}
			return result;
		}
		int32_t operator()(const typeid_t::unresolved_t& e) const{
			interner.interned.push_back({ interner.simple_next_id, type });
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
		const auto itype = make_new_itype_recursive(interner, type);
		return { itype, type };
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






//////////////////////////////////////////////////		general_purpose_ast_t





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



} //	floyd
