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


bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
	return true
		&& lhs._location == rhs._location
		&& lhs._definition_name == rhs._definition_name
		&& lhs._function_type == rhs._function_type
		&& lhs._args == rhs._args
		&& compare_shared_values(lhs._body, rhs._body)
		&& lhs._host_function_id == rhs._host_function_id
		;
}

const typeid_t& get_function_type(const function_definition_t& f){
	return f._function_type;
}

json_t function_def_to_ast_json(const function_definition_t& v) {
	typeid_t function_type = get_function_type(v);

	return std::vector<json_t>{
		typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state),
		v._definition_name,
		members_to_json(v._args),

		v._body ? body_to_json(*v._body) : json_t(),

		json_t(v._host_function_id)
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

	return function_definition_t{
		location1,
		definition_name1,
		function_type1,
		args1,
		body1,
		static_cast<int>(host_function_id0.get_number())
	};
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
	if(_body && _body->check_types_resolved() == false){
		return false;
	}
	return true;
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
			return true;
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
			return true;
		}
		bool operator()(const function_definition_expr_t& e) const{
			return true;
		}
		bool operator()(const load_t& e) const{
			return true;
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

std::string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
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
