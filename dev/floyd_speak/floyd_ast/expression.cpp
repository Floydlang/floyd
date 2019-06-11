//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expression.h"

#include "statement.h"
#include "ast_typeid_helpers.h"
#include "parser_primitives.h"

#include "ast.h"

namespace floyd {

using namespace std;

	json_t expressions_to_json(const std::vector<expression_t> v);



//////////////////////////////////////////////////		function_definition_t

bool function_definition_t::check_invariant() const {
	QUARK_ASSERT(_function_type.is_function());
	QUARK_ASSERT(_function_type.get_function_args().size() == _args.size());

	const auto args0 = _function_type.get_function_args();
	for(int i = 0 ; i < args0.size(); i++){
		QUARK_ASSERT(args0[i] == _args[i]._type);
	}

	struct visitor_t {
		bool operator()(const empty_t& e) const{
			return true;
		}
		bool operator()(const floyd_func_t& e) const{
			QUARK_ASSERT(e._body->check_invariant());
			return true;
		}
		bool operator()(const host_func_t& e) const{
			return true;
		}
	};
	bool result3 = std::visit(visitor_t{}, _contents);
	QUARK_ASSERT(result3);

	return true;
}

bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
	return true
		&& lhs._location == rhs._location
		&& lhs._definition_name == rhs._definition_name
		&& lhs._function_type == rhs._function_type
		&& lhs._args == rhs._args
		&& lhs._contents == rhs._contents
		;
}

const typeid_t& get_function_type(const function_definition_t& f){
	return f._function_type;
}

json_t function_def_to_ast_json(const function_definition_t& v) {
	typeid_t function_type = get_function_type(v);

	auto floyd_func = std::get_if<function_definition_t::floyd_func_t>(&v._contents);
	auto host_func = std::get_if<function_definition_t::host_func_t>(&v._contents);


	return std::vector<json_t>{
		typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state),
		v._definition_name,
		members_to_json(v._args),

		floyd_func ? body_to_json(*floyd_func->_body) : json_t(),

		host_func ? json_t(host_func->_host_function_id) : json_t(0)
	};
}


function_definition_t json_to_function_def(const json_t& p){
	const auto function_type0 = p.get_array_n(0);
	const auto definition_name0 = p.get_array_n(1);
	const auto args0 = p.get_array_n(2);
	const auto body0 = p.get_array_n(3);
	const auto host_function_id0 = p.get_array_n(4);

	const location_t location1 = k_no_location;
	const std::string definition_name1 = definition_name0.get_string();
	const typeid_t function_type1 = typeid_from_ast_json(function_type0);
	const std::vector<member_t> args1 = members_from_json(args0);
	const std::shared_ptr<body_t> body1 = body0.is_null() ? std::shared_ptr<body_t>() : std::make_shared<body_t>(json_to_body(body0));

	if(body1){
		return function_definition_t::make_floyd_func(
			location1,
			definition_name1,
			function_type1,
			args1,
			body1
		);
	}
	else{
		return function_definition_t::make_host_func(
			location1,
			definition_name1,
			function_type1,
			args1,
			static_cast<int>(host_function_id0.get_number())
		);
	}
}

bool function_definition_t::check_types_resolved() const{
	bool result = _function_type.check_types_resolved();
	if(result == false){
		return false;
	}

	for(const auto& e: _args){
		bool result2 = e._type.check_types_resolved();
		if(result2 == false){
			return false;
		}
	}

	struct visitor_t {
		bool operator()(const empty_t& e) const{
			return true;
		}
		bool operator()(const floyd_func_t& e) const{
			return e._body->check_types_resolved();
		}
		bool operator()(const host_func_t& e) const{
			return true;
		}
	};
	bool result3 = std::visit(visitor_t{}, _contents);
	if(result3 == false){
		return false;
	}
	return true;
}

bool function_definition_t::floyd_func_t::operator==(const floyd_func_t& other) const{
	return compare_shared_values(_body, other._body);
}



QUARK_UNIT_TEST("", "", "", ""){
	
	const auto a = function_definition_t::make_floyd_func(k_no_location, "definition_name", typeid_t::make_function(typeid_t::make_string(), {}, epure::pure), {}, std::make_shared<body_t>());
	QUARK_UT_VERIFY(a._args.empty());


	QUARK_UT_VERIFY(a == a);
}



////////////////////////////////////////////		expression_t





//??? Change this to a free function
bool expression_t::check_types_resolved(const std::vector<expression_t>& expressions){
	for(const auto& e: expressions){
		if(e.check_types_resolved() == false){
			return false;
		}
	}
	return true;
}


