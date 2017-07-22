//
//  json_to_ast.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 18/09/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_to_ast.h"

#include "statements.h"
#include "parser_value.h"
#include "utils.h"
#include "json_support.h"
#include "parser_primitives.h"

using namespace floyd_parser;
using namespace std;








shared_ptr<const type_def_t> resolve_type123(const string& id, const map<string, shared_ptr<type_def_t>>& types){
	//	#Basic-types
	if(id == "$null"){
		return type_def_t::make_null_typedef();
	}
	else if(id == "$bool"){
		return type_def_t::make_bool_typedef();
	}
	else if(id == "$int"){
		return type_def_t::make_int_typedef();
	}
	else if(id == "$float"){
		return type_def_t::make_float_typedef();
	}
	else if(id == "$string"){
		return type_def_t::make_string_typedef();
	}
	else{
		const auto it = types.find(id);
		if(it == types.end()){
			QUARK_ASSERT(false);
		}

		return it->second;
	}
}


expression_t conv_expression(const json_value_t& e, const map<string, shared_ptr<type_def_t>>& types){
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
			const auto it = types.find(type.get_string());
			if(it == types.end()){
				QUARK_ASSERT(false);
			}
			const auto base_type = it->second->get_type();
			if(base_type == base_type::k_struct){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_subscope){
				QUARK_ASSERT(false);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
	}
	else if(is_math1_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto expr = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		return expression_t::make_math_operation1(
			expression_t::math1_operation::negate,
			conv_expression(expr, types)
		);
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto lhs_expr = conv_expression(e.get_array_n(1), types);
		const auto rhs_expr = conv_expression(e.get_array_n(2), types);
		const auto op2 = string_to_math2_op(op);
		return expression_t::make_math_operation2(
			op2,
			lhs_expr,
			rhs_expr
		);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 5);
		const auto condition_expr = conv_expression(e.get_array_n(1), types);
		const auto a_expr = conv_expression(e.get_array_n(2), types);
		const auto b_expr = conv_expression(e.get_array_n(3), types);
		const auto expr_type = resolve_type123(e.get_array_n(4).get_string(), types);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto f_address = conv_expression(e.get_array_n(1), types);

		//??? Hack - we should have real expressions for function names.
		//??? Also: we should resolve all names 100% at this point.
		QUARK_ASSERT(f_address._resolve_variable);
		const string func_name = f_address._resolve_variable->_variable_name;


//		const auto function_expr = conv_expression(e.get_array_n(1), types);

		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			const auto arg2 = conv_expression(arg, types);
			args2.push_back(arg2);
		}

		const auto return_type = resolve_type123(e.get_array_n(3).get_string(), types);
		return expression_t::make_function_call(type_identifier_t::make(func_name), args2, return_type);
//		return expression_t::make_function_call(function_expr, args2, return_type);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto base_expr = conv_expression(e.get_array_n(1), types);
		const auto member_name = e.get_array_n(2).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(3).get_string(), types);
		return expression_t::make_resolve_member(base_expr, member_name, expr_type);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto variable_name = e.get_array_n(1).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(2).get_string(), types);

//??? Bound to variable?
		return expression_t::make_resolve_variable(variable_name, expr_type);
	}
	else{
		QUARK_ASSERT(false);
	}
}


/*
	Example:
		[
			{ "expr": 1, "name": "g_version", "type": "<int>" },
			{ "expr": "Welcome!", "name": "message", "type": "<string>" }
		]
*/
std::vector<member_t> conv_members(const json_value_t& members, const map<string, shared_ptr<type_def_t>>& types, bool convert_expressions){
	std::vector<member_t> members2;
	for(const auto i: members.get_array()){
		const string arg_name = i.get_object_element("name").get_string();
		const string arg_type = i.get_object_element("type").get_string();
		const auto init_expr = i.get_optional_object_element("expr");
		QUARK_ASSERT(arg_type[0] == '$');

		const auto arg_type2 = resolve_type123(arg_type, types);

		if(init_expr){
			if(convert_expressions){
				const auto init_expr2 = conv_expression(init_expr, types);

				QUARK_ASSERT(init_expr2._constant != nullptr);
				members2.push_back(member_t(arg_type2, arg_name, *init_expr2._constant));
			}
			else{
				members2.push_back(member_t(arg_type2, arg_name));
			}
		}
		else{
			members2.push_back(member_t(arg_type2, arg_name));
		}
	}
	return members2;
}


