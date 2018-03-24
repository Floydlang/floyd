//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_statement.h"

#include "parse_expression.h"
#include "parser_primitives.h"
#include "floyd_parser.h"
#include "json_support.h"
#include "json_parser.h"

namespace floyd {
	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;



std::pair<ast_json_t, seq_t> parse_statement_body(const seq_t& s){
	const auto start_seq = skip_whitespace(s);
	read_required(start_seq, "{");

	const auto body = get_balanced(start_seq);
	const auto body_contents = seq_t(trim_ends(body.first));
	const auto statements = parse_statements(body_contents);
	return { statements.first, body.second };
}


QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{}")).first._value,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ int y = 11; }")).first._value,
		parse_json(seq_t(
			R"(
				[
					["bind","^int","y",["k",11,"^int"]]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ int y = 11; print(3); }")).first._value,
		parse_json(seq_t(
			R"(
				[
					["bind","^int","y",["k",11,"^int"]],
					["expression-statement", ["call",["@", "print"],[["k",3, "^int"]]] ]
				]
			)"
		)).first
	);
}

//### test nested blocks.
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t(" { int x = 1; int y = 2; } ")).first._value,
		parse_json(seq_t(
			R"(
				[
					["bind","^int","x",["k",1,"^int"]],
					["bind","^int","y",["k",2,"^int"]]
				]
			)"
		)).first
	);
}



//////////////////////////////////////////////////		parse_block()


pair<ast_json_t, seq_t> parse_block(const seq_t& s){
	const auto body = parse_statement_body(s);
	return { ast_json_t{json_t::make_array({ "block", body.first._value })}, body.second };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_compare_jsons(
		parse_block(seq_t(" { int x = 1; int y = 2; } ")).first._value,
		parse_json(seq_t(
			R"(
				[
					"block",
					[
						["bind","^int","x",["k",1,"^int"]],
						["bind","^int","y",["k",2,"^int"]]
					]
				]
			)"
		)).first
	);
}



//////////////////////////////////////////////////		parse_block()



pair<ast_json_t, seq_t> parse_return_statement(const seq_t& s){
	const auto token_pos = if_first(skip_whitespace(s), "return");
	QUARK_ASSERT(token_pos.first);
	const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
	const auto expression1 = parse_expression_all(seq_t(expression_pos.first));

	const auto statement = json_t::make_array({ json_t("return"), expression1._value });
	const auto pos = skip_whitespace(expression_pos.second.rest1());
	return { ast_json_t{statement}, pos };
}

QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto result = parse_return_statement(seq_t("return 0;"));
	QUARK_TEST_VERIFY(json_to_compact_string(result.first._value) == R"(["return", ["k", 0, "^int"]])");
	QUARK_TEST_VERIFY(result.second.get_s() == "");
}



//////////////////////////////////////////////////		parse_if()



/*
	Parse: if (EXPRESSION) { THEN_STATEMENTS }
	if(a){
		a
	}
*/
std::pair<ast_json_t, seq_t> parse_if(const seq_t& pos){
	std::pair<bool, seq_t> a = if_first(pos, keyword_t::k_if);
	QUARK_ASSERT(a.first);

	const auto condition = read_enclosed_in_parantheses(a.second);
	const auto then_body = parse_statement_body(condition.second);
	const auto condition2 = parse_expression_all(seq_t(condition.first));

	return {
		ast_json_t{json_t::make_array({ keyword_t::k_if, condition2._value, then_body.first._value })},
		then_body.second
	};
}

/*
	Parse optional else { STATEMENTS } or chain of else-if
	Ex 1: "some other statements"
	Ex 2: "else { STATEMENTS }"
	Ex 3: "else if (EXPRESSION) { STATEMENTS }"
	Ex 4: "else if (EXPRESSION) { STATEMENTS } else { STATEMENTS }"
	Ex 5: "else if (EXPRESSION) { STATEMENTS } else if (EXPRESSION) { STATEMENTS } else { STATEMENTS }"
*/


