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

#include "parser_primitives.h"
#include "text_parser.h"
#include "steady_vector.h"
#include "parse_statement.h"
#include "parse_expression.h"
#include "statements.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "parser_ast.h"
#include "ast_utils.h"
#include "utils.h"
#include "json_support.h"

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

struct statement_result_t {
	statement_t _statement;
	std::string _rest;
};

statement_result_t read_statement(const ast_t& ast, const scope_ref_t scope_def, const string& pos){
	QUARK_ASSERT(scope_def->check_invariant());

	const auto token_pos = read_until(pos, whitespace_chars);

	//	return statement?
	if(token_pos.first == "return"){
		const auto return_statement_pos = parse_return_statement(pos);
		return { make__return_statement(return_statement_pos.first), skip_whitespace(return_statement_pos.second) };
	}

	//	struct definition?
	else if(token_pos.first == "struct"){
		const auto a = parse_struct_definition(scope_def, pos);
		return { define_struct_statement_t{ a.first }, skip_whitespace(a.second) };
	}

	else {
		const auto type_pos = read_required_type_identifier(pos);
		const auto identifier_pos = read_required_single_symbol(type_pos.second);

		/*
			Function definition?
			"int xyz(string a, string b){ ... }
		*/
		if(peek_string(skip_whitespace(identifier_pos.second), "(")){
			const auto function = parse_function_definition(ast, pos);
			{
				QUARK_SCOPED_TRACE("FUNCTION DEF");
				QUARK_TRACE(json_to_compact_string(scope_def_to_json(*function.first)));
			}
            return { define_function_statement_t{ function.first }, skip_whitespace(function.second) };
		}

		//	Define variable?
		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/

		else if(peek_string(skip_whitespace(identifier_pos.second), "=")){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(pos);
			return { assignment_statement.first, skip_whitespace(assignment_statement.second) };
		}

		else{
			throw std::runtime_error("syntax error");
		}
	}
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	try{
		auto ast = ast_t();
		const auto result = read_statement(ast, ast._global_scope, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto ast = ast_t();
	const auto result = read_statement(ast, ast._global_scope, "float test = testx(1234);\n\treturn 3;\n");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto ast = ast_t();
	const auto result = read_statement(ast, ast._global_scope, test_function1);
	QUARK_TEST_VERIFY(result._statement._define_function);
//	QUARK_TEST_VERIFY(*result._statement._define_function->_function_def == *make_test_function1(global));
	QUARK_TEST_VERIFY(result._statement._define_function->_function_def);
	QUARK_TEST_VERIFY(result._rest == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto ast = ast_t();
	const auto result = read_statement(ast, ast._global_scope, "struct test_struct0 " + k_test_struct0_body + ";");
	QUARK_TEST_VERIFY(result._statement._define_struct);
	QUARK_TEST_VERIFY(*result._statement._define_struct->_struct_def == *make_test_struct0(ast._global_scope));
}





//////////////////////////////////////////////////		program_to_ast()



struct alloc_struct_param : public host_data_i {
	public: virtual ~alloc_struct_param(){};

	alloc_struct_param(const scope_ref_t& struct_def) :
		_struct_def(struct_def)
	{
	}

	scope_ref_t _struct_def;
};


value_t host_function__alloc_struct(const ast_t& ast, const resolved_path_t& path, const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args){
	const alloc_struct_param& a = dynamic_cast<const alloc_struct_param&>(*param.get());

	const auto instance = make_default_struct_value(ast, path, a._struct_def);
	return instance;
}

/*
	Take struct definition and creates all types, member variables, constructors, member functions etc.
	??? add constructors and generated stuff.
*/
scope_ref_t install_struct_support(const ast_t& ast, const scope_ref_t scope_def, const scope_ref_t& struct_def){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(struct_def && struct_def->check_invariant());

	const std::string struct_name = struct_def->_name.to_string();
	const auto struct_name_ident = type_identifier_t::make(struct_name);

	//	Define struct type in current scope.
	auto types_collector2 = define_struct_type(scope_def->_types_collector, struct_name, struct_def);
	scope_ref_t s = resolve_struct_type(types_collector2, struct_name);

	//	Make constructor-function with same name as struct.
	{
//		const auto constructor_name = type_identifier_t::make(struct_name + "");
		const auto constructor_name = type_identifier_t::make(struct_name + "_constructor");
		const auto executable = executable_t(host_function__alloc_struct, make_shared<alloc_struct_param>(s));
		const auto a = make_function_def(constructor_name, struct_name_ident, {}, executable, {}, {});
		types_collector2 = define_function_type(types_collector2, constructor_name.to_string(), a);
	}

	return scope_def->set_types(types_collector2);
}

//??? Track when / order of definitions and binds so statements can't access them before they're in scope.


std::pair<scope_ref_t, std::string> read_statements_into_scope_def(const ast_t& ast, const scope_ref_t scope_def2, const string& s){
	QUARK_ASSERT(scope_def2 && scope_def2->check_invariant());

	//	Copy input scope_def.
	shared_ptr<const scope_def_t> result_scope = make_shared<scope_def_t>(*scope_def2);

	auto pos = skip_whitespace(s);
	std::vector<std::shared_ptr<statement_t> > statements;
	while(!pos.empty()){
		const auto statement_pos = read_statement(ast, result_scope, pos);

		//	Definition statements are immediately removed from AST and the types are defined instead.
		if(statement_pos._statement._define_struct){
			const auto a = statement_pos._statement._define_struct;
			result_scope = install_struct_support(ast, result_scope, a->_struct_def);
		}
		else if(statement_pos._statement._define_function){
			auto function_def = statement_pos._statement._define_function->_function_def;
			const auto function_name = function_def->_name;
			result_scope = result_scope->set_types(define_function_type(result_scope->_types_collector, function_name.to_string(), function_def));
		}
		else if(statement_pos._statement._bind_statement){
			const auto& bind = *statement_pos._statement._bind_statement;

			//	Reserve an entry in _members-vector for our variable.
			const auto members2 = result_scope->_members + std::vector<member_t>{ member_t(bind._type, bind._identifier) };
			executable_t executable2 = result_scope->_executable;
			executable2._statements.push_back(make_shared<statement_t>(statement_pos._statement));
			scope_ref_t scope2 = scope_def_t::make2(
				result_scope->_type,
				result_scope->_name,
				members2,
				executable2,
				result_scope->_types_collector,
				result_scope->_return_type
			);
			result_scope.swap(scope2);
		}
		else{
			executable_t executable2 = result_scope->_executable;
			executable2._statements.push_back(make_shared<statement_t>(statement_pos._statement));
			scope_ref_t scope2 = scope_def_t::make2(
				result_scope->_type,
				result_scope->_name,
				result_scope->_members,
				executable2,
				result_scope->_types_collector,
				result_scope->_return_type
			);
			result_scope.swap(scope2);
		}
		pos = skip_whitespace(statement_pos._rest);
	}

	return { result_scope, pos };
}


ast_t program_to_ast(const string& program){
	ast_t ast;
	string stage0 = json_to_compact_string(ast_to_json(ast));
	const auto statements_pos = read_statements_into_scope_def(ast, ast._global_scope, program);
	string stage1 = json_to_compact_string(ast_to_json(ast));

	ast_t ast2(statements_pos.first);
	string stage2 = json_to_compact_string(ast_to_json(ast2));

	QUARK_ASSERT(stage0 == stage1);
	QUARK_ASSERT(stage1 != stage2);
	trace(ast2);

	QUARK_ASSERT(ast2.check_invariant());
	return ast2;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
		"int main(string args){\n"
		"	return 3;\n"
		"}\n";

	auto result = program_to_ast(kProgram1);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("main"),
		type_identifier_t::make_int(),
		{
			{ type_identifier_t::make_string(), "args" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{},
		{}
	);

//	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "main", f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "main"));
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "main") == *f));
}

QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
		"int f(int x, int y, string z){\n"
		"	return 3;\n"
		"}\n";

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("f"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{},
		{}
	);

//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "f") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "f"));

	const auto f2 = resolve_function_type(result._global_scope->_types_collector, "f");
	QUARK_UT_VERIFY(f2->_type == scope_def_t::k_function_scope);

	const auto body = resolve_function_type(f2->_types_collector, "___body");
	QUARK_UT_VERIFY(body->_type == scope_def_t::k_subscope);
	QUARK_UT_VERIFY(body->_executable._statements.size() == 1);
}


