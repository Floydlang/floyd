//
//  expressions.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expression.h"

#include "statement.h"
#include "ast_helpers.h"

#include "text_parser.h"


namespace floyd {



json_t expressions_to_json(const std::vector<expression_t> v);



//////////////////////////////////////////////////		expression_type




//	WARNING: Make sure all accessed constants have already been initialized!
static std::map<expression_type, std::string> k_expression_to_opcode = {
	{ expression_type::k_arithmetic_add, expression_opcode_t::k_arithmetic_add },
	{ expression_type::k_arithmetic_subtract, expression_opcode_t::k_arithmetic_subtract },
	{ expression_type::k_arithmetic_multiply, expression_opcode_t::k_arithmetic_multiply },
	{ expression_type::k_arithmetic_divide, expression_opcode_t::k_arithmetic_divide },
	{ expression_type::k_arithmetic_remainder, expression_opcode_t::k_arithmetic_remainder },

	{ expression_type::k_logical_and, expression_opcode_t::k_logical_and },
	{ expression_type::k_logical_or, expression_opcode_t::k_logical_or },

	{ expression_type::k_comparison_smaller_or_equal, expression_opcode_t::k_comparison_smaller_or_equal },
	{ expression_type::k_comparison_smaller, expression_opcode_t::k_comparison_smaller },
	{ expression_type::k_comparison_larger_or_equal, expression_opcode_t::k_comparison_larger_or_equal },
	{ expression_type::k_comparison_larger, expression_opcode_t::k_comparison_larger },

	{ expression_type::k_logical_equal, expression_opcode_t::k_logical_equal },
	{ expression_type::k_logical_nonequal, expression_opcode_t::k_logical_nonequal },


	{ expression_type::k_literal, expression_opcode_t::k_literal },

	{ expression_type::k_arithmetic_unary_minus, expression_opcode_t::k_unary_minus },

	{ expression_type::k_conditional_operator, expression_opcode_t::k_conditional_operator },
	{ expression_type::k_call, expression_opcode_t::k_call },

	{ expression_type::k_load, expression_opcode_t::k_load },
	{ expression_type::k_load2, expression_opcode_t::k_load2 },
	{ expression_type::k_resolve_member, expression_opcode_t::k_resolve_member },
	{ expression_type::k_update_member, expression_opcode_t::k_update_member },

	{ expression_type::k_lookup_element, expression_opcode_t::k_lookup_element },

	{ expression_type::k_struct_def, expression_opcode_t::k_struct_def },
	{ expression_type::k_function_def, expression_opcode_t::k_function_def },
	{ expression_type::k_value_constructor, expression_opcode_t::k_value_constructor },

	{ expression_type::k_benchmark, expression_opcode_t::k_benchmark }
};




static std::map<std::string, expression_type> make_reverse(const std::map<expression_type, std::string>& m){
	std::map<std::string, expression_type> temp;
	for(const auto& e: m){
		temp[e.second] = e.first;
	}
	return temp;
}

static std::map<std::string, expression_type> string_to_operation_lookup = make_reverse(k_expression_to_opcode);

std::string expression_type_to_opcode(const expression_type& op){
	const auto r = k_expression_to_opcode.find(op);
	QUARK_ASSERT(r != k_expression_to_opcode.end());
	return r->second;
}

expression_type opcode_to_expression_type(const std::string& op){
	const auto r = string_to_operation_lookup.find(op);
	QUARK_ASSERT(r != string_to_operation_lookup.end());
	return r->second;
}




bool is_arithmetic_expression(expression_type op){
	return false
		|| op == expression_type::k_arithmetic_add
		|| op == expression_type::k_arithmetic_subtract
		|| op == expression_type::k_arithmetic_multiply
		|| op == expression_type::k_arithmetic_divide
		|| op == expression_type::k_arithmetic_remainder

		|| op == expression_type::k_logical_and
		|| op == expression_type::k_logical_or
		;
}

bool is_comparison_expression(expression_type op){
	return false
		|| op == expression_type::k_comparison_smaller_or_equal
		|| op == expression_type::k_comparison_smaller
		|| op == expression_type::k_comparison_larger_or_equal
		|| op == expression_type::k_comparison_larger

		|| op == expression_type::k_logical_equal
		|| op == expression_type::k_logical_nonequal
		;
}

//	Opcode = as in the AST.
expression_type opcode_to_expression_type(const std::string& op);
std::string expression_type_to_opcode(const expression_type& op);


bool is_opcode_arithmetic_expression(const std::string& op){
	return false
		|| op == expression_opcode_t::k_arithmetic_add
		|| op == expression_opcode_t::k_arithmetic_subtract
		|| op == expression_opcode_t::k_arithmetic_multiply
		|| op == expression_opcode_t::k_arithmetic_divide
		|| op == expression_opcode_t::k_arithmetic_remainder

		|| op == expression_opcode_t::k_logical_and
		|| op == expression_opcode_t::k_logical_or
		;
}

bool is_opcode_comparison_expression(const std::string& op){
	return false
		|| op == expression_opcode_t::k_comparison_smaller_or_equal
		|| op == expression_opcode_t::k_comparison_smaller
		|| op == expression_opcode_t::k_comparison_larger_or_equal
		|| op == expression_opcode_t::k_comparison_larger

		|| op == expression_opcode_t::k_logical_equal
		|| op == expression_opcode_t::k_logical_nonequal
		;
}





//////////////////////////////////////////////////		function_definition_t



/*
bool function_definition_t::floyd_func_t::operator==(const floyd_func_t& other) const{
	return compare_shared_values(_body, other._body);
}
*/

bool function_definition_t::check_invariant() const {
	QUARK_ASSERT(_function_type.is_function());
	QUARK_ASSERT(_function_type.get_function_args().size() == _named_args.size());

	const auto args0 = _function_type.get_function_args();
	for(int i = 0 ; i < args0.size(); i++){
		QUARK_ASSERT(args0[i] == _named_args[i]._type);
	}

	if(_optional_body){
		QUARK_ASSERT(_optional_body->check_invariant());
	}

	return true;
}

bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
	return true
		&& lhs._location == rhs._location
		&& lhs._definition_name == rhs._definition_name
		&& lhs._function_type == rhs._function_type
		&& lhs._named_args == rhs._named_args
		&& compare_shared_values(lhs._optional_body, rhs._optional_body)
		;
}




