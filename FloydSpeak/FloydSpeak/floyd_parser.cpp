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

	for(const auto i: program._global_scope->_statements){
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

statement_result_t read_statement(const ast_t& ast1, const scope_def_t& scope_def, const string& pos){
	QUARK_ASSERT(ast1.check_invariant());
	QUARK_ASSERT(scope_def.check_invariant());

	ast_t ast2 = ast1;
	const auto token_pos = read_until(pos, whitespace_chars);

	//	return statement?
	if(token_pos.first == "return"){
		const auto return_statement_pos = parse_return_statement(ast1, pos);
		return { make__return_statement(return_statement_pos.first), ast2, skip_whitespace(return_statement_pos.second) };
	}

	//	struct definition?
	else if(token_pos.first == "struct"){
		const auto a = parse_struct_definition(pos);
		return { define_struct_statement_t{ std::get<0>(a), std::get<1>(a) }, ast1, skip_whitespace(std::get<2>(a)) };
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
			const pair<pair<string, function_def_t>, string> function = parse_function_definition(ast1, scope_def, pos);
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
//			const string& identifier = assignment_statement.first._bind_statement->_identifier;

/*
			//	??? If the value can be evaluted to a constant, do this. Else it's just a variable that can't be changed.
			const auto it = ast2._constant_values.find(identifier);
			if(it != ast2._constant_values.end()){
				throw std::runtime_error("Variable name already in use!");
			}

			shared_ptr<const value_t> blank;
			ast2._constant_values[identifier] = blank;
*/

			return { assignment_statement.first, ast2, skip_whitespace(assignment_statement.second) };
		}

		else{
			throw std::runtime_error("syntax error");
		}
	}
}


