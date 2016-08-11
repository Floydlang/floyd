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


	////////////////////////			types_collector_t


	/*
		Holds all types of program.
	*/

	types_collector_t::types_collector_t(){
		QUARK_ASSERT(check_invariant());


		//	int
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_int;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["int"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}

		//	bool
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_bool;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["bool"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}

		//	string
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_string;
			QUARK_ASSERT(def->check_invariant());

			_identifiers["string"] = { "", def };
			_type_definitions[to_signature(*def)] = def;
		}

		QUARK_ASSERT(_identifiers.size() == 3);
		QUARK_ASSERT(_type_definitions.size() == 3);

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
		//??? Cannot call to_signature() from here - recursion!
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




	std::pair<std::shared_ptr<type_def_t>, types_collector_t> types_collector_t::define_struct_type(const struct_def_t& struct_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(struct_def.check_invariant());

		auto type_def = make_shared<type_def_t>();
		type_def->_base_type = k_struct;
		type_def->_struct_def = make_shared<struct_def_t>(struct_def);

		const string signature = to_signature(*type_def);

		const auto existing_it = _type_definitions.find(signature);
		if(existing_it != _type_definitions.end()){
			return { existing_it->second, *this };
		}
		else{
			auto result = *this;
			result._type_definitions.insert(std::pair<std::string, std::shared_ptr<type_def_t>>(signature, type_def));
			return { type_def, result };
		}
	}

	types_collector_t types_collector_t::define_struct_type(const std::string& new_identifier, const struct_def_t& struct_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(struct_def.check_invariant());

		//	Make struct def, if not already done.
		const auto a = types_collector_t::define_struct_type(struct_def);
		const auto type_def = a.first;
		const auto collector2 = a.second;

		if(new_identifier.empty()){
			return collector2;
		}
		else{
			//	Make a type-identifier too.
			const auto collector3 = collector2.define_type_identifier(new_identifier, type_def);

			return collector3;
		}
	}




	//??? test!
	std::pair<std::shared_ptr<type_def_t>, types_collector_t> types_collector_t::define_function_type(const function_def_t& function_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(function_def.check_invariant());

		auto type_def = make_shared<type_def_t>();
		type_def->_base_type = k_function;
		type_def->_function_def = make_shared<function_def_t>(function_def);

		const string signature = to_signature(*type_def);

		const auto existing_it = _type_definitions.find(signature);
		if(existing_it != _type_definitions.end()){
			return { existing_it->second, *this };
		}
		else{
			auto result = *this;
			result._type_definitions.insert(std::pair<std::string, std::shared_ptr<type_def_t>>(signature, type_def));
			return { type_def, result };
		}
	}

	//??? test!
	types_collector_t types_collector_t::define_function_type(const std::string& new_identifier, const function_def_t& function_def) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(function_def.check_invariant());

		//	Make struct def, if not already done.
		const auto a = types_collector_t::define_function_type(function_def);
		const auto type_def = a.first;
		const auto collector2 = a.second;

		if(new_identifier.empty()){
			return collector2;
		}
		else{
			//	Make a type-identifier too.
			const auto collector3 = collector2.define_type_identifier(new_identifier, type_def);

			return collector3;
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

	std::shared_ptr<struct_def_t> types_collector_t::resolve_struct_type(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto a = resolve_identifier(s);
		if(a && a->_base_type == k_struct){
			return a->_struct_def;
		}
		else {
			return {};
		}
	}

	std::shared_ptr<function_def_t> types_collector_t::resolve_function_type(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto a = resolve_identifier(s);
		if(a && a->_base_type == k_function){
			return a->_function_def;
		}
		else {
			return {};
		}
	}


	std::shared_ptr<type_def_t> types_collector_t::lookup_signature(const std::string& s) const{
		QUARK_ASSERT(check_invariant());

		const auto it = _type_definitions.find(s);
		return it == _type_definitions.end() ? std::shared_ptr<type_def_t>() : it->second;
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
	const auto b = a.define_struct_type("", make_struct2(global));
	const auto t1 = b.lookup_signature("<struct>{}");
	QUARK_TEST_VERIFY(t1);
	QUARK_TEST_VERIFY(to_signature(*t1) == "<struct>{}");
}

QUARK_UNIT_TESTQ("to_signature()", "struct3"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = a.define_struct_type("struct3", make_struct3(global));
	const auto t1 = b.resolve_identifier("struct3");
	const auto s1 = to_signature(*t1);
	QUARK_TEST_VERIFY(s1 == "<struct>{<int>a,<string>b}");
}


QUARK_UNIT_TESTQ("to_signature()", "struct4"){
	auto global = scope_def_t::make_global_scope();
	const auto a = types_collector_t();
	const auto b = a.define_struct_type("struct3", make_struct3(global));
	const auto c = b.define_struct_type("struct4", make_struct4(global));
	const auto t2 = c.resolve_identifier("struct4");
	const auto s2 = to_signature(*t2);
	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<string>z}");
}




//////////////////////////////////////		types_collector_t




QUARK_UNIT_TESTQ("define_function_type()", ""){
	auto global = scope_def_t::make_global_scope();
	QUARK_TEST_VERIFY(to_string(k_int) == "int");
	const auto a = types_collector_t{};
	const auto b =  a.define_function_type("one", make_return_hello(global));
}



QUARK_UNIT_TESTQ("types_collector_t::types_collector_t()", "default construction"){
	const auto a = types_collector_t();
	QUARK_TEST_VERIFY(a.check_invariant());


	const auto b = a.resolve_identifier("int");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int);

	const auto d = a.resolve_identifier("bool");
	QUARK_TEST_VERIFY(d);
	QUARK_TEST_VERIFY(d->_base_type == k_bool);

	const auto c = a.resolve_identifier("string");
	QUARK_TEST_VERIFY(c);
	QUARK_TEST_VERIFY(c->_base_type == k_string);
}




QUARK_UNIT_TESTQ("types_collector_t::resolve_identifier()", "not found"){
	const auto a = types_collector_t();
	const auto b = a.resolve_identifier("xyz");
	QUARK_TEST_VERIFY(!b);
}

QUARK_UNIT_TESTQ("types_collector_t::resolve_identifier()", "int found"){
	const auto a = types_collector_t();
	const auto b = a.resolve_identifier("int");
	QUARK_TEST_VERIFY(b);
}


QUARK_UNIT_TESTQ("types_collector_t::define_alias_identifier()", "int => my_int"){
	auto a = types_collector_t();
	a = a.define_alias_identifier("my_int", "int");
	const auto b = a.resolve_identifier("my_int");
	QUARK_TEST_VERIFY(b);
	QUARK_TEST_VERIFY(b->_base_type == k_int);
}



