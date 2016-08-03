//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

/*
#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true
*/

#include "text_parser.h"
#include "steady_vector.h"
#include "parser_expression.h"
#include "parser_statement.h"
#include "parser_function.h"
#include "parser_struct.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd_parser {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/



struct test_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_value_class_a", "what is needed for basic operations"){
	test_value_class_a a;
	test_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}





/*
//////////////////////////////////////////////////		VISITOR


struct visitor_i {
	virtual ~visitor_i(){};

	virtual void visitor_interface__on_math_operation2(const math_operation2_expr_t& e) = 0;
	virtual void visitor_interface__on_function_def_expr(const function_def_expr_t& e) = 0;

	virtual void visitor_interface__on_bind_statement_statement(const bind_statement_t& s) = 0;
	virtual void visitor_interface__on_return_statement(const return_statement_t& s) = 0;
};

void visit_statement(const statement_t& s, visitor_i& visitor){
	if(s._bind_statement){
		visitor.visitor_interface__on_bind_statement_statement(*s._bind_statement);
	}
	else if(s._return_statement){
		visitor.visitor_interface__on_return_statement(*s._return_statement);
	}
	else{
		QUARK_ASSERT(false);
	}
}


void visit_program(const ast_t& program, visitor_i& visitor){
	for(const auto i: program._top_level_statements){
		visit_statement(*i, visitor);
	}
}
*/



//////////////////////////////////////////////////		TRACE



void trace(const type_identifier_t& v){
	QUARK_TRACE("type_identifier_t <" + v.to_string() + ">");
}



void trace(const ast_t& program){
	QUARK_SCOPED_TRACE("program");

	for(const auto i: program._top_level_statements){
		trace(*i);
	}
}







//////////////////////////////////////////////////		read_statement()


	/*
		Read one statement, including any expressions it uses.
		Supports all statments:
			- return statement
			- struct-definition
			- function-definition
			- define constant, with initializating.

		Never simplifes expressions- the parser is non-lossy.

		Returns:
			_statement
				[return_statement]
					[expression]
				...
			_ast
				May have been updated to hold new function-definition or struct-definition.
			_rest
				will never start with whitespace.
				trailing ";" will be consumed.



		Example return statement:
			#1	"return 3;..."
			#2	"return f(3, 4) + 2;..."

		Example function definitions:
			#1	"int test_func1(){ return 100; };..."
			#2	"string test_func2(int a, float b){ return "sdf" };..."

		Example struct definitions:
			"struct my_image { int width; int height; };"
			"struct my_sprite { string name; my_image image; };..."

		Example variable definitions
			"int a = 10;..."
			"int b = f(a);..."


		FUTURE
		- Add local scopes / blocks.
		- Include comments
		- Add mutable variables
	*/

statement_result_t read_statement(const ast_t& ast1, const string& pos){
	QUARK_ASSERT(ast1.check_invariant());

	ast_t ast2 = ast1;
	const auto token_pos = read_until(pos, whitespace_chars);

	//	return statement?
	if(token_pos.first == "return"){
		const auto return_statement_pos = parse_return_statement(ast1, pos);
		return { make__return_statement(return_statement_pos.first), ast2, skip_whitespace(return_statement_pos.second) };
	}

	//	struct definition?
	else if(token_pos.first == "struct"){
		const auto struct_name = read_required_single_symbol(token_pos.second);
		pair<struct_def_t, string> body_pos = parse_struct_body(skip_whitespace(struct_name.second));


		auto pos2 = skip_whitespace(body_pos.second);
		pos2 = read_required_char(pos2, ';');

        return { define_struct_statement_t{ struct_name.first, body_pos.first }, ast2, skip_whitespace(pos2) };
	}

	else {
		const auto type_pos = read_required_type_identifier(pos);
		const auto identifier_pos = read_required_single_symbol(type_pos.second);

		//	??? check for "a = 3"-type assignment, with inferred type.

		/*
			Function definition?
			"int xyz(string a, string b){ ... }
		*/
		if(peek_string(skip_whitespace(identifier_pos.second), "(")){
			const pair<pair<string, function_def_t>, string> function = parse_function_definition(ast1, pos);
            return { define_function_statement_t{ function.first.first, function.first.second }, ast2, skip_whitespace(function.second) };
		}

		//	Define variable?
		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/

		else if(peek_string(skip_whitespace(identifier_pos.second), "=")){
//		else if(ast.parser_i__is_known_type(token_pos.first))
			pair<statement_t, string> assignment_statement = parse_assignment_statement(ast1, pos);
			const string& identifier = assignment_statement.first._bind_statement->_identifier;

			const auto it = ast2._constant_values.find(identifier);
			if(it != ast2._constant_values.end()){
				throw std::runtime_error("Variable name already in use!");
			}

			shared_ptr<const value_t> blank;
			ast2._constant_values[identifier] = blank;

			return { assignment_statement.first, ast2, skip_whitespace(assignment_statement.second) };
		}

		else{
			throw std::runtime_error("syntax error");
		}
	}
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
	const auto result = read_statement({}, "float test = testx(1234);\n\treturn 3;\n");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, test_function1);
	QUARK_TEST_VERIFY(result._statement._define_function);
	QUARK_TEST_VERIFY(result._statement._define_function->_type_identifier == "test_function1");
	QUARK_TEST_VERIFY(result._statement._define_function->_function_def == make_test_function1());
	QUARK_TEST_VERIFY(result._rest == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, "struct test_struct0 " + k_test_struct0_body + ";");
	QUARK_TEST_VERIFY(result._statement._define_struct);
	QUARK_TEST_VERIFY(result._statement._define_struct->_type_identifier == "test_struct0");
	QUARK_TEST_VERIFY(result._statement._define_struct->_struct_def == make_test_struct0());
}







