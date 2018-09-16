	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"

#include "statement.h"
#include "floyd_parser/floyd_parser.h"
#include "floyd_parser/parser_primitives.h"
#include "ast_value.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "ast.h"

namespace floyd {
using namespace std;


typeid_t resolve_type_name(const ast_json_t& t){
	const auto t2 = typeid_from_ast_json(t);
	return t2;
}

//??? loses output-type for some expressions. Make it nonlossy!
expression_t astjson_to_expression(const quark::trace_context_t& tracer, const json_t& e){
	QUARK_ASSERT(e.check_invariant());
	QUARK_CONTEXT_TRACE(tracer, json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = resolve_type_name(ast_json_t{type});

		if(type2.is_undefined()){
			return expression_t::make_literal_undefined();
		}
		else if(type2.get_base_type() == base_type::k_bool){
			return expression_t::make_literal_bool(value.is_false() ? false : true);
		}
		else if(type2.get_base_type() == base_type::k_int){
			return expression_t::make_literal_int((int)value.get_number());
		}
		else if(type2.get_base_type() == base_type::k_double){
			return expression_t::make_literal_double((double)value.get_number());
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
		const auto expr = astjson_to_expression(tracer, e.get_array_n(1));
		return expression_t::make_unary_minus(expr, nullptr);
	}
	else if(is_simple_expression__2(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto op2 = token_to_expression_type(op);
		const auto lhs_expr = astjson_to_expression(tracer, e.get_array_n(1));
		const auto rhs_expr = astjson_to_expression(tracer, e.get_array_n(2));
		return expression_t::make_simple_expression__2(op2, lhs_expr, rhs_expr, nullptr);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto condition_expr = astjson_to_expression(tracer, e.get_array_n(1));
		const auto a_expr = astjson_to_expression(tracer, e.get_array_n(2));
		const auto b_expr = astjson_to_expression(tracer, e.get_array_n(3));
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr, nullptr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto function_expr = astjson_to_expression(tracer, e.get_array_n(1));
		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(astjson_to_expression(tracer, arg));
		}
		return expression_t::make_call(function_expr, args2, nullptr);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = astjson_to_expression(tracer, e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		return expression_t::make_resolve_member(base_expr, member, nullptr);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto variable_symbol = e.get_array_n(1).get_string();
		return expression_t::make_load(variable_symbol, nullptr);
	}
	else if(op == "@i"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto parent_step = (int)e.get_array_n(1).get_number();
		const auto index = (int)e.get_array_n(2).get_number();
		return expression_t::make_load2(variable_address_t::make_variable_address(parent_step, index), nullptr);
	}
	else if(op == "[]"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto parent_address_expr = astjson_to_expression(tracer, e.get_array_n(1));
		const auto lookup_key_expr = astjson_to_expression(tracer, e.get_array_n(2));
		return expression_t::make_lookup(parent_address_expr, lookup_key_expr, nullptr);
	}
	else if(op == "construct-value"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto value_type = resolve_type_name(ast_json_t{e.get_array_n(1)});
		const auto args = e.get_array_n(2).get_array();

		std::vector<expression_t> args2;
		for(const auto m: args){
			args2.push_back(astjson_to_expression(tracer, m));
		}

		return expression_t::make_construct_value_expr(value_type, args2);
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
		const auto expr = astjson_to_expression(tracer, statement.get_array_n(1));
		return statement_t::make__return_statement(expr);
	}

	//	[ "bind", "double", "x", EXPRESSION, {} ]
	//	Last element is a list of meta info, like "mutable" etc.
	else if(type == "bind"){
		QUARK_ASSERT(statement.get_array_size() == 4 || statement.get_array_size() == 5);
		const auto bind_type = statement.get_array_n(1);
		const auto name = statement.get_array_n(2);
		const auto expr = statement.get_array_n(3);
		const auto meta = statement.get_array_size() >= 5 ? statement.get_array_n(4) : json_t();

		const auto bind_type2 = resolve_type_name(ast_json_t{bind_type});
		const auto name2 = name.get_string();
		const auto expr2 = astjson_to_expression(tracer, expr);
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
		const auto expr2 = astjson_to_expression(tracer, expr);
		return statement_t::make__store(name2, expr2);
	}

	//	[ "store2", parent_index, variable_index, EXPRESSION ]
	else if(type == "store2"){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = astjson_to_expression(tracer, expr);
		return statement_t::make__store2(variable_address_t::make_variable_address(parent_index, variable_index), expr2);
	}

	//	[ "block", [ STATEMENTS ] ]
	else if(type == "block"){
		QUARK_ASSERT(statement.get_array_size() == 2);

		const auto statements = statement.get_array_n(1);
		const auto r = astjson_to_statements(tracer, ast_json_t{statements});

		const auto body = body_t(r);
		return statement_t::make__block_statement(body);
	}

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

	else if(type == "def-func"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto def = statement.get_array_n(1);
		const auto name = def.get_object_element("name");
		const auto args = def.get_object_element("args");
		const auto fstatements = def.get_object_element("statements");
		const auto return_type = def.get_object_element("return_type");

		const auto name2 = name.get_string();
		const auto args2 = members_from_json(args);
		const auto fstatements2 = astjson_to_statements(tracer, ast_json_t{fstatements});
		const auto return_type2 = resolve_type_name(ast_json_t{return_type});
		const auto function_typeid = typeid_t::make_function(return_type2, get_member_types(args2));
		const auto body = body_t{fstatements2};
		const auto function_def = function_definition_t{function_typeid, args2, make_shared<body_t>(body), 0};

		const auto s = statement_t::define_function_statement_t{ name2, std::make_shared<function_definition_t>(function_def) };
		return statement_t::make__define_function_statement(s);
	}

	//	[ "if", CONDITION_EXPR, THEN_STATEMENTS, ELSE_STATEMENTS ]
	//	Else is optional.
	else if(type == keyword_t::k_if){
		QUARK_ASSERT(statement.get_array_size() == 3 || statement.get_array_size() == 4);
		const auto condition_expression = statement.get_array_n(1);
		const auto then_statements = statement.get_array_n(2);
		const auto else_statements = statement.get_array_size() == 4 ? statement.get_array_n(3) : json_t::make_array();

		const auto condition_expression2 = astjson_to_expression(tracer, condition_expression);
		const auto then_statements2 = astjson_to_statements(tracer, ast_json_t{then_statements});
		const auto else_statements2 = astjson_to_statements(tracer, ast_json_t{else_statements});

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

		const auto start_expression2 = astjson_to_expression(tracer, start_expression);
		const auto end_expression2 = astjson_to_expression(tracer, end_expression);
		const auto body_statements2 = astjson_to_statements(tracer, ast_json_t{body_statements});

		const auto range_type = for_mode.get_string() == "open-range" ? statement_t::for_statement_t::k_open_range : statement_t::for_statement_t::k_closed_range;
		return statement_t::make__for_statement(
			iterator_name.get_string(),
			start_expression2,
			end_expression2,
			body_t{ body_statements2 },
			range_type
		);
	}
	else if(type == keyword_t::k_while){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto expression = statement.get_array_n(1);
		const auto body_statements = statement.get_array_n(2);

		const auto expression2 = astjson_to_expression(tracer, expression);
		const auto body_statements2 = astjson_to_statements(tracer, ast_json_t{body_statements});

		return statement_t::make__while_statement(expression2, body_t{body_statements2});
	}
	else if(type == keyword_t::k_software_system){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto json_data = statement.get_array_n(1);

		return statement_t::make__software_system_statement(json_data);
	}

	//	[ "expression-statement", EXPRESSION ]
	else if(type == "expression-statement"){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = statement.get_array_n(1);
		const auto expr2 = astjson_to_expression(tracer, expr);
		return statement_t::make__expression_statement(expr2);
	}

	else{
		throw std::runtime_error("Illegal statement.");
	}
}

const std::vector<std::shared_ptr<statement_t> > astjson_to_statements(const quark::trace_context_t& tracer, const ast_json_t& p){
	QUARK_CONTEXT_SCOPED_TRACE(tracer, "astjson_to_statements()");
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

std::vector<json_t> symbols_to_json(const std::vector<std::pair<std::string, symbol_t>>& symbols){
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbols){
		const auto& symbol = e.second;
		const auto symbol_type_str = symbol._symbol_type == symbol_t::immutable_local ? "immutable_local" : "mutable_local";

		if(symbol._const_value.is_undefined() == false){
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"CONST",
				value_and_type_to_ast_json(symbol._const_value)._value
			});
			r.push_back(e2);
		}
		else{
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"LOCAL",
				json_t::make_object({
					{ "value_type", typeid_to_ast_json(symbol._value_type, json_tags::k_tag_resolve_state)._value },
					{ "type", symbol_type_str }
				})
			});
			r.push_back(e2);
		}

