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


parse_result_t parse_statement_body(const seq_t& s){
	const auto start_seq = skip_whitespace(s);
	const auto statements = parse_statements_bracketted(start_seq);
	return { statements.ast, statements.pos };
}


QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{}")).ast._value,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ let int y = 11; }")).ast._value,
		parse_json(seq_t(
			R"(
				[
					[2, "bind","^int","y",["k",11,"^int"]]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ let int y = 11; print(3); }")).ast._value,
		parse_json(seq_t(
			R"(
				[
					[2, "bind","^int","y",["k",11,"^int"]],
					[18, "expression-statement", ["call",["@", "print"],[["k",3, "^int"]]] ]
				]
			)"
		)).first
	);
}

//### test nested blocks.
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t(" { let int x = 1; let int y = 2; } ")).ast._value,
		parse_json(seq_t(
			R"(
				[
					[3, "bind","^int","x",["k",1,"^int"]],
					[18, "bind","^int","y",["k",2,"^int"]]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_block()


pair<ast_json_t, seq_t> parse_block(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto body = parse_statement_body(start);
	return { make_statement1(location_t(start.pos()), "block", body.ast._value), body.pos };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_compare_jsons(
		parse_block(seq_t(" { let int x = 1; let int y = 2; } ")).first._value,
		parse_json(seq_t(
			R"(
				[
					1,
					"block",
					[
						[3, "bind","^int","x",["k",1,"^int"]],
						[18, "bind","^int","y",["k",2,"^int"]]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_block()


pair<ast_json_t, seq_t> parse_return_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto token_pos = if_first(start, "return");
	QUARK_ASSERT(token_pos.first);
	const auto pos2 = skip_whitespace(token_pos.second);
	const auto expression1 = parse_expression_seq(pos2);

	const auto statement = make_statement1(location_t(start.pos()), "return", expression1.first._value);
	const auto pos = skip_whitespace(expression1.second.rest1());
	return { statement, pos };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_compare_jsons(
		parse_return_statement(seq_t("return 0;")).first._value,
		parse_json(seq_t(
			R"(
				[0, "return", ["k", 0, "^int"]]
			)"
		)).first
	);
}



//////////////////////////////////////////////////		parse_if()


/*
	Parse: if (EXPRESSION) { THEN_STATEMENTS }
	if(a){
		a
	}
*/
std::pair<ast_json_t, seq_t> parse_if(const seq_t& pos){
	const auto start = skip_whitespace(pos);
	const auto a = if_first(start, keyword_t::k_if);
	QUARK_ASSERT(a.first);

	const auto condition = read_enclosed_in_parantheses(a.second);
	const auto then_body = parse_statement_body(condition.second);
	const auto condition2 = parse_expression_all(seq_t(condition.first));

	return {
		make_statement2(location_t(start.pos()), keyword_t::k_if, condition2._value, then_body.ast._value),
		then_body.pos
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


//??? Decide if parse functions accept leading whitespace or asserts on there is none.

std::pair<ast_json_t, seq_t> parse_if_statement(const seq_t& pos){
	const auto start = skip_whitespace(pos);

	const auto if_statement2 = parse_if(start);
	std::pair<bool, seq_t> else_start = if_first(skip_whitespace(if_statement2.second), keyword_t::k_else);
	if(else_start.first){
		const auto pos2 = skip_whitespace(else_start.second);
		std::pair<bool, seq_t> elseif_pos = if_first(pos2, keyword_t::k_if);

		if(elseif_pos.first){
			const auto elseif_statement2 = parse_if_statement(pos2);

			return {
				make_statement3(
					location_t(start.pos()),
					keyword_t::k_if,
					if_statement2.first._value.get_array_n(2),
					if_statement2.first._value.get_array_n(3),
					json_t::make_array({elseif_statement2.first._value})
				),
				elseif_statement2.second
			};
		}
		else{
			const auto else_body = parse_statement_body(pos2);

			return {
				make_statement3(
					location_t(start.pos()),
					keyword_t::k_if,
					if_statement2.first._value.get_array_n(2),
					if_statement2.first._value.get_array_n(3),
					else_body.ast._value
				),
				else_body.pos
			};
		}
	}
	else{
		return if_statement2;
	}
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3 }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						[13, "return", ["k", 3, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3 } else { return 4 }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						[13, "return", ["k", 3, "^int"]]
					],
					[
						[31, "return", ["k", 4, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3 } else { return 4 }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"if",
					[">",["k",1,"^int"],["k",2,"^int"]],
					[
						[13, "return", ["k", 3, "^int"]]
					],
					[
						[31, "return", ["k", 4, "^int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){} else if(){} else {}", ""){
	ut_compare_jsons(
		parse_if_statement(
			seq_t("if (1 == 1) { return 1 } else if(2 == 2) { return 2 } else if(3 == 3) { return 3 } else { return 4 }")
		).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"if", ["==",["k",1,"^int"],["k",1,"^int"]],
					[
						[14, "return", ["k", 1, "^int"]]
					],
					[
						[ 30, "if", ["==",["k",2,"^int"],["k",2,"^int"]],
							[
								[43, "return", ["k", 2, "^int"]]
							],
							[
								[ 59, "if", ["==",["k",3,"^int"],["k",3,"^int"]],
									[
										[72, "return", ["k", 3, "^int"]]
									],
									[
										[90, "return", ["k", 4, "^int"]]
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

struct range_def_t {
	std::string _start;
	std::string _range_type;
	seq_t _end_pos;
};

//	Closed range == "1 ... 5 ".
//	Open range == "1 ..< 5 ".
//	left_and_right == "1 ", " 5 ".
range_def_t parse_range(const seq_t& pos){
	const auto start_end = split_at(pos, "...");
	if(start_end.first != ""){
		return { start_end.first, "...", start_end.second };
	}

	const auto start_end2 = split_at(pos, "..<");
	if(start_end2.first != ""){
		return { start_end2.first, "..<", start_end2.second };
	}

	throw std::runtime_error("For loop has illegal range syntax.");
}


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

	const auto range = skip_whitespace(in_str);

	const auto range_parts = parse_range(range);
	const auto start = range_parts._start;
	const auto range_type = range_parts._range_type;
	const auto end_pos = range_parts._end_pos;

	const auto start_expr = parse_expression_all(seq_t(start));
	const auto end_expr = parse_expression_all(end_pos);

	const auto r = make_statement_n(
		location_t(pos.pos()),
		keyword_t::k_for,
		{
			range_type == "..." ? "closed-range" : "open-range",
			iterator_name.first,
			start_expr._value,
			end_expr._value,
			body.ast._value
		}
	);
	return { r, body.pos };
}

QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_for_statement(seq_t("for ( index in 1...5 ) { let int y = 11 }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"for",
					"closed-range",
					"index",
					["k",1,"^int"],
					["k",5,"^int"],
					[
						[25, "bind","^int","y",["k",11,"^int"]]
					]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_for_statement(seq_t("for ( index in 1..<5 ) { let int y = 11 }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"for",
					"open-range",
					"index",
					["k",1,"^int"],
					["k",5,"^int"],
					[
						[25, "bind","^int","y",["k",11,"^int"]]
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
	const auto r = make_statement_n(
		location_t(pos.pos()),
		keyword_t::k_while,
		{
			condition_expr._value,
			body.ast._value
		}
	);
	return { r, body.pos };
}

QUARK_UNIT_TEST("", "parse_while_statement()", "while(){}", ""){
	ut_compare_jsons(
		parse_while_statement(seq_t("while (a < 10) { print(a) }")).first._value,
		parse_json(seq_t(
			R"(
				[
					0,
					"while",
					["<", ["@", "a"], ["k",10,"^int"]],
					[
						[17, "expression-statement",
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


//////////////////////////////////////////////////		parse_software_system()


/*
	software-system: JSON
*/



pair<ast_json_t, seq_t> parse_software_system(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto ss_pos = if_first(start, keyword_t::k_software_system);
	if(ss_pos.first == false){
		throw std::runtime_error("Syntax error");
	}

	//??? Instead of parsing a static JSON literal, we could parse a Floyd expression that results in a JSON value = use variables etc.
	std::pair<json_t, seq_t> json_pos = parse_json(ss_pos.second);
	const auto r = make_statement1(location_t(start.pos()), keyword_t::k_software_system, json_pos.first);
	return { r, json_pos.second };
}


//////////////////////////////////////////////////		parse_container_def()


/*
	container-def: JSON
*/



pair<ast_json_t, seq_t> parse_container_def(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto ss_pos = if_first(start, keyword_t::k_container_def);
	if(ss_pos.first == false){
		throw std::runtime_error("Syntax error");
	}

	//??? Instead of parsing a static JSON literal, we could parse a Floyd expression that results in a JSON value = use variables etc.
	std::pair<json_t, seq_t> json_pos = parse_json(ss_pos.second);

	const auto r = make_statement1(location_t(start.pos()), keyword_t::k_container_def, json_pos.first);
	return { r, json_pos.second };
}




}	//	floyd