const typeid_t& get_function_type(const function_definition_t& f){
	return f._function_type;
}

json_t function_def_to_ast_json(const function_definition_t& v) {
	typeid_t function_type = get_function_type(v);

	auto result = std::vector<json_t>{
		typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state),
		v._definition_name,
		members_to_json(v._named_args)
	};
	if(v._optional_body){
		result.push_back(body_to_json(*v._optional_body));
	}
	else{
		result.push_back(json_t());
	}
	return result;
}

function_definition_t json_to_function_def(const json_t& p){
	const auto function_type0 = p.get_array_n(0);
	const auto definition_name0 = p.get_array_n(1);
	const auto args0 = p.get_array_n(2);
	const auto body0 = p.get_array_n(3);

	const location_t location1 = k_no_location;
	const std::string definition_name1 = definition_name0.get_string();
	const typeid_t function_type1 = typeid_from_ast_json(function_type0);
	const std::vector<member_t> args1 = members_from_json(args0);
	const std::shared_ptr<body_t> body1 = body0.is_null() ? std::shared_ptr<body_t>() : std::make_shared<body_t>(json_to_body(body0));

	return function_definition_t::make_func(
		location1,
		definition_name1,
		function_type1,
		args1,
		body1
	);
}






QUARK_UNIT_TEST("", "", "", ""){
	
	const auto a = function_definition_t::make_func(k_no_location, "definition_name", typeid_t::make_function(typeid_t::make_string(), {}, epure::pure), {}, std::make_shared<body_t>());
	QUARK_UT_VERIFY(a._named_args.empty());


	QUARK_UT_VERIFY(a == a);
}



////////////////////////////////////////////		expression_t





expression_t::expression_t(const expression_variant_t& contents, const std::shared_ptr<typeid_t>& output_type) :
#if DEBUG_DEEP
	_debug(""),
