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
#include "ast_utils.h"


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
	void trace(const type_def_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		const auto type = t.get_type();
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
			trace(t.get_struct_def());
		}
		else if(type == base_type::k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == base_type::k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
			trace(t.get_function_def());
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



	////////////////////////			type_def_t


	bool type_def_t::check_invariant() const{
		if(_base_type == base_type::k_null){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_bool){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_int){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_float){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_string){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_struct){
			QUARK_ASSERT(_struct_def);
			QUARK_ASSERT(_struct_def->check_invariant());
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_vector){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == base_type::k_function){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(_function_def);
			QUARK_ASSERT(_function_def->check_invariant());
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

	bool type_def_t::operator==(const type_def_t& other) const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_base_type != other._base_type){
			return false;
		}
		if(!compare_shared_values(_struct_def, other._struct_def)){
			return false;
		}
		if(!compare_shared_values(_vector_def, other._vector_def)){
			return false;
		}
		if(!compare_shared_values(_function_def, other._function_def)){
			return false;
		}
		return true;
	}

	void type_def_t::swap(type_def_t& rhs){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(rhs.check_invariant());

		std::swap(_base_type, rhs._base_type);
		std::swap(_struct_def, rhs._struct_def);
		std::swap(_vector_def, rhs._vector_def);
		std::swap(_function_def, rhs._function_def);

		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(rhs.check_invariant());
	}


	std::string type_def_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_base_type == base_type::k_struct){
			return _struct_def->_name.to_string();
		}
		else if(_base_type == base_type::k_vector){
			return _vector_def->_name.to_string();
		}
		else if(_base_type == base_type::k_function){
			return _function_def->_name.to_string();
		}
		else{
			return base_type_to_string(_base_type);
		}
	}

	std::string to_signature(const type_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto btype = t.get_type();
		const auto base_type = base_type_to_string(btype);

		const string label = "";
		if(btype == base_type::k_struct){
			return to_signature(t.get_struct_def());
		}
		else if(btype == base_type::k_vector){
			const auto vector_value_s = "";
			return label + "<vector>" + "[" + vector_value_s + "]";
		}
		else if(btype == base_type::k_function){
			return to_signature(t.get_function_def());
		}
		else{
			return label + "<" + base_type + ">";
		}
	}






	//////////////////////////////////////////////////		type_identifier_t



	type_identifier_t type_identifier_t::make(const std::string& s){
		QUARK_ASSERT(is_valid_type_identifier(s));

		const type_identifier_t result(s);

		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	type_identifier_t::type_identifier_t(const type_identifier_t& other) :
		_type_magic(other._type_magic)
	{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());
	}

