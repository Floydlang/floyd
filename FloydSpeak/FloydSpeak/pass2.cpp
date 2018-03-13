	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"

#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "parser_primitives.h"
#include "ast.h"

namespace floyd {
using namespace std;


typeid_t resolve_type_name(const ast_json_t& t){
	const auto t2 = typeid_from_ast_json(t);
	return t2;
}

expression_t parser_expression_to_ast(const quark::trace_context_t& tracer, const json_t& e){
	QUARK_ASSERT(e.check_invariant());
	QUARK_CONTEXT_TRACE(tracer, json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = resolve_type_name(ast_json_t{type});

		if(type2.is_null()){
			return expression_t::make_literal_null();
		}
		else if(type2.get_base_type() == base_type::k_bool){
			return expression_t::make_literal_bool(value.is_false() ? false : true);
		}
		else if(type2.get_base_type() == base_type::k_int){
			return expression_t::make_literal_int((int)value.get_number());
		}
		else if(type2.get_base_type() == base_type::k_float){
			return expression_t::make_literal_float((float)value.get_number());
		}
		else if(type2.get_base_type() == base_type::k_string){
			return expression_t::make_literal_string(value.get_string());
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else if(op == "unary_minus"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		return expression_t::make_unary_minus(expr, nullptr);
	}
	else if(is_simple_expression__2(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto op2 = token_to_expression_type(op);
		const auto lhs_expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		const auto rhs_expr = parser_expression_to_ast(tracer, e.get_array_n(2));
		return expression_t::make_simple_expression__2(op2, lhs_expr, rhs_expr, nullptr);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto condition_expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		const auto a_expr = parser_expression_to_ast(tracer, e.get_array_n(2));
		const auto b_expr = parser_expression_to_ast(tracer, e.get_array_n(3));
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr, nullptr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto function_expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(parser_expression_to_ast(tracer, arg));
		}
		return expression_t::make_call(function_expr, args2, nullptr);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		return expression_t::make_resolve_member(base_expr, member, nullptr);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto variable_symbol = e.get_array_n(1).get_string();
		return expression_t::make_variable_expression(variable_symbol, nullptr);
	}
	else if(op == "@i"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto parent_step = (int)e.get_array_n(1).get_number();
		const auto index = (int)e.get_array_n(2).get_number();
		return expression_t::make_load2(variable_address_t::make_variable_address(parent_step, index), nullptr);
	}
	else if(op == "[]"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto parent_address_expr = parser_expression_to_ast(tracer, e.get_array_n(1));
		const auto lookup_key_expr = parser_expression_to_ast(tracer, e.get_array_n(2));
		return expression_t::make_lookup(parent_address_expr, lookup_key_expr, nullptr);
	}
	else if(op == "vector-def"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto element_type = resolve_type_name(ast_json_t{e.get_array_n(1)});
		const auto elements = e.get_array_n(2).get_array();

		std::vector<expression_t> elements2;
		for(const auto m: elements){
			elements2.push_back(parser_expression_to_ast(tracer, m));
		}

		return expression_t::make_vector_definition(element_type, elements2);
	}
	else if(op == "dict-def"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto value_type = resolve_type_name(ast_json_t{e.get_array_n(1)});
		const auto elements = e.get_array_n(2).get_array();
		QUARK_ASSERT((elements.size() % 2) == 0);

		std::map<string, expression_t> elements2;
		for(int i = 0 ; i < elements.size() ; i += 2){
			const expression_t key_expr = parser_expression_to_ast(tracer, elements[i + 0]);
			const expression_t value_expr = parser_expression_to_ast(tracer, elements[i + 1]);
			const auto key_string = key_expr.get_literal().get_string_value();
			elements2.insert(pair<string, expression_t>(key_string, value_expr));
		}
		return expression_t::make_dict_definition(value_type, elements2);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





statement_t astjson_to_statement__nonlossy(const quark::trace_context_t& tracer, const ast_json_t& statement0){
	QUARK_CONTEXT_SCOPED_TRACE(tracer, "astjson_to_statement__nonlossy()");
	QUARK_ASSERT(statement0._value.check_invariant());
	QUARK_ASSERT(statement0._value.is_array());

	const auto statement = statement0._value;
	const auto type = statement.get_array_n(0);

	//	[ "return", [ "k", 3, "int" ] ]
	if(type == "return"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = parser_expression_to_ast(tracer, statement.get_array_n(1));
		return statement_t::make__return_statement(expr);
	}

	//	[ "bind", "float", "x", EXPRESSION, {} ]
	//	Last element is a list of meta info, like "mutable" etc.
	else if(type == "bind"){
		QUARK_ASSERT(statement.get_array_size() == 4 || statement.get_array_size() == 5);
		const auto bind_type = statement.get_array_n(1);
		const auto name = statement.get_array_n(2);
		const auto expr = statement.get_array_n(3);
		const auto meta = statement.get_array_size() >= 5 ? statement.get_array_n(4) : json_t();

		const auto bind_type2 = resolve_type_name(ast_json_t{bind_type});
		const auto name2 = name.get_string();
		const auto expr2 = parser_expression_to_ast(tracer, expr);
		bool mutable_flag = !meta.is_null() && meta.does_object_element_exist("mutable");
		const auto mutable_mode = mutable_flag ? statement_t::bind_local_t::k_mutable : statement_t::bind_local_t::k_immutable;
		return statement_t::make__bind_local(name2, bind_type2, expr2, mutable_mode);
	}

	//	[ "store", "x", EXPRESSION ]
	else if(type == "store"){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto name = statement.get_array_n(1);
		const auto expr = statement.get_array_n(2);

		const auto name2 = name.get_string();
		const auto expr2 = parser_expression_to_ast(tracer, expr);
		return statement_t::make__store(name2, expr2);
	}

	//	[ "store2", parent_index, variable_index, EXPRESSION ]
	else if(type == "store2"){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = parser_expression_to_ast(tracer, expr);
		return statement_t::make__store2(variable_address_t::make_variable_address(parent_index, variable_index), expr2);
	}

	//	[ "block", [ STATEMENTS ] ]
	else if(type == "block"){
		QUARK_ASSERT(statement.get_array_size() == 2);

		const auto statements = statement.get_array_n(1);
		const auto r = parser_statements_to_ast__lossy(tracer, ast_json_t{statements});

		const auto body = body_t(r);
		return statement_t::make__block_statement(body);
	}

	/*
		INPUT:
			[
				"def-func",
				{
					"args": [

					],
					"name": "main",
					"return_type": "int",
					"statements": [
						[ "return", [ "k", 3, "int" ] ]
					]
				}
			]

		OUTPUT:
			"main": [
				{
					"base_type": "function",
					"scope_def": {
						"args": [],
						"locals": [],
						"members": [],
						"name": "main",
						"return_type": "int",
						"statements": [???],
						"type": "function",
						"types": {}
					}
				}
			]
	*/
/*
	else if(type == "def-func"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto def = statement.get_array_n(1);
		const auto name = def.get_object_element("name");
		const auto args = def.get_object_element("args");
		const auto fstatements = def.get_object_element("statements");
		const auto return_type = def.get_object_element("return_type");

		const auto name2 = name.get_string();
		const auto args2 = members_from_json(args);
		const auto fstatements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{fstatements});
		const auto return_type2 = resolve_type_name(ast_json_t{return_type});

		const auto function_typeid = typeid_t::make_function(return_type2, get_member_types(args2));
		const auto function_def = function_definition_t(args2, fstatements2, return_type2);
		const auto function_def_expr = expression_t::make_function_definition(function_def);


		const auto s = statement_t::define_struct_statement_t{ name, struct_def2 };
		return statement_t::make__define_struct_statement(s);

//??? use bind-local instead of bind_or_assign_statement_t
//??? current code makes round-trip lossy.
		statements2.push_back(make_shared<statement_t>(statement_t::make__bind"bind"_statement(
			name2,
			function_typeid,
			function_def_expr, statement_t::bind_or_assign_statement_t::mutable_mode::k_no_mutable_tag
		)));
*/


	/*
		INPUT:
			[
				"def-struct",
				{
					"members": [
						{ "name": "red", "type": "float" },
						{ "name": "green", "type": "float" },
						{ "name": "blue", "type": "float" }
					],
					"name": "pixel"
				}
			],

		OUTPUT:
			"pixel": [
				{
					"base_type": "struct",
					"scope_def": {
						"args": [],
						"locals": [],
						"members": [
							{ "name": "red", "type": "float" },
							{ "name": "green", "type": "float" },
							{ "name": "blue", "type": "float" }
						],
						"name": "pixel",
						"return_type": "",
						"statements": [],
						"type": "struct",
						"types": {}
					}
				}
			]
	*/
	else if(type == "def-struct"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto struct_def = statement.get_array_n(1);
		const auto name = struct_def.get_object_element("name").get_string();
		const auto members = struct_def.get_object_element("members").get_array();

		const auto members2 = members_from_json(members);
		const auto struct_def2 = struct_definition_t(members2);

		const auto s = statement_t::define_struct_statement_t{ name, std::make_shared<struct_definition_t>(struct_def2) };
		return statement_t::make__define_struct_statement(s);
	}

	//	[ "if", CONDITION_EXPR, THEN_STATEMENTS, ELSE_STATEMENTS ]
	//	Else is optional.
	else if(type == keyword_t::k_if){
		QUARK_ASSERT(statement.get_array_size() == 3 || statement.get_array_size() == 4);
		const auto condition_expression = statement.get_array_n(1);
		const auto then_statements = statement.get_array_n(2);
		const auto else_statements = statement.get_array_size() == 4 ? statement.get_array_n(3) : json_t::make_array();

		const auto condition_expression2 = parser_expression_to_ast(tracer, condition_expression);
		const auto then_statements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{then_statements});
		const auto else_statements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{else_statements});

		return statement_t::make__ifelse_statement(
			condition_expression2,
			body_t{then_statements2},
			body_t{else_statements2}
		);
	}
	else if(type == keyword_t::k_for){
		QUARK_ASSERT(statement.get_array_size() == 6);
		const auto for_mode = statement.get_array_n(1);
		const auto iterator_name = statement.get_array_n(2);
		const auto start_expression = statement.get_array_n(3);
		const auto end_expression = statement.get_array_n(4);
		const auto body_statements = statement.get_array_n(5);

		const auto start_expression2 = parser_expression_to_ast(tracer, start_expression);
		const auto end_expression2 = parser_expression_to_ast(tracer, end_expression);
		const auto body_statements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{body_statements});

		return statement_t::make__for_statement(
			iterator_name.get_string(),
			start_expression2,
			end_expression2,
			body_t{body_statements2}
		);
	}
	else if(type == keyword_t::k_while){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto expression = statement.get_array_n(1);
		const auto body_statements = statement.get_array_n(2);

		const auto expression2 = parser_expression_to_ast(tracer, expression);
		const auto body_statements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{body_statements});

		return statement_t::make__while_statement(expression2, body_t{body_statements2});
	}

	//	[ "expression-statement", EXPRESSION ]
	else if(type == "expression-statement"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = statement.get_array_n(1);
		const auto expr2 = parser_expression_to_ast(tracer, expr);
		return statement_t::make__expression_statement(expr2);
	}

	else{
		throw std::runtime_error("Illegal statement.");
	}
}





const std::vector<std::shared_ptr<statement_t> > parser_statements_to_ast__lossy(const quark::trace_context_t& tracer, const ast_json_t& p){
	QUARK_CONTEXT_SCOPED_TRACE(tracer, "parser_statements_to_ast()");
	QUARK_ASSERT(p._value.check_invariant());
	QUARK_ASSERT(p._value.is_array());

	vector<shared_ptr<statement_t>> statements2;

	for(const auto statement: p._value.get_array()){
		const auto type = statement.get_array_n(0);

		//	SPECIAL HANDLING
		//	Convertd parsers function-definition statement to bind + function-def expression.
		/*
			INPUT:
				[
					"def-func",
					{
						"args": [
							
						],
						"name": "main",
						"return_type": "int",
						"statements": [
							[ "return", [ "k", 3, "int" ] ]
						]
					}
				]

			OUTPUT:
				"main": [
					{
						"base_type": "function",
						"scope_def": {
							"args": [],
							"locals": [],
							"members": [],
							"name": "main",
							"return_type": "int",
							"statements": [???],
							"type": "function",
							"types": {}
						}
					}
				]
		*/
		if(type == "def-func"){
			QUARK_ASSERT(statement.get_array_size() == 2);
			const auto def = statement.get_array_n(1);
			const auto name = def.get_object_element("name");
			const auto args = def.get_object_element("args");
			const auto fstatements = def.get_object_element("statements");
			const auto return_type = def.get_object_element("return_type");

			const auto name2 = name.get_string();
			const auto args2 = members_from_json(args);
			const auto fstatements2 = parser_statements_to_ast__lossy(tracer, ast_json_t{fstatements});
			const auto return_type2 = resolve_type_name(ast_json_t{return_type});
			const auto function_typeid = typeid_t::make_function(return_type2, get_member_types(args2));
			const auto body = body_t{fstatements2};
			const auto function_def = function_definition_t(args2, make_shared<body_t>(body), return_type2);
			const auto function_def_expr = expression_t::make_function_definition(function_def);

			//??? use bind-local instead of bind_or_assign_statement_t?
			statements2.push_back(make_shared<statement_t>(
				statement_t::make__bind_local(
					name2,
					function_typeid,
					function_def_expr, statement_t::bind_local_t::k_immutable
				))
			);
		}

		else {
			const auto s2 = astjson_to_statement__nonlossy(tracer, ast_json_t{ statement });
			statements2.push_back(make_shared<statement_t>(s2));
		}
	}

	return statements2;
}

const std::vector<std::shared_ptr<statement_t> > statements_to_ast__nonlossy(const quark::trace_context_t& tracer, const ast_json_t& p){
	QUARK_CONTEXT_SCOPED_TRACE(tracer, "statements_to_ast__nonlossy()");
	QUARK_ASSERT(p._value.check_invariant());
	QUARK_ASSERT(p._value.is_array());

	vector<shared_ptr<statement_t>> statements2;
	for(const auto statement: p._value.get_array()){
		const auto type = statement.get_array_n(0);
		const auto s2 = astjson_to_statement__nonlossy(tracer, ast_json_t{ statement });
		statements2.push_back(make_shared<statement_t>(s2));
	}
	return statements2;
}

ast_json_t body_to_json(const body_t& e){
	std::vector<json_t> statements;
	for(const auto& i: e._statements){
		statements.push_back(statement_to_json(*i)._value);
	}

	std::vector<json_t> symbols;
	for(const auto& i: e._symbols){
		const auto e2 = json_t::make_object({
			{ "name", i.first },
			{ "symbol", symbol_to_json(i.second)._value }
		});
		symbols.push_back(e2);
	}

	return ast_json_t{
		json_t::make_object({
			std::pair<std::string, json_t>{ "statements", json_t::make_array(statements) },
			std::pair<std::string, json_t>{ "symbols", json_t::make_array(symbols) }
		})
	};
}



ast_json_t symbol_to_json(const symbol_t& e){
	const auto symbol_type_str = e._symbol_type == symbol_t::immutable_local ? "immutable_local" : "mutable_local";

	return ast_json_t{
		json_t::make_object({
			{ "type", symbol_type_str },
			{ "value_type", typeid_to_ast_json(e._value_type)._value },
			{ "const_value", value_to_ast_json(e._const_value)._value }
		})
	};
}


ast_json_t statement_to_json(const statement_t& e){
	QUARK_ASSERT(e.check_invariant());

	if(e._return){
		return ast_json_t{json_t::make_array({
			json_t(keyword_t::k_return),
			expression_to_json(e._return->_expression)._value
		})};
	}
	else if(e._def_struct){
		return ast_json_t{json_t::make_array({
			json_t("def-struct"),
			json_t(e._def_struct->_name),
			struct_definition_to_ast_json(*e._def_struct->_def)._value
		})};
	}
	else if(e._bind_local){
		bool mutable_flag = e._bind_local->_locals_mutable_mode == statement_t::bind_local_t::k_mutable;
		const auto meta = mutable_flag
			? json_t::make_object({
				std::pair<std::string, json_t>{"mutable", mutable_flag}
			})
			: json_t();

		return ast_json_t{make_array_skip_nulls({
			json_t("bind"),
			e._bind_local->_new_local_name,
			typeid_to_ast_json(e._bind_local->_bindtype)._value,
			expression_to_json(e._bind_local->_expression)._value,
			meta
		})};
	}
	else if(e._store){
		return ast_json_t{make_array_skip_nulls({
			json_t("store"),
			e._store->_local_name,
			expression_to_json(e._store->_expression)._value
		})};
	}
	else if(e._store2){
		return ast_json_t{make_array_skip_nulls({
			json_t("store2"),
			e._store2->_dest_variable._parent_steps,
			e._store2->_dest_variable._index,
			expression_to_json(e._store2->_expression)._value
		})};
	}
	else if(e._block){
		return ast_json_t{json_t::make_array({
			json_t("block"),
			body_to_json(e._block->_body)._value
		})};
	}
	else if(e._if){
		return ast_json_t{json_t::make_array({
			json_t(keyword_t::k_if),
			expression_to_json(e._if->_condition)._value,
			body_to_json(e._if->_then_body)._value,
			body_to_json(e._if->_else_body)._value
		})};
	}
	else if(e._for){
		return ast_json_t{json_t::make_array({
			json_t(keyword_t::k_for),
			json_t("open_range"),
			expression_to_json(e._for->_start_expression)._value,
			expression_to_json(e._for->_end_expression)._value,
			body_to_json(e._for->_body)._value
		})};
	}
	else if(e._while){
		return ast_json_t{json_t::make_array({
			json_t(keyword_t::k_while),
			expression_to_json(e._while->_condition)._value,
			body_to_json(e._while->_body)._value
		})};
	}
	else if(e._expression){
		return ast_json_t{json_t::make_array({
			json_t("expression-statement"),
			expression_to_json(e._expression->_expression)._value
		})};
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




ast_json_t expression_to_json(const expression_t& e){
	const auto a = e.get_expr()->expr_base__to_json();

	if(e.is_annotated_shallow() && e.has_builtin_type() == false){
		const auto t = e.get_annotated_type();
		auto a2 = a._value.get_array();
		const auto type_json = typeid_to_ast_json(t);
		a2.push_back(type_json._value);
		return ast_json_t{ json_t::make_array(a2) };
	}
	else{
		return a;
	}
}

ast_json_t expressions_to_json(const std::vector<expression_t> v){
	std::vector<json_t> r;
	for(const auto e: v){
		r.push_back(expression_to_json(e)._value);
	}
	return ast_json_t{json_t::make_array(r)};
}

std::string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json._value);
}




ast_t run_pass2(const quark::trace_context_t& tracer, const ast_json_t& parse_tree){
	QUARK_CONTEXT_TRACE(tracer, json_to_pretty_string(parse_tree._value));

	const auto program_body = parser_statements_to_ast__lossy(tracer, parse_tree);
	return ast_t(body_t{program_body});
}




///////////////////////////////////////			TESTS



const std::string k_test_program_0_parserout = R"(
	[
		[
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ] ]
				]
			}
		]
	]
)";
const std::string k_test_program_0_pass2output = R"(
	{
		"globals": {
			"statements": [
				[
					"bind",
					"main",
					[ "function", "int", [] ],
					[
						"func-def",
						[ "function", "int", [] ],
						[],
						{
							"statements": [
								[
									"return", [ "k", 3, "int" ]
								]
							],
							"symbols": [
							]
						},
						"int"
					]
				]
			],
			"symbols": [
			]
		}
	}


