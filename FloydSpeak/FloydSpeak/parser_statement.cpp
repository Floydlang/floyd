//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_statement.hpp"



#include "parser_types.h"
#include "parser_expression.hpp"


namespace floyd_parser {

	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;


	statement_t make__bind_statement(const bind_statement_t& value){
		return statement_t(value);
	}

	statement_t make__bind_statement(const std::string& identifier, const expression_t& e){
		return statement_t(bind_statement_t{identifier, std::make_shared<expression_t>(e)});
	}

	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}




void trace(const statement_t& s){
	if(s._bind_statement){
		const auto s2 = s._bind_statement;
		std::string t = "bind_statement_t: \"" + s2->_identifier + "\"";
		QUARK_SCOPED_TRACE(t);
		trace(*s2->_expression);
	}
	else if(s._return_statement){
		const auto s2 = s._return_statement;
		QUARK_SCOPED_TRACE("return_statement_t");
		trace(*s2->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

	//////////////////////////////////////		statement_t



	bool statement_t::check_invariant() const {
		if(_bind_statement){
			QUARK_ASSERT(_bind_statement);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_return_statement){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(_return_statement);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}






//////////////////////////////////////////////////		PARSE STATEMENTS




namespace {

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

}


	/*
		Named function:

		int myfunc(string a, int b){
			...
			return b + 1;
		}


		LATER:
		Lambda:

		int myfunc(string a){
			() => {
			}
		}
	*/
std::pair<std::pair<string, function_def_t>, string> parse_function_definition_statement(const ast_t& ast, const string& pos){
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

QUARK_UNIT_TESTQ("parse_function_definition_statement()", ""){
	try{
		const auto result = parse_function_definition_statement({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("parse_function_definition_statement()", ""){
	const auto result = parse_function_definition_statement({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first.first == "f");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY(result.first.second._args.empty());
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TESTQ("parse_function_definition_statement()", "Test many arguments of different types"){
	const auto result = parse_function_definition_statement({}, "int printf(string a, float barry, int c){}");
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

/*
QUARK_UNIT_TEST("", "parse_function_definition_statement()", "Test exteme whitespaces", ""){
	const auto result = parse_function_definition_statement("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
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







pair<statement_t, string> parse_assignment_statement(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_assignment_statement()");
	QUARK_ASSERT(ast.check_invariant());

	const auto token_pos = read_until(s, whitespace_chars);
	const auto type = make_type_identifier(token_pos.first);
	QUARK_ASSERT(ast.parser_i__is_known_type(read_until(s, whitespace_chars).first));

	const auto variable_pos = read_until(skip_whitespace(token_pos.second), whitespace_chars + "=");
	const auto equal_rest = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until(skip_whitespace(equal_rest), ";");

	const auto expression = parse_expression(ast, expression_pos.first);

	const auto statement = make__bind_statement(variable_pos.first, expression);
	trace(statement);

	//	Skip trailing ";".
	return { statement, expression_pos.second.substr(1) };
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "int"){
	const auto a = parse_assignment_statement(ast_t(), "int a = 10; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "a");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(10));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "float"){
	const auto a = parse_assignment_statement(make_test_ast(), "float b = 0.3; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "b");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(0.3f));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "function call"){
	const auto ast = make_test_ast();
	const auto a = parse_assignment_statement(ast, "float test = log(\"hello\");\n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_call_function_expr->_inputs[0]->_constant ==value_t("hello"));
	QUARK_TEST_VERIFY(a.second == "\n");
}









std::pair<struct_def_t, std::string> parse_struct_body(const string& s){
	const auto s2 = skip_whitespace(s);
	read_required_char(s2, '{');
	const auto body_pos = get_balanced(s2);

	vector<arg_t> members;
	auto pos = trim_ends(body_pos.first);
	while(!pos.empty()){
		const auto arg_type = read_type(pos);
		const auto arg_name = read_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ';');

		const auto a = arg_t{ make_type_identifier(arg_type.first), arg_name.first };
		members.push_back(a);
		pos = optional_comma.second;
	}

	return { struct_def_t{members}, body_pos.second };
}

QUARK_UNIT_TESTQ("parse_struct_body", ""){
	QUARK_TEST_VERIFY((parse_struct_body("{}") == pair<struct_def_t, string>({}, "")));
}

QUARK_UNIT_TESTQ("parse_struct_body", ""){
	QUARK_TEST_VERIFY((parse_struct_body(" {} x") == pair<struct_def_t, string>({}, " x")));
}


struct_def_t make_test_struct0(){
	return {
		vector<arg_t>
		{
			{ make_type_identifier("int"), "x" },
			{ make_type_identifier("string"), "y" },
			{ make_type_identifier("float"), "z" }
		}
	};
}

QUARK_UNIT_TESTQ("parse_struct_body", ""){
	const auto r = parse_struct_body("{int x; string y; float z;}");
	QUARK_TEST_VERIFY((
		r == pair<struct_def_t, string>(make_test_struct0(), "" )
	));
}




//////////////////////////////////////////////////		read_statement()


pair<statement_t, string> read_statement(const ast_t& ast, const string& pos){
	QUARK_ASSERT(ast.check_invariant());

	const auto word1 = read_until(pos, " ");

	//	struct?
	if(word1.first == "struct"){
		const auto struct_name = read_required_identifier(word1.second);

		pair<struct_def_t, string> body_pos = parse_struct_body(struct_name.second);
		struct_def_expr_t struct_def_expr{ make_shared<struct_def_t>(body_pos.first) };
		const auto bind = bind_statement_t{ struct_name.first, make_shared<expression_t>(struct_def_expr) };

/*
		//	Add struct
		const auto foundIt = result._structs.find(identifier);
		if(foundIt != result._structs.end()){
			throw std::runtime_error("Struct \"" + identifier + "\" already defined.");
		}

		result._structs[identifier] = e->_struct_def_expr;
*/
		return { bind, body_pos.second };
	}

	//	Function?
	else {
		const auto type_pos = read_required_type_identifier(pos);
		const auto identifier_pos = read_required_identifier(type_pos.second);

		const pair<pair<string, function_def_t>, string> function = parse_function_definition_statement(ast, pos);
		const auto function_def_expr = function_def_expr_t{make_shared<function_def_t>(function.first.second)};

		const auto bind = bind_statement_t{ function.first.first, make_shared<expression_t>(expression_t{function_def_expr}) };
		return { bind, function.second };
	}
}


const string test_function1 = "int test_function1(){ return 100; }";

function_def_t make_test_function1(){
	return {
		type_identifier_t::make_type("int"),
		vector<arg_t>{
		},
		function_body_t{
			{ make_shared<statement_t>(return_statement_t{ std::make_shared<expression_t>(make_constant(value_t(100))) }) }
		}
	};
}

const string test_function2 = "string test_function2(int a, float b){ return \"sdf\"; }";

function_def_t make_test_function2(){
	return {
		type_identifier_t::make_type("string"),
		vector<arg_t>{
			{ make_type_identifier("int"), "a" },
			{ make_type_identifier("float"), "b" }
		},
		function_body_t{
			{ make_shared<statement_t>(return_statement_t{ std::make_shared<expression_t>(make_constant(value_t("sdf"))) }) }
		}
	};
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	try{
		const auto result = read_statement({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, test_function1);
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "test_function1");
	const auto expr = result.first._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(expr);

/*
	const auto test = make_test_function1();
	QUARK_TEST_VERIFY(expr->_args == test._args);
	QUARK_TEST_VERIFY(expr->_return_type == test._return_type);
	QUARK_TEST_VERIFY(expr->_body == test._body);
*/
	QUARK_TEST_VERIFY(*expr->_def == make_test_function1());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, test_function2);
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "test_function2");
	const auto expr = result.first._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(expr);

	QUARK_TEST_VERIFY(*expr->_def == make_test_function2());
	QUARK_TEST_VERIFY(result.second == "");
}


QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, "struct test_struct0 {int x; string y; float z;}");
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "test_struct0");
	const auto expr = result.first._bind_statement->_expression->_struct_def_expr;

	QUARK_TEST_VERIFY(*expr->_def == make_test_struct0());
}



}	//	floyd_parser
