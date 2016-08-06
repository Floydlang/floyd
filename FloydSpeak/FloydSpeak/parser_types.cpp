
//
//  parser_types.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_types.h"

#include "quark.h"

#include "parser_primitives.h"
#include "parser_statement.h"
#include "parser_function.h"
#include "parts/sha1_class.h"


using std::make_shared;
using std::string;
using std::shared_ptr;
using std::vector;


namespace floyd_parser {

	//////////////////////////////////////////////////		type_identifier_t


	type_identifier_t type_identifier_t::make_type(std::string s){
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



	string to_string(const frontend_base_type t){
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


	bool is_valid_type_identifier(const std::string& s){
		return true;
	}








	////////////////////////			arg_t



	void trace(const arg_t& arg){
		QUARK_TRACE("<arg_t> data type: <" + arg._type.to_string() + "> indentifier: \"" + arg._identifier + "\"");
	}


	//////////////////////////////////////		function_def_t



	function_def_t::function_def_t(type_identifier_t return_type, const std::vector<arg_t>& args, const std::vector<std::shared_ptr<statement_t> > statements) :
		_return_type(return_type),
		_args(args)
	{
		_scope_def = std::make_shared<scope_def_t>();
		_scope_def->_statements = statements;

		QUARK_ASSERT(check_invariant());
	}

	function_def_t::function_def_t(type_identifier_t return_type, const std::vector<arg_t>& args, hosts_function_t f, std::shared_ptr<host_data_i> param) :
		_return_type(return_type),
		_args(args)
	{
		_scope_def = std::make_shared<scope_def_t>();
		_scope_def->_host_function = f;
		_scope_def->_host_function_param = param;

		QUARK_ASSERT(check_invariant());
	}

	bool function_def_t::check_invariant() const {
		QUARK_ASSERT(_return_type.check_invariant());
		QUARK_ASSERT(_scope_def && _scope_def->check_invariant());
		return true;
	}

	bool function_def_t::operator==(const function_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return *_scope_def == *other._scope_def && _return_type == other._return_type && _args == other._args;
	}

	void trace(const function_def_t& e){
		QUARK_SCOPED_TRACE("function_def_t");

		{
			QUARK_SCOPED_TRACE("return");
			QUARK_TRACE(e._return_type.to_string());
		}
		{
			trace_vec("arguments", e._args);
		}

		trace(e._scope_def->_statements);
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


	function_def_t make_function_def(type_identifier_t return_type, const vector<arg_t>& args, const vector<statement_t>& statements){
		vector<shared_ptr<statement_t>> statements2;
		for(const auto i: statements){
			statements2.push_back(make_shared<statement_t>(i));
		}
		return function_def_t(return_type, args,statements2);
	}


	struct_def_t make_struct_def(const vector<member_t>& members){
		return struct_def_t{ members };
	}




	////////////////////////			member_t


	member_t::member_t(const value_t& type_and_default_value, const std::string& name) :
		_type_and_default_value(make_shared<value_t>(type_and_default_value)),
		_name(name)
	{
		QUARK_ASSERT(!type_and_default_value.is_null());

		QUARK_ASSERT(check_invariant());
	}

	member_t::member_t(const type_identifier_t& type, const std::string& name) :
		_type_and_default_value(make_shared<value_t>(value_t::make_default_value(type))),
		_name(name)
	{
		QUARK_ASSERT(name.size() > 0);
		QUARK_ASSERT(type.check_invariant());

		QUARK_ASSERT(check_invariant());
	}

	bool member_t::check_invariant() const{
		QUARK_ASSERT(_name.size() > 0);
		QUARK_ASSERT(_type_and_default_value);
		QUARK_ASSERT(_type_and_default_value->check_invariant());
		return true;
	}

	bool member_t::operator==(const member_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return *_type_and_default_value == *other._type_and_default_value && _name == other._name;
	}


	void trace(const member_t& member){
		QUARK_TRACE("<member> type: <" + member._type_and_default_value->get_type().to_string() + "> name: \"" + member._name + "\"");
	}




	////////////////////////			struct_def_t


	bool struct_def_t::check_invariant() const{
		return true;
	}
	bool struct_def_t::operator==(const struct_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return other._members == _members;
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
			const type_identifier_t typedef_s = member._type_and_default_value->get_type();
			const string member_type = "<" + typedef_s.to_string() + ">";

			//	"<string>first_name"
			const string member_result = member_type + member_name;

			body = body + member_result + ",";
		}
		body = remove_trailing_comma(body);

		return label + "<struct>" + "{" + body + "}";
	}




	////////////////////	Helpers for making tests.




	struct_def_t make_struct0(){
		return make_struct_def({});
	}

	struct_def_t make_struct1(){
		return make_struct_def(
			{
				{ make_type_identifier("float"), "x" },
				{ make_type_identifier("float"), "y" },
				{ make_type_identifier("string"), "name" }
			}
		);
	}


	struct_def_t make_struct2(){
		return make_struct0();
	}

	struct_def_t make_struct3(){
		return make_struct_def(
			{
				{ make_type_identifier("int"), "a" },
				{ make_type_identifier("string"), "b" }
			}
		);
	}

	struct_def_t make_struct4(){
		return make_struct_def(
			{
				{ make_type_identifier("string"), "x" },
//				{ make_type_identifier("struct3"), "y" },
				{ make_type_identifier("string"), "z" }
			}
		);
	}


//??? check for duplicate member names.
struct_def_t make_struct5(){
	return make_struct_def(
		{
			{ make_type_identifier("bool"), "a" },
			// pad
			// pad
			// pad
			{ make_type_identifier("int"), "b" },
			{ make_type_identifier("bool"), "c" },
			{ make_type_identifier("bool"), "d" },
			{ make_type_identifier("bool"), "e" },
			{ make_type_identifier("bool"), "f" },
			{ make_type_identifier("string"), "g" },
			{ make_type_identifier("bool"), "h" }
		}
	);
}


} //	floyd_parser;




using namespace floyd_parser;



//////////////////////////////////////		to_string(frontend_base_type)


QUARK_UNIT_TESTQ("to_string(frontend_base_type)", ""){
	QUARK_TEST_VERIFY(to_string(k_int) == "int");
	QUARK_TEST_VERIFY(to_string(k_bool) == "bool");
	QUARK_TEST_VERIFY(to_string(k_string) == "string");
	QUARK_TEST_VERIFY(to_string(k_struct) == "struct");
	QUARK_TEST_VERIFY(to_string(k_vector) == "vector");
	QUARK_TEST_VERIFY(to_string(k_function) == "function");
}


//??? more

