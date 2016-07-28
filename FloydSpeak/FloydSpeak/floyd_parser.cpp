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










//////////////////////////////////////////////////		program_to_ast()


	//### better if ast_t is closed for modification -- internals should use different storage to collect ast into.
ast_t program_to_ast(const ast_t& init, const string& program){
	ast_t result = init;

	auto pos = program;
	pos = skip_whitespace(pos);
	while(!pos.empty()){
		const auto statement_pos = read_statement(result, pos);
		result._top_level_statements.push_back(make_shared<statement_t>(statement_pos.first));

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
	QUARK_TEST_VERIFY(expr->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((expr->_args == vector<arg_t>{ arg_t{ type_identifier_t::make_type("string"), "args" }}));
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
	QUARK_TEST_VERIFY(expr->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((expr->_args == vector<arg_t>{
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
	QUARK_TEST_VERIFY(hello->_return_type == type_identifier_t::make_type("string"));
	QUARK_TEST_VERIFY((hello->_args == vector<arg_t>{
		arg_t{ type_identifier_t::make_type("int"), "x" },
		arg_t{ type_identifier_t::make_type("int"), "y" },
		arg_t{ type_identifier_t::make_type("string"), "z" }
	}));


	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[1]->_bind_statement->_expression->_function_def_expr);

	const auto main = result._top_level_statements[1]->_bind_statement->_expression->_function_def_expr;
	QUARK_TEST_VERIFY(main->_return_type == type_identifier_t::make_type("int"));
	QUARK_TEST_VERIFY((main->_args == vector<arg_t>{
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
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_return_type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_args[0]._type == make_type_identifier("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_args[0]._identifier == "v");
	QUARK_TEST_VERIFY(r._top_level_statements[0]->_bind_statement->_expression->_function_def_expr->_body._statements.size() == 1);

	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_return_type == make_type_identifier("int"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_args[0]._type == make_type_identifier("string"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_args[0]._identifier == "args");
	QUARK_TEST_VERIFY(r._top_level_statements[1]->_bind_statement->_expression->_function_def_expr->_body._statements.size() == 2);
	//### Test body?
}


}	//	floyd_parser



#if false
QUARK_UNIT_TEST("", "run()", "", ""){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return \"hello, world!\";\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t("hello, world!"));
}
#endif

