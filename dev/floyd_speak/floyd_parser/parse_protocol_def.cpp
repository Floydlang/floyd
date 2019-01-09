//
//  parse_protocol_def.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "parse_protocol_def.h"

#include "parse_expression.h"
#include "parser_primitives.h"
#include "json_support.h"
#include "ast_typeid.h"
#include "ast_typeid_helpers.h"


namespace floyd {
	using std::string;
	using std::vector;
	using std::pair;


	std::pair<ast_json_t, seq_t>  parse_protocol_definition_body(const seq_t& p, const std::string& name){
		const auto s2 = skip_whitespace(p);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<member_t> functions;

		auto pos = seq_t(trim_ends(body_pos.first));
		while(!pos.empty()){
			const auto func_pos = read_required(skip_whitespace(pos), keyword_t::k_func);
			const auto return_type_pos = read_required_type(func_pos);
			const auto function_name_pos = read_required_identifier(return_type_pos.second);
			const auto args_pos = read_function_arg_parantheses(function_name_pos.second);
			pos = read_optional_char(skip_whitespace(args_pos.second), ';').second;
			pos = skip_whitespace(pos);


			std::vector<typeid_t> arg_types;
			for(const auto& e: args_pos.first){
				arg_types.push_back(e._type);
			}
			const member_t f = {
				typeid_t::make_function(return_type_pos.first, arg_types),
				function_name_pos.first
			};
			functions.push_back(f);
		}

		const auto r = json_t::make_array({
			"def-protocol",
			json_t::make_object({
				{ "name", name },
				{ "members", members_to_json(functions)
				}
			})
		});
		return { ast_json_t{r}, skip_whitespace(body_pos.second) };
	}

	std::pair<ast_json_t, seq_t>  parse_protocol_definition(const seq_t& p){
		const auto pos0 = skip_whitespace(p);
		std::pair<bool, seq_t> token_pos = if_first(pos0, keyword_t::k_protocol);
		QUARK_ASSERT(token_pos.first);

		const auto protocol_name_pos = read_required_identifier(token_pos.second);

		const auto s2 = skip_whitespace(protocol_name_pos.second);
		return parse_protocol_definition_body(s2, protocol_name_pos.first);
	}

	const std::string k_test_protocol0 = R"(
		protocol prot {
			func int f(string a, double b)
			func string g(bool c)
		}
	)";


	QUARK_UNIT_TEST("parse_protocol_definition", "", "", ""){
		const auto r = parse_protocol_definition(seq_t(k_test_protocol0));

		const auto expected =
		json_t::make_array({
			"def-protocol",
			json_t::make_object({
				{ "name", "prot" },
				{
					"members",
					json_t::make_array({
						json_t::make_object({
							{ "name", "f"},
							{
								"type",
								json_t::make_array({ "fun", "^int", json_t::make_array({"^string", "^double"}) })
							}
						}),
						json_t::make_object({
							{ "name", "g"},
							{
								"type",
								json_t::make_array({ "fun", "^string", json_t::make_array({"^bool"}) })
							}
						})
					})
				},
			})
		});

		ut_compare_jsons(r.first._value, expected);
		quark::ut_compare_strings(r.second.str(), "");
	}


}	//	namespace floyd


