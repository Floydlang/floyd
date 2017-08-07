//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_ast.h"

#include "statements.h"
#include "parser_value.h"
#include "text_parser.h"
#include "parser_primitives.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>
#include "parts/sha1_class.h"
#include "utils.h"
#include "json_support.h"
#include "json_writer.h"


namespace floyd_parser {
	using std::vector;
	using std::string;
	using std::pair;
	using std::make_shared;


	//////////////////////////////////////////////////		base_type


	string base_type_to_string(const base_type t){
		if(t == base_type::k_null){
			return "null";
		}
		else if(t == base_type::k_bool){
			return "bool";
		}
		else if(t == base_type::k_int){
			return "int";
		}
		else if(t == base_type::k_float){
			return "float";
		}
		else if(t == base_type::k_string){
			return "string";
		}
		else if(t == base_type::k_struct){
			return "struct";
		}
		else if(t == base_type::k_vector){
			return "vector";
		}
		else if(t == base_type::k_function){
			return "function";
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	//??? use json
	void trace(const typeid_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_base_type();
		if(type == base_type::k_bool){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == base_type::k_int){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == base_type::k_float){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == base_type::k_string){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == base_type::k_struct){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_struct_def());
		}
		else if(type == base_type::k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == base_type::k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(t.get_function_def());
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		base_type_to_string(base_type)


	QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_bool) == "bool");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_int) == "int");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_float) == "float");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_string) == "string");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "function");
	}



	////////////////////////			typeid_t


	bool typeid_t::check_invariant() const{
		if (_unresolved_type_symbol.empty() == false){
				QUARK_ASSERT(_base_type == base_type::k_null);
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id == "");
		}
		else{
			if(_base_type == base_type::k_null){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == base_type::k_bool){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == base_type::k_int){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == base_type::k_float){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == base_type::k_string){
				QUARK_ASSERT(_parts.empty());
				QUARK_ASSERT(_struct_def_id.empty());
			}
			else if(_base_type == base_type::k_struct){
				QUARK_ASSERT(_parts.empty() == true);
				QUARK_ASSERT(_struct_def_id.empty() == false);
			}
			else if(_base_type == base_type::k_vector){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else if(_base_type == base_type::k_function){
				QUARK_ASSERT(_parts.empty() == false);
				QUARK_ASSERT(_struct_def_id.empty() == true);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		return true;
	}


	void typeid_t::swap(typeid_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

		std::swap(_base_type, other._base_type);
		_parts.swap(other._parts);
		_struct_def_id.swap(other._struct_def_id);
		_unresolved_type_symbol.swap(other._unresolved_type_symbol);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}


	std::string typeid_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_unresolved_type_symbol != ""){
			return "unresolved:" + _unresolved_type_symbol;
		}
		else if(_base_type == base_type::k_struct){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == base_type::k_vector){
			return "[" + _parts[0].to_string() + "]";
		}
		else if(_base_type == base_type::k_function){
			auto s = _parts[0].to_string() + " (";
			if(_parts.size() > 2){
				for(int i = 1 ; i < _parts.size() - 1 ; i++){
					s = s + _parts[i].to_string() + ",";
				}
				s = s + _parts[_parts.size() - 1].to_string();
			}
			s = s + ")";
			return s;
		}
		else{
			return base_type_to_string(_base_type);
		}
//		return json_to_compact_string(typeid_to_json(*this));
	}

	QUARK_UNIT_TESTQ("typeid_t{}", ""){
	}




	//////////////////////////////////////////////////		scope_def_t



	scope_ref_t scope_def_t::make_struct_object(const std::vector<member_t>& members){
		auto r = std::make_shared<scope_def_t>(scope_def_t(
			etype::k_struct_scope,
			{},
			{},
			members,
			{},
			{},
			{},
			{}
		));
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_ref_t scope_def_t::make_global_scope(
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const std::map<std::string, symbol_t>& symbols,
		const std::map<std::string, std::shared_ptr<const scope_def_t> > objects
	)
	{
		auto r = std::make_shared<scope_def_t>(
			scope_def_t(
				etype::k_global_scope,
				{},
				{},
				{},
				statements,
				{},
				symbols,
				objects
			)
		);

		QUARK_ASSERT(r->check_invariant());
		return r;
	}


	scope_def_t::scope_def_t(
		etype type,
		const std::vector<member_t>& args,
		const std::vector<member_t>& local_variables,
		const std::vector<member_t>& members,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const typeid_t& return_type,
		const std::map<std::string, symbol_t>& symbols,
		const std::map<std::string, std::shared_ptr<const scope_def_t> > objects
		)
	:
		_type(type),
		_args(args),
		_local_variables(local_variables),
		_members(members),
		_statements(statements),
		_return_type(return_type),
		_symbols(symbols),
		_objects(objects)
	{
		QUARK_ASSERT(check_invariant());
	}


	scope_def_t::scope_def_t(const scope_def_t& other) :
		_type(other._type),
		_args(other._args),
		_local_variables(other._local_variables),
		_members(other._members),
		_statements(other._statements),
		_return_type(other._return_type),
		_symbols(other._symbols),
		_objects(other._objects)
	{
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	bool scope_def_t::shallow_check_invariant() const {
//		QUARK_ASSERT(_types_collector.check_invariant());
		return true;
	}

	bool scope_def_t::check_invariant() const {
		//??? Check for duplicates? Other things?
		for(const auto& m: _args){
			QUARK_ASSERT(m.check_invariant());
		}
		for(const auto& m: _local_variables){
			QUARK_ASSERT(m.check_invariant());
		}
		for(const auto& m: _members){
			QUARK_ASSERT(m.check_invariant());
		}


		if(_type == etype::k_function_scope){
//			QUARK_ASSERT(_return_type._base_type != base_type::k_null && _return_type.check_invariant());
		}
		else if(_type == etype::k_struct_scope){
			QUARK_ASSERT(_return_type._base_type == base_type::k_null);
		}
		else if(_type == etype::k_global_scope){
			QUARK_ASSERT(_return_type._base_type == base_type::k_null);
		}
		else if(_type == etype::k_block){
			QUARK_ASSERT(_return_type._base_type != base_type::k_null && _return_type.check_invariant());
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

	bool scope_def_t::operator==(const scope_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_type != other._type){
			return false;
		}
		if(_args != other._args){
			return false;
		}
		if(_local_variables != other._local_variables){
			return false;
		}
		if(_members != other._members){
			return false;
		}
		if(!(_statements == other._statements)){
			return false;
		}
		if(!(_return_type == other._return_type)){
			return false;
		}
		if(!(_symbols == other._symbols)){
			return false;
		}
		if(_objects != other._objects){
			return false;
		}
		return true;
	}

	QUARK_UNIT_TESTQ("scope_def_t::operator==", ""){
		const auto a = scope_def_t::make_global_scope({}, {}, {});
		const auto b = scope_def_t::make_global_scope({}, {}, {});
		QUARK_TEST_VERIFY(*a == *b);
	}





	string scope_type_to_string(scope_def_t::etype type){
		if(type == scope_def_t::etype::k_function_scope){
			return "function";
		}
		else if(type == scope_def_t::etype::k_struct_scope){
			return "struct";
		}
		else if(type == scope_def_t::etype::k_global_scope){
			return "global";
		}
		else if(type == scope_def_t::etype::k_block){
			return "subscope";
		}
		else{
			QUARK_ASSERT(false);
		}
	}




	//////		JSON



	json_t vector_def_to_json(const vector_def_t& s){
		return {
		};
	}

	json_t typeid_to_json(const typeid_t& t){
		if(t._parts.empty() && t._struct_def_id.empty()){
			return base_type_to_string(t.get_base_type());
		}
		else{
			auto parts = json_t::make_array();
			for(const auto e: t._parts){
				parts = push_back(parts, typeid_to_json(e));
			}
			return make_object({
				{ "base_type", json_t(base_type_to_string(t.get_base_type())) },
				{ "parts", parts },
				{ "struct_def_id", t._struct_def_id.empty() == false ? t._struct_def_id : json_t() }
			});
		}
	}


	json_t member_to_json(const std::vector<member_t>& members){
		std::vector<json_t> r;
		for(const auto i: members){
			const auto member = make_object({
				{ "type", typeid_to_json(i._type) },
				{ "value", i._value ? value_to_json(*i._value) : json_t() },
				{ "name", json_t(i._name) }
			});
			r.push_back(json_t(member));
		}
		return r;
	}


	json_t symbols_to_json(const std::map<std::string, symbol_t>& s){
		std::map<string, json_t> r;
		for(const auto i: s){
			const auto t = i.second._type;
			std::string symbol_type = "";
			if (t == symbol_t::symbol_type::k_null){
				symbol_type = "null";
			}
			else if (t == symbol_t::symbol_type::k_struct_def_object){
				symbol_type = "struct_def_object";
			}
			else if (t == symbol_t::symbol_type::k_constant){
				symbol_type = "constant";
			}
			else{
				QUARK_ASSERT(false);
			}
			const auto e = make_object({
				{ "symtype", symbol_type },
				{ "object_id", i.second._object_id },
				{ "constant", i.second._constant ? expression_to_json(*i.second._constant) : json_t() },
				{ "typeid", typeid_to_json(i.second._typeid) }
			});
			r[i.first] = e;
		}
		return r;
	}

	json_t objects_to_json(const std::map<std::string, std::shared_ptr<const scope_def_t> >& s){
		std::map<string, json_t> r;
		for(const auto i: s){
			r[i.first] = scope_def_to_json(*i.second);
		}
		return r;
	}

	json_t scope_def_to_json(const scope_def_t& scope_def){
		const auto args = member_to_json(scope_def._args);
		const auto locals = member_to_json(scope_def._local_variables);
		const auto members = member_to_json(scope_def._members);

		std::vector<json_t> statements;
		for(const auto i: scope_def._statements){
			statements.push_back(statement_to_json(*i));
		}
		json_t statements2(statements);

		const auto symbols = symbols_to_json(scope_def.get_symbols());
		const auto objects = objects_to_json(scope_def.get_objects());

		return make_object({
			{ "objtype", json_t(scope_type_to_string(scope_def._type)) },
			{ "args", args.get_array_size() == 0 ? json_t() : json_t(args) },
			{ "locals", locals.get_array_size() == 0 ? json_t() : json_t(locals) },
			{ "members", members.get_array_size() == 0 ? json_t() : json_t(members) },
			{ "statements", statements2.get_array_size() == 0 ? json_t() : json_t(statements2) },
			{ "return_type", scope_def._return_type.is_null() ? json_t() : scope_def._return_type.to_string() },
			{ "symbols", symbols.get_object_size() == 0 ? json_t() : symbols },
			{ "objects", objects.get_object_size() == 0 ? json_t() : objects }
		});
	}

	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}

	scope_ref_t scope_def_t::make_function_object(
		const std::vector<member_t>& args,
		const std::vector<member_t>& local_variables,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const typeid_t& return_type,
		const std::map<std::string, symbol_t>& symbols,
		const std::map<std::string, std::shared_ptr<const scope_def_t> > objects
	)
	{
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		for(const auto i: local_variables){ QUARK_ASSERT(i.check_invariant()); };

		auto function = make_shared<scope_def_t>(scope_def_t(
			scope_def_t::etype::k_function_scope,
			args,
			local_variables,
			{},
			statements,
			return_type,
			symbols,
			objects
		));
		return function;
	}


	////////////////////////			symbol_t


		bool symbol_t::operator==(const symbol_t& other) const{
			return _type == other._type
				&& _object_id == other._object_id
				&& compare_shared_values(_constant, other._constant)
				&& _typeid == other._typeid;
		}

	////////////////////////			member_t


	member_t::member_t(const typeid_t& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(type._base_type != base_type::k_null && type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type._base_type != base_type::k_null && _type.check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		QUARK_ASSERT(!_value || _value->check_invariant());
		if(_value){
			QUARK_ASSERT(_type == _value->get_type());
		}
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return (_type == other._type)
			&& (_name == other._name)
			&& compare_shared_values(_value, other._value);
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type.to_string() + "> name: \"" + member._name + "\"");
	}


	//??? more. Use to_json().
	void trace(const scope_ref_t& e){
		QUARK_ASSERT(e && e->check_invariant());
/*
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("struct_def_t");
		trace_vec("members", e._members);
*/
	}



	//////////////////////////////////////		vector_def_t



	vector_def_t vector_def_t::make2(
		const type_identifier_t& name,
		const typeid_t& element_type)
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(element_type._base_type != base_type::k_null && element_type.check_invariant());

		vector_def_t result;
		result._name = name;
		result._element_type = element_type;

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_name.to_string().size() > 0 );

		QUARK_ASSERT(_element_type._base_type != base_type::k_null && _element_type.check_invariant());
		return true;
	}

	bool vector_def_t::operator==(const vector_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_name != other._name){
			return false;
		}
		else if(!(_element_type == other._element_type)){
			return false;
		}
		return true;
	}

	void trace(const vector_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("vector_def_t");
		QUARK_TRACE_SS("element_type: " << e._element_type.to_string());
	}




	////////////////////////			ast_t


	ast_t::ast_t() :
		_global_scope(scope_def_t::make_global_scope({}, {}, {}))
	{
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(
		std::shared_ptr<const scope_def_t> global_scope
	) :
		_global_scope(global_scope)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		QUARK_ASSERT(_global_scope && _global_scope->check_invariant());
		return true;
	}



	void trace(const ast_t& program){
		QUARK_ASSERT(program.check_invariant());
		QUARK_SCOPED_TRACE("program");

		const auto s = json_to_pretty_string(ast_to_json(program));
		QUARK_TRACE(s);
	}

	json_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return scope_def_to_json(*ast.get_global_scope());
	}



	////////////////////	Helpers for making tests.


} //	floyd_parser