/*
	type_identifier_t type_identifier_t::operator=(const type_identifier_t& other){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		type_identifier_t temp(other);
		temp.swap(*this);
		return *this;
	}
*/

	bool type_identifier_t::operator==(const type_identifier_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _type_magic == other._type_magic /*&& compare_shared_values(_resolved, other._resolved)*/;
	}

	bool type_identifier_t::operator!=(const type_identifier_t& other) const{
		return !(*this == other);
	}


	type_identifier_t::type_identifier_t(const char s[]) :
		_type_magic(s)
	{
		QUARK_ASSERT(s != nullptr);
		QUARK_ASSERT(is_valid_type_identifier(std::string(s)));

		QUARK_ASSERT(check_invariant());
	}

	type_identifier_t::type_identifier_t(const std::string& s) :
		_type_magic(s)
	{
		QUARK_ASSERT(is_valid_type_identifier(s));

		QUARK_ASSERT(check_invariant());
	}

	void type_identifier_t::swap(type_identifier_t& other){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		_type_magic.swap(other._type_magic);
	}

	std::string type_identifier_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		return _type_magic;
	}

	bool type_identifier_t::check_invariant() const {
		QUARK_ASSERT(_type_magic != "");
		QUARK_ASSERT(is_valid_type_identifier(_type_magic));
		return true;
	}


	void trace(const type_identifier_t& v){
		QUARK_TRACE("type_identifier_t <" + v.to_string() + ">");
	}





	//////////////////////////////////////////////////		scope_def_t



	scope_ref_t scope_def_t::make_struct(const type_identifier_t& name, const std::vector<member_t>& members){
		auto r = std::make_shared<scope_def_t>(scope_def_t(etype::k_struct_scope, name, {}, {}, members, {}, {}, efunc_variant::k_not_relevant));
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_ref_t scope_def_t::make_global_scope(){
		auto r = std::make_shared<scope_def_t>(
			scope_def_t(etype::k_global_scope, type_identifier_t::make("global"), {}, {}, {}, {}, {}, efunc_variant::k_not_relevant)
		);

		QUARK_ASSERT(r->check_invariant());
		return r;
	}


	scope_def_t::scope_def_t(
		etype type,
		const type_identifier_t& name,
		const std::vector<member_t>& args,
		const std::vector<member_t>& local_variables,
		const std::vector<member_t>& members,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const std::shared_ptr<const type_def_t>& return_type,
		const efunc_variant& function_variant
		)
	:
		_type(type),
		_name(name),
		_args(args),
		_local_variables(local_variables),
		_members(members),
		_statements(statements),
		_return_type(return_type),
		_function_variant(function_variant)
	{
		QUARK_ASSERT(check_invariant());
	}

	scope_def_t::scope_def_t(const scope_def_t& other) :
		_type(other._type),
		_name(other._name),
		_args(other._args),
		_local_variables(other._local_variables),
		_members(other._members),
		_statements(other._statements),
		_return_type(other._return_type),
		_function_variant(other._function_variant)
	{
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	bool scope_def_t::shallow_check_invariant() const {
//		QUARK_ASSERT(_types_collector.check_invariant());
		return true;
	}

	bool scope_def_t::check_invariant() const {
		QUARK_ASSERT(_name.check_invariant());

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
			QUARK_ASSERT(_function_variant != efunc_variant::k_not_relevant);
			QUARK_ASSERT(_return_type && _return_type->check_invariant());
		}
		else if(_type == etype::k_struct_scope){
			QUARK_ASSERT(_function_variant == efunc_variant::k_not_relevant);
			QUARK_ASSERT(!_return_type);
		}
		else if(_type == etype::k_global_scope){
			QUARK_ASSERT(_function_variant == efunc_variant::k_not_relevant);
			QUARK_ASSERT(!_return_type);
		}
		else if(_type == etype::k_subscope){
			QUARK_ASSERT(_function_variant == efunc_variant::k_not_relevant);
			QUARK_ASSERT(_return_type && _return_type->check_invariant());
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
		if(_name != other._name){
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
		if(_return_type != other._return_type){
			return false;
		}
		if(_function_variant != other._function_variant){
			return false;
		}
		return true;
	}

	QUARK_UNIT_TESTQ("scope_def_t::operator==", ""){
		const auto a = scope_def_t::make_global_scope();
		const auto b = scope_def_t::make_global_scope();
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
		else if(type == scope_def_t::etype::k_subscope){
			return "subscope";
		}
		else{
			QUARK_ASSERT(false);
		}
	}




	//////		JSON



	json_value_t vector_def_to_json(const vector_def_t& s){
		return {
		};
	}

	json_value_t type_def_to_json(const type_def_t& type_def){
		return make_object({
			{ "_base_type", json_value_t(type_def.to_string()) },
			{ "_struct_def", type_def.get_type() == base_type::k_struct ? scope_def_to_json(*type_def.get_struct_def()) : json_value_t() },
			{ "_vector_def", type_def.get_type() == base_type::k_vector ? vector_def_to_json(*type_def.get_vector_def()) : json_value_t() },
			{ "_function_def", type_def.get_type() == base_type::k_function ? scope_def_to_json(*type_def.get_function_def()) : json_value_t() }
		});
	}

	json_value_t scope_def_to_json(const scope_def_t& scope_def){
		std::vector<json_value_t> members;
		for(const auto i: scope_def._members){
			const auto member = make_object({
				{ "_type", json_value_t(i._type->to_string()) },
				{ "_value", i._value ? value_to_json(*i._value) : json_value_t() },
				{ "_name", json_value_t(i._name) }
		});
			members.push_back(json_value_t(member));
		}

		std::vector<json_value_t> statements;
		for(const auto i: scope_def._statements){
			statements.push_back(statement_to_json(*i));
		}

		return make_object({
			{ "_type", json_value_t(scope_type_to_string(scope_def._type)) },
			{ "_name", json_value_t(scope_def._name.to_string()) },
			{ "_members", members.empty() ? json_value_t() :json_value_t(members) },
			{ "_statements", json_value_t(statements) },
			{ "_return_type", scope_def._return_type ? scope_def._return_type->to_string() : json_value_t() }
		});
	}



	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}

	TSHA1 calc_function_body_hash(const scope_ref_t& f){
		QUARK_ASSERT(f && f->check_invariant());

		static int s_counter = 1000;
		s_counter++;
		return CalcSHA1(std::to_string(s_counter));
	}



	scope_ref_t scope_def_t::make_function_def(
		const type_identifier_t& name,
		const std::vector<member_t>& args,
		const std::vector<member_t>& local_variables,
		const std::vector<std::shared_ptr<statement_t> >& statements,
		const std::shared_ptr<const type_def_t>& return_type
	)
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(return_type && return_type->check_invariant());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		for(const auto i: local_variables){ QUARK_ASSERT(i.check_invariant()); };

		auto function = make_shared<scope_def_t>(scope_def_t(
			scope_def_t::etype::k_function_scope,
			name,
			args,
			local_variables,
			{},
			statements,
			return_type,
			efunc_variant::k_interpreted
		));
		return function;
	}

	scope_ref_t scope_def_t::make_builtin_function_def(const type_identifier_t& name, efunc_variant function_variant, const std::shared_ptr<const type_def_t>& type){
		QUARK_ASSERT(name.check_invariant());
		QUARK_UT_VERIFY(function_variant != efunc_variant::k_not_relevant && function_variant != efunc_variant::k_interpreted);
		QUARK_ASSERT(type && type->check_invariant());

		auto function = make_shared<scope_def_t>(scope_def_t(
			scope_def_t::etype::k_function_scope,
			name,
			{},
			{},
			{},
			{},
			type,
			efunc_variant::k_default_constructor
		));
		return function;
	}



	////////////////////////			member_t


	member_t::member_t(const std::shared_ptr<const type_def_t>& type, const std::string& name, const value_t& init_value) :
		_type(type),
		_name(name),
		_value(make_shared<value_t>(init_value))
	{
		QUARK_ASSERT(type && type->check_invariant());
		QUARK_ASSERT(name.size() > 0);
		QUARK_ASSERT(init_value.check_invariant());
		QUARK_ASSERT(type == init_value.get_type());

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const std::shared_ptr<const type_def_t>& type, const std::string& name) :
		_type(type),
		_name(name)
	{
		QUARK_ASSERT(type && type->check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type && _type->check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		QUARK_ASSERT(!_value || _value->check_invariant());
		if(_value){
			QUARK_ASSERT(*_type == *_value->get_type());
		}
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return (*_type == *other._type)
			&& (_name == other._name)
			&& compare_shared_values(_value, other._value);
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type->to_string() + "> name: \"" + member._name + "\"");
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

	//??? more
	std::string to_signature(const scope_ref_t& t){
		QUARK_ASSERT(t && t->check_invariant());

		string body;
		for(const auto& member : t->_members) {
			const auto member_name = member._name;
			const auto typedef_s = *member._type;
			const string member_type = "<" + typedef_s.to_string() + ">";

			//	"<string>first_name"
			const string member_result = member_type + member_name;

			body = body + member_result + ",";
		}
		body = remove_trailing_comma(body);

		if(t->_type== scope_def_t::etype::k_function_scope || t->_type== scope_def_t::etype::k_subscope){
			const auto body_hash = calc_function_body_hash(t);
			body = body + std::string("<body_hash>") + SHA1ToStringPlain(body_hash);
		}
		else{
		}
		return "<" + scope_type_to_string(t->_type) + ">" + "{" + body + "}";
	}


	//////////////////////////////////////		vector_def_t



	vector_def_t vector_def_t::make2(
		const type_identifier_t& name,
		const std::shared_ptr<type_def_t>& element_type)
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(element_type && element_type->check_invariant());

		vector_def_t result;
		result._name = name;
		result._element_type = element_type;

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_name.to_string().size() > 0 );

		QUARK_ASSERT(_element_type && _element_type->check_invariant());
		return true;
	}

	bool vector_def_t::operator==(const vector_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_name != other._name){
			return false;
		}
		else if(!(*_element_type == *other._element_type)){
			return false;
		}
		return true;
	}

	void trace(const vector_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("vector_def_t");
		QUARK_TRACE_SS("element_type: " << e._element_type->to_string());
	}


	std::string to_signature(const vector_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		return std::string("<vector>") + "[" + t._element_type->to_string() + "]";
	}




	////////////////////////			ast_t





	ast_t::ast_t() :
		_global_scope(scope_def_t::make_global_scope())
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		QUARK_ASSERT(_global_scope && _global_scope->check_invariant());
		return true;
	}


	json_value_t symbols_to_json(const std::map<std::string, std::shared_ptr<type_def_t>>& symbols){
		std::map<string, json_value_t> m;
		for(const auto i: symbols){
			m[i.first] = type_def_to_json(*i.second);
		}
		
		return json_value_t::make_object(m);
	}


	void trace(const ast_t& program){
		QUARK_ASSERT(program.check_invariant());
		QUARK_SCOPED_TRACE("program");

		const auto s = json_to_pretty_string(ast_to_json(program));
		QUARK_TRACE(s);
	}

	json_value_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return make_object({
			{ "_symbols", symbols_to_json(ast._symbols) },
			{ "_global_scope", scope_def_to_json(*ast._global_scope) }
		});
	}



	////////////////////	Helpers for making tests.



