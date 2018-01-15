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



std::pair<json_t, seq_t> parse_statement_body(const seq_t& s){
	const auto pos = skip_whitespace(s);
	read_required(pos, "{");
	const auto b_str = get_balanced(pos);
	const auto body_str = seq_t(trim_ends(b_str.first));
	const auto body_statements2 = parse_statements(body_str);
	return { body_statements2.first, b_str.second };
}


QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{}")).first,
		parse_json(seq_t(
			R"(
				[
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ int y = 11; }")).first,
		parse_json(seq_t(
			R"(
				[
					["bind","int","y",["k",11,"int"], {}]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t("{ int y = 11; print(3); }")).first,
		parse_json(seq_t(
			R"(
				[
					["bind","int","y",["k",11,"int"], {}],
					["expression-statement",
						["call",["@", "print"],[["k",3, "int"]]]
					]
				]
			)"
		)).first
	);
}




//////////////////////////////////////////////////		parse_block()


pair<json_t, seq_t> parse_block(const seq_t& s){
	const auto pos = skip_whitespace(s);
	QUARK_ASSERT(pos.first1() == "{");

	const auto b_str = get_balanced(pos);
	const auto body_str = seq_t(trim_ends(b_str.first));
	const auto body_statements2 = parse_statements(body_str);
	return { json_t::make_array({ "block", body_statements2.first }), b_str.second };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_compare_jsons(
		parse_block(seq_t(" { int x = 1; int y = 2; } ")).first,
		parse_json(seq_t(
			R"(
				[
					"block",
					[
						["bind","int","x",["k",1,"int"], {}],
						["bind","int","y",["k",2,"int"], {}]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_block()


pair<json_t, seq_t> parse_return_statement(const seq_t& s){
	const auto token_pos = if_first(skip_whitespace(s), "return");
	QUARK_ASSERT(token_pos.first);
	const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
	const auto expression1 = parse_expression_all(seq_t(expression_pos.first));
	const auto statement = json_t::make_array({ json_t("return"), expression1 });
	//	Skip trailing ";".
	const auto pos = skip_whitespace(expression_pos.second.rest1());
	return { statement, pos };
}

QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto result = parse_return_statement(seq_t("return 0;"));
	QUARK_TEST_VERIFY(json_to_compact_string(result.first) == R"(["return", ["k", 0, "int"]])");
	QUARK_TEST_VERIFY(result.second.get_s() == "");
}



//////////////////////////////////////////////////		parse_if()



//??? Idea: Have explicit whitespaces - fail to parse.


/*
	Parse: if (EXPRESSION) { THEN_STATEMENTS }
	if(a){
		a
	}
*/
std::pair<json_t, seq_t> parse_if(const seq_t& pos){
	std::pair<bool, seq_t> a = if_first(pos, "if");
	QUARK_ASSERT(a.first);

	const auto pos2 = skip_whitespace(a.second);
	read_required(pos2, "(");
	const auto condition_parantheses = get_balanced(pos2);
	const auto condition = trim_ends(condition_parantheses.first);

	const auto pos3 = skip_whitespace(condition_parantheses.second);
	read_required(pos3, "{");
	const auto then_statements_parantheses = get_balanced(pos3);
	const auto then_statements = trim_ends(then_statements_parantheses.first);

	auto return_pos = then_statements_parantheses.second;

	const auto condition2 = parse_expression_all(seq_t(condition));
	const auto then_statements2 = parse_statements(seq_t(then_statements)).first;

	return { json_t::make_array({ "if", condition2, then_statements2 }), return_pos };
}

/*
	Parse optional else { STATEMENTS } or chain of else-if
	Ex 1: "some other statements"
	Ex 2: "else { STATEMENTS }"
	Ex 3: "else if (EXPRESSION) { STATEMENTS }"
	Ex 4: "else if (EXPRESSION) { STATEMENTS } else { STATEMENTS }"
	Ex 5: "else if (EXPRESSION) { STATEMENTS } else if (EXPRESSION) { STATEMENTS } else { STATEMENTS }"
*/


std::pair<json_t, seq_t> parse_if_statement(const seq_t& pos){
	const auto if_statement2 = parse_if(pos);
	std::pair<bool, seq_t> else_start = if_first(skip_whitespace(if_statement2.second), "else");
	if(else_start.first){
		const auto pos2 = skip_whitespace(else_start.second);
		std::pair<bool, seq_t> elseif_pos = if_first(pos2, "if");
		if(elseif_pos.first){
			const auto elseif_statement2 = parse_if_statement(pos2);
			return { json_t::make_array(
				{ "if", if_statement2.first.get_array_n(1), if_statement2.first.get_array_n(2), json_t::make_array({elseif_statement2.first}) }),
				elseif_statement2.second
			};
		}
		else{
			read_required(pos2, "{");
			const auto else_statements_parantheses = get_balanced(pos2);
			const auto else_statements = trim_ends(else_statements_parantheses.first);
			const auto else_statements2 = parse_statements(seq_t(else_statements));

			return { json_t::make_array(
				{ "if", if_statement2.first.get_array_n(1), if_statement2.first.get_array_n(2), else_statements2.first }),
				else_statements_parantheses.second
			};
		}
	}
	else{
		return if_statement2;
	}
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; }")).first,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"int"],["k",2,"int"]],
					[
						["return", ["k", 3, "int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; } else { return 4; }")).first,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"int"],["k",2,"int"]],
					[
						["return", ["k", 3, "int"]]
					],
					[
						["return", ["k", 4, "int"]]
					]
				]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_if_statement()", "if(){}else{}", ""){
	ut_compare_jsons(
		parse_if_statement(seq_t("if (1 > 2) { return 3; } else { return 4; }")).first,
		parse_json(seq_t(
			R"(
				[
					"if",
					[">",["k",1,"int"],["k",2,"int"]],
					[
						["return", ["k", 3, "int"]]
					],
					[
						["return", ["k", 4, "int"]]
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
		).first,
		parse_json(seq_t(
			R"(
				[
					"if", ["==",["k",1,"int"],["k",1,"int"]],
					[
						["return", ["k", 1, "int"]]
					],
					[
						[ "if", ["==",["k",2,"int"],["k",2,"int"]],
							[
								["return", ["k", 2, "int"]]
							],
							[
								[ "if", ["==",["k",3,"int"],["k",3,"int"]],
									[
										["return", ["k", 3, "int"]]
									],
									[
										["return", ["k", 4, "int"]]
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



std::pair<json_t, seq_t> parse_for_statement(const seq_t& pos){
	std::pair<bool, seq_t> pos1 = if_first(pos, "for");
	QUARK_ASSERT(pos1.first);
	const auto pos2 = skip_whitespace(pos1.second);
	read_required(pos2, "(");
	const auto header_in_parantheses = get_balanced(pos2);

	const auto pos3 = skip_whitespace(header_in_parantheses.second);
	read_required(pos3, "{");
	const auto body = get_balanced(pos3);
	const auto body_statements_str = trim_ends(body.first);


	//	header_in_parantheses == "( index in 1 ... 5 )"
	//	header == " index in 1 ... 5 "
	const auto header = seq_t(trim_ends(header_in_parantheses.first));

	//	iterator == "index".
	const auto iterator_name = read_required_single_symbol(header);
	if(iterator_name.first.empty()){
		throw std::runtime_error("For loop requires iterator name.");
	}


	//???parse_statement_body()

	//	range == "1 ... 5 ".
	const auto range = skip_whitespace(read_required(skip_whitespace(iterator_name.second), "in"));

	//	left_and_right == "1 ", " 5 ".
	const auto left_and_right = split_at(range, "...");
	const auto start = left_and_right.first;
	const auto end = left_and_right.second;

	const auto start_expr = parse_expression_all(seq_t(start));
	const auto end_expr = parse_expression_all(end);


	const auto body_statements2 = parse_statements(seq_t(body_statements_str));

	const auto r = json_t::make_array(
		{
			"for",
			"open_range",
			iterator_name.first,
			start_expr,
			end_expr,
			body_statements2.first
		
		}
	);
	return { r, body.second };
}

QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_for_statement(seq_t("for ( index in 1...5 ) { int y = 11; }")).first,
		parse_json(seq_t(
			R"(
				[
					"for",
					"open_range",
					"index",
					["k",1,"int"],
					["k",5,"int"],
					[
						["bind","int","y",["k",11,"int"], {}]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_while_statement()



std::pair<json_t, seq_t> parse_while_statement(const seq_t& pos){
/*
	std::pair<bool, seq_t> while_pos = if_first(pos, "while");
	QUARK_ASSERT(while_pos.first);
	const auto pos2 = skip_whitespace(while_pos.second);
	read_required(pos2, "(");
	const auto header_in_parantheses = get_balanced(pos2);


	const auto pos3 = skip_whitespace(header_in_parantheses.second);
	read_required(pos3, "{");
	const auto body = get_balanced(pos3);
	const auto body_statements_str = trim_ends(body.first);

	//???parse_statement_body()


	//	header_in_parantheses == "( index in 1 ... 5 )"
	//	header == " index in 1 ... 5 "
	const auto header = seq_t(trim_ends(header_in_parantheses.first));

	//	iterator == "index".
	const auto iterator_name = read_required_single_symbol(header);
	if(iterator_name.first.empty()){
		throw std::runtime_error("For loop requires iterator name.");
	}

	//	range == "1 ... 5 ".
	const auto range = skip_whitespace(read_required(skip_whitespace(iterator_name.second), "in"));

	//	left_and_right == "1 ", " 5 ".
	const auto left_and_right = split_at(range, "...");
	const auto start = left_and_right.first;
	const auto end = left_and_right.second;

	const auto start_expr = parse_expression_all(seq_t(start));
	const auto end_expr = parse_expression_all(end);


	const auto body_statements2 = parse_statements(seq_t(body_statements_str));

	const auto r = json_t::make_array(
		{
			"while",
			condition_expr,
			body_statements2.first
		}
	);
	return { r, body.second };
*/
	return std::pair<json_t, seq_t>(json_t(), seq_t(""));
}

/*
QUARK_UNIT_TEST("", "parse_while_statement()", "for(){}", ""){
	ut_compare_jsons(
		parse_while_statement(seq_t("while (a < 10) { print(a); }")).first,
		parse_json(seq_t(
			R"(
				[
					"while",
					["k",1,"int"],
					["k",5,"int"],
					[
						["print","int","y",["k",11,"int"], {}]
					]
				]
			)"
		)).first
	);
}

*/



}	//	floyd
