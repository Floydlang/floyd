//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_struct_def.h"

#include "parse_expression.h"
#include "parser_primitives.h"
#include "json_support.h"


namespace floyd {
	using std::string;
	using std::vector;
	using std::pair;


	std::pair<json_t, seq_t>  parse_struct_definition(const seq_t& pos0){
		std::pair<bool, seq_t> token_pos = if_first(pos0, keyword_t::k_struct);
		QUARK_ASSERT(token_pos.first);

		const auto struct_name_pos = read_required_identifier(token_pos.second);

		const auto s2 = skip_whitespace(struct_name_pos.second);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<member_t> members;
		auto pos = seq_t(trim_ends(body_pos.first));
		while(!pos.empty()){
			const auto member_type = read_required_type(pos);
			const auto member_name = read_required_identifier(member_type.second);
			members.push_back(member_t(member_type.first, member_name.first));
			pos = read_required_char(skip_whitespace(member_name.second), ';');
			pos = skip_whitespace(pos);
		}

		const auto r = json_t::make_array({
			"def-struct",
			json_t::make_object({
				{ "name", json_t(struct_name_pos.first) },
				{ "members", members_to_json(members) }
			})
		});
		return { r, skip_whitespace(body_pos.second) };
	}

	const std::string k_test_struct0 = "struct a {int x; string y; float z;}";


	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto r = parse_struct_definition(seq_t(k_test_struct0));

		const auto expected =
		json_t::make_array({
			"def-struct",
			json_t::make_object({
				{ "name", "a" },
				{ "members", json_t::make_array({
					json_t::make_object({ { "name", "x"}, { "type", "int"} }),
					json_t::make_object({ { "name", "y"}, { "type", "string"} }),
					json_t::make_object({ { "name", "z"}, { "type", "float"} })
				}) },
			})
		});

		QUARK_TEST_VERIFY(r == (std::pair<json_t, seq_t>(expected, seq_t(""))));
	}


}	//	namespace floyd