#endif
	location(k_no_location),
	_expression_variant(contents),
	_output_type(output_type)
{
#if DEBUG_DEEP
	_debug = expression_to_json_string(*this);
#endif
}





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
				expression_type::k_arithmetic_add, expression_t::make_literal_int(2), expression_t::make_literal_int(3), nullptr)
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

json_t expression_to_json(const expression_t& e){
	struct visitor_t {
		const expression_t& expr;

		json_t operator()(const expression_t::literal_exp_t& e) const{
			return make_ast_node(
				floyd::k_no_location,
				expression_opcode_t::k_literal,
				{
					value_to_ast_json(e.value, json_tags::k_tag_resolve_state),
					typeid_to_ast_json(e.value.get_type(), json_tags::k_tag_resolve_state)
				}
			);
		}
		json_t operator()(const expression_t::arithmetic_t& e) const{
			return make_ast_node(
				expr.location,
				expression_type_to_opcode(e.op),
				{
					expression_to_json(*e.lhs),
					expression_to_json(*e.rhs)
				}
			);
		}
		json_t operator()(const expression_t::comparison_t& e) const{
			return make_ast_node(
				expr.location,
				expression_type_to_opcode(e.op),
				{
					expression_to_json(*e.lhs),
					expression_to_json(*e.rhs)
				}
			);
		}
		json_t operator()(const expression_t::unary_minus_t& e) const{
			return make_ast_node(floyd::k_no_location, expression_opcode_t::k_unary_minus, { expression_to_json(*e.expr) } );
		}
		json_t operator()(const expression_t::conditional_t& e) const{
			const auto a = make_ast_node(
				floyd::k_no_location,
				expression_opcode_t::k_conditional_operator,
				{
					expression_to_json(*e.condition),
					expression_to_json(*e.a),
					expression_to_json(*e.b)
				}
			);
			return a;
		}

		json_t operator()(const expression_t::call_t& e) const{
			std::vector<json_t> args;
			for(const auto& m: e.args){
				args.push_back(expression_to_json(m));
			}
			return make_ast_node(floyd::k_no_location, expression_opcode_t::k_call, { expression_to_json(*e.callee), json_t::make_array(args) } );
		}
		json_t operator()(const expression_t::intrinsic_t& e) const{
			std::vector<json_t> args;
			for(const auto& m: e.args){
				args.push_back(expression_to_json(m));
			}
			return make_ast_node(floyd::k_no_location, expression_opcode_t::k_intrinsic, { e.call_name, args } );
		}


		json_t operator()(const expression_t::struct_definition_expr_t& e) const{
			return make_ast_node(expr.location, expression_opcode_t::k_struct_def, { e.name, struct_definition_to_ast_json(*e.def) } );
		}
		json_t operator()(const expression_t::function_definition_expr_t& e) const{
			return make_ast_node(
				e.def._location,
				expression_opcode_t::k_function_def,
				function_def_to_ast_json(e.def).get_array()
			);
		}
		json_t operator()(const expression_t::load_t& e) const{
			return make_ast_node(expr.location, expression_opcode_t::k_load, { json_t(e.variable_name) } );
		}
		json_t operator()(const expression_t::load2_t& e) const{
			return make_ast_node(
				expr.location,
				expression_opcode_t::k_load2,
				{ json_t(e.address._parent_steps), json_t(e.address._index) }
			);
		}

		json_t operator()(const expression_t::resolve_member_t& e) const{
			return make_ast_node(
				expr.location,
				expression_opcode_t::k_resolve_member,
				{ expression_to_json(*e.parent_address), json_t(e.member_name) }
			);
		}
		json_t operator()(const expression_t::update_member_t& e) const{
			return make_ast_node(
				expr.location,
				expression_opcode_t::k_update_member,
				{
					expression_to_json(*e.parent_address),
					json_t(e.member_index),
					expression_to_json(*e.new_value)
				}
			);
		}
		json_t operator()(const expression_t::lookup_t& e) const{
			return make_ast_node(
				expr.location,
				expression_opcode_t::k_lookup_element,
				{
					expression_to_json(*e.parent_address),
					expression_to_json(*e.lookup_key)
				}
			);
		}
		json_t operator()(const expression_t::value_constructor_t& e) const{
			return make_ast_node(
				expr.location,
				expression_opcode_t::k_value_constructor,
				{
					typeid_to_ast_json(e.value_type, json_tags::k_tag_resolve_state),
					expressions_to_json(e.elements)
				}
			);
		}
		json_t operator()(const expression_t::benchmark_expr_t& e) const{
			return make_ast_node(expr.location, expression_opcode_t::k_benchmark, { body_to_json(*e.body) } );
		}
	};

	const json_t result0 = std::visit(visitor_t{e}, e._expression_variant);

	//	Add annotated-type element to json?
	if(e.is_annotated_shallow() && e.has_builtin_type() == false){
		const auto t = e.get_output_type();
		auto a2 = result0.get_array();
		const auto type_json = typeid_to_ast_json(t, json_tags::k_tag_resolve_state);
		a2.push_back(type_json);
		return json_t::make_array(a2);
	}
	else{
		return result0;
	}
}

