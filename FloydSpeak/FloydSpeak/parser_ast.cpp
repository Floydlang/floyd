//
//  parser_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_ast.h"

#include "parser_statement.h"
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


namespace floyd_parser {
	using std::vector;
	using std::string;
	using std::pair;
	using std::make_shared;


	//////////////////////////////////////////////////		base_type


	string to_string(const base_type t){
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


	void trace(const type_def_t& t, const std::string& label){
		QUARK_ASSERT(t.check_invariant());

		if(t._base_type == k_int){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_bool){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_string){
			QUARK_TRACE("<" + to_string(t._base_type) + "> " + label);
		}
		else if(t._base_type == k_struct){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
			for(const auto m: t._struct_def->_members){
				trace(m);
			}
		}
		else if(t._base_type == k_vector){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
//			trace(*t._vector_def->_value_type, "");
		}
		else if(t._base_type == k_function){
			QUARK_SCOPED_TRACE("<" + to_string(t._base_type) + "> " + label);
			trace(*t._function_def);
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		to_string(base_type)


	QUARK_UNIT_TESTQ("to_string(base_type)", ""){
		QUARK_TEST_VERIFY(to_string(k_int) == "int");
		QUARK_TEST_VERIFY(to_string(k_bool) == "bool");
		QUARK_TEST_VERIFY(to_string(k_string) == "string");
		QUARK_TEST_VERIFY(to_string(k_struct) == "struct");
		QUARK_TEST_VERIFY(to_string(k_vector) == "vector");
		QUARK_TEST_VERIFY(to_string(k_function) == "function");
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

	std::string to_signature(const type_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		const auto base_type = to_string(t._base_type);

		const string label = "";
		if(t._base_type == k_struct){
			return to_signature(*t._struct_def);
		}
		else if(t._base_type == k_vector){
			const auto vector_value_s = "";
			return label + "<vector>" + "[" + vector_value_s + "]";
		}
		else if(t._base_type == k_function){
//			return label + "<function>" + "[" + vector_value_s + "]";

			string arguments;
			for(const auto& arg : t._function_def->_args) {
				//	"<string>first_name"
				const auto a = std::string("") + "<"  + arg._type.to_string() + ">" + arg._identifier;

				arguments = arguments + a + ",";
			}
			arguments = remove_trailing_comma(arguments);

			const auto body_hash = calc_function_body_hash(*t._function_def);

			return label + "<function>" + "args(" + arguments + ") body_hash:" + SHA1ToStringPlain(body_hash);

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

	type_identifier_t type_identifier_t::operator=(const type_identifier_t& other){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		type_identifier_t temp(other);
		temp.swap(*this);
		return *this;
	}

	bool type_identifier_t::operator==(const type_identifier_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return other._type_magic == _type_magic;
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
//		QUARK_ASSERT(_type_magic == "" || _type_magic == "string" || _type_magic == "int" || _type_magic == "float" || _type_magic == "value_type");
		return true;
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

		//	int
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_int;
			result = result.define_type_xyz("int", def);
		}

		//	bool
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_bool;
			result = result.define_type_xyz("bool", def);
		}

		//	string
		{
			auto def = make_shared<type_def_t>();
			def->_base_type = k_string;
			QUARK_ASSERT(def->check_invariant());
			result = result.define_type_xyz("string", def);
		}


//		QUARK_ASSERT(_identifiers.size() == 3);
//		QUARK_ASSERT(_type_definitions.size() == 3);
		QUARK_ASSERT(result.check_invariant());
		return result;
	}




QUARK_UNIT_TESTQ("add_builtin_types()", ""){
	const auto t = types_collector_t();
	const auto a = add_builtin_types(t);
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





	scope_ref_t scope_def_t::make2(etype type, const type_identifier_t& name, const std::vector<member_t>& values, const scope_ref_t parent_scope, const executable_t& executable, const types_collector_t& types_collector){
		auto r = std::make_shared<scope_def_t>(scope_def_t(type, name, values, parent_scope, executable, types_collector));
		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_ref_t scope_def_t::make_global_scope(){
		auto r = std::make_shared<scope_def_t>(
			scope_def_t(k_global, type_identifier_t::make("global"), {}, std::shared_ptr<scope_def_t>(), executable_t({}), {})
		);

		r->_types_collector = add_builtin_types(r->_types_collector);


		QUARK_ASSERT(r->check_invariant());
		return r;
	}

	scope_def_t::scope_def_t(etype type, const type_identifier_t& name, const std::vector<member_t>& values, const scope_ref_t parent_scope, const executable_t& executable, const types_collector_t& types_collector) :
		_type(type),
		_name(name),
		_values(values),
		_parent_scope(parent_scope),
		_executable(executable),
		_types_collector(types_collector)
	{
		QUARK_ASSERT(parent_scope == nullptr || parent_scope->check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	scope_def_t::scope_def_t(const scope_def_t& other) :
		_type(other._type),
		_name(other._name),
		_values(other._values),
		_parent_scope(other._parent_scope),
		_executable(other._executable),
		_types_collector(other._types_collector)
	{
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}

	bool scope_def_t::shallow_check_invariant() const {
		const auto s = _parent_scope.lock();
		QUARK_ASSERT(!s || s->shallow_check_invariant());
//		QUARK_ASSERT(_types_collector.check_invariant());
		return true;
	}

	//??? check name,values,type
	bool scope_def_t::check_invariant() const {
		QUARK_ASSERT(!_parent_scope.lock() || _parent_scope.lock()->shallow_check_invariant());
		QUARK_ASSERT(_executable.check_invariant());
		QUARK_ASSERT(_types_collector.check_invariant());
		return true;
	}

	bool scope_def_t::operator==(const scope_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		const auto a = _parent_scope.lock();
		const auto b = other._parent_scope.lock();

		//	Must point to the SAME OBJECT / address.
		if(a != b){
			return false;
		}

		if(_type != other._type){
			return false;
		}
		if(_name != other._name){
			return false;
		}
		if(_values != other._values){
			return false;
		}
		if(!(_executable == other._executable)){
			return false;
		}
		if(!(_types_collector == other._types_collector)){
			return false;
		}
		return true;
	}

	QUARK_UNIT_TESTQ("scope_def_t::operator==", ""){
		const auto a = scope_def_t::make_global_scope();
		const auto b = scope_def_t::make_global_scope();
		QUARK_TEST_VERIFY(*a == *b);
	}







//////		JSON




	json_value_t struct_def_to_json(const struct_def_t& s){
		std::vector<json_value_t> members;
		for(const auto i: s._members){
			const auto member = std::map<string, json_value_t>{
				{ "_type", json_value_t(i._type->to_string()) },
				{ "_value", i._value ? value_to_json(*i._value) : json_value_t() },
				{ "_name", json_value_t(i._name) }
			};
			members.push_back(json_value_t(member));
		}

		return {
			std::map<string, json_value_t>{
				{ "_name", json_value_t(s._name.to_string()) },
				{ "_members", json_value_t(members) },
				{ "_struct_scope", scope_def_to_json(*s._struct_scope) }
			}
		};
	}

	json_value_t vector_def_to_json(const vector_def_t& s){
		return {
		};
	}

	json_value_t function_def_to_json(const function_def_t& s){
		std::vector<json_value_t> args;
		for(const auto i: s._args){
			const auto arg = std::map<string, json_value_t>{
				{ "_type", json_value_t(i._type.to_string()) },
				{ "_identifier", json_value_t(i._identifier) }
			};
			args.push_back(json_value_t(args));
		}

		return {
			std::map<string, json_value_t>{
				{ "_name", json_value_t(s._name.to_string()) },
				{ "_return_type", json_value_t(s._return_type.to_string()) },
				{ "_args", json_value_t(args) },
				{ "_function_scope", scope_def_to_json(*s._function_scope) }
			}
		};
	}

	json_value_t type_def_to_json(const type_def_t& type_def){
		return {
			std::map<string, json_value_t>{
				{ "_base_type", json_value_t(to_string(type_def._base_type)) },
				{ "_struct_def", type_def._struct_def ? struct_def_to_json(*type_def._struct_def) : json_value_t() },
				{ "_vector_def", type_def._vector_def ? vector_def_to_json(*type_def._vector_def) : json_value_t() },
				{ "_function_def", type_def._function_def ? function_def_to_json(*type_def._function_def) : json_value_t() }
			}
		};
	}

	json_value_t executable_to_json(const executable_t& e){
		std::vector<json_value_t> statements;
		for(const auto i: e._statements){
			statements.push_back(statement_to_json(*i));
		}

		return std::map<string, json_value_t> {
			{ "_host_function_param", e._host_function_param ? json_value_t("USED") : json_value_t() },
			{ "_statements", json_value_t(statements) },
		};
	}

	json_value_t scope_def_to_json(const scope_def_t& scope_def){
		const std::map<string, json_value_t> a {
			{ "_parent_scope", json_value_t(123.0f) },
			{ "_executable", executable_to_json(scope_def._executable) },
			{ "_types_collector", types_collector_to_json(scope_def._types_collector) }
		};
		return a;
	}


	////////////////////////			arg_t



	void trace(const arg_t& arg){
		QUARK_TRACE("<arg_t> data type: <" + arg._type.to_string() + "> indentifier: \"" + arg._identifier + "\"");
	}


	//////////////////////////////////////		function_def_t



	function_def_t::function_def_t(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<arg_t>& args,
		const scope_ref_t parent_scope,
		const executable_t& executable,
		const types_collector_t& types_collector
	)
	:
		_name(name),
		_return_type(return_type),
		_args(args),
		_function_scope(scope_def_t::make2(scope_def_t::k_function, name, {}, parent_scope, executable, types_collector))
	{
		QUARK_ASSERT(name.check_invariant());
		QUARK_ASSERT(name.to_string().size() > 0);
		QUARK_ASSERT(return_type.check_invariant());
		for(const auto a: args){
			QUARK_ASSERT(a.check_invariant());
		}
		QUARK_ASSERT(executable.check_invariant());
		QUARK_ASSERT(types_collector.check_invariant());

		QUARK_ASSERT(check_invariant());
	}

	bool function_def_t::check_invariant() const {
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_name.to_string().size() > 0);
		QUARK_ASSERT(_return_type.check_invariant());
		for(const auto a: _args){
			QUARK_ASSERT(a.check_invariant());
		}
		QUARK_ASSERT(_function_scope && _function_scope->check_invariant());
		return true;
	}

	bool function_def_t::operator==(const function_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_name != other._name){
			return false;
		}

		//	The _function_scope objects are EQUAL not same ptr.
		if(!(*_function_scope == *other._function_scope)){
			return false;
		}
		if(_return_type != other._return_type){
			return false;
		}
		if(_args != other._args){
			return false;
		}
		return true;
	}

	void trace(const function_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE(std::string("function_def_t: '") + e._name.to_string() + "'");

		{
			QUARK_SCOPED_TRACE("return");
			QUARK_TRACE(e._return_type.to_string());
		}
		{
			trace_vec("arguments", e._args);
		}

		trace(e._function_scope->_executable._statements);
	}

	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}

	TSHA1 calc_function_body_hash(const function_def_t& f){
		QUARK_ASSERT(f.check_invariant());

		static int s_counter = 1000;
		s_counter++;
		return CalcSHA1(std::to_string(s_counter));
	}


	function_def_t make_function_def(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<arg_t>& args,
		const scope_ref_t parent_scope,
		const executable_t& executable,
		const types_collector_t& types_collector
	)
	{
		return function_def_t(name, return_type, args, parent_scope, executable, types_collector);
	}


	QUARK_UNIT_TESTQ("function_def_t(function_def_t&)", ""){
		const auto scope_ref = scope_def_t::make_global_scope();
		const auto a = make_function_def(type_identifier_t::make("a"), type_identifier_t::make_int(), {}, scope_ref, executable_t({}), {});

		const auto b(a);
		QUARK_TEST_VERIFY(b.check_invariant());
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
			QUARK_ASSERT(*_type == _value->get_type());
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


	////////////////////////			struct_def_t



	struct_def_t struct_def_t::make2(
		const type_identifier_t& name,
		const std::vector<member_t>& members,
		const scope_ref_t parent_scope)
	{
		QUARK_ASSERT(name.check_invariant());
		for(const auto m: members){
			QUARK_ASSERT(m.check_invariant());
		}

		struct_def_t result;
		result._name = name;
		result._members = members;
		result._struct_scope = scope_def_t::make2(scope_def_t::k_struct, name, {}, parent_scope, executable_t({}), {});

		QUARK_ASSERT(result.check_invariant());
		return result;
	}

	bool struct_def_t::check_invariant() const{
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_name.to_string().size() > 0 );

		for(const auto m: _members){
			QUARK_ASSERT(m.check_invariant());
		}
		QUARK_ASSERT(_struct_scope && _struct_scope->check_invariant());
		return true;
	}

	bool struct_def_t::operator==(const struct_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_name != other._name){
			return false;
		}
		if(_members != other._members){
			return false;
		}
		if(!(*_struct_scope == *other._struct_scope)){
			return false;
		}
		return true;
	}

	void trace(const struct_def_t& e){
		QUARK_ASSERT(e.check_invariant());
		QUARK_SCOPED_TRACE("struct_def_t");
		trace_vec("members", e._members);
	}


	std::string to_signature(const struct_def_t& t){
		QUARK_ASSERT(t.check_invariant());

		const string label = "";
		string body;
		for(const auto& member : t._members) {
			const auto member_name = member._name;
			const type_identifier_t typedef_s = *member._type;
			const string member_type = "<" + typedef_s.to_string() + ">";

			//	"<string>first_name"
			const string member_result = member_type + member_name;

			body = body + member_result + ",";
		}
		body = remove_trailing_comma(body);

		return label + "<struct>" + "{" + body + "}";
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



	void trace(const ast_t& program){
		QUARK_SCOPED_TRACE("program");

		const auto a = scope_def_to_json(*program._global_scope);
		const auto s = json_to_compact_string(a);
		QUARK_TRACE(s);
	}




	////////////////////	Helpers for making tests.




	struct_def_t make_struct0(scope_ref_t scope_def){
		return struct_def_t::make2(type_identifier_t::make("struct0"), {}, scope_def);
	}

	struct_def_t make_struct1(scope_ref_t scope_def){
		return struct_def_t::make2(
			type_identifier_t::make("struct1"),
			{
				{ type_identifier_t::make_float(), "x" },
				{ type_identifier_t::make_float(), "y" },
				{ type_identifier_t::make_string(), "name" }
			},
			scope_def
		);
	}

	struct_def_t make_struct2(scope_ref_t scope_def){
		return make_struct0(scope_def);
	}

	struct_def_t make_struct3(scope_ref_t scope_def){
		return struct_def_t::make2(
			type_identifier_t::make("struct3"),
			{
				{ type_identifier_t::make_int(), "a" },
				{ type_identifier_t::make_string(), "b" }
			},
			scope_def
		);
	}

	struct_def_t make_struct4(scope_ref_t scope_def){
		return struct_def_t::make2(
			type_identifier_t::make("struct4"),
			{
				{ type_identifier_t::make_string(), "x" },
//				{ type_identifier_t::make("struct3"), "y" },
				{ type_identifier_t::make_string(), "z" }
			},
			scope_def
		);
	}

	//??? check for duplicate member names.
	struct_def_t make_struct5(scope_ref_t scope_def){
		return struct_def_t::make2(
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
			},
			scope_def
		);
	}

	struct_def_t make_struct6(scope_ref_t scope_def){
		return struct_def_t::make2(
			type_identifier_t::make("struct6"),
			{
				{ type_identifier_t::make_bool(), "_bool_true", value_t(true) },
				{ type_identifier_t::make_bool(), "_bool_false", value_t(false) },
				{ type_identifier_t::make_int(), "_int", value_t(111) },
//				{ type_identifier_t::make_float(), "_float" },
				{ type_identifier_t::make_string(), "_string", value_t("test 123") },
				{ type_identifier_t::make("pixel"), "_pixel" }
			},
			scope_def
		);
	}


} //	floyd_parser

