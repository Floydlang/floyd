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




math_operation2_expr_t::operation string_to_math2_op(const string& op){
	if(op == "+"){
		return math_operation2_expr_t::operation::k_add;
	}
	else if(op == "-"){
		return math_operation2_expr_t::operation::k_subtract;
	}
	else if(op == "*"){
		return math_operation2_expr_t::operation::k_multiply;
	}
	else if(op == "/"){
		return math_operation2_expr_t::operation::k_divide;
	}
	else if(op == "%"){
		return math_operation2_expr_t::operation::k_remainder;
	}

	else if(op == "<="){
		return math_operation2_expr_t::operation::k_smaller_or_equal;
	}
	else if(op == "<"){
		return math_operation2_expr_t::operation::k_smaller;
	}
	else if(op == ">="){
		return math_operation2_expr_t::operation::k_larger_or_equal;
	}
	else if(op == ">"){
		return math_operation2_expr_t::operation::k_larger;
	}

	else if(op == "=="){
		return math_operation2_expr_t::operation::k_logical_equal;
	}
	else if(op == "!="){
		return math_operation2_expr_t::operation::k_logical_nonequal;
	}
	else if(op == "&&"){
		return math_operation2_expr_t::operation::k_logical_and;
	}
	else if(op == "||"){
		return math_operation2_expr_t::operation::k_logical_or;
	}

	else{
		QUARK_ASSERT(false);
	}
}




shared_ptr<const type_def_t> resolve_type123(const string& id, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	const auto it = temp_type_defs.find(id);
	if(it == temp_type_defs.end()){
		QUARK_ASSERT(false);
	}

	return it->second;
}


expression_t conv_expression(const json_value_t& e, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	QUARK_ASSERT(e.is_array() && e.get_array_size() >= 1);

	const string op = e.get_array_n(0).get_string();
	if(op == "k"){
//???
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);

		const auto it = temp_type_defs.find(type.get_string());
		if(it == temp_type_defs.end()){
			QUARK_ASSERT(false);
		}
		const auto base_type = it->second->get_type();
		if(base_type == base_type::k_null){
			return expression_t::make_constant(value_t());
		}
		else if(base_type == base_type::k_bool){
			return expression_t::make_constant(value.is_true());
		}
		else if(base_type == base_type::k_int){
			return expression_t::make_constant((int)value.get_number());
		}
		else if(base_type == base_type::k_float){
			return expression_t::make_constant((float)value.get_number());
		}
		else if(base_type == base_type::k_string){
			return expression_t::make_constant(value.get_string());
		}
		else if(base_type == base_type::k_struct){
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
	else if(is_math1_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto expr = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		return expression_t::make_math_operation1(
			math_operation1_expr_t::negate,
			conv_expression(expr, temp_type_defs)
		);
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto lhs_expr = conv_expression(e.get_array_n(1), temp_type_defs);
		const auto rhs_expr = conv_expression(e.get_array_n(2), temp_type_defs);
		const auto op2 = string_to_math2_op(op);
		return expression_t::make_math_operation2(
			op2,
			lhs_expr,
			rhs_expr
		);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 5);
		const auto condition_expr = conv_expression(e.get_array_n(1), temp_type_defs);
		const auto a_expr = conv_expression(e.get_array_n(2), temp_type_defs);
		const auto b_expr = conv_expression(e.get_array_n(3), temp_type_defs);
		const auto expr_type = resolve_type123(e.get_array_n(4).get_string(), temp_type_defs);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto f_address = conv_expression(e.get_array_n(1), temp_type_defs);

		//??? Hack - we should have real experssions for function names.
		//??? Also: we should resolve all names 100% at this point.
		QUARK_ASSERT(f_address._resolve_variable);
		const string func_name = f_address._resolve_variable->_variable_name;


		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			const auto arg2 = conv_expression(arg, temp_type_defs);
			args2.push_back(arg2);
		}

		const auto return_type = resolve_type123(e.get_array_n(3).get_string(), temp_type_defs);
		return expression_t::make_function_call(type_identifier_t::make(func_name), args2, return_type);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto base_expr = conv_expression(e.get_array_n(1), temp_type_defs);
		const auto member_name = e.get_array_n(2).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(3).get_string(), temp_type_defs);
		return expression_t::make_resolve_member(make_shared<expression_t>(base_expr), member_name, expr_type);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto variable_name = e.get_array_n(1).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(2).get_string(), temp_type_defs);

