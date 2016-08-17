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

#include "parser_statement.h"
#include "parser_function.h"
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


	//////////////////////////////////////		types_collector_t


	bool type_indentifier_data_ref::operator==(const type_indentifier_data_ref& other) const{
		if(_alias_type_identifier != other._alias_type_identifier){
			return false;
		}
		if(!compare_shared_values(_optional_def, other._optional_def)){
			return false;
		}
		return true;
	}

	QUARK_UNIT_TESTQ("type_indentifier_data_ref::operator==()", "operator==()"){
		const type_indentifier_data_ref a{ "xyz", {} };
		const type_indentifier_data_ref b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}

	QUARK_UNIT_TESTQ("type_indentifier_data_ref::operator==()", ""){
		const type_indentifier_data_ref a{ "xyz", make_shared<type_def_t>(type_def_t::make_int()) };
		const type_indentifier_data_ref b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}



	////////////////////////			types_collector_t


	/*
		Holds all types of program.
	*/
	types_collector_t::types_collector_t(){
		QUARK_ASSERT(check_invariant());
	}
	
	bool types_collector_t::check_invariant() const{
		for(const auto it: _identifiers){
			QUARK_ASSERT(it.first != "");
			const auto data = it.second;
			if(!data._alias_type_identifier.empty()){
				QUARK_ASSERT(!data._optional_def);

//				const auto a = lookup_identifier_deep(data._alias_type_identifier);
//				QUARK_ASSERT(a);//### test deeper?
			}
			else{
				if(data._optional_def){
					QUARK_ASSERT(data._optional_def->check_invariant());
				}
				else{
				}
			}
		}

		for(const auto it: _type_definitions){
		//!!! Cannot call to_signature() from here - recursion!
//			const auto signature = to_signature(*it.second);
//			QUARK_ASSERT(it.first == signature);
			QUARK_ASSERT(it.second->check_invariant());
		}

		//	Make sure all types referenced from _identifiers are also stored inside _type_definition.
		for(const auto identifiers_it: _identifiers){
			if(identifiers_it.second._optional_def){
				auto defs_it = _type_definitions.begin();
				while(defs_it != _type_definitions.end() && defs_it->second != identifiers_it.second._optional_def){
					 defs_it++;
				}

				QUARK_ASSERT(defs_it != _type_definitions.end());
			}
		}
		return true;
	}

	bool types_collector_t::operator==(const types_collector_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_identifiers == other._identifiers)){
			return false;
		}