QUARK_UNIT_TEST("", "program_to_ast()", "Local variables", ""){
	const string kProgram1 =
		"int main(string args){\n"
		"	int a = 4;\n"
		"	return 3;\n"
		"}\n";

	auto result = program_to_ast(kProgram1);
	const auto f2 = resolve_function_type(result._global_scope->_types_collector, "main");
	QUARK_UT_VERIFY(f2);
	QUARK_UT_VERIFY(f2->_type == scope_def_t::k_function_scope);

	const auto body = resolve_function_type(f2->_types_collector, "___body");
	QUARK_UT_VERIFY(body);
	QUARK_UT_VERIFY(body->_type == scope_def_t::k_subscope);
	QUARK_UT_VERIFY(body->_members.size() == 1);
	QUARK_UT_VERIFY(body->_members[0]._name == "a");
	QUARK_UT_VERIFY(body->_executable._statements.size() == 2);

}



#if false
QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
		"string hello(int x, int y, string z){\n"
		"	return \"test abc\";\n"
		"}\n"
		"int main(string args){\n"
		"	return 3;\n"
		"}\n";

	QUARK_TRACE(kProgram);

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("hello"),
		type_identifier_t::make_string(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant("test abc")))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "hello") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "hello"));

	const auto f2 = make_function_def(
		type_identifier_t::make("main"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_string(), "args" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "main") == *f2));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "main"));
}
#endif


#if false
QUARK_UNIT_TESTQ("program_to_ast()", "Call function a from function b"){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto result = program_to_ast(kProgram2);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("testx"),
		type_identifier_t::make_float(),
		{
			member_t{ type_identifier_t::make_float(), "v" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(13.4f)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "testx") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "testx"));

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_int(),
			vector<member_t>{
				member_t{ type_identifier_t::make_string(), "arg" }
			},
			{
				make__return_statement(bind_statement_t{"test", function_call_expr_t{"testx", }})
				make__return_statement(make_constant(value_t(13.4f)))
			}
		)
	));
*/
}
#endif



////////////////////////////		STRUCT SUPPORT




QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can instantiate a struct"){
	const auto result = program_to_ast(
		"struct pixel { string s; }"
		"string main(){\n"
		"	return \"\";"
		"}\n"
	);


	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_string(),
			vector<member_t>{},
			{
				make__return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}

QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can address a struct member variable"){
	const auto result = program_to_ast(
		"string main(){\n"
		"	return p.s + a;"
		"}\n"
	);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_string(),
			vector<member_t>{},
			{
				make__return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}



}	//	floyd_parser