#if false
	scope_ref_t make_struct0(scope_ref_t scope_def){
		return scope_def_t::make_struct(type_identifier_t::make("struct0"), {});
	}

	scope_ref_t make_struct1(scope_ref_t scope_def){
		return scope_def_t::make_struct(
			type_identifier_t::make("struct1"),
			{
				{ type_identifier_t::make_float(), "x" },
				{ type_identifier_t::make_float(), "y" },
				{ type_identifier_t::make_string(), "name" }
			}
		);
	}

	scope_ref_t make_struct2(scope_ref_t scope_def){
		return make_struct0(scope_def);
	}

	scope_ref_t make_struct3(scope_ref_t scope_def){
		return scope_def_t::make_struct(
			type_identifier_t::make("struct3"),
			{
				{ type_identifier_t::make_int(), "a" },
				{ type_identifier_t::make_string(), "b" }
			}
		);
	}

	scope_ref_t make_struct4(scope_ref_t scope_def){
		return scope_def_t::make_struct(
			type_identifier_t::make("struct4"),
			{
				{ type_identifier_t::make_string(), "x" },
//				{ type_identifier_t::make("struct3"), "y" },
				{ type_identifier_t::make_string(), "z" }
			}
		);
	}

	scope_ref_t make_struct5(scope_ref_t scope_def){
		return scope_def_t::make_struct(
			type_identifier_t::make("struct5"),
			{
				{ type_identifier_t::make_bool(), "a" },
				// pad
				// pad
				// pad
				{ type_identifier_t::make_int(), "b" },
				{ type_identifier_t::make_bool(), "c" },
				{ type_identifier_t::make_bool(), "d" },
				{ type_identifier_t::make_bool(), "e" },
				{ type_identifier_t::make_bool(), "f" },
				{ type_identifier_t::make_string(), "g" },
				{ type_identifier_t::make_bool(), "h" }
			}
		);
	}

	scope_ref_t make_struct6(scope_ref_t scope_def){
		return scope_def_t::make_struct(
			type_identifier_t::make("struct6"),
			{
				{ type_identifier_t::make_bool(), "_bool_true", value_t(true) },
				{ type_identifier_t::make_bool(), "_bool_false", value_t(false) },
				{ type_identifier_t::make_int(), "_int", value_t(111) },
//				{ type_identifier_t::make_float(), "_float" },
				{ type_identifier_t::make_string(), "_string", value_t("test 123") },
				{ type_identifier_t::make("pixel"), "_pixel" }
			}
		);
	}