QUARK_UNIT_TESTQ("read_statement()", ""){
	try{
		auto scope_def = make_shared<scope_def_t>(scope_def_t(true, nullptr));
		const auto result = read_statement({}, *scope_def, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}


QUARK_UNIT_TESTQ("read_statement()", ""){
	auto scope_def = make_shared<scope_def_t>(scope_def_t(true, nullptr));
	const auto result = read_statement({}, *scope_def, "float test = testx(1234);\n\treturn 3;\n");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto scope_def = make_shared<scope_def_t>(scope_def_t(true, nullptr));
	const auto result = read_statement({}, *scope_def, test_function1);
	QUARK_TEST_VERIFY(result._statement._define_function);
	QUARK_TEST_VERIFY(result._statement._define_function->_type_identifier == "test_function1");
	QUARK_TEST_VERIFY(result._statement._define_function->_function_def == make_test_function1());
	QUARK_TEST_VERIFY(result._rest == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto scope_def = make_shared<scope_def_t>(scope_def_t(true, nullptr));
	const auto result = read_statement({}, *scope_def, "struct test_struct0 " + k_test_struct0_body + ";");
	QUARK_TEST_VERIFY(result._statement._define_struct);
	QUARK_TEST_VERIFY(result._statement._define_struct->_type_identifier == "test_struct0");
	QUARK_TEST_VERIFY(result._statement._define_struct->_struct_def == make_test_struct0());
}





//////////////////////////////////////////////////		program_to_ast()



//??? second pass for semantics (resolve all types and symbols, precalculate expressions.

//??? add default-constructor to every type, even built-in types.
value_t make_default_value(const floyd_parser::type_identifier_t& type){
	QUARK_ASSERT(type.check_invariant());

	if(type == type_identifier_t("null")){
		return value_t();
	}
	else if(type == type_identifier_t("bool")){
		return value_t(false);
	}
	else if(type == type_identifier_t("int")){
		return value_t(0);
	}
	else if(type == type_identifier_t("float")){
		return value_t(0.0f);
	}
	else if(type == type_identifier_t("string")){
		return value_t("");
	}

	//	Ce
//	std::shared_ptr<floyd_parser::type_def_> str = vm.resolve_type(type.to_string());

	//??? Need better way
	else if(type == type_identifier_t("struct_instance")){
		QUARK_ASSERT(false);
		return value_t("");
	}
	else if(type == type_identifier_t("__type_value")){
		QUARK_ASSERT(false);
		return value_t("");
	}
	else{
		QUARK_ASSERT(false);
	}
	return value_t();
}



//////////////////////////////////////////////////		program_to_ast()



struct alloc_struct_param : public host_data_i {
	public: virtual ~alloc_struct_param(){};

	alloc_struct_param(const shared_ptr<struct_def_t>& struct_def) :
		_struct_def(struct_def)
	{
	}

	std::shared_ptr<struct_def_t> _struct_def;
};



value_t make_struct_instance(const struct_def_t& def){
	auto instance = make_shared<struct_instance_t>();

	instance->__def = &def;
	for(int i = 0 ; i < def._members.size() ; i++){
		const auto& member_def = def._members[i];

		//	If there is an initial value for this member, use that. Else use default value for this type.
		value_t value = member_def._value ? *member_def._value : make_default_value(*member_def._type);
		instance->_member_values[member_def._name] = value;
	}
	return value_t(instance);
}

value_t hosts_function__alloc_struct(const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args){
	const alloc_struct_param& a = dynamic_cast<const alloc_struct_param&>(*param.get());

	std::shared_ptr<struct_def_t> b = a._struct_def;
	if(!b){
		throw std::runtime_error("Undefined struct!");
	}

	const auto instance = make_struct_instance(*b);
	return instance;
}

/*
	Take struct definition and creates all types, member variables, constructors, member functions etc.
			//??? add constructors and generated stuff.
*/
vector<shared_ptr<statement_t>> install_struct_support(scope_def_t& scope_def, const std::string& struct_name, const struct_def_t& struct_def){
	QUARK_ASSERT(scope_def.check_invariant());
	QUARK_ASSERT(struct_name.size() > 0);
	QUARK_ASSERT(struct_def.check_invariant());

	const auto struct_name_ident = type_identifier_t::make_type(struct_name);

	//	Define struct type and data members.
	scope_def._types_collector = scope_def._types_collector.define_struct_type(struct_name, struct_def);
	std::shared_ptr<struct_def_t> s = scope_def._types_collector.resolve_struct_type(struct_name);


	//	Make constructor with same name as struct.
	/*{
		const auto b = "pixel pixel_constructor(){ pixel temp = allocate_struct(\"pixel\"); return temp; }";
		const auto statement_pos = read_statement(ast2, b);
		ast2 = statement_pos._ast;
		QUARK_ASSERT(statement_pos._statement._define_function);
		const auto define_function_statement = *statement_pos._statement._define_function;
		ast2._types_collector = ast2._types_collector.define_function_type(define_function_statement._type_identifier, define_function_statement._function_def);
	}*/

	{
		const auto param = make_shared<alloc_struct_param>(s);
		auto function_scope = make_shared<scope_def_t>(true, &scope_def);
		function_scope->_host_function = hosts_function__alloc_struct;
		function_scope->_host_function_param = param;
		const auto a = make_function_def(struct_name_ident, struct_name_ident, {}, function_scope);
		scope_def._types_collector = scope_def._types_collector.define_function_type(struct_name + "_constructor", a);
	}

//	statements.push_back(make_shared<statement_t>(statement_pos._statement));

	//??? easier to use source code template and compile it.
/*
	const function_def_t function_def {
		type_identifier_t::make_type(struct_name),
		vector<arg_t>{
			{type_identifier_t::make_type("int"), "this" }
		},
		{}
	};
*/

/*
		else if(statement_pos._statement._define_function){
			ast2._types_collector = ast2._types_collector.define_function_type(statement_pos._statement._define_function->_type_identifier, statement_pos._statement._define_function->_function_def);
		}
*/

	std::vector<std::shared_ptr<statement_t> > statements;
	return statements;
}




//??? Make this a separate parse pass, that resolves symbols and take decisions.
ast_t program_to_ast(const ast_t& init, const string& program){
	ast_t ast2 = init;

	auto pos = program;
	pos = skip_whitespace(pos);

	scope_def_t& scope_def = *init._global_scope;
	std::vector<std::shared_ptr<statement_t> > statements;
	while(!pos.empty()){
		const auto statement_pos = read_statement(ast2, scope_def, pos);
		ast2 = statement_pos._ast;

		if(statement_pos._statement._define_struct){
			const auto a = statement_pos._statement._define_struct;
			const auto b = install_struct_support(scope_def, a->_type_identifier, a->_struct_def);
			statements.insert(statements.end(), b.begin(), b.end());
		}
		else if(statement_pos._statement._define_function){
			scope_def._types_collector = scope_def._types_collector.define_function_type(statement_pos._statement._define_function->_type_identifier, statement_pos._statement._define_function->_function_def);
		}
		else{
			statements.push_back(make_shared<statement_t>(statement_pos._statement));
//			throw std::runtime_error("Unexpected statement.");
		}
		pos = skip_whitespace(statement_pos._rest);
	}

	ast2._global_scope->_statements = statements;

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
	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

	auto scope_def = make_shared<scope_def_t>(true, result._global_scope.get());
	scope_def->_statements = {
		make_shared<statement_t>(makie_return_statement(make_constant(3)))
	};
	QUARK_TEST_VERIFY((*result._global_scope->_types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("main"),
			type_identifier_t::make_type("int"),
			vector<arg_t>{ arg_t{ type_identifier_t::make_type("string"), "args" }},
			scope_def
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
	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

	auto scope_def = make_shared<scope_def_t>(true, result._global_scope.get());
	scope_def->_statements = {
		make_shared<statement_t>(makie_return_statement(make_constant(3)))
	};
	QUARK_TEST_VERIFY((*result._global_scope->_types_collector.resolve_function_type("f") ==
		make_function_def(
			type_identifier_t::make_type("f"),
			type_identifier_t::make_type("int"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("int"), "x" },
				arg_t{ type_identifier_t::make_type("int"), "y" },
				arg_t{ type_identifier_t::make_type("string"), "z" }
			},
			scope_def
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
	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

	auto scope_def = make_shared<scope_def_t>(true, result._global_scope.get());
	scope_def->_statements = {
		make_shared<statement_t>(makie_return_statement(make_constant("test abc")))
	};
	QUARK_TEST_VERIFY((*result._global_scope->_types_collector.resolve_function_type("hello") ==
		make_function_def(
			type_identifier_t::make_type("hello"),
			type_identifier_t::make_type("string"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("int"), "x" },
				arg_t{ type_identifier_t::make_type("int"), "y" },
				arg_t{ type_identifier_t::make_type("string"), "z" }
			},
			scope_def
		)
	));

	auto scope_def2 = make_shared<scope_def_t>(true, result._global_scope.get());
	scope_def2->_statements = {
		make_shared<statement_t>(makie_return_statement(make_constant(3)))
	};
	QUARK_TEST_VERIFY((*result._global_scope->_types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_type("main"),
			type_identifier_t::make_type("int"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("string"), "args" }
			},
			scope_def2
		)
	));
}


QUARK_UNIT_TESTQ("program_to_ast()", "Call function a from function b"){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto result = program_to_ast({}, kProgram2);
	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

	auto scope_def = make_shared<scope_def_t>(true, result._global_scope.get());
	scope_def->_statements = {
		make_shared<statement_t>(makie_return_statement(make_constant(13.4f)))
	};
	QUARK_TEST_VERIFY((*result._global_scope->_types_collector.resolve_function_type("testx") ==
		make_function_def(
			type_identifier_t::make_type("testx"),
			type_identifier_t::make_type("float"),
			vector<arg_t>{
				arg_t{ type_identifier_t::make_type("float"), "v" }
			},
			scope_def
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




QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can instantiate a struct"){
	const auto result = program_to_ast(
		{},
		"struct pixel { string s; };"
		"string main(){\n"
		"	return \"\";"
		"}\n"
	);


	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

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

QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can address a struct member variable"){
	const auto result = program_to_ast(
		{},
		"string main(){\n"
		"	return p.s + a;"
		"}\n"
	);
	QUARK_TEST_VERIFY(result._global_scope->_statements.size() == 0);

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