//??? Change this to a free function
bool expression_t::check_types_resolved() const{
	QUARK_ASSERT(check_invariant());

	if(_output_type == nullptr || _output_type->check_types_resolved() == false){
		return false;
	}

	struct visitor_t {
		bool operator()(const literal_exp_t& e) const{
			return e.value.get_type().check_types_resolved();
		}
		bool operator()(const arithmetic_t& e) const{
			return e.lhs->check_types_resolved() && e.rhs->check_types_resolved();
		}
		bool operator()(const comparison_t& e) const{
			return e.lhs->check_types_resolved() && e.rhs->check_types_resolved();
		}
		bool operator()(const unary_minus_t& e) const{
			return e.expr->check_types_resolved();
		}
		bool operator()(const conditional_t& e) const{
			return e.condition->check_types_resolved() && e.a->check_types_resolved() && e.b->check_types_resolved();
		}

		bool operator()(const call_t& e) const{
			return e.callee->check_types_resolved() && check_types_resolved(e.args);
		}


		bool operator()(const struct_definition_expr_t& e) const{
			return e.def->check_types_resolved();
		}
		bool operator()(const function_definition_expr_t& e) const{
			return e.def->check_types_resolved();
		}
		bool operator()(const load_t& e) const{
			return false;
		}
		bool operator()(const load2_t& e) const{
			return true;
		}

		bool operator()(const resolve_member_t& e) const{
			return e.parent_address->check_types_resolved();
		}
		bool operator()(const lookup_t& e) const{
			return e.parent_address->check_types_resolved() && e.lookup_key->check_types_resolved();
		}
		bool operator()(const value_constructor_t& e) const{
			return check_types_resolved(e.elements);
		}
	};

	bool result = std::visit(visitor_t{}, _contents);
	return result;
}



////////////////////////////////////////////		JSON SUPPORT



QUARK_UNIT_TEST("expression_t", "expression_to_json()", "literals", ""){
	ut_verify(QUARK_POS, expression_to_json_string(expression_t::make_literal_int(13)), R"(["k", 13, "^int"])");
}
QUARK_UNIT_TEST("expression", "expression_to_json()", "literals", ""){
	ut_verify(QUARK_POS, expression_to_json_string(expression_t::make_literal_string("xyz")), R"(["k", "xyz", "^string"])");
}
QUARK_UNIT_TEST("expression", "expression_to_json()", "literals", ""){
	ut_verify(QUARK_POS, expression_to_json_string(expression_t::make_literal_double(14.0f)), R"(["k", 14, "^double"])");
}
QUARK_UNIT_TEST("expression", "expression_to_json()", "literals", ""){
	ut_verify(QUARK_POS, expression_to_json_string(expression_t::make_literal_bool(true)), R"(["k", true, "^bool"])");
}
QUARK_UNIT_TEST("expression", "expression_to_json()", "literals", ""){
	ut_verify(QUARK_POS, expression_to_json_string(expression_t::make_literal_bool(false)), R"(["k", false, "^bool"])");
}

QUARK_UNIT_TEST("expression_t", "expression_to_json()", "math2", ""){
	ut_verify(
		QUARK_POS,
		expression_to_json_string(
			expression_t::make_arithmetic(
				expression_type::k_arithmetic_add__2, expression_t::make_literal_int(2), expression_t::make_literal_int(3), nullptr)
			),
		R"(["+", ["k", 2, "^int"], ["k", 3, "^int"]])"
	);
}

QUARK_UNIT_TEST("expression_t", "expression_to_json()", "call", ""){
	ut_verify(
		QUARK_POS,
		expression_to_json_string(
			expression_t::make_call(
				expression_t::make_load("my_func", nullptr),
				{
					expression_t::make_literal_string("xyz"),
					expression_t::make_literal_int(123)
				},
				nullptr
			)
		),
		R"(["call", ["@", "my_func"], [["k", "xyz", "^string"], ["k", 123, "^int"]]])"
	);
}

QUARK_UNIT_TEST("expression_t", "expression_to_json()", "lookup", ""){
	ut_verify(QUARK_POS, 
		expression_to_json_string(
			expression_t::make_lookup(
				expression_t::make_load("hello", nullptr),
				expression_t::make_literal_string("xyz"),
				nullptr
			)
		),
		R"(["[]", ["@", "hello"], ["k", "xyz", "^string"]])"
	);
}

json_t function_def_expression_to_ast_json(const function_definition_t& v) {
	typeid_t function_type = get_function_type(v);
	return make_expression_n(
		v._location,
		expression_opcode_t::k_function_def,
		function_def_to_ast_json(v).get_array()
	);
}

