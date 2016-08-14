//
//  parser_function.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_function.h"

#include "parser_expression.h"
#include "text_parser.h"
#include "parser_statement.h"
#include "floyd_parser.h"
#include "parser_primitives.h"

#include <cmath>

namespace floyd_parser {
	using std::pair;
	using std::string;
	using std::vector;
	using std::shared_ptr;
	using std::make_shared;

	/*
		()
		(int a)
		(int x, int y)
	*/
vector<arg_t> parse_functiondef_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<arg_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = read_type(str);
		const auto arg_name = read_required_single_symbol(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ type_identifier_t::make(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_functiondef_arguments("()") == vector<arg_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto r = parse_functiondef_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<arg_t>{
		{ type_identifier_t::make_int(), "x" },
		{ type_identifier_t::make_string(), "y" },
		{ type_identifier_t::make_float(), "z" }
	}
	));
}


std::vector<shared_ptr<statement_t>> parse_function_body(scope_ref_t function_scope, const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(function_scope->check_invariant());
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	std::vector<shared_ptr<statement_t>> statements;

	string pos = body_str;
	while(!pos.empty()){
		const auto statement_seq = read_statement(function_scope, pos);
		pos = statement_seq._rest;
		statements.push_back(make_shared<statement_t>(statement_seq._statement));
	}
//	trace(statements);
	return statements;
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	auto global = scope_def_t::make_global_scope();
	QUARK_TEST_VERIFY((parse_function_body(global, "{}").empty()));
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	auto global = scope_def_t::make_global_scope();
	QUARK_TEST_VERIFY(parse_function_body(global, "{return 3;}").size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	auto global = scope_def_t::make_global_scope();
	QUARK_TEST_VERIFY(parse_function_body(global, "{\n\treturn 3;\n}").size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto a = parse_function_body(
		global,
		"{	float test = log(10.11);\n"
		"	return 3;\n}"
	);
	QUARK_TEST_VERIFY(a.size() == 2);
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_expression->_call->_function_name == "log");
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_expression->_call->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a[0]->_bind_statement->_expression->_call->_inputs[0]->_constant == value_t(10.11f));

	QUARK_TEST_VERIFY(*a[1]->_return_statement->_expression->_constant == value_t(3));
}


std::pair<function_def_t, std::string> parse_function_definition(scope_ref_t scope_def, const string& pos){
	QUARK_ASSERT(scope_def->check_invariant());

	const auto return_type_pos = read_required_type_identifier(pos);
	const auto function_name_pos = read_required_single_symbol(return_type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(function_name_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);


	const auto function_name = type_identifier_t::make(function_name_pos.first);
	const auto function_def = make_function_def(
		function_name,
		return_type_pos.first,
		args,
		scope_def,
		executable_t({}),
		{}
	);
	const auto statements = parse_function_body(function_def._function_scope, body_pos.first);
	function_def._function_scope->_executable = executable_t(statements);

	return { function_def, body_pos.second };
}

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	try{
		const auto global = scope_def_t::make_global_scope();
		const auto result = parse_function_definition(global, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	const auto global = scope_def_t::make_global_scope();
	const auto result = parse_function_definition(global, "int f(){}");
	QUARK_TEST_VERIFY(result.first._name == type_identifier_t::make("f"));
	QUARK_TEST_VERIFY(result.first._return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY(result.first._args.empty());
	QUARK_TEST_VERIFY(result.first._function_scope->_executable._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TESTQ("parse_function_definition()", "Test many arguments of different types"){
	const auto global = scope_def_t::make_global_scope();
	const auto result = parse_function_definition(global, "int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first._name == type_identifier_t::make("printf"));
	QUARK_TEST_VERIFY(result.first._return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY((result.first._args == vector<arg_t>{
		{ type_identifier_t::make_string(), "a" },
		{ type_identifier_t::make_float(), "barry" },
		{ type_identifier_t::make_int(), "c" },
	}));
	QUARK_TEST_VERIFY(result.first._function_scope->_executable._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "parse_function_definition()", "Test exteme whitespaces", ""){
	const auto result = parse_function_definition("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ type_identifier_t::make_string(), "a" },
		{ type_identifier_t::make_float(), "barry" },
		{ type_identifier_t::make_int(), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/

function_def_t make_test_function1(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_function1"),
		type_identifier_t::make_int(),
		{},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant(100)))
		}),
		{}
	);
}

function_def_t make_test_function2(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_function2"),
		type_identifier_t::make_string(),
		{
			{ type_identifier_t::make_int(), "a" },
			{ type_identifier_t::make_float(), "b" }
		},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant("sdf")))
		}),
		{}
	);
}

function_def_t make_log_function(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_log_function"),
		type_identifier_t::make_float(),
		{
			{type_identifier_t::make_float(), "value"}
		},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant(123.f)))
		}),
		{}
	);
}

function_def_t make_log2_function(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_log2_function"),
		type_identifier_t::make_float(),
		{
			{ type_identifier_t::make_string(), "s" },
			{ type_identifier_t::make_float(), "v" }
		},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant(456.7f)))
		}),
		{}
	);
}

function_def_t make_return5(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("return5"),
		type_identifier_t::make_float(),
		{
		},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant(5)))
		}),
		{}
	);
}

function_def_t make_return_hello(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("hello"),
		type_identifier_t::make_int(),
		{
		},
		scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(make_constant("hello")))
		}),
		{}
	);
}


}	//	floyd_parser

