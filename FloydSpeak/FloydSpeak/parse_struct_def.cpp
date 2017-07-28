//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_struct_def.h"

#include "parse_expression.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "parser_primitives.h"
#include "json_support.h"
#include "json_writer.h"

namespace floyd_parser {
	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;


	std::pair<json_value_t, std::string>  parse_struct_definition1(const string& pos0){
		QUARK_ASSERT(pos0.size() > 0);

		const auto token_pos = read_until(pos0, whitespace_chars);
		QUARK_ASSERT(token_pos.first == "struct");

		const auto struct_name_pos = read_required_single_symbol(token_pos.second);

		const auto s2 = skip_whitespace(struct_name_pos.second);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<json_value_t> members;
		auto pos = trim_ends(body_pos.first);
		while(!pos.empty()){
			const auto member_type = read_type(pos);
			const auto member_name = read_required_single_symbol(member_type.second);

			string default_value;
			const auto optional_default_value = read_optional_char(skip_whitespace(member_name.second), '=');
			if(optional_default_value.first){
				pos = skip_whitespace(optional_default_value.second);

				const auto constant_expr_pos_s = read_until(pos, ";");

				const auto constant_expr_pos = parse_expression_seq(seq_t(constant_expr_pos_s.first));
				const auto constant_expr = constant_expr_pos.first;

				const auto a = make_member_def("<" + member_type.first + ">", member_name.first, constant_expr_pos.first);
				members.push_back(a);
				pos = skip_whitespace(constant_expr_pos_s.second);
			}
			else{
				const auto a = make_member_def("<" + member_type.first + ">", member_name.first, json_value_t());
				members.push_back(a);
				pos = skip_whitespace(optional_default_value.second);
			}
			pos = skip_whitespace(read_required_char(pos, ';'));
		}

		json_value_t obj = make_scope_def();
		obj = store_object_member(obj, "type", "struct");
		obj = store_object_member(obj, "name", json_value_t(struct_name_pos.first));
		obj = store_object_member(obj, "members", members);
		return { obj, skip_whitespace(body_pos.second) };
	}

	const std::string k_test_struct0 = "struct a {int x; string y; float z;}";

	QUARK_UNIT_TESTQ("parse_struct_definition1", ""){
		const auto r = parse_struct_definition1(k_test_struct0);

		const auto expected = json_value_t::make_object({
			{ "name", "a" },
			{ "members", json_value_t::make_array2({
				json_value_t::make_object({ { "name", "x"}, { "type", "<int>"} }),
				json_value_t::make_object({ { "name", "y"}, { "type", "<string>"} }),
				json_value_t::make_object({ { "name", "z"}, { "type", "<float>"} })
			}) },

			{ "args", json_value_t::make_array() },
			{ "locals", json_value_t::make_array() },
			{ "return_type", "" },
			{ "statements", json_value_t::make_array() },
			{ "type", "struct" },
			{ "types", json_value_t::make_object() }
		});

		QUARK_TEST_VERIFY(r == (std::pair<json_value_t, std::string>(expected, "")));
	}


	std::pair<json_value_t, std::string>  parse_struct_definition2(const string& s){
		const auto a = parse_struct_definition1(s);

		const auto r = json_value_t::make_array2({
			"def-struct",
			json_value_t::make_object({
				{ "name", a.first.get_object_element("name") },
				{ "members", a.first.get_object_element("members") }
			})
		});
		return { r, a.second };
	}

	QUARK_UNIT_TESTQ("parse_struct_definition2", ""){
		const auto r = parse_struct_definition2(k_test_struct0);

		const auto expected =
		json_value_t::make_array2({
			"def-struct",
			json_value_t::make_object({
				{ "name", "a" },
				{ "members", json_value_t::make_array2({
					json_value_t::make_object({ { "name", "x"}, { "type", "<int>"} }),
					json_value_t::make_object({ { "name", "y"}, { "type", "<string>"} }),
					json_value_t::make_object({ { "name", "z"}, { "type", "<float>"} })
				}) },
			})
		});

		QUARK_TEST_VERIFY(r == (std::pair<json_value_t, std::string>(expected, "")));
	}



}	//	floyd_parser


