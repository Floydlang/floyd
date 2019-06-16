//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "ast_json.h"

#include "ast_value.h"
#include "text_parser.h"
#include "json_support.h"
#include "compiler_basics.h"

namespace floyd {


json_t make_ast_entry(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

json_t make_statement_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	return make_ast_entry(location, opcode, params);
}
QUARK_UNIT_TEST("", "make_statement_n()", "", ""){
	const auto r = make_statement_n(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify(QUARK_POS, r, json_t::make_array({ 1234.0, "def-struct" }));
}

json_t make_statement1(const location_t& location, const std::string& opcode, const json_t& params){
	return make_statement_n(location, opcode, { params });
}
json_t make_statement2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2){
	return make_statement_n(location, opcode, { param1, param2 });
}
json_t make_statement3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3){
	return make_statement_n(location, opcode, { param1, param2, param3 });
}
json_t make_statement4(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3, const json_t& param4){
	return make_statement_n(location, opcode, { param1, param2, param3, param4 });
}





json_t make_expression_n(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	return make_ast_entry(location, opcode, params);
}

json_t make_expression1(const location_t& location, const std::string& opcode, const json_t& param){
	return make_expression_n(location, opcode, { param });
}

json_t make_expression2(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2){
	return make_expression_n(location, opcode, { param1, param2 });
}
json_t make_expression3(const location_t& location, const std::string& opcode, const json_t& param1, const json_t& param2, const json_t& param3){
	return make_expression_n(location, opcode, { param1, param2, param3 });
}





json_t maker__make_identifier(const std::string& s){
	return make_expression1(floyd::k_no_location, expression_opcode_t::k_load, json_t(s));
}

json_t maker__make_unary_minus(const json_t& expr){
	return make_expression1(floyd::k_no_location, expression_opcode_t::k_unary_minus, expr);
}

json_t maker__make2(const std::string op, const json_t& lhs, const json_t& rhs){
	QUARK_ASSERT(op != "");
	return make_expression2(floyd::k_no_location, op, lhs, rhs);
}

json_t maker__make_conditional_operator(const json_t& e1, const json_t& e2, const json_t& e3){
	return make_expression3(floyd::k_no_location, expression_opcode_t::k_conditional_operator, e1, e2, e3);
}

json_t maker__call(const json_t& f, const std::vector<json_t>& args){
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_call, f, json_t::make_array(args));
}
json_t maker__corecall(const std::string& name, const std::vector<json_t>& args){
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_corecall, name, json_t::make_array(args));
}
json_t maker__update(const json_t& parent, const json_t& key, const json_t& new_value){
	return make_expression3(floyd::k_no_location, expression_opcode_t::k_update, parent, key, new_value);
}

json_t maker_vector_definition(const std::string& element_type, const std::vector<json_t>& elements){
	QUARK_ASSERT(element_type == "");

	const auto element_type2 = typeid_to_ast_json(typeid_t::make_vector(typeid_t::make_undefined()), json_tags::k_tag_resolve_state);
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_value_constructor, element_type2, json_t::make_array(elements));
}
json_t maker_dict_definition(const std::string& value_type, const std::vector<json_t>& elements){
	QUARK_ASSERT(value_type == "");

	const auto element_type2 = typeid_to_ast_json(typeid_t::make_dict(typeid_t::make_undefined()), json_tags::k_tag_resolve_state);
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_value_constructor, element_type2, json_t::make_array(elements));
}

json_t maker__member_access(const json_t& address, const std::string& member_name){
	return make_expression2(floyd::k_no_location, expression_opcode_t::k_resolve_member, address, json_t(member_name));
}

json_t maker__make_constant(const value_t& value){
	return make_expression2(
		floyd::k_no_location,
		expression_opcode_t::k_literal,
		value_to_ast_json(value, json_tags::k_tag_resolve_state),
		typeid_to_ast_json(value.get_type(), json_tags::k_tag_resolve_state)
	);
}


std::pair<json_t, location_t> unpack_loc(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;

		const auto elements = s.get_array();
		const std::vector<json_t> elements2 = { elements.begin() + 1, elements.end() };
		const auto statement = json_t::make_array(elements2);

		return { statement, source_offset };
	}
	else{
		return { s, k_no_location };
	}
}

location_t unpack_loc2(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;
		return source_offset;
	}
	else{
		return k_no_location;
	}
}



void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
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
