//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expression.h"

#include "statement.h"
namespace floyd {

using namespace std;

	ast_json_t expressions_to_json(const std::vector<expression_t> v);



//////////////////////////////////////////////////		function_definition_t


bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
	return true
		&& lhs._function_type == rhs._function_type
		&& lhs._args == rhs._args
		&& compare_shared_values(lhs._body, rhs._body)
		&& lhs._host_function_id == rhs._host_function_id
		;
}

const typeid_t& get_function_type(const function_definition_t& f){
	return f._function_type;
}

ast_json_t function_def_to_ast_json(const function_definition_t& v) {
	typeid_t function_type = get_function_type(v);
	return ast_json_t{json_t::make_array({
		"func-def",
		typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state)._value,
		members_to_json(v._args),
		/***v._body ? body_to_json(*v._body)._value :***/ json_t(),

		json_t(v._host_function_id),

		//	Duplicate info -- we have convered this using function_type above.
		typeid_to_ast_json(v._function_type.get_function_return(), json_tags::k_tag_resolve_state)._value
	})};
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




////////////////////////////////////////////		JSON SUPPORT


QUARK_UNIT_TESTQ("expression_to_json()", "literals"){
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_int(13)), R"(["k", 13, "^int"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_string("xyz")), R"(["k", "xyz", "^string"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_double(14.0f)), R"(["k", 14, "^double"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(true)), R"(["k", true, "^bool"])");
	quark::ut_compare_strings(expression_to_json_string(expression_t::make_literal_bool(false)), R"(["k", false, "^bool"])");
}

QUARK_UNIT_TESTQ("expression_to_json()", "math2"){
	quark::ut_compare_strings(
		expression_to_json_string(
			expression_t::make_simple_expression__2(
				expression_type::k_arithmetic_add__2, expression_t::make_literal_int(2), expression_t::make_literal_int(3), nullptr)
			),
		R"(["+", ["k", 2, "^int"], ["k", 3, "^int"]])"
	);
}

QUARK_UNIT_TESTQ("expression_to_json()", "call"){
	quark::ut_compare_strings(
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

QUARK_UNIT_TESTQ("expression_to_json()", "lookup"){
	quark::ut_compare_strings(
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



ast_json_t expression_to_json_internal(const expression_t& e){
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

ast_json_t expressions_to_json(const std::vector<expression_t> v){
	std::vector<json_t> r;
	for(const auto e: v){
		r.push_back(expression_to_json(e)._value);
	}
	return ast_json_t{json_t::make_array(r)};
}

ast_json_t expression_to_json(const expression_t& e){
	const auto a = expression_to_json_internal(e);

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

std::string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json._value);
}


}	//	floyd