		symbol_index++;
	}
	return r;
}

ast_json_t body_to_json(const body_t& e){
	std::vector<json_t> statements;
	for(const auto& i: e._statements){
		statements.push_back(statement_to_json(*i)._value);
	}

	const auto symbols = symbols_to_json(e._symbols);

	return ast_json_t{
		json_t::make_object({
			std::pair<std::string, json_t>{ "statements", json_t::make_array(statements) },
			std::pair<std::string, json_t>{ "symbols", json_t::make_array(symbols) }
		})
	};
}

ast_json_t symbol_to_json(const symbol_t& e){
	const auto symbol_type_str = e._symbol_type == symbol_t::immutable_local ? "immutable_local" : "mutable_local";

	if(e._const_value.is_undefined() == false){
		return ast_json_t{
			json_t::make_object({
				{ "const_value", value_to_ast_json(e._const_value, json_tags::k_tag_resolve_state)._value }
			})
		};
	}
	else{
		return ast_json_t{
			json_t::make_object({
				{ "type", symbol_type_str },
				{ "value_type", typeid_to_ast_json(e._value_type, json_tags::k_tag_resolve_state)._value }
			})
		};
	}
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
	else if(e._def_function){
		return ast_json_t{json_t::make_array({
			json_t("def-func"),
			json_t(e._def_function->_name),
			function_def_to_ast_json(*e._def_function->_def)._value
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
			typeid_to_ast_json(e._bind_local->_bindtype, json_tags::k_tag_resolve_state)._value,
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
			json_t("closed_range"),
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
	else if(e._software_system){
		return ast_json_t{json_t::make_array({
			json_t(keyword_t::k_software_system),
			e._software_system->_json_data
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


ast_json_t expression_to_json_xyz(const expression_t& e){
	const auto opcode = e.get_operation();

	if(opcode == expression_type::k_literal){
		return ast_json_t{json_t::make_array({
			"k",
			value_to_ast_json(e._value, json_tags::k_tag_resolve_state)._value,
			typeid_to_ast_json(e._value.get_type(), json_tags::k_tag_resolve_state)._value
		})};
	}
	else if(is_arithmetic_expression(opcode) || is_comparison_expression(opcode)){
		return ast_json_t{
			json_t::make_array({
				expression_type_to_token(opcode),
				expression_to_json(e._input_exprs[0])._value,
				expression_to_json(e._input_exprs[1])._value
			})
		};
	}
	else if(opcode == expression_type::k_arithmetic_unary_minus__1){
		return ast_json_t{json_t::make_array({
			expression_to_json(e._input_exprs[0])._value
		})};
	}
	if(opcode == expression_type::k_conditional_operator3){
		const auto a = ast_json_t{json_t::make_array({
			"?:",
			expression_to_json(e._input_exprs[0])._value,
			expression_to_json(e._input_exprs[1])._value,
			expression_to_json(e._input_exprs[2])._value,
		})};
		return a;
	}
	else if(opcode == expression_type::k_call){
		const auto callee = e._input_exprs[0];
		const auto args = vector<expression_t>(e._input_exprs.begin() + 1, e._input_exprs.end());

		return ast_json_t{json_t::make_array({
			"call",
			expression_to_json(callee)._value,
			expressions_to_json(args)._value
		})};
	}
	else if(opcode == expression_type::k_define_struct){
		return ast_json_t{json_t::make_array({
			"def-struct",
			struct_definition_to_ast_json(*e._struct_def)._value
		})};
	}
	else if(opcode == expression_type::k_define_function){
		return ast_json_t{function_def_to_ast_json(*e._function_def)};
	}
	else if(opcode == expression_type::k_load){
		return ast_json_t{json_t::make_array({ "@", json_t(e._variable_name) })};
	}
	else if(opcode == expression_type::k_load2){
		return ast_json_t{json_t::make_array({ "@i", json_t(e._address._parent_steps), json_t(e._address._index) })};
	}

	else if(opcode == expression_type::k_resolve_member){
		return ast_json_t{json_t::make_array({
			"->",
			expression_to_json(e._input_exprs[0])._value,
			json_t(e._variable_name)
		})};
	}
	else if(opcode == expression_type::k_lookup_element){
		return ast_json_t{json_t::make_array({
			"[]",
			expression_to_json(e._input_exprs[0])._value,
			expression_to_json(e._input_exprs[1])._value
		})};
	}

	else if(opcode == expression_type::k_construct_value){
		return ast_json_t{json_t::make_array({
			"construct-value",
			typeid_to_ast_json(*e._output_type, json_tags::k_tag_resolve_state)._value,
			expressions_to_json(e._input_exprs)._value
		})};
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


ast_json_t expression_to_json(const expression_t& e){
	const auto a = expression_to_json_xyz(e);

	//	Add annotated-type element to json?
	if(e.is_annotated_shallow() && e.has_builtin_type() == false){
		const auto t = e.get_output_type();
		auto a2 = a._value.get_array();
		const auto type_json = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
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

	const auto program_body = astjson_to_statements(tracer, parse_tree);
	return ast_t{body_t{program_body}, {}};
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


}	//	floyd