#endif

} //	floyd_parser







#if 0

class visitor_i {
	public: virtual ~visitor_i(){};
	public: virtual void visitor_i_on_scope(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_function_def(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_struct_def(const scope_ref_t& scope_def){};
	public: virtual void visitor_i_on_global_scope(const scope_ref_t& scope_def){};

	public: virtual void visitor_i_on_expression(const scope_ref_t& scope_def){};
};

scope_ref_t visit_scope(const scope_ref_t& scope_def){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());

	if(scope_def->_type == scope_def_t::base_type::k_function){
		check_type(scope_def->_parent_scope.lock(), scope_def->_return_type.to_string());
	}

	//	Make sure all statements can resolve their symbols.
	for(const auto t: scope_def->_executable._statements){
		are_symbols_resolvable(scope_def, *t);
	}

	//	Make sure all types can resolve their symbols.
	for(const auto t: scope_def->_types_collector._type_definitions){
		const auto type_def = t.second;

		if(type_def->get_type() == base_type::k_struct){
			are_symbols_resolvable(type_def->get_struct_def());
		}
		else if(type_def->get_type() == base_type::k_vector){
//			type_def->_struct_def
		}
		else if(type_def->get_type() == base_type::k_function){
			are_symbols_resolvable(type_def->get_function_def());
		}
	}
}
#endif