//	{ "statements": [["bind", "main", ["function", "int", []], ["func-def", ["function", "int", []], [], [["return", ["k", 3, "int", "int"]]], "int", ["function", "int", []]]]] }
)";


/*
	These tests verify that:
	- the transform of the AST worked as expected
	- Correct errors are emitted.
*/
QUARK_UNIT_TESTQ("run_pass2()", "k_test_program_0"){
	const auto tracer = quark::make_default_tracer();

	const auto pass1 = parse_json(seq_t(floyd::k_test_program_0_parserout)).first;
	const auto pass2 = run_pass2(tracer, ast_json_t{pass1});
	const auto pass2_output = parse_json(seq_t(floyd::k_test_program_0_pass2output)).first;
	ut_compare_jsons(ast_to_json(pass2)._value, pass2_output);
}

/*
QUARK_UNIT_TESTQ("run_pass2()", "k_test_program_100"){
	const auto pass1 = parse_json(seq_t(k_test_program_100_parserout)).first;
	const auto pass2 = run_pass2(pass1);
	const auto pass2_output = parse_json(seq_t("[12344]")).first;
	ut_compare_jsons(pass2, pass2_output);
}

void test_error(const string& program, const string& error_string){
	const auto pass1 = floyd::parse_program2(context, program);
	try{
		const auto pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		if(error_string != ""){
			const auto s = string(e.what());
			quark::ut_compare(s, error_string);
		}
	}
}

QUARK_UNIT_TESTQ("run_pass2()", "1001"){
	test_error(
		R"(
			string main(){
				return 10 * "hello";
			}
		)",
		"1001 - Left & right side of math2 must have same type."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1002"){
	test_error(
		R"(
			string main(){
				int p = f();
				return p;
			}
		)",
		""//"1002 - Undefined function \"f\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1003"){
	test_error(
		R"(
			string main(){
				int p = main(42);
				return p;
			}
		)",
		"1003 - Wrong number of argument to function \"main\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1004"){
	test_error(
		R"(
			int a(string p1){
				return 31;
			}
			string main(){
				int p = a(42);
				return p;
			}
		)",
		""//"1004 - Argument 0 to function \"a\" mismatch."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1005"){
	test_error(
		R"(
			string main(){
				return p;
			}
		)",
		""//"1005 - Undefined variable \"p\"."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "Return undefine type"){
	test_error(
		R"(
			xyz main(){
				return 10;
			}
		)",
		"Undefined type \"xyz\""
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1006"){
	test_error(
		R"(
			int main(){
				int a = "hello";
				return 0;
			}
		)",
		"1006 - Bind type mismatch."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1007"){
	test_error(
		R"(
			return 4;
			int main(){
				return 0;
			}
		)",
		"1007 - Return-statement not allowed outside function definition."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1008"){
	test_error(
		R"(
			int main(){
				return "goodbye";
			}
		)",
		"1008 - return value doesn't match function return type."
	);
}


QUARK_UNIT_TESTQ("run_pass2()", "1010"){
	test_error(
		R"(
			struct pixel { string s = "two"; }
			string main(){
				pixel p = pixel_constructor();
				int a = p.xyz;
				return 1;
			}
		)",
		"Unresolved member \"xyz\""
	);
}
*/

}	//	floyd

