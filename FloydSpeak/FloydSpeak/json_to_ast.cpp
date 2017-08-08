//
//  json_to_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 18/09/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_to_ast.h"
#include <utility>

#include "statements.h"
#include "parser_value.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "json_writer.h"
#include "parser_primitives.h"

using namespace floyd_parser;
using namespace std;

#if false

typeid_t resolve_type123(const string& id, const map<string, typeid_t>& typeids){
	//	#Basic-types
	if(id == "$null"){
		return typeid_t::make_null();
	}
	else if(id == "$bool"){
		return typeid_t::make_bool();
	}
	else if(id == "$int"){
		return typeid_t::make_int();
	}
	else if(id == "$float"){
		return typeid_t::make_float();
	}
	else if(id == "$string"){
		return typeid_t::make_string();
	}
	else{
		const auto it = typeids.find(id);
		if(it == typeids.end()){
			QUARK_ASSERT(false);
		}

		return it->second;
	}
}

/*
	Example:
		[
			{ "name": "g_version", "type": "<int>" },
			{ "name": "message", "type": "<string>" },
			{ "name": "pos", "type": "$3999" }
		]
*/
std::vector<member_t> conv_members(const json_t& members, const map<string, typeid_t>& typeids){
	std::vector<member_t> members2;
	for(const auto i: members.get_array()){
		const string arg_name = i.get_object_element("name").get_string();
		const string arg_type = i.get_object_element("type").get_string();
		QUARK_ASSERT(arg_type[0] == '$');

		const auto arg_type2 = resolve_type123(arg_type, typeids);
		members2.push_back(member_t(arg_type2, arg_name));
	}
	return members2;
}


//??? Simplify now that expression_t doesn't have classes for each expression type.
expression_t expression_from_json(const json_t& e, const map<string, typeid_t>& typeids){
	QUARK_ASSERT(e.is_array() && e.get_array_size() >= 1);

	const string op = e.get_array_n(0).get_string();
	if(op == "k"){
//???
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);

		//	#Basic-types
		if(type == "$null"){
			return expression_t::make_constant(value_t());
		}
		else if(type == "$bool"){
			return expression_t::make_constant(value.is_true());
		}
		else if(type == "$int"){
			return expression_t::make_constant((int)value.get_number());
		}
		else if(type == "$float"){
			return expression_t::make_constant((float)value.get_number());
		}
		else if(type == "$string"){
			return expression_t::make_constant(value.get_string());
		}
		else{
			const auto it = typeids.find(type.get_string());
			if(it == typeids.end()){
				QUARK_ASSERT(false);
			}
			const auto base_type = it->second.get_base_type();
			if(base_type == base_type::k_struct){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(false);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto lhs_expr = expression_from_json(e.get_array_n(1), typeids);
		const auto rhs_expr = expression_from_json(e.get_array_n(2), typeids);
		const auto op2 = string_to_math2_op(op);
		return expression_t::make_math2_operation(op2, lhs_expr, rhs_expr);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 5);
		const auto condition_expr = expression_from_json(e.get_array_n(1));
		const auto a_expr = expression_from_json(e.get_array_n(2));
		const auto b_expr = expression_from_json(e.get_array_n(3));
		const auto expr_type = resolve_type123(e.get_array_n(4).get_string(), typeids);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto f_address = expression_from_json(e.get_array_n(1));

		//??? Hack - we should have real expressions for function names.
		//??? Also: we should resolve all names 100% at this point.
		QUARK_ASSERT(f_address.get_operation() == expression_t::operation::k_resolve_variable);
		const string func_name = f_address.get_symbol();


//		const auto function_expr = expression_from_json(e.get_array_n(1), types);

		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			const auto arg2 = expression_from_json(arg, typeids);
			args2.push_back(arg2);
		}

		const auto return_type = resolve_type123(e.get_array_n(3).get_string(), typeids);
		return expression_t::make_function_call(type_identifier_t::make(func_name), args2, return_type);
//		return expression_t::make_function_call(function_expr, args2, return_type);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto base_expr = expression_from_json(e.get_array_n(1), typeids);
		const auto member_name = e.get_array_n(2).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(3).get_string(), typeids);
		return expression_t::make_resolve_member(base_expr, member_name, expr_type);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto variable_name = e.get_array_n(1).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(2).get_string(), typeids);

//??? Bound to variable?
		return expression_t::make_resolve_variable(variable_name, expr_type);
	}
	else{
		QUARK_ASSERT(false);
	}
}




/*
	{
		"global": {
			"args": [],
			"locals": [],
			"members": [],
			"name": "global",
			"return_type": "",
			"statements": [],
			"type": "global"
		},
		"lookup": {
			"$1000": {
				"base_type": "function",
				"path": "global/main",
				"scope_def": {
					"args": [{ "name": "args", "type": "$string" }],
					"locals": [],
					"members": [],
					"name": "main",
					"return_type": "$string",
					"statements": [
						["return", ["+", ["k", "123", "$string"], ["k", "456", "$string"], "$string"]]
					],
					"type": "function",
					"types": {}
				}
			}
		}
	}
*/

