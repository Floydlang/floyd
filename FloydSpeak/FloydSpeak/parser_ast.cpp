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
#include "parser_types_collector.h"
#include "ast_utils.h"


namespace floyd_parser {
	using std::vector;
	using std::string;
	using std::pair;
	using std::make_shared;


	//////////////////////////////////////////////////		base_type


	string base_type_to_string(const base_type t){
		if(t == k_int){
			return "int";
		}
		if(t == k_bool){
			return "bool";
		}
		else if(t == k_string){
			return "string";
		}
		else if(t == k_struct){
			return "struct";
		}
		else if(t == k_vector){
			return "vector";
		}
		else if(t == k_function){
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
		if(type == k_int){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == k_bool){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == k_string){
			QUARK_TRACE("<" + base_type_to_string(type) + "> " + label);
		}
		else if(type == k_struct){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
			trace(t.get_struct_def());
		}
		else if(type == k_vector){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(type == k_function){
			QUARK_SCOPED_TRACE("<" + base_type_to_string(type) + "> " + label);
			trace(t.get_function_def());
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		base_type_to_string(base_type)


	QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
		QUARK_TEST_VERIFY(base_type_to_string(k_int) == "int");
		QUARK_TEST_VERIFY(base_type_to_string(k_bool) == "bool");
		QUARK_TEST_VERIFY(base_type_to_string(k_string) == "string");
		QUARK_TEST_VERIFY(base_type_to_string(k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(k_function) == "function");
	}



	////////////////////////			type_def_t


	bool type_def_t::check_invariant() const{
		if(_base_type == k_int){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == k_bool){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == k_string){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == k_struct){
			QUARK_ASSERT(_struct_def);
			QUARK_ASSERT(_struct_def->check_invariant());
			QUARK_ASSERT(!_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == k_vector){
			QUARK_ASSERT(!_struct_def);
			QUARK_ASSERT(_vector_def);
			QUARK_ASSERT(!_function_def);
		}
		else if(_base_type == k_function){
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

	std::string type_def_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(_base_type == k_struct){
			return _struct_def->_name.to_string();
		}
		else if(_base_type == k_vector){
			return base_type_to_string(_base_type);
		}
		else if(_base_type == k_function){
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
		if(btype == k_struct){
			return to_signature(t.get_struct_def());
		}
		else if(btype == k_vector){
			const auto vector_value_s = "";
			return label + "<vector>" + "[" + vector_value_s + "]";
		}
		else if(btype == k_function){
			return to_signature(t.get_function_def());
		}
		else{
			return label + "<" + base_type + ">";
		}
	}


	//////////////////////////////////////////////////		type_identifier_t


	type_identifier_t type_identifier_t::resolve(const std::shared_ptr<const type_def_t>& resolved){
		QUARK_ASSERT(resolved && resolved->check_invariant());

		type_identifier_t result;
		result._type_magic = "";
		result._resolved = resolved;
		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	type_identifier_t type_identifier_t::make(const std::string& s){
		QUARK_ASSERT(is_valid_type_identifier(s));

		const type_identifier_t result(s);

		QUARK_ASSERT(result.check_invariant());
		return result;
	}


	type_identifier_t::type_identifier_t(const type_identifier_t& other) :
		_type_magic(other._type_magic),
		_resolved(other._resolved)
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

		return _type_magic == other._type_magic && compare_shared_values(_resolved, other._resolved);
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
		_resolved.swap(other._resolved);
	}

	std::string type_identifier_t::to_string() const {
		QUARK_ASSERT(check_invariant());

		if(is_resolved()){
			return _resolved->to_string();
		}
		else{
			return _type_magic;
		}
	}

	bool type_identifier_t::check_invariant() const {
		if(_resolved){
			QUARK_ASSERT(_type_magic == "");
			QUARK_ASSERT(_resolved->check_invariant());
		}
		else{
			QUARK_ASSERT(!_resolved);
			QUARK_ASSERT(_type_magic != "");
			QUARK_ASSERT(is_valid_type_identifier(_type_magic));
		}
		return true;
	}


	std::shared_ptr<const type_def_t> type_identifier_t::get_resolved() const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_resolved());

		return _resolved;
	}

	bool type_identifier_t::is_resolved() const{
		QUARK_ASSERT(check_invariant());

		return _resolved ? true : false;
	}




	void trace(const type_identifier_t& v){
		QUARK_TRACE("type_identifier_t <" + v.to_string() + ">");
	}


	//////////////////////////////////////////////////		executable_t


		executable_t::executable_t(hosts_function_t host_function, std::shared_ptr<host_data_i> host_function_param) :
			_host_function(host_function),
			_host_function_param(host_function_param)
		{
			QUARK_ASSERT(host_function != nullptr);
			QUARK_ASSERT(host_function_param);

			QUARK_ASSERT(check_invariant());
		}

		executable_t::executable_t(const std::vector<std::shared_ptr<statement_t> >& statements) :
			_host_function(nullptr),
			_statements(statements)
		{
			QUARK_ASSERT(check_invariant());
		}

		bool executable_t::check_invariant() const{
			if(_host_function == nullptr){
				for(const auto s: _statements){
					QUARK_ASSERT(s && s->check_invariant());
				}
				QUARK_ASSERT(!_host_function_param);
			}
			else{
				QUARK_ASSERT(_statements.empty());
				QUARK_ASSERT(_host_function_param /*&& _host_function_param->check_invariant()*/);
			}
			return true;
		 }

		 bool executable_t::operator==(const executable_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			if(_host_function != other._host_function
//???			|| ()compare_shared_values(_host_function_param, other._host_function_param)
			|| _statements.size() != other._statements.size()){
				return false;
			}

			if(_host_function == nullptr){
				for(int i = 0 ; i < _statements.size() ; i++){
					if(!(*_statements[i] == *other._statements[i])){
						return false;
					}
				}
			}
			return true;
		}


	//////////////////////////////////////////////////		scope_def_t

	types_collector_t add_builtin_types(const types_collector_t& types){
		auto result = types;

		//	bool
		{
			auto def = make_shared<type_def_t>(type_def_t::make(k_bool));
			result = result.define_type_xyz("bool", def);
		}

		//	int
		{
			result = result.define_type_xyz("int", make_shared<type_def_t>(type_def_t::make_int()));
		}

		//	string
		{
			auto def = make_shared<type_def_t>(type_def_t::make(k_string));
			result = result.define_type_xyz("string", def);
		}

		QUARK_ASSERT(result.check_invariant());
		return result;
	}



	QUARK_UNIT_TESTQ("add_builtin_types()", ""){
		const auto t = types_collector_t();
		const auto a = add_builtin_types(t);
		QUARK_TEST_VERIFY(a.check_invariant());

/*
???
		const auto d = a.resolve_identifier("bool");
		QUARK_TEST_VERIFY(d);
		QUARK_TEST_VERIFY(d->get_type() == k_bool);

		const auto b = a.resolve_identifier("int");
		QUARK_TEST_VERIFY(b);
		QUARK_TEST_VERIFY(b->get_type() == k_int);

		const auto c = a.resolve_identifier("string");
		QUARK_TEST_VERIFY(c);
		QUARK_TEST_VERIFY(c->get_type() == k_string);
*/
	}



	scope_ref_t scope_def_t::make_struct(const type_identifier_t& name, const std::vector<member_t>& members){
		auto r = std::make_shared<scope_def_t>(scope_def_t(k_struct_scope, name, members, executable_t({}), types_collector_t(), {}));
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	std::shared_ptr<scope_def_t> scope_def_t::make2(etype type, const type_identifier_t& name, const std::vector<member_t>& members, const executable_t& executable, const types_collector_t& types_collector, const type_identifier_t& return_type){
		auto r = std::make_shared<scope_def_t>(scope_def_t(type, name, members, executable, types_collector, return_type));
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_ref_t scope_def_t::make_global_scope(){
		auto r = std::make_shared<scope_def_t>(
			scope_def_t(k_global_scope, type_identifier_t::make("global"), {}, executable_t({}), {}, {})
		);

		r->_types_collector = add_builtin_types(r->_types_collector);


		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_def_t::scope_def_t(etype type, const type_identifier_t& name, const std::vector<member_t>& members, const executable_t& executable, const types_collector_t& types_collector, const type_identifier_t& return_type) :
		_type(type),
		_name(name),
		_members(members),
		_executable(executable),
		_types_collector(types_collector),
		_return_type(return_type)
	{
		QUARK_ASSERT(check_invariant());
	}

	scope_def_t::scope_def_t(const scope_def_t& other) :
		_type(other._type),
		_name(other._name),
		_members(other._members),
		_executable(other._executable),
		_types_collector(other._types_collector),
		_return_type(other._return_type)
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

		QUARK_ASSERT(_executable.check_invariant());
		QUARK_ASSERT(_types_collector.check_invariant());
		QUARK_ASSERT(_return_type.check_invariant());


		//??? Check for duplicates? Other things?
		for(const auto& m: _members){
			QUARK_ASSERT(m.check_invariant());
		}


		if(_type == k_function_scope){
		}
		else if(_type == k_struct_scope){
			QUARK_ASSERT(_return_type.is_null());
		}
		else if(_type == k_global_scope){
			QUARK_ASSERT(_return_type.is_null());
		}
		else if(_type == k_subscope){
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
		if(_members != other._members){
			return false;
		}
		if(!(_executable == other._executable)){
			return false;
		}
		if(!(_types_collector == other._types_collector)){
			return false;
		}
		if(_return_type != other._return_type){
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

	json_value_t executable_to_json(const executable_t& e){
		std::vector<json_value_t> statements;
		for(const auto i: e._statements){
			statements.push_back(statement_to_json(*i));
		}

		return make_object({
			{ "_host_function_param", e._host_function_param ? json_value_t("USED") : json_value_t() },
			{ "_statements", json_value_t(statements) },
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

		return make_object({
			{ "_type", json_value_t(scope_type_to_string(scope_def._type)) },
			{ "_name", json_value_t(scope_def._name.to_string()) },
			{ "_members", members.empty() ? json_value_t() :json_value_t(members) },
			{ "_executable", executable_to_json(scope_def._executable) },
			{ "_types", types_collector_to_json(scope_def._types_collector) },
			{ "_return_type", json_value_t(scope_def._return_type.to_string()) }
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
		const type_identifier_t& return_type,
		const std::vector<member_t>& args,
		const executable_t& executable,
		const types_collector_t& types_collector,
		const std::vector<member_t>& local_variables
	)
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(return_type.check_invariant());
		for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };
		QUARK_ASSERT(executable.check_invariant());
		QUARK_ASSERT(types_collector.check_invariant());
		for(const auto i: local_variables){ QUARK_ASSERT(i.check_invariant()); };

		const auto body_identifier = type_identifier_t::make("___body");

		const auto f_statements = std::vector<std::shared_ptr<statement_t> >{
			make_shared<statement_t>(
				statement_t{ make__return_statement(expression_t::make_function_call(body_identifier, std::vector<expression_t>{}, return_type)) }
			)
		};

		auto function = scope_def_t::make2(scope_def_t::k_function_scope, name, args, executable_t(f_statements), types_collector, return_type);
		auto body = scope_def_t::make2(scope_def_t::k_subscope, body_identifier, local_variables, executable, {}, return_type);
		auto body_type_def = make_shared<type_def_t>(type_def_t::make_function_def(body));

		function->_types_collector = function->_types_collector.define_type_xyz(body_identifier.to_string(), body_type_def);
		return function;
	}





	scope_ref_t make_function_def(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<member_t>& args,
		const executable_t& executable,
		const types_collector_t& types_collector,
		const std::vector<member_t>& local_variables
	)
	{
		return scope_def_t::make_function_def(name, return_type, args, executable, types_collector, local_variables);
	}


	QUARK_UNIT_TESTQ("function_def_t(function_def_t&)", ""){
		const auto a = make_function_def(
			type_identifier_t::make("a"),
			type_identifier_t::make_int(),
			{},
			executable_t({}),
			{},
			{}
		);
		const auto b(a);
		QUARK_TEST_VERIFY(b->check_invariant());
	}



	////////////////////////			member_t


	member_t::member_t(const type_identifier_t& type, const std::string& name, const value_t& init_value) :
		_type(make_shared<type_identifier_t>(type)),
		_name(name),
		_value(make_shared<value_t>(init_value))
	{
		QUARK_ASSERT(type.check_invariant());
		QUARK_ASSERT(name.size() > 0);
		QUARK_ASSERT(init_value.check_invariant());
		QUARK_ASSERT(type == init_value.get_type());

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const type_identifier_t& type, const std::string& name) :
		_type(make_shared<type_identifier_t>(type)),
		_name(name)
	{
		QUARK_ASSERT(type.check_invariant());
		QUARK_ASSERT(name.size() > 0);

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_type && _type->check_invariant());
		QUARK_ASSERT(_name.size() > 0);
		QUARK_ASSERT(!_value || _value->check_invariant());
		if(_value){
			QUARK_ASSERT(_type->to_string() == _value->get_type().to_string());
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
			const type_identifier_t typedef_s = *member._type;
			const string member_type = "<" + typedef_s.to_string() + ">";

			//	"<string>first_name"
			const string member_result = member_type + member_name;

			body = body + member_result + ",";
		}
		body = remove_trailing_comma(body);

		if(t->_type== scope_def_t::k_function_scope || t->_type== scope_def_t::k_subscope){
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
		const type_identifier_t& element_type)
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(element_type.check_invariant());

		vector_def_t result;
		result._name = name;
		result._element_type = element_type;

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_name.to_string().size() > 0 );

		QUARK_ASSERT(_element_type.check_invariant());
		QUARK_ASSERT(_element_type.to_string().size() > 0 );
		return true;
	}

	bool vector_def_t::operator==(const vector_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_name != other._name){
			return false;
		}
		if(_element_type != other._element_type){
			return false;
		}
		return true;
	}

	void trace(const vector_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("vector_def_t");
		QUARK_TRACE_SS("element_type: " << e._element_type.to_string());
	}


	std::string to_signature(const vector_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		return std::string("<vector>") + "[" + t._element_type.to_string() + "]";
	}




	////////////////////////			ast_t





	ast_t::ast_t() :
		_global_scope(scope_def_t::make_global_scope())
	{
		QUARK_ASSERT(check_invariant());
	}

	ast_t::ast_t(const std::shared_ptr<const scope_def_t>& global_scope) :
		_global_scope(global_scope)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool ast_t::check_invariant() const {
		QUARK_ASSERT(_global_scope);
		QUARK_ASSERT(_global_scope->check_invariant());
		return true;
	}




	void trace(const ast_t& program){
		QUARK_ASSERT(program.check_invariant());
		QUARK_SCOPED_TRACE("program");

		const auto s = json_to_compact_string(ast_to_json(program));
		QUARK_TRACE(s);
	}

	json_value_t ast_to_json(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return make_object({
			{ "_global_scope", scope_def_to_json(*ast._global_scope) }
		});
	}

	resolved_path_t make_resolved_root(const ast_t& ast){
		QUARK_ASSERT(ast.check_invariant());

		return resolved_path_t{ { ast._global_scope } };
	}


	////////////////////	Helpers for making tests.




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

	if(scope_def->_type == scope_def_t::k_function){
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



