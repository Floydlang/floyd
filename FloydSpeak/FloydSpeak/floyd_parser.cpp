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
#include "parser_expression.hpp"
#include "parser_statement.hpp"
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

		const pair<pair<string, function_def_t>, string> function = parse_function_definition(ast, pos);
		const auto function_def_expr = function_def_expr_t{make_shared<function_def_t>(function.first.second)};

		const auto bind = bind_statement_t{ function.first.first, make_shared<expression_t>(expression_t{function_def_expr}) };
		return { bind, function.second };
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
	const auto result = read_statement({}, test_function1);
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "test_function1");
	const auto expr = result.first._bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(expr);

	QUARK_TEST_VERIFY(*expr->_def == make_test_function1());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	const auto result = read_statement({}, "struct test_struct0 " + k_test_struct0);
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "test_struct0");
	const auto expr = result.first._bind_statement->_expression->_struct_def_expr;

	QUARK_TEST_VERIFY(*expr->_def == make_test_struct0());
}







//////////////////////////////////////////////////		program_to_ast()




ast_t program_to_ast(const ast_t& init, const string& program){
	ast_t result = init;

	auto pos = program;
	pos = skip_whitespace(pos);

	std::vector<std::shared_ptr<statement_t> > statements;
	while(!pos.empty()){
		const auto statement_pos = read_statement(result, pos);
		statements.push_back(make_shared<statement_t>(statement_pos.first));

		if(statement_pos.first._bind_statement){
			string identifier = statement_pos.first._bind_statement->_identifier;
			const auto e = statement_pos.first._bind_statement->_expression;

			if(e->_function_def_expr){
				const auto foundIt = result._functions.find(identifier);
				if(foundIt != result._functions.end()){
					throw std::runtime_error("Function \"" + identifier + "\" already defined.");
				}

				result._functions[identifier] = e->_function_def_expr;
			}
			else if(e->_struct_def_expr){
				const auto foundIt = result._structs.find(identifier);
				if(foundIt != result._structs.end()){
					throw std::runtime_error("Struct \"" + identifier + "\" already defined.");
				}

				result._structs[identifier] = e->_struct_def_expr;
			}
			else{
				throw std::runtime_error("Unexpected expression.");
			}
		}
		else{
			throw std::runtime_error("Unexpected statement.");
		}
		pos = skip_whitespace(statement_pos.second);
	}

	result._top_level_statements = statements;

	QUARK_ASSERT(result.check_invariant());
	trace(result);

	return result;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

	const auto result = program_to_ast({}, kProgram1);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr);

	const auto expr = result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(expr->_def->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((expr->_def->_args == vector<arg_t>{ arg_t{ type_identifier_t::make_type("string"), "args" }}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	const auto result = program_to_ast({}, kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_identifier == "f");
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr);

	const auto expr = result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(expr->_def->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((expr->_def->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("int"), "x" },
		arg_t{ type_identifier_t::make_type("int"), "y" },
		arg_t{ type_identifier_t::make_type("string"), "z" }
	}));
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
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 2);

	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_identifier == "hello");
	QUARK_TEST_VERIFY(result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr);

	const auto hello = result._top_level_statements[0]->_bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(hello->_def->_return_type == type_identifier_t::make_type("string"));
	QUARK_TEST_VERIFY((hello->_def->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("int"), "x" },
		arg_t{ type_identifier_t::make_type("int"), "y" },
		arg_t{ type_identifier_t::make_type("string"), "z" }
	}));


	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement->_expression->_function_def_expr);

	const auto main = result._top_level_statements[1]->_bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(main->_def->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((main->_def->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("string"), "args" }
	}));

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
	auto r = program_to_ast({}, kProgram2);
	QUARK_TEST_VERIFY(r._top_level_statements.size() == 2);
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_identifier == "testx");
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_def->_return_type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_def->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_def->_args[0]._type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_def->_args[0]._identifier == "v");
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_def->_statements.size() == 1);

	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_def->_return_type == make_type_identifier("int"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_def->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_def->_args[0]._type == make_type_identifier("string"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_def->_args[0]._identifier == "args");
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_def->_statements.size() == 2);
	//### Test body?
}


}	//	floyd_parser