/*
	{
		"_args": [],
		"_locals": [],
		"_members": [],
		"_name": "main",
		"_return_type": "$1001",
		"_statements": [["return", ["k", 3, "$1001"]]],
		"_type": "function",
		"_types": {}
	}
*/
std::shared_ptr<const scope_def_t> conv_scope_def__no_expressions(const json_value_t& scope_def, const map<string, shared_ptr<type_def_t>>& types){
	const string type = scope_def.get_object_element("_type").get_string();
	const string name = scope_def.get_object_element("_name").get_string();
	const string return_type_id = scope_def.get_object_element("_return_type").get_string();
	const auto args = scope_def.get_optional_object_element("_args", json_value_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("_locals", json_value_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("_members", json_value_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("_statements").get_array();

	std::vector<member_t> args2 = conv_members(args, types, false);
	std::vector<member_t> local_variables2 = conv_members(local_variables, types, false);
	std::vector<member_t> members2 = conv_members(members, types, false);

	if(type == "function"){
		std::vector<std::shared_ptr<statement_t> > statements2;
		const auto function_type = scope_def.get_optional_object_element("_function_type");
		if(function_type && function_type.get_string() == "def-constructor"){
			return scope_def_t::make_builtin_function_def(
				type_identifier_t::make(name),
				scope_def_t::efunc_variant::k_default_constructor,
				resolve_type123(return_type_id, types)
			);
		}
		else{
			return scope_def_t::make_function_def(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id, types)
			);
		}
	}
	else if(type == "struct"){
		return scope_def_t::make_struct(type_identifier_t::make(name), members2);
	}
	else if(type == "global"){
		return scope_def_t::make_global_scope();
	}
	else{
		QUARK_ASSERT(false);
	}
	return {};
}
std::shared_ptr<const scope_def_t> conv_scope_def__expressions(const json_value_t& scope_def, const map<string, shared_ptr<type_def_t>>& types){
	const string type = scope_def.get_object_element("_type").get_string();
	const string name = scope_def.get_object_element("_name").get_string();
	const string return_type_id = scope_def.get_object_element("_return_type").get_string();
	const auto args = scope_def.get_optional_object_element("_args", json_value_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("_locals", json_value_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("_members", json_value_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("_statements").get_array();

	std::vector<member_t> args2 = conv_members(args, types, true);
	std::vector<member_t> local_variables2 = conv_members(local_variables, types, true);
	std::vector<member_t> members2 = conv_members(members, types, true);

	if(type == "function"){
//???		QUARK_ASSERT(statements.size() > 0);
		std::vector<std::shared_ptr<statement_t> > statements2;
		for(const auto s: statements){
			const string op = s.get_array_n(0).get_string();
			if(op == "return"){
				QUARK_ASSERT(s.get_array_size() == 2);
				const auto expr = conv_expression(s.get_array_n(1), types);
				statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
			}
			else if(op == "bind"){
				QUARK_ASSERT(s.get_array_size() == 4);
				const auto bind_type = resolve_type123(s.get_array_n(1).get_string(), types);
				const auto identifier = s.get_array_n(2).get_string();
				const auto expr = conv_expression(s.get_array_n(3), types);
				statements2.push_back(make_shared<statement_t>(make__bind_statement(bind_type, identifier, expr)));
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		const auto function_type = scope_def.get_optional_object_element("_function_type");
		if(function_type && function_type.get_string() == "def-constructor"){
			return scope_def_t::make_builtin_function_def(
				type_identifier_t::make(name),
				scope_def_t::efunc_variant::k_default_constructor,
				resolve_type123(return_type_id, types)
			);
		}
		else{
			return scope_def_t::make_function_def(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id, types)
			);
		}
	}
	else if(type == "struct"){
		return scope_def_t::make_struct(type_identifier_t::make(name), members2);
	}
	else if(type == "global"){
		return scope_def_t::make_global_scope();
	}
	else{
		QUARK_ASSERT(false);
	}
	return {};
}

std::shared_ptr<type_def_t> conv_type_def__no_expressions(const json_value_t& def, const map<string, shared_ptr<type_def_t>>& types){
	QUARK_ASSERT(def.check_invariant());

	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");
	const auto path = def.get_object_element("path");

	if(base_type == "null"){
		return type_def_t::make_null_typedef();
	}
	else if(base_type == "bool"){
		return type_def_t::make_bool_typedef();
	}
	else if(base_type == "int"){
		return type_def_t::make_int_typedef();
	}
	else if(base_type == "float"){
		return type_def_t::make_float_typedef();
	}
	else if(base_type == "string"){
		return type_def_t::make_string_typedef();
	}

	else if(base_type == "struct"){
		return type_def_t::make_struct_type_def(conv_scope_def__no_expressions(scope_def, types));
	}
	else if(base_type == "vector"){
		QUARK_ASSERT(false);
	}
	else if(base_type == "function"){
		return type_def_t::make_function_type_def(conv_scope_def__no_expressions(scope_def, types));
	}
	else if(base_type == "subscope"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::shared_ptr<type_def_t> conv_type_def__expressions_and_statements(const json_value_t& def, const map<string, shared_ptr<type_def_t>>& types){
	QUARK_ASSERT(def.check_invariant());

	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");
	const auto path = def.get_object_element("path");

	if(base_type == "struct"){
		return type_def_t::make_struct_type_def(conv_scope_def__expressions(scope_def, types));
	}
	else if(base_type == "function"){
		return type_def_t::make_function_type_def(conv_scope_def__expressions(scope_def, types));
	}
	else if(base_type == "subscope"){
		QUARK_ASSERT(false);
	}
	else{
		return nullptr;
	}
}

/*
	"$3": { "name": "string", "base_type": "string" },
	"$4": { "name": "pixel_t", "base_type": "function", "scope_def":
		{
			"parent_scope": "$5",		//	Parent scope.
			"base_type": "function",
			"scope_def": {
				"_name": "pixel_t_constructor",
				"_type": "function",
*/

/*
	Input tree has all types and variables resolved.
	Returns immutable AST.
*/
ast_t json_to_ast(const json_value_t& program){
	//	Make placeholder type-defs for each symbol. We use pointer to type_def as its identity.
	map<string, shared_ptr<type_def_t>> temp_type_defs;
	for(const auto& s: program.get_object_element("lookup").get_object()){
		temp_type_defs[s.first] = type_def_t::make_null_typedef();
	}

	//	Make a shallow pass through all types. Convert JSON based types to type_def_t:s.
	//	Only definitions of the types, not yet expressions or statements that USE these types.
	for(const auto& s: program.get_object_element("lookup").get_object()){
		const auto id = s.first;
		const json_value_t& def = s.second;
		std::shared_ptr<type_def_t> type_def = conv_type_def__no_expressions(def, temp_type_defs);
		shared_ptr<type_def_t> identity_type_def = temp_type_defs.at(id);

		//	Update our identity type defs.
		identity_type_def->swap(*type_def.get());
	}

	//	Second pass - do expressions & statements -- make them USE the new type_def_t.
	for(const auto& s: program.get_object_element("lookup").get_object()){
		const auto id = s.first;
		const json_value_t& def = s.second;
		std::shared_ptr<type_def_t> type_def = conv_type_def__expressions_and_statements(def, temp_type_defs);
		if(type_def){
			shared_ptr<type_def_t> identity_type_def = temp_type_defs.at(id);

			//	Update our identify type defs.
			identity_type_def->swap(*type_def.get());
		}
	}


	const auto global_scope = conv_scope_def__no_expressions(program.get_object_element("global"), temp_type_defs);

	const auto global_scope2 = scope_def_t::make_global_scope();

	ast_t result;
	result._symbols = temp_type_defs;
	result._global_scope = global_scope2;
	QUARK_ASSERT(result.check_invariant());
	return result;
}
