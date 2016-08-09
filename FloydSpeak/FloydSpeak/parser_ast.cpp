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

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>
#include "parts/sha1_class.h"


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


	void trace_frontend_type(const type_def_t& t, const std::string& label){
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
//			trace_frontend_type(*t._vector_def->_value_type, "");
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
		}
		else{
			QUARK_ASSERT(false);
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


	/*
		A string naming a type. "int", "string", "my_struct" etc.
		It is guaranteed to contain correct characters.
		It is NOT guaranteed to map to an actual type in the language or program.
	*/

	type_identifier_t type_identifier_t::make(std::string s){
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

		QUARK_ASSERT(check_invariant());
	}

	type_identifier_t::type_identifier_t(const std::string& s) :
		_type_magic(s)
	{
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
//		QUARK_ASSERT(_type_magic == "" || _type_magic == "string" || _type_magic == "int" || _type_magic == "float" || _type_magic == "value_type");
		return true;
	}



	bool is_valid_type_identifier(const std::string& s){
		return true;
	}


	void trace(const type_identifier_t& v){
		QUARK_TRACE("type_identifier_t <" + v.to_string() + ">");
	}




	//////////////////////////////////////////////////		scope_def_t



	bool scope_def_t::check_invariant() const {
		for(const auto s: _statements){
			QUARK_ASSERT(s);
			QUARK_ASSERT(s->check_invariant());
		}
		return true;
	}

	bool scope_def_t::operator==(const scope_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_statements.size() != other._statements.size()){
			return false;
		}
		for(int i = 0 ; i < _statements.size() ; i++){
			if(!(*_statements[i] == *other._statements[i])){
				return false;
			}
		}
		return true;
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
		const std::shared_ptr<const scope_def_t>& function_scope
	)
	:
		_name(name),
		_return_type(return_type),
		_args(args),
		_function_scope(function_scope)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool function_def_t::check_invariant() const {
		QUARK_ASSERT(_name.check_invariant());
		QUARK_ASSERT(_return_type.check_invariant());
		QUARK_ASSERT(_function_scope && _function_scope->check_invariant());
		return true;
	}

	bool function_def_t::operator==(const function_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _name == other._name && *_function_scope == *other._function_scope && _return_type == other._return_type && _args == other._args;
	}

	void trace(const function_def_t& e){
		QUARK_SCOPED_TRACE(std::string("function_def_t: '") + e._name.to_string() + "'");

		{
			QUARK_SCOPED_TRACE("return");
			QUARK_TRACE(e._return_type.to_string());
		}
		{
			trace_vec("arguments", e._args);
		}

		trace(e._function_scope->_statements);
	}

	void trace(const std::vector<std::shared_ptr<statement_t>>& e){
		QUARK_SCOPED_TRACE("statements");
		for(const auto s: e){
			trace(*s);
		}
	}

	TSHA1 calc_function_body_hash(const function_def_t& f){
		static int s_counter = 1000;
		s_counter++;
		return CalcSHA1(std::to_string(s_counter));
	}


	function_def_t make_function_def(
		const type_identifier_t& name,
		const type_identifier_t& return_type,
		const std::vector<arg_t>& args,
		const std::shared_ptr<const scope_def_t>& function_scope
	)
	{
		return function_def_t(name, return_type, args, function_scope);
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
			&& (
				(!_value && !other._value)
				||
				(_value && other._value && *_value == *other._value)
			);
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type->to_string() + "> name: \"" + member._name + "\"");
	}




	////////////////////////			struct_def_t


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

		return _name == other._name && other._members == _members && *_struct_scope == *other._struct_scope;
	}

	struct_def_t struct_def_t::make(
		const type_identifier_t& name,
		const std::vector<member_t>& members,
		const std::shared_ptr<const scope_def_t>& struct_scope)
	{
		return struct_def_t{ name, members, struct_scope };
	}


	void trace(const struct_def_t& e){
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



	////////////////////////			ast_t



	void trace(const ast_t& program){
		QUARK_SCOPED_TRACE("program");

		for(const auto i: program._global_scope->_statements){
			trace(*i);
		}
	}




	////////////////////	Helpers for making tests.




	struct_def_t make_struct0(const scope_def_t& scope_def){
		auto struct_scope = scope_def_t::make_subscope(scope_def);
		return struct_def_t::make(type_identifier_t::make("struct0"), {}, struct_scope);
	}

	struct_def_t make_struct1(const scope_def_t& scope_def){
		auto struct_scope = scope_def_t::make_subscope(scope_def);
		return struct_def_t::make(
			type_identifier_t::make("struct1"),
			{
				{ type_identifier_t::make_float(), "x" },
				{ type_identifier_t::make_float(), "y" },
				{ type_identifier_t::make_string(), "name" }
			},
			struct_scope
		);
	}


	struct_def_t make_struct2(const scope_def_t& scope_def){
		return make_struct0(scope_def);
	}

	struct_def_t make_struct3(const scope_def_t& scope_def){
		auto struct_scope = scope_def_t::make_subscope(scope_def);
		return struct_def_t::make(
			type_identifier_t::make("struct3"),
			{
				{ type_identifier_t::make_int(), "a" },
				{ type_identifier_t::make_string(), "b" }
			},
			struct_scope
		);
	}

	struct_def_t make_struct4(const scope_def_t& scope_def){
		auto struct_scope = scope_def_t::make_subscope(scope_def);
		return struct_def_t::make(
			type_identifier_t::make("struct4"),
			{
				{ type_identifier_t::make_string(), "x" },
//				{ type_identifier_t::make("struct3"), "y" },
				{ type_identifier_t::make_string(), "z" }
			},
			struct_scope
		);
	}


	//??? check for duplicate member names.
	struct_def_t make_struct5(const scope_def_t& scope_def){
		auto struct_scope = scope_def_t::make_subscope(scope_def);
		return struct_def_t::make(
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
			struct_scope
		);
	}

} //	floyd_parser

