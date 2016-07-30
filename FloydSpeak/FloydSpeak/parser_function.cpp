//
//  parser_function.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_function.h"


#include "parser_expression.hpp"
#include "text_parser.h"
#include "parser_statement.hpp"

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
		const auto arg_name = read_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ make_type_identifier(arg_type.first), arg_name.first };
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
		{ make_type_identifier("int"), "x" },
		{ make_type_identifier("string"), "y" },
		{ make_type_identifier("float"), "z" }
	}
	));
}


/*
	s:
		Must start with "return".

		Examples:
			return 0;
			return x + y;

*/
pair<return_statement_t, string> parse_return_statement(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_return_statement()");
	QUARK_ASSERT(s.size() >= string("return").size());

	QUARK_ASSERT(peek_string(s, "return"));

	const auto token_pos = read_until(s, whitespace_chars);
	const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
	const auto expression1 = parse_expression(ast, expression_pos.first);
//			const auto expression2 = evaluate3(local_scope, expression1);
	const auto statement = return_statement_t{ make_shared<expression_t>(expression1) };

	//	Skip trailing ";".
	const auto pos = skip_whitespace(expression_pos.second.substr(1));
	return pair<return_statement_t, string>(statement, pos);
}


QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto t = make_shared<expression_t>(make_constant(value_t(0)));
	
	QUARK_TEST_VERIFY((
		parse_return_statement({}, "return 0;") == pair<return_statement_t, string>(return_statement_t{t}, "")
	));
}

QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto t = make_shared<expression_t>(make_constant(value_t(123)));
	
	QUARK_TEST_VERIFY((
		parse_return_statement({}, "return \t123\t;\t\nxyz}") == pair<return_statement_t, string>(return_statement_t{t}, "xyz}")
	));
}




	/*
		Never simplifes - the parser is non-lossy.

		Must not have whitespace before / after {}.
		{}

		{
			return 3;
		}

		{
			return 3 + 4;
		}
		{
			return f(3, 4) + 2;
		}


		//	Example: binding constants to constants, result of function calls and math operations.
		{
			int a = 10;
			int b = f(a);
			int c = a + b;
			return c;
		}

		//	Local scope.
		{
			{
				int a = 10;
			}
		}
		{
			struct point2d {
				int _x;
				int _y;
			}
		}

		{
			int my_func(string a, string b){
				int c = a + b;
				return c;
			}
		}

		FUTURE
		- Include comments
		- Split-out parse_statement().
		- Add struct {}
		- Add variables
		- Add local functions
	*/
std::vector<std::shared_ptr<statement_t>> parse_function_body(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	vector<shared_ptr<statement_t>> statements;

	ast_t local_scope = ast;

	string pos = body_str;
	while(!pos.empty()){

		//	Examine function body, one statement at a time. Only statements are allowed.
		const auto token_pos = read_until(pos, whitespace_chars);

		//	return statement?
		if(token_pos.first == "return"){
			const auto return_statement_pos = parse_return_statement(local_scope, pos);
			statements.push_back(make_shared<statement_t>(make__return_statement(return_statement_pos.first)));
			pos = return_statement_pos.second;
		}

		//	Define local variable?
		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/
		else if(ast.parser_i__is_known_type(token_pos.first)){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(local_scope, pos);
			const string& identifier = assignment_statement.first._bind_statement->_identifier;

			const auto it = local_scope._constant_values.find(identifier);
			if(it != local_scope._constant_values.end()){
				throw std::runtime_error("Variable name already in use!");
			}

			shared_ptr<const value_t> blank;
			local_scope._constant_values[identifier] = blank;

			statements.push_back(make_shared<statement_t>(assignment_statement.first));

			//	Skips trailing ";".
			pos = skip_whitespace(assignment_statement.second);
		}
		else{
			throw std::runtime_error("syntax error");
		}
	}
	trace(statements);
	return statements;
}


QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY((parse_function_body({}, "{}").empty()));
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{return 3;}").size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{\n\treturn 3;\n}").size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	const auto a = parse_function_body(make_test_ast(),
		"{	float test = log(10.11);\n"
		"	return 3;\n}"
	);
	QUARK_TEST_VERIFY(a.size() == 2);
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a[0]->_bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a[0]->_bind_statement->_expression->_call_function_expr->_inputs[0]->_constant == value_t(10.11f));

	QUARK_TEST_VERIFY(*a[1]->_return_statement->_expression->_constant == value_t(3));
}



















	//	Temporarily add the function's input argument to the identifers, so the body can access them.
	ast_t add_arg_identifiers(const ast_t& ast, const std::vector<arg_t> arg_types){
		QUARK_ASSERT(ast.check_invariant());
		for(const auto i: arg_types){ QUARK_ASSERT(i.check_invariant()); };

		auto local_scope = ast;
		for(const auto arg: arg_types){
			const auto& arg_name = arg._identifier;
			std::shared_ptr<value_t> blank_arg_value;
			local_scope._constant_values[arg_name] = blank_arg_value;
		}
		return local_scope;
	}




std::pair<std::pair<string, function_def_t>, string> parse_function_definition(const ast_t& ast, const string& pos){
	QUARK_ASSERT(ast.check_invariant());

	const auto type_pos = read_required_type_identifier(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(identifier_pos.second);

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

	auto local_scope = add_arg_identifiers(ast, args);

	const auto body = parse_function_body(local_scope, body_pos.first);
	const auto a = function_def_t{ type_pos.first, args, body };

	return { { identifier_pos.first, a }, body_pos.second };
}

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	try{
		const auto result = parse_function_definition({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	const auto result = parse_function_definition({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first.first == "f");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY(result.first.second._args.empty());
	QUARK_TEST_VERIFY(result.first.second._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TESTQ("parse_function_definition()", "Test many arguments of different types"){
	const auto result = parse_function_definition({}, "int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ make_type_identifier("string"), "a" },
		{ make_type_identifier("float"), "barry" },
		{ make_type_identifier("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "parse_function_definition()", "Test exteme whitespaces", ""){
	const auto result = parse_function_definition("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ make_type_identifier("string"), "a" },
		{ make_type_identifier("float"), "barry" },
		{ make_type_identifier("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/







function_def_t make_test_function1(){
	return make_function_def(
		make_type_identifier("int"),
		{},
		{
			makie_return_statement(make_constant(100))
		}
	);
}

function_def_t make_test_function2(){
	return make_function_def(
		make_type_identifier("string"),
		{
			{ make_type_identifier("int"), "a" },
			{ make_type_identifier("float"), "b" }
		},
		{
			makie_return_statement(make_constant(value_t("sdf")))
		}
	);
}

function_def_t make_log_function(){
	return make_function_def(
		make_type_identifier("float"),
		{ {make_type_identifier("float"), "value"} },
		{
			makie_return_statement(make_constant(123.f))
		}
	);
}

function_def_t make_log2_function(){
	return make_function_def(
		make_type_identifier("float"),
		{ { make_type_identifier("string"), "s" }, { make_type_identifier("float"), "v" } },
		{
			makie_return_statement(make_constant(456.7f))
		}
	);
}

function_def_t make_return5(){
	return make_function_def(
		make_type_identifier("int"),
		{ },
		{
			makie_return_statement(make_constant(value_t(5)))
		}
	);
}







}	//	floyd_parser