//??? Bound to variable?
		return expression_t::make_resolve_variable(variable_name, expr_type);
	}
	else{
		QUARK_ASSERT(false);
	}
}


	/*
		{ "expr": 1, "name": "g_version", "type": "<int>" },
		{ "expr": "Welcome!", "name": "message", "type": "<string>" }
	*/
std::vector<member_t> conv_members(const json_value_t& members, const map<string, shared_ptr<type_def_t>>& temp_type_defs, bool convert_expressions){
	std::vector<member_t> members2;
	for(const auto i: members.get_array()){
		const string arg_name = i.get_object_element("name").get_string();
		const string arg_type = i.get_object_element("type").get_string();
		const auto init_expr = i.get_optional_object_element("expr");
		QUARK_ASSERT(arg_type[0] == '$');

		const auto arg_type2 = resolve_type123(arg_type, temp_type_defs);

		if(init_expr){
			if(convert_expressions){
				const auto init_expr2 = conv_expression(init_expr, temp_type_defs);

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
std::shared_ptr<const scope_def_t> conv_scope_def__no_expressions(const json_value_t& scope_def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	const string type = scope_def.get_object_element("_type").get_string();
	const string name = scope_def.get_object_element("_name").get_string();
	const string return_type_id = scope_def.get_object_element("_return_type").get_string();
	const auto args = scope_def.get_optional_object_element("_args", json_value_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("_locals", json_value_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("_members", json_value_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("_statements").get_array();

	std::vector<member_t> args2 = conv_members(args, temp_type_defs, false);
	std::vector<member_t> local_variables2 = conv_members(local_variables, temp_type_defs, false);
	std::vector<member_t> members2 = conv_members(members, temp_type_defs, false);

	if(type == "function"){
		std::vector<std::shared_ptr<statement_t> > statements2;
/*
		for(const auto s: statements){
			const string op = s.get_array_n(0).get_string();
			if(op == "return"){
				QUARK_ASSERT(s.get_array_size() == 2);
				const auto expr = conv_expression(s.get_array_n(1), temp_type_defs);
				statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
			}
			else if(op == "bind"){
				QUARK_ASSERT(s.get_array_size() == 4);
				const auto bind_type = resolve_type123(s.get_array_n(1).get_string(), temp_type_defs);
				const auto identifier = s.get_array_n(2).get_string();
				const auto expr = conv_expression(s.get_array_n(3), temp_type_defs);
				statements2.push_back(make_shared<statement_t>(make__bind_statement(bind_type, identifier, expr)));
			}
			else{
				QUARK_ASSERT(false);
			}
		}
*/


//					constructor_def = store_object_member(constructor_def, "_function_type", "def-constructor");

		const auto function_type = scope_def.get_optional_object_element("_function_type");
		if(function_type && function_type.get_string() == "def-constructor"){
			return scope_def_t::make_builtin_function_def(
				type_identifier_t::make(name),
				scope_def_t::efunc_variant::k_default_constructor,
				resolve_type123(return_type_id, temp_type_defs)
			);
		}
		else{
			return scope_def_t::make_function_def(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id, temp_type_defs)
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
std::shared_ptr<const scope_def_t> conv_scope_def__expressions(const json_value_t& scope_def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	const string type = scope_def.get_object_element("_type").get_string();
	const string name = scope_def.get_object_element("_name").get_string();
	const string return_type_id = scope_def.get_object_element("_return_type").get_string();
	const auto args = scope_def.get_optional_object_element("_args", json_value_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("_locals", json_value_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("_members", json_value_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("_statements").get_array();

	std::vector<member_t> args2 = conv_members(args, temp_type_defs, true);
	std::vector<member_t> local_variables2 = conv_members(local_variables, temp_type_defs, true);
	std::vector<member_t> members2 = conv_members(members, temp_type_defs, true);

	if(type == "function"){
//???		QUARK_ASSERT(statements.size() > 0);
		std::vector<std::shared_ptr<statement_t> > statements2;
		for(const auto s: statements){
			const string op = s.get_array_n(0).get_string();
			if(op == "return"){
				QUARK_ASSERT(s.get_array_size() == 2);
				const auto expr = conv_expression(s.get_array_n(1), temp_type_defs);
				statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
			}
			else if(op == "bind"){
				QUARK_ASSERT(s.get_array_size() == 4);
				const auto bind_type = resolve_type123(s.get_array_n(1).get_string(), temp_type_defs);
				const auto identifier = s.get_array_n(2).get_string();
				const auto expr = conv_expression(s.get_array_n(3), temp_type_defs);
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
				resolve_type123(return_type_id, temp_type_defs)
			);
		}
		else{
			return scope_def_t::make_function_def(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id, temp_type_defs)
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

std::shared_ptr<type_def_t> conv_type_def__no_expressions(const json_value_t& def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	QUARK_ASSERT(def.check_invariant());

	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");
	const auto path = def.get_object_element("path");

	if(base_type == "null"){
		return make_shared<type_def_t>(type_def_t::make(base_type::k_null));
	}
	else if(base_type == "bool"){
		return make_shared<type_def_t>(type_def_t::make_bool());
	}
	else if(base_type == "int"){
		return make_shared<type_def_t>(type_def_t::make_int());
	}
	else if(base_type == "string"){
		return make_shared<type_def_t>(type_def_t::make(base_type::k_string));
	}
	else if(base_type == "struct"){
		return make_shared<type_def_t>(type_def_t::make_struct_def(conv_scope_def__no_expressions(scope_def, temp_type_defs)));
	}
	else if(base_type == "vector"){
		QUARK_ASSERT(false);
	}
	else if(base_type == "function"){
		return make_shared<type_def_t>(type_def_t::make_function_def(conv_scope_def__no_expressions(scope_def, temp_type_defs)));
	}
	else if(base_type == "subscope"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::shared_ptr<type_def_t> conv_expressions_and_statements(const json_value_t& def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	QUARK_ASSERT(def.check_invariant());

	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");
	const auto path = def.get_object_element("path");

	if(base_type == "struct"){
		return make_shared<type_def_t>(type_def_t::make_struct_def(conv_scope_def__expressions(scope_def, temp_type_defs)));
	}
	else if(base_type == "function"){
		return make_shared<type_def_t>(type_def_t::make_function_def(conv_scope_def__expressions(scope_def, temp_type_defs)));
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
		temp_type_defs[s.first] = make_shared<type_def_t>(type_def_t());
	}

	//	Make a shallow pass through all types -- no expressions.
	for(const auto& s: program.get_object_element("lookup").get_object()){
		const auto id = s.first;
		const json_value_t& def = s.second;
		std::shared_ptr<type_def_t> type_def = conv_type_def__no_expressions(def, temp_type_defs);
		shared_ptr<type_def_t> identity_type_def = temp_type_defs.at(id);

		//	Update our identify type defs.
		identity_type_def->swap(*type_def.get());
	}

	//	Second pass - do expressions.
	for(const auto& s: program.get_object_element("lookup").get_object()){
		const auto id = s.first;
		const json_value_t& def = s.second;
		std::shared_ptr<type_def_t> type_def = conv_expressions_and_statements(def, temp_type_defs);
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