json_t expressions_to_json(const std::vector<expression_t> v){
	std::vector<json_t> r;
	for(const auto& e: v){
		r.push_back(expression_to_json(e));
	}
	return json_t::make_array(r);
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
expression_t ast_json_to_expression(const json_t& e){
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


		const auto value2 = ast_json_to_value(type2, value);
		return expression_t::make_literal(value2);
	}
	else if(op == expression_opcode_t::k_unary_minus){
		QUARK_ASSERT(e.get_array_size() == 2 || e.get_array_size() == 3);

		const auto expr = ast_json_to_expression(e.get_array_n(1));
		const auto annotated_type = get_optional_typeid(e, 2);
		return expression_t::make_unary_minus(expr, annotated_type);
	}
	else if(is_opcode_arithmetic_expression(op)){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto op2 = opcode_to_expression_type(op);
		const auto lhs_expr = ast_json_to_expression(e.get_array_n(1));
		const auto rhs_expr = ast_json_to_expression(e.get_array_n(2));
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_arithmetic(op2, lhs_expr, rhs_expr, annotated_type);
	}
	else if(is_opcode_comparison_expression(op)){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto op2 = opcode_to_expression_type(op);
		const auto lhs_expr = ast_json_to_expression(e.get_array_n(1));
		const auto rhs_expr = ast_json_to_expression(e.get_array_n(2));
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_comparison(op2, lhs_expr, rhs_expr, annotated_type);
	}
	else if(op == expression_opcode_t::k_conditional_operator){
		QUARK_ASSERT(e.get_array_size() == 4 || e.get_array_size() == 5);

		const auto condition_expr = ast_json_to_expression(e.get_array_n(1));
		const auto a_expr = ast_json_to_expression(e.get_array_n(2));
		const auto b_expr = ast_json_to_expression(e.get_array_n(3));
		const auto annotated_type = get_optional_typeid(e, 4);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr, annotated_type);
	}
	else if(op == expression_opcode_t::k_call){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto function_expr = ast_json_to_expression(e.get_array_n(1));
		const auto args = e.get_array_n(2);
		std::vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(ast_json_to_expression(arg));
		}

		const auto annotated_type = get_optional_typeid(e, 3);

		return expression_t::make_call(function_expr, args2, annotated_type);
	}
	else if(op == expression_opcode_t::k_intrinsic){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto function_name = e.get_array_n(1).get_string();
		const auto args = e.get_array_n(2);
		std::vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(ast_json_to_expression(arg));
		}

		const auto annotated_type = get_optional_typeid(e, 3);

		return expression_t::make_intrinsic(function_name, args2, annotated_type);
	}
	else if(op == expression_opcode_t::k_resolve_member){
		QUARK_ASSERT(e.get_array_size() == 3 || e.get_array_size() == 4);

		const auto base_expr = ast_json_to_expression(e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_resolve_member(base_expr, member, annotated_type);
	}
	else if(op == expression_opcode_t::k_update_member){
		QUARK_ASSERT(e.get_array_size() == 4 || e.get_array_size() == 5);

		const auto base_expr = ast_json_to_expression(e.get_array_n(1));
		const auto member_index = e.get_array_n(2).get_number();
		const auto new_value_expr = ast_json_to_expression(e.get_array_n(3));
		const auto annotated_type = get_optional_typeid(e, 4);
		return expression_t::make_update_member(base_expr, (int)member_index, new_value_expr, annotated_type);
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

		const auto parent_address_expr = ast_json_to_expression(e.get_array_n(1));
		const auto lookup_key_expr = ast_json_to_expression(e.get_array_n(2));
		const auto annotated_type = get_optional_typeid(e, 3);
		return expression_t::make_lookup(parent_address_expr, lookup_key_expr, annotated_type);
	}
	else if(op == expression_opcode_t::k_struct_def){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto struct_name = e.get_array_n(1).get_string();
		const auto members = members_from_json(e.get_array_n(2));
		return expression_t::make_struct_definition(struct_name, std::make_shared<struct_definition_t>(members));
	}
	else if(op == expression_opcode_t::k_function_def){
		QUARK_ASSERT(e.get_array_size() == 5);

		const auto function_type = typeid_from_ast_json(e.get_array_n(1));
		const auto function_name = e.get_array_n(2).get_string();
		const auto named_args = members_from_json(e.get_array_n(3));

		//	Null or BODY as an object. Null: this is a declaration only.
		const auto body0 = e.get_array_n(4);

		const std::shared_ptr<body_t> body1 = body0.is_null() ? std::shared_ptr<body_t>() : std::make_shared<body_t>(json_to_body(body0));

		auto def = function_definition_t::make_func(
			k_no_location,
			function_name,
			function_type,
			named_args,
			body1
		);
		return expression_t::make_function_definition(def);
	}
	else if(op == expression_opcode_t::k_value_constructor){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value_type = typeid_from_ast_json(e.get_array_n(1));
		const auto args = e.get_array_n(2).get_array();

		std::vector<expression_t> args2;
		for(const auto& m: args){
			args2.push_back(ast_json_to_expression(m));
		}

		return expression_t::make_construct_value_expr(value_type, args2);
	}
	else if(op == expression_opcode_t::k_benchmark){
		QUARK_ASSERT(e.get_array_size() == 2);

		//??? should use json_to_body()!?
		const auto body_statements2 = ast_json_to_statements(e.get_array_n(1));
		const auto body = body_t{ body_statements2 };
//		const auto body_statements2 = json_to_body(e.get_array_n(1));
		return expression_t::make_benchmark_expr(body);
	}
	else{
		quark::throw_exception();
	}
}