std::pair<ast_json_t, seq_t> parse_if_statement(const seq_t& pos){
	const auto if_statement2 = parse_if(pos);
	std::pair<bool, seq_t> else_start = if_first(skip_whitespace(if_statement2.second), keyword_t::k_else);
	if(else_start.first){
		const auto pos2 = skip_whitespace(else_start.second);
		std::pair<bool, seq_t> elseif_pos = if_first(pos2, keyword_t::k_if);

		if(elseif_pos.first){
			const auto elseif_statement2 = parse_if_statement(pos2);

			return {
				ast_json_t{json_t::make_array({
					keyword_t::k_if,
					if_statement2.first._value.get_array_n(1),
					if_statement2.first._value.get_array_n(2),
					json_t::make_array({elseif_statement2.first._value})
				})},
				elseif_statement2.second
			};
		}
		else{
			const auto else_body = parse_statement_body(pos2);

			return {
				ast_json_t{json_t::make_array({
					keyword_t::k_if,
					if_statement2.first._value.get_array_n(1),
					if_statement2.first._value.get_array_n(2),
					else_body.first._value
				})},
				else_body.second
			};
		}
	}
	else{
		return if_statement2;
	}
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; }")).first._value,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						["return", ["k", 3, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; } else { return 4; }")).first._value,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						["return", ["k", 3, "^int"]]
					],
					[
						["return", ["k", 4, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; } else { return 4; }")).first._value,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						["return", ["k", 3, "^int"]]
					],
					[
						["return", ["k", 4, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){} else if(){} else {}", ""){
	ut_compare_jsons(
		parse_if_statement(
			seq_t("if (1 == 1) { return 1; } else if(2 == 2) { return 2; } else if(3 == 3) { return 3; } else { return 4; }")
		).first._value,
		parse_json(seq_t(
			R"(
				[
					"if", ["==",["k",1,"^int"],["k",1,"^int"]],
					[
						["return", ["k", 1, "^int"]]
					],
					[
						[ "if", ["==",["k",2,"^int"],["k",2,"^int"]],
							[
								["return", ["k", 2, "^int"]]
							],
							[
								[ "if", ["==",["k",3,"^int"],["k",3,"^int"]],
									[
										["return", ["k", 3, "^int"]]
									],
									[
										["return", ["k", 4, "^int"]]
									]
								]
							]
						]
					]
				]
			)"
		)).first
	);
}





//////////////////////////////////////////////////		parse_for_statement()



std::pair<ast_json_t, seq_t> parse_for_statement(const seq_t& pos){
	std::pair<bool, seq_t> for_pos = if_first(pos, keyword_t::k_for);
	QUARK_ASSERT(for_pos.first);

	//	header == " index in 1 ... 5 "
	const auto header = read_enclosed_in_parantheses(for_pos.second);

	const auto body = parse_statement_body(header.second);

	//	iterator == "index".
	const auto iterator_name = read_required_identifier(seq_t(header.first));
	if(iterator_name.first.empty()){
		throw std::runtime_error("For loop requires iterator name.");
	}

	const auto in_str = read_required(skip_whitespace(iterator_name.second), "in");

	//	range == "1 ... 5 ".
	const auto range = skip_whitespace(in_str);

	//	left_and_right == "1 ", " 5 ".
	const auto start_end = split_at(range, "...");
	const auto start = start_end.first;
	const auto end = start_end.second;

	const auto start_expr = parse_expression_all(seq_t(start));
	const auto end_expr = parse_expression_all(end);

	const auto r = ast_json_t{json_t::make_array(
		{
			keyword_t::k_for,
			"open_range",
			iterator_name.first,
			start_expr._value,
			end_expr._value,
			body.first._value
		}
	)};
	return { r, body.second };
}

QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_for_statement(seq_t("for ( index in 1...5 ) { int y = 11; }")).first._value,
		parse_json(seq_t(
			R"(
				[
					"for",
					"open_range",
					"index",
					["k",1,"^int"],
					["k",5,"^int"],
					[
						["bind","^int","y",["k",11,"^int"]]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_while_statement()



std::pair<ast_json_t, seq_t> parse_while_statement(const seq_t& pos){
	std::pair<bool, seq_t> while_pos = if_first(pos, keyword_t::k_while);
	QUARK_ASSERT(while_pos.first);

	const auto condition = read_enclosed_in_parantheses(while_pos.second);
	const auto body = parse_statement_body(condition.second);

	const auto condition_expr = parse_expression_all(seq_t(condition.first));
	const auto r = ast_json_t{json_t::make_array(
		{
			keyword_t::k_while,
			condition_expr._value,
			body.first._value
		}
	)};
	return { r, body.second };
//	return std::pair<json_t, seq_t>(json_t(), seq_t(""));
}

QUARK_UNIT_TEST("", "parse_while_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_while_statement(seq_t("while (a < 10) { print(a); }")).first._value,
		parse_json(seq_t(
			R"(
				[
					"while",
					["<", ["@", "a"], ["k",10,"^int"]],
					[
						["expression-statement",
							["call",
								["@", "print"],
								[
									["@","a"]
								]
							]
						]
					]
				]
			)"
		)).first
	);
}



}	//	floyd