json_t expression_to_json_internal(const expression_t& e){
	struct visitor_t {
		const expression_t& expr;

		json_t operator()(const expression_t::literal_exp_t& e) const{
			return maker__make_constant(e.value);
		}
		json_t operator()(const expression_t::arithmetic_t& e) const{
			return make_expression2(
				expr.location,
				expression_type_to_token(e.op),
				expression_to_json(*e.lhs),
				expression_to_json(*e.rhs)
			);
		}
		json_t operator()(const expression_t::comparison_t& e) const{
			return make_expression2(
				expr.location,
				expression_type_to_token(e.op),
				expression_to_json(*e.lhs),
				expression_to_json(*e.rhs)
			);
		}
		json_t operator()(const expression_t::unary_minus_t& e) const{
			return maker__make_unary_minus(expression_to_json(*e.expr));
		}
		json_t operator()(const expression_t::conditional_t& e) const{
			const auto a = maker__make_conditional_operator(
				expression_to_json(*e.condition),
				expression_to_json(*e.a),
				expression_to_json(*e.b)
			);
			return a;
		}

		json_t operator()(const expression_t::call_t& e) const{
			vector<json_t> args;
			for(const auto& m: e.args){
				args.push_back(expression_to_json(m));
			}
			return maker__call(expression_to_json(*e.callee), args);
		}


		json_t operator()(const expression_t::struct_definition_expr_t& e) const{
			return make_expression1(expr.location, expression_opcode_t::k_struct_def, struct_definition_to_ast_json(*e.def));
		}
		json_t operator()(const expression_t::function_definition_expr_t& e) const{
			return function_def_expression_to_ast_json(*e.def);
		}
		json_t operator()(const expression_t::load_t& e) const{
			return make_expression1(expr.location, expression_opcode_t::k_load, json_t(e.variable_name));
		}
		json_t operator()(const expression_t::load2_t& e) const{
			return make_expression2(expr.location, expression_opcode_t::k_load2, json_t(e.address._parent_steps), json_t(e.address._index));
		}

		json_t operator()(const expression_t::resolve_member_t& e) const{
			return make_expression2(expr.location, expression_opcode_t::k_resolve_member, expression_to_json(*e.parent_address), json_t(e.member_name));
		}
		json_t operator()(const expression_t::lookup_t& e) const{
			return make_expression2(
				expr.location,
				expression_opcode_t::k_lookup_element,
				expression_to_json(*e.parent_address),
				expression_to_json(*e.lookup_key)
			);
		}
		json_t operator()(const expression_t::value_constructor_t& e) const{
			return make_expression2(
				expr.location,
				expression_opcode_t::k_value_constructor,
				typeid_to_ast_json(e.value_type, json_tags::k_tag_resolve_state),
				expressions_to_json(e.elements)
			);
		}
	};

	const json_t result = std::visit(visitor_t{e}, e._contents);
	return result;
}

json_t expressions_to_json(const std::vector<expression_t> v){
	std::vector<json_t> r;
	for(const auto& e: v){
		r.push_back(expression_to_json(e));
	}
	return json_t::make_array(r);
}

json_t expression_to_json(const expression_t& e){
	const auto a = expression_to_json_internal(e);

	//	Add annotated-type element to json?
	if(e.is_annotated_shallow() && e.has_builtin_type() == false){
		const auto t = e.get_output_type();
		auto a2 = a.get_array();
		const auto type_json = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
		a2.push_back(type_json);
		return json_t::make_array(a2);
	}
	else{
		return a;
	}
}


std::shared_ptr<typeid_t> get_optional_typeid(const json_t& json_array, int optional_index){
	if(optional_index < json_array.get_array_size()){
		const auto e = json_array.get_array_n(optional_index);
		return std::make_shared<typeid_t>(typeid_from_ast_json(e));
	}
	else{
		return nullptr;
	}
}