/*
		{
			if(_identifiers.size() != other._identifiers.size()){
				return false;
			}

			//??? deeper check?
			auto j = other._identifiers.begin();
			for(auto i = _identifiers.begin() ; i != _identifiers.end(); ++i){
				if(i->first != j->first){
					return false;
				}
				if(!(i->second == j->second)){
					return false;
				}
				j++;
			}
		}
*/
		{
			if(_type_definitions.size() != other._type_definitions.size()){
				return false;
			}

			auto j = other._type_definitions.begin();
			for(auto i = _type_definitions.begin() ; i != _type_definitions.end(); ++i){
				if(i->first != j->first){
					return false;
				}
				if(!(*i->second == *j->second)){
					return false;
				}
				j++;
			}
		}

		return true;
	}

	bool types_collector_t::is_type_identifier_fully_defined(const std::string& type_identifier) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!type_identifier.empty());

		const auto existing_it = _identifiers.find(type_identifier);
		if(existing_it != _identifiers.end() && (!existing_it->second._alias_type_identifier.empty() || existing_it->second._optional_def)){
			return true;
		}
		else{
			return false;
		}
	}

	types_collector_t types_collector_t::define_alias_identifier(const std::string& new_identifier, const std::string& existing_identifier) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!new_identifier.empty());
		QUARK_ASSERT(!existing_identifier.empty());

		if(_identifiers.find(existing_identifier) == _identifiers.end()){
			throw std::runtime_error("unknown type identifier to base alias on");
		}

		if(is_type_identifier_fully_defined(new_identifier)){
			throw std::runtime_error("new type identifier already defined");
		}

		auto result = *this;
		result._identifiers[new_identifier] = { existing_identifier, {} };

		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	types_collector_t types_collector_t::define_type_identifier(const std::string& new_identifier, const std::shared_ptr<type_def_t>& type_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(!new_identifier.empty());

		//	### Be quite if existing identifier matches new one 100% == same type_def.
		if(is_type_identifier_fully_defined(new_identifier)){
			throw std::runtime_error("new type identifier already defined");
		}

		auto result = *this;
		result._identifiers[new_identifier] = { "", type_def };

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	types_collector_t types_collector_t::define_type_xyz(const std::string& new_identifier, const std::shared_ptr<type_def_t>& type_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(type_def && type_def->check_invariant());

		auto types2 = *this;
		const string signature = to_signature(*type_def);

		//	Add to _type_definitions if not already there.
		if(_type_definitions.find(signature) == _type_definitions.end()){
			types2._type_definitions.insert(std::pair<std::string, std::shared_ptr<type_def_t>>(signature, type_def));
		}

		//	Add type-identifier if needed.
		if(new_identifier.empty()){
			QUARK_ASSERT(types2.check_invariant());
			return types2;
		}
		else{
			//	Make a type-identifier too.
			const auto types3 = types2.define_type_identifier(new_identifier, type_def);
			QUARK_ASSERT(types3.check_invariant());
			return types3;
		}
	}



	std::shared_ptr<type_indentifier_data_ref> types_collector_t::lookup_identifier_shallow(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _identifiers.find(s);
		return it == _identifiers.end() ? std::shared_ptr<type_indentifier_data_ref>() : make_shared<type_indentifier_data_ref>(it->second);
	}

	std::shared_ptr<type_indentifier_data_ref> types_collector_t::lookup_identifier_deep(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _identifiers.find(s);
		if(it == _identifiers.end()){
			return {};
		}
		else {
			const auto alias = it->second._alias_type_identifier;
			if(!alias.empty()){
				return lookup_identifier_deep(alias);
			}
			else{
				return make_shared<type_indentifier_data_ref>(it->second);
			}
		}
	}

	std::shared_ptr<type_def_t> types_collector_t::resolve_identifier(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto identifier_data = lookup_identifier_deep(s);
		if(!identifier_data){
			return {};
		}
		else{
			return identifier_data->_optional_def;
		}
	}


	std::shared_ptr<type_def_t> types_collector_t::lookup_signature(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _type_definitions.find(s);
		return it == _type_definitions.end() ? std::shared_ptr<type_def_t>() : it->second;
	}




	//////////////////////////////////////		free



	json_value_t type_indentifier_data_ref_to_json(const type_indentifier_data_ref& data_ref){
		const std::map<string, json_value_t> a{
			{ "_alias_type_identifier", json_value_t(data_ref._alias_type_identifier) },
			{ "_optional_def", data_ref._optional_def ? json_value_t(to_signature(*data_ref._optional_def)) : json_value_t() }
		};
		return a;
	}

	json_value_t identifiers_to_json(const std::map<std::string, type_indentifier_data_ref >& identifiers){
		std::map<string, json_value_t> a;
		for(const auto i: identifiers){
			a[i.first] = type_indentifier_data_ref_to_json(i.second);
		}
		return a;
	}

	json_value_t type_definitions_to_json(const std::map<std::string, std::shared_ptr<type_def_t> >& type_definitions){
		std::map<string, json_value_t> result;
		for(const auto i: type_definitions){
			result[i.first] = type_def_to_json(*i.second);
		}
		return result;
	}


	json_value_t types_collector_to_json(const types_collector_t& types){
		const std::map<string, json_value_t> a {
			{ "_identifiers", identifiers_to_json(types._identifiers) },
			{ "_type_definitions", type_definitions_to_json(types._type_definitions) }
		};
		return a;
	}






	types_collector_t define_struct_type(const types_collector_t& types, const std::string& new_identifier, const std::shared_ptr<scope_def_t>& struct_def){
		QUARK_ASSERT(types.check_invariant());

		auto type_def = make_shared<type_def_t>(type_def_t::make_struct_def(struct_def));
		return types.define_type_xyz(new_identifier, type_def);
	}


	types_collector_t define_function_type(const types_collector_t& types, const std::string& new_identifier, const std::shared_ptr<scope_def_t>& function_def){
		QUARK_ASSERT(types.check_invariant());

		auto type_def = make_shared<type_def_t>(type_def_t::make_function_def(function_def));
		return types.define_type_xyz(new_identifier, type_def);
	}




	std::shared_ptr<scope_def_t> resolve_struct_type(const types_collector_t& types, const std::string& s){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(s.size() > 0);

		const auto a = types.resolve_identifier(s);
		if(a && a->get_type() == base_type::k_struct){
			return a->get_struct_def();
		}
		else {
			return {};
		}
	}

	std::shared_ptr<scope_def_t> resolve_function_type(const types_collector_t& types, const std::string& s){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(s.size() > 0);

		const auto a = types.resolve_identifier(s);
		if(a && a->get_type() == base_type::k_function){
			return a->get_function_def();
		}
		else {
			return {};
		}
	}






}	//	floyd_parser


using namespace floyd_parser;



//////////////////////////////////////		to_signature()


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

QUARK_UNIT_TESTQ("to_signature()", "empty unnamed struct"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "", make_struct2(global));
	const auto t1 = b.lookup_signature("<struct>{}");
	QUARK_TEST_VERIFY(t1);
	QUARK_TEST_VERIFY(to_signature(*t1) == "<struct>{}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct3"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "struct3", make_struct3(global));
	const auto t1 = b.resolve_identifier("struct3");
	const auto s1 = to_signature(*t1);
	QUARK_TEST_VERIFY(s1 == "<struct>{<int>a,<string>b}");
}


QUARK_UNIT_TESTQ("to_signature()", "struct4"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = define_struct_type(a, "struct3", make_struct3(global));
	const auto c = define_struct_type(b, "struct4", make_struct4(global));
	const auto t2 = c.resolve_identifier("struct4");
	const auto s2 = to_signature(*t2);
	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<string>z}");
}




//////////////////////////////////////		types_collector_t




QUARK_UNIT_TESTQ("define_function_type()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t{};
	const auto b =  define_function_type(a, "one", make_return_hello(global));
}


QUARK_UNIT_TESTQ("types_collector_t::operator==()", ""){
	const auto a = types_collector_t();
	const auto b = types_collector_t();

	QUARK_TEST_VERIFY(a == b);
}





QUARK_UNIT_TESTQ("types_collector_t::resolve_identifier()", "not found"){
	const auto a = types_collector_t();
	const auto b = a.resolve_identifier("xyz");
	QUARK_TEST_VERIFY(!b);
}

/*
QUARK_UNIT_TESTQ("types_collector_t::define_alias_identifier()", "int => my_int"){
	auto a = types_collector_t();
	a = a.define_alias_identifier("my_int", "int");
	const auto b = a.resolve_identifier("my_int");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int);
}
*/


