//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_struct.h"


#include "parser_types.h"
#include "parser_expression.h"
#include "parser_function.h"
#include "parser_struct.h"
#include "parser_primitives.h"


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
	std::pair<struct_def_t, std::string> parse_struct_body(const string& s){
		const auto s2 = skip_whitespace(s);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<member_t> members;
		auto pos = trim_ends(body_pos.first);
		while(!pos.empty()){
			const auto arg_type = read_type(pos);
			const auto member_type = make_type_identifier(arg_type.first);
			const auto arg_name = read_required_single_symbol(arg_type.second);

			string default_value;
			const auto optional_default_value = read_optional_char(skip_whitespace(arg_name.second), '=');
			if(optional_default_value.first){
				pos = skip_whitespace(optional_default_value.second);

				const auto constant_expr_pos_s = read_until(pos, ";");

				const auto constant_expr_pos = parse_single({}, constant_expr_pos_s.first);
				const auto constant_expr = constant_expr_pos.first;
				if(!constant_expr._constant){
					throw std::runtime_error("Struct member defaults must be constants only, not expressions.");
				}
				if(constant_expr._constant->get_type() != member_type){
					throw std::runtime_error("Struct member defaults type mismatch.");
				}

				const auto a = member_t(*constant_expr._constant, arg_name.first);
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

		return { struct_def_t{members}, body_pos.second };
	}


	std::tuple<std::string, struct_def_t, std::string>  parse_struct_definition(const string& pos){
		QUARK_ASSERT(pos.size() > 0);

		const auto token_pos = read_until(pos, whitespace_chars);
		QUARK_ASSERT(token_pos.first == "struct");

		const auto struct_name = read_required_single_symbol(token_pos.second);
		pair<struct_def_t, string> body_pos = parse_struct_body(skip_whitespace(struct_name.second));

		auto pos2 = skip_whitespace(body_pos.second);
		pos2 = read_required_char(pos2, ';');

	//	return { define_struct_statement_t{ struct_name.first, body_pos.first }, ast, skip_whitespace(pos2) };
		return { struct_name.first, body_pos.first, pos2 };
	}


	struct_def_t make_test_struct0(){
		return {
			vector<member_t>
			{
				{ make_type_identifier("int"), "x" },
				{ make_type_identifier("string"), "y" },
				{ make_type_identifier("float"), "z" }
			}
		};
	}





	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		QUARK_TEST_VERIFY((parse_struct_body("{}") == pair<struct_def_t, string>({}, "")));
	}

	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		QUARK_TEST_VERIFY((parse_struct_body(" {} x") == pair<struct_def_t, string>({}, " x")));
	}


	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		const auto r = parse_struct_body(k_test_struct0_body);
		QUARK_TEST_VERIFY((
			r == pair<struct_def_t, string>(make_test_struct0(), "" )
		));
	}


	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto r = parse_struct_definition("struct pixel { int red; int green; int blue;};");
		QUARK_TEST_VERIFY(std::get<0>(r) == "pixel");
		QUARK_TEST_VERIFY(std::get<1>(r) == make_struct_def
			(
				vector<member_t>{
					member_t(type_identifier_t("int"), "red"),
					member_t(type_identifier_t("int"), "green"),
					member_t(type_identifier_t("int"), "blue")
				}
			)
		);
	}

	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto r = parse_struct_definition("struct pixel { int red = 255; int green = 255; int blue = 255; };");
		QUARK_TEST_VERIFY(std::get<0>(r) == "pixel");
		QUARK_TEST_VERIFY(std::get<1>(r) == make_struct_def
			(
				vector<member_t>{
					member_t(value_t(255), "red"),
					member_t(value_t(255), "green"),
					member_t(value_t(255), "blue")
				}
			)
		);
	}

	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto r = parse_struct_definition("struct pixel { string name = \"lisa\"; float height = 12.3f; };");
		QUARK_TEST_VERIFY(std::get<0>(r) == "pixel");
		QUARK_TEST_VERIFY(std::get<1>(r) == make_struct_def
			(
				vector<member_t>{
					member_t(value_t("lisa"), "name"),
					member_t(value_t(12.3f), "height")
				}
			)
		);
	}


//		struct pixel { int red; int green; int blue; };
//		struct pixel { int red = 255; int green = 255; int blue = 255; };


}	//	floyd_parser