//////////////////////////////////////////////////		program_to_ast()




ast_t program_to_ast(const ast_t& init, const string& program){
	ast_t ast2 = init;

	auto pos = program;
	pos = skip_whitespace(pos);

	std::vector<std::shared_ptr<statement_t> > statements;
	while(!pos.empty()){
		const auto statement_pos = read_statement(ast2, pos);
		ast2 = statement_pos._ast;

		if(statement_pos._statement._define_struct){
			ast2._types_collector = ast2._types_collector.define_struct_type(statement_pos._statement._define_struct->_type_identifier, statement_pos._statement._define_struct->_struct_def);
		}
		else if(statement_pos._statement._define_function){
			ast2._types_collector = ast2._types_collector.define_function_type(statement_pos._statement._define_function->_type_identifier, statement_pos._statement._define_function->_function_def);
		}
		else{
			statements.push_back(make_shared<statement_t>(statement_pos._statement));
//			throw std::runtime_error("Unexpected statement.");
		}
		pos = skip_whitespace(statement_pos._rest);
	}

	ast2._top_level_statements = statements;

	QUARK_ASSERT(ast2.check_invariant());
	trace(ast2);

	return ast2;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

	const auto result = program_to_ast({}, kProgram1);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("int"),
			vector<arg_t>{ arg_t{ type_identifier_t::make_type("string"), "args" }},
			{
				makie_return_statement(make_constant(3))
			}
		)
	));
}


QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	const auto result = program_to_ast({}, kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("f") ==
		make_function_def(
			type_identifier_t::make_type("int"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("int"), "x" },
				arg_t{ type_identifier_t::make_type("int"), "y" },
				arg_t{ type_identifier_t::make_type("string"), "z" }
			},
			{
				makie_return_statement(make_constant(3))
			}
		)
	));
}


QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"
	;
	QUARK_TRACE(kProgram);

	const auto result = program_to_ast({}, kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("hello") ==
		make_function_def(
			type_identifier_t::make_type("string"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("int"), "x" },
				arg_t{ type_identifier_t::make_type("int"), "y" },
				arg_t{ type_identifier_t::make_type("string"), "z" }
			},
			{
				makie_return_statement(make_constant("test abc"))
			}
		)
	));

	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("int"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("string"), "args" }
			},
			{
				makie_return_statement(make_constant(3))
			}
		)
	));
}


QUARK_UNIT_TESTQ("program_to_ast()", ""){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto result = program_to_ast({}, kProgram2);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("testx") ==
		make_function_def(
			type_identifier_t::make_type("float"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("float"), "v" }
			},
			{
				makie_return_statement(make_constant(13.4f))
			}
		)
	));

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("int"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("string"), "arg" }
			},
			{
				makie_return_statement(bind_statement_t{"test", function_call_expr_t{"testx", }})
				makie_return_statement(make_constant(value_t(13.4f)))
			}
		)
	));
*/
}

////////////////////////////		STRUCT SUPPORT



QUARK_UNIT_TEST("", "program_to_ast()", "k_test_program_100", ""){
	const auto result = program_to_ast({}, k_test_program_100);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("string"),
			vector<arg_t>{},
			{
				makie_return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}

QUARK_UNIT_TEST("", "program_to_ast()", "k_test_program_101", ""){
	const auto result = program_to_ast({}, k_test_program_101);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("string"),
			vector<arg_t>{},
			{
				makie_return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}






}	//	floyd_parser