std::string expression_to_json_string(const expression_t& e){
	const auto json = expression_to_json(e);
	return json_to_compact_string(json);
}






expression_type get_expression_type(const expression_t& e){
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
			return expression_type::k_arithmetic_unary_minus;
		}
		expression_type operator()(const expression_t::conditional_t& e) const{
			return expression_type::k_conditional_operator;
		}

		expression_type operator()(const expression_t::call_t& e) const{
			return expression_type::k_call;
		}
		expression_type operator()(const expression_t::intrinsic_t& e) const{
			return expression_type::k_intrinsic;
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
		expression_type operator()(const expression_t::update_member_t& e) const{
			return expression_type::k_update_member;
		}
		expression_type operator()(const expression_t::lookup_t& e) const{
			return expression_type::k_lookup_element;
		}
		expression_type operator()(const expression_t::value_constructor_t& e) const{
			return expression_type::k_value_constructor;
		}
		expression_type operator()(const expression_t::benchmark_expr_t& e) const{
			return expression_type::k_benchmark;
		}
	};
	return std::visit(visitor_t{}, e._expression_variant);
}



QUARK_UNIT_TEST("", "", "", ""){
	const auto e = expression_t::make_literal(value_t::make_string("hello"));

	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(
		std::get<expression_t::literal_exp_t>(e._expression_variant).value.get_string_value() == "hello"
	);

	const auto copy = e;

	auto b = expression_t::make_literal(value_t::make_string("yes"));
	b = copy;
}







QUARK_UNIT_TEST("", "", "", ""){
	const floyd::variable_address_t dummy;
	const auto copy(dummy);
	floyd::variable_address_t b;
	b = dummy;
	(void)copy;
}
}	//	floyd