//??? loses output-type for some expressions. Make it nonlossy!
expression_t astjson_to_expression(const json_t& e){
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_array_n(0).get_string();
	QUARK_ASSERT(op != "");
	if(op.empty()){
		quark::throw_exception();
	}

	if(op == expression_opcode_t::k_literal){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = typeid_from_ast_json(type);

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
			quark::throw_exception();
		}
	}
	else if(op == expression_opcode_t::k_unary_minus){
		QUARK_ASSERT(e.get_array_size() == 2 || e.get_array_size() == 3);

		const auto expr = astjson_to_expression(e.get_array_n(1));
		const auto annotated_type = get_optional_typeid(e, 2);
		return expression_t::make_unary_minus(expr, annotated_type);
	}
	else if(is_simple_expression__2(op)){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto op2 = token_to_expression_type(op);
		const auto lhs_expr = astjson_to_expression(e.get_array_n(1));
		const auto rhs_expr = astjson_to_expression(e.get_array_n(2));
		const auto annotated_type = get_optional_typeid(e, 3);
		if(is_arithmetic_expression(op2)){
			return expression_t::make_arithmetic(op2, lhs_expr, rhs_expr, annotated_type);
		}
		else if(is_comparison_expression(op2)){
			return expression_t::make_comparison(op2, lhs_expr, rhs_expr, annotated_type);
		}
		else{
			throw std::exception();
		}
	}
	else if(op == expression_opcode_t::k_conditional_operator){
		QUARK_ASSERT(e.get_array_size() == 4 || e.get_array_size() == 5);

		const auto condition_expr = astjson_to_expression(e.get_array_n(1));
		const auto a_expr = astjson_to_expression(e.get_array_n(2));
		const auto b_expr = astjson_to_expression(e.get_array_n(3));
		const auto annotated_type = get_optional_typeid(e, 4);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr, annotated_type);
	}
	else if(op == expression_opcode_t::k_call){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto function_expr = astjson_to_expression(e.get_array_n(1));
		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(astjson_to_expression(arg));
		}

		const auto annotated_type = get_optional_typeid(e, 3);

		return expression_t::make_call(function_expr, args2, annotated_type);
	}
	else if(op == expression_opcode_t::k_resolve_member){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto base_expr = astjson_to_expression(e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_resolve_member(base_expr, member, annotated_type);
	}
	else if(op == expression_opcode_t::k_load){
		QUARK_ASSERT(e.get_array_size() == 2 || e.get_array_size() == 3);

		const auto variable_symbol = e.get_array_n(1).get_string();
		const auto annotated_type = get_optional_typeid(e, 2);
		return expression_t::make_load(variable_symbol, annotated_type);
	}
	else if(op == expression_opcode_t::k_load2){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto parent_step = (int)e.get_array_n(1).get_number();
		const auto index = (int)e.get_array_n(2).get_number();
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_load2(variable_address_t::make_variable_address(parent_step, index), annotated_type);
	}
	else if(op == expression_opcode_t::k_lookup_element){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto parent_address_expr = astjson_to_expression(e.get_array_n(1));
		const auto lookup_key_expr = astjson_to_expression(e.get_array_n(2));
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_lookup(parent_address_expr, lookup_key_expr, annotated_type);
	}
	else if(op == expression_opcode_t::k_value_constructor){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value_type = typeid_from_ast_json(e.get_array_n(1));
		const auto args = e.get_array_n(2).get_array();

		std::vector<expression_t> args2;
		for(const auto& m: args){
			args2.push_back(astjson_to_expression(m));
		}

		return expression_t::make_construct_value_expr(value_type, args2);
	}
	else{
		quark::throw_exception();
	}
}




std::string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}






expression_type get_opcode(const expression_t& e){
	struct visitor_t {
		expression_type operator()(const expression_t::literal_exp_t& e) const{
			return expression_type::k_literal;
		}
		expression_type operator()(const expression_t::arithmetic_t& e) const{
			return e.op;
		}
		expression_type operator()(const expression_t::comparison_t& e) const{
			return e.op;
		}
		expression_type operator()(const expression_t::unary_minus_t& e) const{
			return expression_type::k_arithmetic_unary_minus__1;
		}
		expression_type operator()(const expression_t::conditional_t& e) const{
			return expression_type::k_conditional_operator3;
		}

		expression_type operator()(const expression_t::call_t& e) const{
			return expression_type::k_call;
		}


		expression_type operator()(const expression_t::struct_definition_expr_t& e) const{
			return expression_type::k_struct_def;
		}
		expression_type operator()(const expression_t::function_definition_expr_t& e) const{
			return expression_type::k_function_def;
		}
		expression_type operator()(const expression_t::load_t& e) const{
			return expression_type::k_load;
		}
		expression_type operator()(const expression_t::load2_t& e) const{
			return expression_type::k_load2;
		}

		expression_type operator()(const expression_t::resolve_member_t& e) const{
			return expression_type::k_resolve_member;
		}
		expression_type operator()(const expression_t::lookup_t& e) const{
			return expression_type::k_lookup_element;
		}
		expression_type operator()(const expression_t::value_constructor_t& e) const{
			return expression_type::k_value_constructor;
		}
	};
	return std::visit(visitor_t{}, e._contents);
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto e = expression_t::make_literal(value_t::make_string("hello"));

	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(
		std::get<expression_t::literal_exp_t>(e._contents).value.get_string_value() == "hello"
	);

	const auto copy = e;

	auto b = expression_t::make_literal(value_t::make_string("yes"));
	b = copy;
}


}	//	floyd