//??? Make test AST that tests alla scenarios of lexical scopes.

/*
	struct symbol_t {
		enum symbol_type {
			k_null,
			k_function_def_object,
			k_struct_def_object,
			k_constant
		};

		symbol_type _type;
		std::string _object_id;
		std::shared_ptr<value_t> _constant;
		typeid_t _typeid
	};
*/

pair< map<string, symbol_t>, map<string, shared_ptr<const lexical_scope_t> > > make_symbols_and_objects(const json_t& program, const std::map<string, typeid_t>& typeids){
	map<string, symbol_t> symbols;
	std::map<string, std::shared_ptr<const lexical_scope_t>> objects;
	const auto lookup = program.get_object_element("lookup").get_object();
	for(const auto& e: lookup){
		const auto base_type = e.second.get_object_element("base_type");
		const auto scope_def = e.second.get_optional_object_element("scope_def");

		//	Get scope_def.
		const auto type = scope_def.get_object_element("type").get_string();
		const auto name = scope_def.get_object_element("name").get_string();
		const auto return_type_id = scope_def.get_object_element("return_type").get_string();
		const auto args = scope_def.get_optional_object_element("args", json_t::make_array()).get_array();
		const auto locals = scope_def.get_optional_object_element("locals", json_t::make_array()).get_array();
		const auto statements = scope_def.get_object_element("statements").get_array();

		const auto args2 = conv_members(args, typeids);
		const auto local_variables2 = conv_members(locals, typeids);
		const auto return_type2 = resolve_type123(return_type_id, typeids);

		std::vector<std::shared_ptr<statement_t> > statements2;
		for(const auto s: statements){
			const string op = s.get_array_n(0).get_string();
			if(op == "return"){
				QUARK_ASSERT(s.get_array_size() == 2);
				const auto expr = expression_from_json(s.get_array_n(1), typeids);
				statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
			}
			else if(op == "bind"){
				QUARK_ASSERT(s.get_array_size() == 4);
				const auto bind_type = resolve_type123(s.get_array_n(1).get_string(), typeids);
				const auto identifier = s.get_array_n(2).get_string();
				const auto expr = expression_from_json(s.get_array_n(3), typeids);
				statements2.push_back(make_shared<statement_t>(make__bind_statement(bind_type, identifier, expr)));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		if(base_type == "struct"){
			symbols[e.first] = symbol_t{ symbol_t::k_struct_def_object, {}, {}, typeid_t::make_struct("") };
		}
		else if(base_type == "vector"){
		}
		else if(base_type == "function"){
			vector<typeid_t> arg_types;
			for(const auto arg_e: args2){
				arg_types.push_back(arg_e._type);
			}
			const auto function_def = lexical_scope_t::make_function_object(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				return_type2
			);
			const auto function_type = typeid_t::make_function(return_type2, arg_types);

			objects[e.first] = function_def;
			symbols[e.first] = symbol_t{ symbol_t::k_function_def_object, {}, {}, {} };
		}
		else if(base_type == "subscope"){
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	return { symbols, objects };
}

/*
	"$1000": {
		"base_type": "function",
		"path": "global/main",
		"scope_def": {
			"args": [{ "name": "args", "type": "$string" }],
			"locals": [],
			"members": [],
			"name": "main",
			"return_type": "$string",
			"statements": [
				["return", ["+", ["k", "123", "$string"], ["k", "456", "$string"], "$string"]]
			],
			"type": "function",
			"types": {}
		}
	}
*/
typeid_t lookupelement_to_typeid(const std::pair<std::string, json_t> lookup_element){
	const auto id = lookup_element.first;
	const auto base_type = lookup_element.second.get_object_element("base_type");
	const auto scope_def = lookup_element.second.get_optional_object_element("scope_def");

	if(base_type == "$null"){
		return typeid_t::make_null();
	}
	else if(base_type == "$bool"){
		return typeid_t::make_bool();
	}
	else if(base_type == "$int"){
		return typeid_t::make_int();
	}
	else if(base_type == "$float"){
		return typeid_t::make_float();
	}
	else if(base_type == "$string"){
		return typeid_t::make_string();
	}

	else if(base_type == "$string"){
		return typeid_t::make_string();
	}

	else if(base_type == "$struct"){
		QUARK_ASSERT(false);
	}

	else if(base_type == "$vector"){
		QUARK_ASSERT(false);
	}

	else if(base_type == "$function"){
		const auto return_type_id = scope_def.get_object_element("return_type").get_string();
		const auto args = scope_def.get_optional_object_element("args", json_t::make_array()).get_array();
		const auto args2 = conv_members(args, typeids);
		const auto return_type2 = resolve_type123(return_type_id, typeids);

		vector<typeid_t> arg_types;
		for(const auto arg_e: args2){
			arg_types.push_back(arg_e._type);
		}
		return typeid_t::make_function(return_type2, arg_types);
	}

	else{
		QUARK_ASSERT(false);
	}
}



map<string, typeid_t> lookup_to_typeids(const json_t& program){
	map<string, typeid_t> typeids;
	const auto lookup = program.get_object_element("lookup").get_object();
	for(const auto& e: lookup){
		typeid_t id = lookupelement_to_typeid(e);
		typeids[e.first] = id;
	}
	return typeids;
}



std::shared_ptr<const lexical_scope_t> conv_scope_def__expressions(const json_t& scope_def){
	const string type = scope_def.get_object_element("type").get_string();
	const string name = scope_def.get_object_element("name").get_string();
	const string return_type_id = scope_def.get_object_element("return_type").get_string();
	const auto args = scope_def.get_optional_object_element("args", json_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("locals", json_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("members", json_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("statements").get_array();

	std::vector<member_t> args2 = conv_members(args);
	std::vector<member_t> local_variables2 = conv_members(local_variables);
	std::vector<member_t> members2 = conv_members(members);

	if(type == "function"){
//???		QUARK_ASSERT(statements.size() > 0);
		std::vector<std::shared_ptr<statement_t> > statements2;
		for(const auto s: statements){
			const string op = s.get_array_n(0).get_string();
			if(op == "return"){
				QUARK_ASSERT(s.get_array_size() == 2);
				const auto expr = expression_from_json(s.get_array_n(1));
				statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
			}
			else if(op == "bind"){
				QUARK_ASSERT(s.get_array_size() == 4);
				const auto bind_type = resolve_type123(s.get_array_n(1).get_string());
				const auto identifier = s.get_array_n(2).get_string();
				const auto expr = expression_from_json(s.get_array_n(3));
				statements2.push_back(make_shared<statement_t>(make__bind_statement(bind_type, identifier, expr)));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		const auto function_type = scope_def.get_optional_object_element("function_type");
		if(function_type && function_type.get_string() == "def-constructor"){
			return lexical_scope_t::make_builtin_function_object(
				type_identifier_t::make(name),
				lexical_scope_t::efunc_variant::k_default_constructor,
				resolve_type123(return_type_id)
			);
		}
		else{
			return lexical_scope_t::make_function_object(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id)
			);
		}
	}
	else if(type == "struct"){
		return lexical_scope_t::make_struct(type_identifier_t::make(name), members2);
	}
	else if(type == "global"){
		return lexical_scope_t::make_global_scope();
	}
	else{
		QUARK_ASSERT(false);
	}
	return {};
}




/*
	Input tree has all types and variables resolved.
	Returns immutable AST.
*/
ast_t json_to_ast(const json_t& program){
	const auto typeids = lookup_to_typeids(program);
	const auto s = make_symbols_and_objects(program, typeids);

/*
	//	Make objects table.
	const auto lookup = program.get_object_element("lookup").get_object();
	for(const auto& e: lookup){
		const auto base_type = e.second.get_object_element("base_type");
		const auto scope_def = e.second.get_optional_object_element("scope_def");

		const string type = scope_def.get_object_element("type").get_string();
		const string name = scope_def.get_object_element("name").get_string();
		const string return_type_id = scope_def.get_object_element("return_type").get_string();
		const auto args = scope_def.get_optional_object_element("args", json_t::make_array()).get_array();

		std::vector<member_t> args2 = conv_members(args, symbols);

		if(base_type == "struct"){
			symbols[e.first] =typeid_t::make_struct("");
		}
		else if(base_type == "vector"){
			symbols[e.first] =typeid_t::make_struct("");
		}
		else if(base_type == "function"){
			vector<typeid_t> args;
			for(const auto e2: r->_args){
				args.push_back(e2._type);
			}
			return typeid_t::make_function(r->_return_type, args);
		}
		else if(base_type == "subscope"){
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
*/

	const auto global_scope = conv_scope_def__expressions(program.get_object_element("global"));
	ast_t result(global_scope, s.first, s.second);
	return result;
}


QUARK_UNIT_TESTQ("json_to_ast()", "Minimum program"){
	const auto a = parse_json(seq_t(R"(
			{
				"global": {
					"args": [],
					"locals": [],
					"members": [],
					"name": "global",
					"return_type": "",
					"statements": [],
					"type": "global"
				},
				"lookup": {
					"$1000": {
						"base_type": "function",
						"path": "global/main",
						"scope_def": {
							"args": [],
							"locals": [],
							"members": [],
							"name": "main",
							"return_type": "$int",
							"statements": [["return", ["k", 3, "$int"]]],
							"type": "function",
							"types": {}
						}
					}
				}
			}
	)"));

	QUARK_TRACE(json_to_pretty_string(a.first));

	const auto r = json_to_ast(a.first);

	const auto b = ast_to_json(r);
	QUARK_TRACE(json_to_pretty_string(b));

	QUARK_ASSERT(a.first.is_null() == false);
}
#endif
