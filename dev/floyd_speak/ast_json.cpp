//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "ast_json.h"

using std::string;
using std::vector;

#include "ast_value.h"
#include "text_parser.h"
#include "json_support.h"
#include "compiler_basics.h"

namespace floyd {







ast_json_t make_ast_entry(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		const auto r = json_t(e);
		return ast_json_t::make(r);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		const auto r = json_t(e);
		return ast_json_t::make(r);
	}
}


ast_json_t make_statement_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	return make_ast_entry(location, opcode, params);
}

ast_json_t make_statement1(const location_t& location, const std::string& opcode, const json_t& params){
	return make_statement_n(location, opcode, { params });
}
ast_json_t make_statement2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2){
	return make_statement_n(location, opcode, { param1, param2 });
}
ast_json_t make_statement3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3){
	return make_statement_n(location, opcode, { param1, param2, param3 });
}
ast_json_t make_statement4(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3, const json_t& param4){
	return make_statement_n(location, opcode, { param1, param2, param3, param4 });
}


QUARK_UNIT_TEST("", "make_statement_n()", "", ""){
	const auto r = make_statement_n(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify(QUARK_POS, r._value, json_t::make_array({ 1234.0, "def-struct" }));
}




ast_json_t make_expression_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	return make_ast_entry(location, opcode, params);
}

ast_json_t make_expression1(const location_t& location, const std::string& opcode, const json_t& param){
	return make_expression_n(location, opcode, { param });
}

ast_json_t make_expression2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2){
	return make_expression_n(location, opcode, { param1, param2 });
}
ast_json_t make_expression3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3){
	return make_expression_n(location, opcode, { param1, param2, param3 });
}





ast_json_t maker__make_identifier(const std::string& s){
	return make_expression1(floyd::k_no_location, expression_opcode_t::k_load, json_t(s));
}

ast_json_t maker__make_unary_minus(const json_t& expr){
	return make_expression1(floyd::k_no_location, expression_opcode_t::k_unary_minus, expr);
}

ast_json_t maker__make2(const std::string op, const json_t& lhs, const json_t& rhs){
	QUARK_ASSERT(op != "");
	return make_expression2(floyd::k_no_location, op, lhs, rhs);
}

ast_json_t maker__make_conditional_operator(const json_t& e1, const json_t& e2, const json_t& e3){
	return make_expression3(floyd::k_no_location, expression_opcode_t::k_conditional_operator, e1, e2, e3);
}

ast_json_t maker__call(const json_t& f, const std::vector<json_t>& args){
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_call, f, json_t::make_array(args));
}

ast_json_t maker_vector_definition(const std::string& element_type, const std::vector<json_t>& elements){
	QUARK_ASSERT(element_type == "");

//	const auto element_type2 = element_type.empty() ? typeid_t::make_undefined() : typeid_t::make_unresolved_type_identifier(element_type);
	const auto element_type2 = typeid_to_ast_json(typeid_t::make_vector(typeid_t::make_undefined()), json_tags::k_tag_resolve_state);
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_value_constructor, element_type2._value, json_t::make_array(elements));
}
ast_json_t maker_dict_definition(const std::string& value_type, const std::vector<json_t>& elements){
	QUARK_ASSERT(value_type == "");

//	const auto type = typeid_t::make_dict(typeid_t::make_unresolved_type_identifier(value_type));
	const auto element_type2 = typeid_to_ast_json(typeid_t::make_dict(typeid_t::make_undefined()), json_tags::k_tag_resolve_state);
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_value_constructor, element_type2._value, json_t::make_array(elements));

/*
	return make_expression2(
		e._location,
		"construct-value",
		typeid_to_ast_json(typeid_t::make_vector(typeid_t::make_undefined()), json_tags::k_tag_resolve_state)._value,
		expr_vector_to_json_array(e._exprs)._value
	);
*/

}

ast_json_t maker__member_access(const json_t& address, const std::string& member_name){
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_resolve_member, address, json_t(member_name));
}

ast_json_t maker__make_constant(const value_t& value){
	return make_expression2(
		floyd::k_no_location,
		expression_opcode_t::k_literal,
		value_to_ast_json(value, json_tags::k_tag_resolve_state)._value,
		typeid_to_ast_json(value.get_type(), json_tags::k_tag_resolve_state)._value
	);
}







void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<ast_json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first._value,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
}


void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected){
	if(result != expected){
		QUARK_TRACE("result:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(result)._value));
		QUARK_TRACE("expected:");
		QUARK_TRACE(json_to_pretty_string(value_and_type_to_ast_json(expected)._value));

		fail_test(context);
	}
}

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify(context, result.first, expected.first);
		ut_verify(context, result.second.str(), expected.second.str());
	}
}


}
