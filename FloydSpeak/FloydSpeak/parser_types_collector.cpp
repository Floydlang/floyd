//
//  parser_types_collector.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 05/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_types_collector.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "statements.h"
#include "parse_function_def.h"
#include "parser_value.h"
#include "parts/sha1_class.h"
#include "parser_ast.h"
#include "utils.h"
#include "json_support.h"
#include "json_writer.h"
#include "parser_primitives.h"


using std::make_shared;
using std::string;
using std::shared_ptr;
using std::vector;


namespace floyd_parser {


	bool is_valid_identifier(const std::string& name){
		if(name.empty()){
			return false;
		}
		return true;
	}


	//////////////////////////////////////		types_collector_t


	bool type_name_entry_t::check_invariant() const{
		for(const auto& def: _defs){
			QUARK_ASSERT(def);
			QUARK_ASSERT(def->check_invariant());
		}
		return true;
	}

	bool type_name_entry_t::operator==(const type_name_entry_t& other) const{
		if(_defs.size() != other._defs.size()){
			return false;
		}
		for(auto i = 0 ; i < _defs.size() ; i++){
			if(!compare_shared_values(_defs[i], other._defs[i])){
				return false;
			}
		}
		return true;
	}

/*
	QUARK_UNIT_TESTQ("type_name_entry_t::operator==()", "operator==()"){
		const type_name_entry_t a{ "xyz", {} };
		const type_name_entry_t b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}

	QUARK_UNIT_TESTQ("type_name_entry_t::operator==()", ""){
		const type_name_entry_t a{ "xyz", make_shared<type_def_t>(type_def_t::make_int()) };
		const type_name_entry_t b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}
*/


	////////////////////////			types_collector_t




	types_collector_t::types_collector_t(){
		QUARK_ASSERT(check_invariant());
	}
	
	bool types_collector_t::check_invariant() const{
		for(const auto it: _types){
			QUARK_ASSERT(it.second.check_invariant());
		}
		return true;
	}

	bool types_collector_t::operator==(const types_collector_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _types == other._types;
	}

	types_collector_t types_collector_t::define_type_xyz(const std::string& new_name, const std::shared_ptr<type_def_t>& type_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_valid_identifier(new_name));
		QUARK_ASSERT(type_def && type_def->check_invariant());

		auto types2 = *this;

		type_name_entry_t entry;
		const auto it = types2._types.find(new_name);
		if(it != types2._types.end()){
			entry = it->second;
		}

		entry._defs.push_back(type_def);
		types2._types[new_name] = entry;
		return types2;
	}

	std::vector<std::shared_ptr<type_def_t>> types_collector_t::resolve_identifier(const std::string& name) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_valid_identifier(name));
		
		const auto it = _types.find(name);
		if(it == _types.end()){
			return {};
		}
		else {
			return it->second._defs;
		}
	}




	//////////////////////////////////////		free




	json_value_t type_name_entry_t_to_json(const type_name_entry_t& data_ref){
		QUARK_ASSERT(data_ref.check_invariant());

		std::vector<json_value_t> defs;
		for(const auto i: data_ref._defs){
			defs.push_back(type_def_to_json(*i));
		}
		return make_object({
			{ "_defs", defs }
		});
	}

	json_value_t type_definitions_to_json(const std::map<std::string, type_name_entry_t>& types){
		std::map<string, json_value_t> a;
		for(const auto i: types){
			a[i.first] = type_name_entry_t_to_json(i.second);
		}
		return a;
	}

	json_value_t types_collector_to_json(const types_collector_t& types){
		QUARK_ASSERT(types.check_invariant());

		if(types._types.empty()){
			return json_value_t();
		}
		else{
			return make_object({
				{ "_type_definitions", type_definitions_to_json(types._types) }
			});
		}
	}






	types_collector_t define_struct_type(const types_collector_t& types, const std::string& new_name, const std::shared_ptr<const scope_def_t>& struct_def){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(is_valid_identifier(new_name));
		QUARK_ASSERT(struct_def && struct_def->check_invariant());

		auto type_def = make_shared<type_def_t>(type_def_t::make_struct_def(struct_def));
		return types.define_type_xyz(new_name, type_def);
	}


	types_collector_t define_function_type(const types_collector_t& types, const std::string& new_name, const std::shared_ptr<const scope_def_t>& function_def){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(is_valid_identifier(new_name));
		QUARK_ASSERT(function_def && function_def->check_invariant());

		auto type_def = make_shared<type_def_t>(type_def_t::make_function_def(function_def));
		return types.define_type_xyz(new_name, type_def);
	}




	std::shared_ptr<const scope_def_t> resolve_struct_type(const types_collector_t& types, const std::string& name){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(is_valid_identifier(name));

		const auto a = types.resolve_identifier(name);
		for(const auto& t: a){
			if(t->get_type() == base_type::k_struct){
				return t->get_struct_def();
			}
		}
		return {};
	}

	std::shared_ptr<const scope_def_t> resolve_function_type(const types_collector_t& types, const std::string& name){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(is_valid_identifier(name));

		const auto a = types.resolve_identifier(name);
		for(const auto& t: a){
			if(t->get_type() == base_type::k_function){
				return t->get_function_def();
			}
		}
		return {};
	}






}	//	floyd_parser


using namespace floyd_parser;



//////////////////////////////////////		to_signature()

#if false
QUARK_UNIT_TESTQ("to_signature()", "empty unnamed struct"){
	auto global = scope_def_t::make_global_scope();
	quark::ut_compare(to_signature(make_struct0(global)), "<struct>{}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct3"){
	auto global = scope_def_t::make_global_scope();
	quark::ut_compare(to_signature(make_struct3(global)), "<struct>{<int>a,<string>b}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct4"){
	auto global = scope_def_t::make_global_scope();
	quark::ut_compare(to_signature(make_struct4(global)), "<struct>{<string>x,<string>z}");
}

/*
QUARK_UNIT_TESTQ("to_signature()", "empty unnamed struct"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "", make_struct2(global));
	const auto t1 = b.lookup_signature("<struct>{}");
	QUARK_TEST_VERIFY(t1);
	QUARK_TEST_VERIFY(to_signature(*t1) == "<struct>{}");
}
*/

QUARK_UNIT_TESTQ("to_signature()", "struct3"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "struct3", make_struct3(global));
	const auto t1 = b.resolve_identifier("struct3");
	QUARK_UT_VERIFY(t1.size() == 1);
//	const auto s1 = to_signature(*t1);
//	QUARK_TEST_VERIFY(s1 == "<struct>{<int>a,<string>b}");
}


QUARK_UNIT_TESTQ("to_signature()", "struct4"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "struct3", make_struct3(global));
	const auto c = define_struct_type(b, "struct4", make_struct4(global));
	const auto t2 = c.resolve_identifier("struct4");
	QUARK_UT_VERIFY(t2.size() == 1);
//	const auto s2 = to_signature(*t2);
//	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<string>z}");
}

#endif


//////////////////////////////////////		types_collector_t



#if false
QUARK_UNIT_TESTQ("define_function_type()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t{};
	const auto b =  define_function_type(a, "one", make_return_hello(global));
}
#endif


QUARK_UNIT_TESTQ("types_collector_t::operator==()", ""){
	const auto a = types_collector_t();
	const auto b = types_collector_t();

	QUARK_TEST_VERIFY(a == b);
}





QUARK_UNIT_TESTQ("types_collector_t::resolve_identifier()", "not found"){
	const auto a = types_collector_t();
	const auto b = a.resolve_identifier("xyz");
	QUARK_TEST_VERIFY(b.empty());
}



