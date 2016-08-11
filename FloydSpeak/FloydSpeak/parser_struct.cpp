//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_struct.h"

#include "parser_expression.h"
#include "parser_function.h"
#include "parser_struct.h"
#include "parser_primitives.h"
#include "parser_ast.h"

namespace floyd_parser {
	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;

	/*
		{}
		{int a;}
		{int a = 13;}
	*/
	std::pair<vector<member_t>, std::string> parse_struct_body(scope_ref_t struct_scope, const string& s){
		QUARK_ASSERT(struct_scope && struct_scope->check_invariant());
		QUARK_ASSERT(peek_string(skip_whitespace(s), "{"));

		const auto s2 = skip_whitespace(s);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<member_t> members;
		auto pos = trim_ends(body_pos.first);
		while(!pos.empty()){
			const auto arg_type = read_type(pos);
			const auto member_type = type_identifier_t::make(arg_type.first);
			const auto arg_name = read_required_single_symbol(arg_type.second);

			string default_value;
			const auto optional_default_value = read_optional_char(skip_whitespace(arg_name.second), '=');
			if(optional_default_value.first){
				pos = skip_whitespace(optional_default_value.second);

				const auto constant_expr_pos_s = read_until(pos, ";");

				const auto constant_expr_pos = parse_single(constant_expr_pos_s.first);
				const auto constant_expr = constant_expr_pos.first;
				if(!constant_expr._constant){
					throw std::runtime_error("Struct member defaults must be constants only, not expressions.");
				}
				if(constant_expr._constant->get_type() != member_type){
					throw std::runtime_error("Struct member defaults type mismatch.");
				}

				const auto a = member_t(member_type, arg_name.first, *constant_expr._constant);
				members.push_back(a);
				pos = skip_whitespace(constant_expr_pos_s.second);
			}
			else{
				const auto a = member_t(member_type, arg_name.first);
				members.push_back(a);
				pos = skip_whitespace(optional_default_value.second);
			}
			pos = skip_whitespace(read_required_char(pos, ';'));
		}

		return { members, body_pos.second };
	}

	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		const auto global = scope_def_t::make_global_scope();
		QUARK_TEST_VERIFY((parse_struct_body(global, "{}") == pair<vector<member_t>, string>({}, "")));
	}

	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		const auto global = scope_def_t::make_global_scope();
		QUARK_TEST_VERIFY((parse_struct_body(global, " {} x") == pair<vector<member_t>, string>({}, " x")));
	}


	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		const auto global = scope_def_t::make_global_scope();
		const auto r = parse_struct_body(global, k_test_struct0_body);
		QUARK_TEST_VERIFY((
			r == pair<vector<member_t>, string>(make_test_struct0(global)._members, "" )
		));
	}


	std::pair<struct_def_t, std::string>  parse_struct_definition(scope_ref_t scope_def, const string& pos){
		QUARK_ASSERT(scope_def && scope_def->check_invariant());
		QUARK_ASSERT(pos.size() > 0);

		const auto token_pos = read_until(pos, whitespace_chars);
		QUARK_ASSERT(token_pos.first == "struct");

		const auto struct_name = read_required_single_symbol(token_pos.second);
		const auto name = type_identifier_t::make(struct_name.first);

		auto struct_def = struct_def_t::make2(name, {}, scope_def);
		pair<vector<member_t>, string> body_pos = parse_struct_body(struct_def._struct_scope, skip_whitespace(struct_name.second));
		struct_def._members = body_pos.first;

		auto pos2 = skip_whitespace(body_pos.second);
//		pos2 = read_required_char(pos2, ';');

		return { struct_def, pos2 };
	}

	struct_def_t make_test_struct0(scope_ref_t scope_def){
		return struct_def_t::make2(
			type_identifier_t::make("test_struct0"),
			vector<member_t>
			{
				{ type_identifier_t::make_int(), "x" },
				{ type_identifier_t::make_string(), "y" },
				{ type_identifier_t::make_float(), "z" }
			},
			scope_def
		);
	}

	QUARK_UNIT_TESTQ("type_indentifier_data_ref::operator==()", "operator==()"){
		const type_indentifier_data_ref a{ "xyz", {} };
		const type_indentifier_data_ref b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}

	QUARK_UNIT_TESTQ("type_indentifier_data_ref::operator==()", ""){
		const type_indentifier_data_ref a{ "xyz", make_shared<type_def_t>(type_def_t::make_int()) };
		const type_indentifier_data_ref b{ "xyz", {} };
		QUARK_UT_VERIFY(b == b);
	}


QUARK_UNIT_TESTQ("types_collector_t::operator==()", ""){
	const auto a = types_collector_t();
	const auto b = types_collector_t();

	QUARK_TEST_VERIFY(a == b);
}

	QUARK_UNIT_TESTQ("scope_def_t::operator==", ""){
		const auto a = scope_def_t::make_global_scope();
		const auto b = scope_def_t::make_global_scope();
		QUARK_TEST_VERIFY(*a == *b);
	}

	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto global = scope_def_t::make_global_scope();
		const auto r = parse_struct_definition(global, "struct pixel { int red; int green; int blue;};");
		const auto b = struct_def_t::make2(
			type_identifier_t::make("pixel"),
			vector<member_t>{
				member_t(type_identifier_t::make_int(), "red"),
				member_t(type_identifier_t::make_int(), "green"),
				member_t(type_identifier_t::make_int(), "blue")
			},
			global
		);

		QUARK_TEST_VERIFY(r.first == b);
	}

	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto global = scope_def_t::make_global_scope();
		const auto r = parse_struct_definition(global, "struct pixel { int red = 255; int green = 255; int blue = 255; }");
		QUARK_TEST_VERIFY(r.first == struct_def_t::make2
			(
				type_identifier_t::make("pixel"),
				vector<member_t>{
					member_t(type_identifier_t::make_int(), "red", value_t(255)),
					member_t(type_identifier_t::make_int(), "green", value_t(255)),
					member_t(type_identifier_t::make_int(), "blue", value_t(255))
				},
				global
			)
		);
	}

	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto global = scope_def_t::make_global_scope();
		const auto r = parse_struct_definition(global, "struct pixel { string name = \"lisa\"; float height = 12.3f; }xxx");
		QUARK_TEST_VERIFY(r.first == struct_def_t::make2
			(
				type_identifier_t::make("pixel"),
				vector<member_t>{
					member_t(type_identifier_t::make_string(), "name", value_t("lisa")),
					member_t(type_identifier_t::make_float(), "height", value_t(12.3f))
				},
				global
			)
		);
	}

}	//	floyd_parser


