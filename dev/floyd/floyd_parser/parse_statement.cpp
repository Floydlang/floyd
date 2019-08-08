//
//  parser_statement.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_statement.h"

#include "parse_expression.h"
#include "parser_primitives.h"
#include "floyd_parser.h"
#include "json_support.h"
#include "floyd_syntax.h"
#include "compiler_basics.h"

#include "typeid.h"


namespace floyd {
namespace parser {

//??? Decide policy if parse functions accept leading whitespace or asserts on there is none.


//////////////////////////////////////////////////		parse_statement_body()


parse_result_t parse_statement_body(const seq_t& s){
	return parse_statements_bracketted(s);
}

QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t("{}")).parse_tree,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t("{ let int y = 11; }")).parse_tree,
		parse_json(seq_t(
			R"(
				[
					[2, "init-local","^int","y",["k",11,"^int"]]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t("{ let int y = 11; print(3); }")).parse_tree,
		parse_json(seq_t(
			R"(
				[
					[2, "init-local","^int","y",["k",11,"^int"]],
					[18, "expression-statement", ["call",["@", "print"],[["k",3, "^int"]]] ]
				]
			)"
		)).first
	);
}

//### test nested blocks.
QUARK_UNIT_TEST("", "parse_statement_body()", "", ""){
	ut_verify(QUARK_POS,
		parse_statement_body(seq_t(" { let int x = 1; let int y = 2; } ")).parse_tree,
		parse_json(seq_t(
			R"(
				[
					[3, "init-local","^int","x",["k",1,"^int"]],
					[18, "init-local","^int","y",["k",2,"^int"]]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_block()


std::pair<json_t, seq_t> parse_block(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto body = parse_statement_body(start);
	return { make_parser_node(location_t(start.pos()), parse_tree_statement_opcode::k_block, { body.parse_tree } ), body.pos };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_verify(QUARK_POS,
		parse_block(seq_t(" { let int x = 1; let int y = 2; } ")).first,
		parse_json(seq_t(
			R"(
				[
					1,
					"block",
					[
						[3, "init-local","^int","x",["k",1,"^int"]],
						[18, "init-local","^int","y",["k",2,"^int"]]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_return_statement()


std::pair<json_t, seq_t> parse_return_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto token_pos = if_first(start, keyword_t::k_return);
	QUARK_ASSERT(token_pos.first);
	const auto pos2 = skip_whitespace(token_pos.second);
	const auto expression1 = parse_expression(pos2);

	const auto statement = make_parser_node(location_t(start.pos()), parse_tree_statement_opcode::k_return, { expression1.first } );
	const auto pos = skip_whitespace(expression1.second.rest1());
	return { statement, pos };
}

QUARK_UNIT_TEST("", "parse_block()", "Block with two binds", ""){
	ut_verify(QUARK_POS,
		parse_return_statement(seq_t("return 0;")).first,
		parse_json(seq_t(
			R"(
				[0, "return", ["k", 0, "^int"]]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_bind_statement()


//	Finds identifier-string at RIGHT side of string. Stops at whitespace.
//	"1234()=<whatever> IDENTIFIER": "1234()=<whatever>", "IDENTIFIER"
std::size_t split_at_tail_identifier(const std::string& s){
	auto i = s.size();
	while(i > 0 && k_whitespace_chars.find(s[i - 1]) != std::string::npos){
		i--;
	}
	while(i > 0 && k_identifier_chars.find(s[i - 1]) != std::string::npos){
		i--;
	}
	return i;
}

//	BIND:			int x = 10
//	BIND:			x = 10
//	BIND:			int (string a) x = f(4 == 5)
//	BIND:			int x = 10
//	BIND:			x = 10
struct a_result_t {
	typeid_t type;
	std::string identifier;

	//	Points to "=" or end of sequence if no "=" was found.
	seq_t rest;
};

a_result_t parse_a(const seq_t& p, const location_t& loc){
	const auto pos = skip_whitespace(p);

	//	Notice: if there is no type, only and identifier -- then we still get a type back: with an unresolved identifier.
	const auto optional_type_pos = read_type(pos);
	const auto identifier_pos = read_identifier(optional_type_pos.second);

	if(optional_type_pos.first && identifier_pos.first != ""){
		return a_result_t{ *optional_type_pos.first, identifier_pos.first, identifier_pos.second };
	}
	else if(!optional_type_pos.first && identifier_pos.first != ""){
		QUARK_ASSERT(false);
		return a_result_t{ typeid_t::make_undefined(), optional_type_pos.first->get_unresolved(), identifier_pos.second };
	}
	else if(optional_type_pos.first && optional_type_pos.first->is_unresolved_type_identifier() && identifier_pos.first == ""){
		return a_result_t{ typeid_t::make_undefined(), optional_type_pos.first->get_unresolved(), identifier_pos.second };
	}
	else{
		throw_compiler_error(loc, "Require a value for new bind.");
	}
}

std::pair<json_t, seq_t> parse_let(const seq_t& pos, const location_t& loc){
	const auto a_result = parse_a(pos, loc);
	if(a_result.rest.empty()){
		throw_compiler_error(loc, "Require a value for new bind.");
	}
	const auto equal_sign = read_required(skip_whitespace(a_result.rest), "=");
	const auto expression_pos = parse_expression(equal_sign);

	const auto params = std::vector<json_t>{
		typeid_to_ast_json(a_result.type, json_tags::k_tag_resolve_state),
		a_result.identifier,
		expression_pos.first,
	};
	const auto statement = make_parser_node(loc, parse_tree_statement_opcode::k_init_local, params);
	return { statement, expression_pos.second };
}

std::pair<json_t, seq_t> parse_mutable(const seq_t& pos, const location_t& loc){
	const auto a_result = parse_a(pos, loc);
	if(a_result.rest.empty()){
		throw_compiler_error(loc, "Require a value for new bind.");
	}
	const auto equal_sign = read_required(skip_whitespace(a_result.rest), "=");
	const auto expression_pos = parse_expression(equal_sign);

	const auto meta = (json_t::make_object({ std::pair<std::string, json_t>{"mutable", true } }));

	const auto params = std::vector<json_t>{
		typeid_to_ast_json(a_result.type, json_tags::k_tag_resolve_state),
		a_result.identifier,
		expression_pos.first,
		meta
	};
	const auto statement = make_parser_node(loc, parse_tree_statement_opcode::k_init_local, params);

	return { statement, expression_pos.second };
}

//	BIND:			let int x = 10
//	BIND:			let x = 10
//	BIND:			let int (string a) x = f(4 == 5)
//	BIND:			mutable int x = 10
//	BIND:			mutable x = 10

//	ERROR:			let int x
//	ERROR:			let x
//	ERROR:			mutable int x
//	ERROR:			mutable x

//	ERROR:			x = 10
//	[let]/[mutable] TYPE identifier = EXPRESSION
//					|<----------->|		call this section a.

std::pair<json_t, seq_t> parse_bind_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto loc = location_t(start.pos());

	const auto let_pos = if_first(start, keyword_t::k_let);
	if(let_pos.first){
		return parse_let(let_pos.second, loc);
	}

	const auto mutable_pos = if_first(skip_whitespace(s), keyword_t::k_mutable);
	if(mutable_pos.first){
		return parse_mutable(mutable_pos.second, loc);
	}

	throw_compiler_error(loc, "Bind syntax error.");
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	const auto input = R"(

		mutable string row

	)";

	try {
		ut_verify_json_and_rest(
			QUARK_POS,
			parse_bind_statement(seq_t(input)),
			R"(
				[ 0, "init-local", "^int", "test", ["k", 123, "^int"]]
			)",
			" let int a = 4 "
		);
		fail_test(QUARK_POS);
	}
	catch(const std::runtime_error& e){
		//	Should come here.
		ut_verify(QUARK_POS, e.what(), "Expected \'=\' character.");
	}
}


QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify_json_and_rest(
		QUARK_POS,
		parse_bind_statement(seq_t("let int test = 123 let int a = 4 ")),
		R"(
			[ 0, "init-local", "^int", "test", ["k", 123, "^int"]]
		)",
		" let int a = 4 "
	);
}



QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("let bool bb = true")).first,
		parse_json(seq_t(
			R"(
				[ 0, "init-local", "^bool", "bb", ["k", true, "^bool"]]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("let int hello = 3")).first,
		parse_json(seq_t(
			R"(
				[ 0, "init-local", "^int", "hello", ["k", 3, "^int"]]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("mutable int a = 14")).first,
		parse_json(seq_t(
			R"(
				[ 0, "init-local", "^int", "a", ["k", 14, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify(QUARK_POS,
		parse_bind_statement(seq_t("mutable hello = 3")).first,
		parse_json(seq_t(
			R"(
				[ 0, "init-local", "^**undef**", "hello", ["k", 3, "^int"], { "mutable": true }]
			)"
		)).first
	);
}

QUARK_UNIT_TEST("parse_bind_statement", "", "", ""){
	ut_verify_json_and_rest(
		QUARK_POS,
		parse_bind_statement(seq_t("let int (double, [string]) test = 123 let int a = 4 ")),
		R"(
			[0, "init-local", ["func", "^int", ["^double", ["vector", "^string"]], true], "test", ["k", 123, "^int"]]
		)",
		" let int a = 4 "
	);
}

//### test float literal
//### test string literal


//////////////////////////////////////////////////		parse_assign_statement()


std::pair<json_t, seq_t> parse_assign_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto variable_pos = read_identifier(start);
	if(variable_pos.first.empty()){
		throw_compiler_error(location_t(s.pos()), "Assign syntax error.");
	}
	const auto equal_pos = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto rhs_seq = skip_whitespace(equal_pos);
	const auto expression_fr = parse_expression(rhs_seq);

	const auto statement = make_parser_node(location_t(start.pos()), parse_tree_statement_opcode::k_assign, { variable_pos.first, expression_fr.first });
	return { statement, expression_fr.second };
}

QUARK_UNIT_TEST("", "parse_assign_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_assign_statement(seq_t("x = 10;")).first,
		parse_json(seq_t(
			R"(
				[ 0, "assign","x",["k",10,"^int"] ]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_expression_statement()


std::pair<json_t, seq_t> parse_expression_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto expression_fr = parse_expression(start);

	const auto statement = make_parser_node(location_t(start.pos()), parse_tree_statement_opcode::k_expression_statement, { expression_fr.first } );
	return { statement, expression_fr.second };
}

QUARK_UNIT_TEST("", "parse_expression_statement()", "", ""){
	ut_verify(QUARK_POS,
		parse_expression_statement(seq_t("print(14);")).first,
		parse_json(seq_t(
			R"(
				[ 0, "expression-statement", [ "call", ["@", "print"], [["k", 14, "^int"]] ] ]
			)"
		)).first
	);
}



//////////////////////////////////////////////////		parse_function_definition_statement()


parse_result_t parse_optional_statement_body(const seq_t& s){
	const auto bracket_pos = read_optional_char(skip_whitespace(s), '{');
	if(bracket_pos.first){
		const auto body = parse_statement_body(s);
		return body;
	}
	else{
		return { json_t(), s };
	}
}


std::pair<json_t, seq_t> parse_function_definition_statement(const seq_t& pos){
	const auto start = skip_whitespace(pos);
	const auto func_pos = read_required(start, keyword_t::k_func);
	const auto return_type_pos = read_required_type(func_pos);
	const auto function_name_pos = read_required_identifier(return_type_pos.second);
	const auto named_args_pos = read_functiondef_arg_parantheses(skip_whitespace(function_name_pos.second));

	const auto impure_pos = if_first(skip_whitespace(named_args_pos.second), keyword_t::k_impure);

	const auto body = parse_optional_statement_body(impure_pos.second);

	const auto named_args = members_to_json(named_args_pos.first);
	const auto function_name = function_name_pos.first;

	const auto body_json = body.parse_tree.is_null()
	? json_t()
	: json_t::make_object({
		{ "statements", body.parse_tree },
		{ "symbols", {} }
	});

	std::vector<typeid_t> arg_types;
	for(const auto& e: named_args_pos.first){
		arg_types.push_back(e._type);
	}

	const auto function_type = typeid_t::make_function(return_type_pos.first, arg_types, impure_pos.first ? epure::impure : epure::pure);
	const auto func_def_expr = make_parser_node(
		location_t(k_no_location),
		parse_tree_expression_opcode_t::k_function_def,
		{
			typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state),
			function_name,
			named_args,
			body_json
		}
	);

	const auto s = make_parser_node(
		location_t(start.pos()),
		parse_tree_statement_opcode::k_init_local,
		{
			typeid_to_ast_json(function_type, json_tags::k_tag_resolve_state),
			function_name,
			func_def_expr
		}
	);
	return { s, body.pos };
}

struct test {
	std::string desc;
	std::string input;
	std::string output;
};

QUARK_UNIT_TEST("", "parse_function_definition_statement()", "Minimal function IMPURE", ""){
	const std::string input = "func int f() impure{ return 3; }";
	const std::string expected = R"(
		[
			0,
			"init-local",
			["func", "^int", [], false],
			"f",
			["function-def", ["func", "^int", [], false], "f", [], { "statements": [[21, "return", ["k", 3, "^int"]]], "symbols": null }]
		]
	)";
	ut_verify(QUARK_POS, parse_function_definition_statement(seq_t(input)).first, parse_json(seq_t(expected)).first);
}


QUARK_UNIT_TEST("", "parse_function_definition_statement()", "function", "Correct output JSON"){
	ut_verify(
		QUARK_POS,
		parse_function_definition_statement(seq_t("func int f(){ return 3; }")).first,
		parse_json(seq_t(
			R"___(

				[
					0,
					"init-local",
					["func", "^int", [], true],
					"f",
					["function-def", ["func", "^int", [], true], "f", [], { "statements": [[14, "return", ["k", 3, "^int"]]], "symbols": null }]
				]

			)___"
		)).first
	);
}


QUARK_UNIT_TEST("", "parse_function_definition_statement()", "3 args of different types", "Correct output JSON"){
	ut_verify(
		QUARK_POS,
		parse_function_definition_statement(seq_t("func int printf(string a, double barry, int c){ return 3; }")).first,
		parse_json(seq_t(
			R"___(

				[
					0,
					"init-local",
					["func", "^int", ["^string", "^double", "^int"], true],
					"printf",
					[
						"function-def",
						["func", "^int", ["^string", "^double", "^int"], true],
						"printf",
						[{ "name": "a", "type": "^string" }, { "name": "barry", "type": "^double" }, { "name": "c", "type": "^int" }],
						{ "statements": [[48, "return", ["k", 3, "^int"]]], "symbols": null }
					]
				]

			)___"
		)).first
	);
}


QUARK_UNIT_TEST("", "parse_function_definition_statement()", "Max whitespace", "Correct output JSON"){
	ut_verify(
		QUARK_POS,
		parse_function_definition_statement(seq_t(" func  \t int \t printf( \t string \t a \t , \t double \t b \t ){ \t return \t 3 \t ; \t } \t ")).first,
		parse_json(seq_t(
			R"___(

				[
					1,
					"init-local",
					["func", "^int", ["^string", "^double"], true],
					"printf",
					[
						"function-def",
						["func", "^int", ["^string", "^double"], true],
						"printf",
						[{ "name": "a", "type": "^string" }, { "name": "b", "type": "^double" }],
						{ "statements": [[60, "return", ["k", 3, "^int"]]], "symbols": null }
					]
				]

			)___"
		)).first
	);
}

QUARK_UNIT_TEST("", "parse_function_definition_statement()", "Min whitespace", "Correct output JSON"){
	ut_verify(
		QUARK_POS,
		parse_function_definition_statement(seq_t("func int printf(string a,double b){return 3;}")).first,
		parse_json(seq_t(
			R"___(

				[
					0,
					"init-local",
					["func", "^int", ["^string", "^double"], true],
					"printf",
					[
						"function-def",
						["func", "^int", ["^string", "^double"], true],
						"printf",
						[{ "name": "a", "type": "^string" }, { "name": "b", "type": "^double" }],
						{ "statements": [[35, "return", ["k", 3, "^int"]]], "symbols": null }
					]
				]

			)___"
		)).first
	);
}



//////////////////////////////////////////////////		parse_struct_definition_statement()


std::pair<json_t, seq_t>  parse_struct_definition_body(const seq_t& p, const std::string& name, const location_t& location){
	const auto s2 = skip_whitespace(p);
	const auto start = s2;
	auto pos = read_required_char(s2, '{');
	std::vector<member_t> members;
	while(!pos.empty() && pos.first() != "}"){
		const auto member_type = read_required_type(pos);
		const auto member_name = read_required_identifier(member_type.second);
		members.push_back(member_t(member_type.first, member_name.first));
		pos = read_optional_char(skip_whitespace(member_name.second), ';').second;
		pos = skip_whitespace(pos);
	}
	pos = read_required(pos, "}");

	const auto struct_def_expr = make_parser_node(
		location_t(k_no_location),
		parse_tree_expression_opcode_t::k_struct_def,
		{
			name,
			members_to_json(members)
		}
	);

	const auto struct_type = typeid_t::make_struct2(members);
	const auto s = make_parser_node(
		location_t(start.pos()),
		parse_tree_statement_opcode::k_init_local,
		{
			typeid_to_ast_json(typeid_t::make_typeid(), json_tags::k_tag_resolve_state),
			name,
			struct_def_expr
		}
	);
	return { s, pos };

}

std::pair<json_t, seq_t>  parse_struct_definition_statement(const seq_t& pos0){
	std::pair<bool, seq_t> token_pos = if_first(pos0, keyword_t::k_struct);
	QUARK_ASSERT(token_pos.first);

	const auto struct_name_pos = read_required_identifier(token_pos.second);
	const auto location = location_t(pos0.pos());

	const auto s2 = skip_whitespace(struct_name_pos.second);
	const auto b = parse_struct_definition_body(s2, struct_name_pos.first, location);
	return b;
}


QUARK_UNIT_TEST("parser", "parse_struct_definition_statement", "", ""){
	const auto r = parse_struct_definition_statement(seq_t("struct a {int x; string y; double z;}"));

	const auto expected = parse_json(seq_t(
		R"___(

			[
				9,
				"init-local",
				"^typeid",
				"a",
				["struct-def", "a", [{ "name": "x", "type": "^int" }, { "name": "y", "type": "^string" }, { "name": "z", "type": "^double" }]]
			]

		)___"
	)).first;

	ut_verify(QUARK_POS, r.first, expected);
	ut_verify(QUARK_POS, r.second.str(), "");
}


#if 0
//////////////////////////////////////////////////		parse_protocol_definition_statement()


std::pair<json_t, seq_t>  parse_protocol_definition_body(const seq_t& p, const std::string& name){
	const auto start = skip_whitespace(p);
	read_required_char(start, '{');
	const auto body_pos = get_balanced(start);

	std::vector<member_t> functions;

	auto pos = seq_t(trim_ends(body_pos.first));
	while(!pos.empty()){
		const auto func_pos = read_required(skip_whitespace(pos), keyword_t::k_func);
		const auto return_type_pos = read_required_type(func_pos);
		const auto function_name_pos = read_required_identifier(return_type_pos.second);
		const auto args_pos = read_functiondef_arg_parantheses(function_name_pos.second);
		pos = read_optional_char(skip_whitespace(args_pos.second), ';').second;
		pos = skip_whitespace(pos);


		std::vector<typeid_t> arg_types;
		for(const auto& e: args_pos.first){
			arg_types.push_back(e._type);
		}
		const member_t f = {
			typeid_t::make_function(return_type_pos.first, arg_types, epure::pure),
			function_name_pos.first
		};
		functions.push_back(f);
	}

	const auto r = make_ast_node(
		location_t(start.pos()),
		parse_tree_statement_opcode::k_def_protocol,
		{
			json_t::make_object({
				{ "name", name },
				{ "members", members_to_json(functions)
				}
			})
		}
	);
	return { r, skip_whitespace(body_pos.second) };
}

std::pair<json_t, seq_t>  parse_protocol_definition_statement(const seq_t& p){
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

OFF_QUARK_UNIT_TEST("parse_protocol_definition_statement", "", "", ""){
	const auto r = parse_protocol_definition_statement(seq_t(k_test_protocol0));

	const auto expected =
	json_t::make_array({
		17,
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
							json_t::make_array({ "func", "^int", json_t::make_array({"^string", "^double"}), true })
						}
					}),
					json_t::make_object({
						{ "name", "g"},
						{
							"type",
							json_t::make_array({ "func", "^string", json_t::make_array({"^bool"}), true })
						}
					})
				})
			},
		})
	});

	ut_verify(QUARK_POS, r.first, expected);
	ut_verify(QUARK_POS, r.second.str(), "");
}
#endif


//////////////////////////////////////////////////		parse_if()


/*
	Parse: if (EXPRESSION) { THEN_STATEMENTS }
	if(a){
		a
	}
*/
std::pair<json_t, seq_t> parse_if(const seq_t& pos){
	const auto start = skip_whitespace(pos);
	const auto a = if_first(start, keyword_t::k_if);
	QUARK_ASSERT(a.first);

	const auto condition = read_enclosed_in_parantheses(a.second);
	const auto then_body = parse_statement_body(condition.second);
	const auto condition2 = parse_expression(seq_t(condition.first));

	return {
		make_parser_node(location_t(start.pos()), parse_tree_statement_opcode::k_if, { condition2.first, then_body.parse_tree } ),
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
std::pair<json_t, seq_t> parse_if_statement(const seq_t& pos){
	const auto start = skip_whitespace(pos);

	const auto if_statement2 = parse_if(start);
	std::pair<bool, seq_t> else_start = if_first(skip_whitespace(if_statement2.second), keyword_t::k_else);
	if(else_start.first){
		const auto pos2 = skip_whitespace(else_start.second);
		std::pair<bool, seq_t> elseif_pos = if_first(pos2, keyword_t::k_if);

		if(elseif_pos.first){
			const auto elseif_statement2 = parse_if_statement(pos2);

			return {
				make_parser_node(
					location_t(start.pos()),
					parse_tree_statement_opcode::k_if,
					{
						if_statement2.first.get_array_n(2),
						if_statement2.first.get_array_n(3),
						json_t::make_array({elseif_statement2.first})
					}
				),
				elseif_statement2.second
			};
		}
		else{
			const auto else_body = parse_statement_body(pos2);

			return {
				make_parser_node(
					location_t(start.pos()),
					parse_tree_statement_opcode::k_if,
					{
						if_statement2.first.get_array_n(2),
						if_statement2.first.get_array_n(3),
						else_body.parse_tree
					}
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
	ut_verify(QUARK_POS,
		parse_if_statement(seq_t("if (1 > 2) { return 3 }")).first,
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
	ut_verify(QUARK_POS,
		parse_if_statement(seq_t("if (1 > 2) { return 3 } else { return 4 }")).first,
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
	ut_verify(QUARK_POS,
		parse_if_statement(seq_t("if (1 > 2) { return 3 } else { return 4 }")).first,
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
	ut_verify(QUARK_POS,
		parse_if_statement(
			seq_t("if (1 == 1) { return 1 } else if(2 == 2) { return 2 } else if(3 == 3) { return 3 } else { return 4 }")
		).first,
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

	throw_compiler_error(location_t(pos.pos()), "For loop has illegal range syntax.");
}


std::pair<json_t, seq_t> parse_for_statement(const seq_t& pos){
	std::pair<bool, seq_t> for_pos = if_first(pos, keyword_t::k_for);
	QUARK_ASSERT(for_pos.first);

	//	header == " index in 1 ... 5 "
	const auto header = read_enclosed_in_parantheses(for_pos.second);

	const auto body = parse_statement_body(header.second);

	//	iterator == "index".
	const auto iterator_name = read_required_identifier(seq_t(header.first));
	if(iterator_name.first.empty()){
		throw_compiler_error(location_t(pos.pos()), "For loop requires iterator name.");
	}

	const auto in_str = read_required(skip_whitespace(iterator_name.second), "in");

	const auto range = skip_whitespace(in_str);

	const auto range_parts = parse_range(range);
	const auto start = range_parts._start;
	const auto range_type = range_parts._range_type;
	const auto end_pos = range_parts._end_pos;

	const auto start_expr = parse_expression(seq_t(start)).first;
	const auto end_expr = parse_expression(end_pos).first;

	const auto r = make_parser_node(
		location_t(pos.pos()),
		parse_tree_statement_opcode::k_for,
		{
			range_type == "..." ? "closed-range" : "open-range",
			iterator_name.first,
			start_expr,
			end_expr,
			body.parse_tree
		}
	);
	return { r, body.pos };
}

QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_verify(QUARK_POS,
		parse_for_statement(seq_t("for ( index in 1...5 ) { let int y = 11 }")).first,
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
						[25, "init-local","^int","y",["k",11,"^int"]]
					]
				]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_for_statement()", "for(){}", ""){
	ut_verify(QUARK_POS,
		parse_for_statement(seq_t("for ( index in 1..<5 ) { let int y = 11 }")).first,
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
						[25, "init-local","^int","y",["k",11,"^int"]]
					]
				]
			)"
		)).first
	);
}


//////////////////////////////////////////////////		parse_while_statement()


std::pair<json_t, seq_t> parse_while_statement(const seq_t& pos){
	std::pair<bool, seq_t> pos2 = if_first(pos, keyword_t::k_while);
	QUARK_ASSERT(pos2.first);

	const auto condition = read_enclosed_in_parantheses(pos2.second);
	const auto body = parse_statement_body(condition.second);

	const auto condition_expr = parse_expression(seq_t(condition.first)).first;
	const auto r = make_parser_node(
		location_t(pos.pos()),
		parse_tree_statement_opcode::k_while,
		{
			condition_expr,
			body.parse_tree
		}
	);
	return { r, body.pos };
}

QUARK_UNIT_TEST("", "parse_while_statement()", "while(){}", ""){
	ut_verify(QUARK_POS, 
		parse_while_statement(seq_t("while (a < 10) { print(a) }")).first,
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



//////////////////////////////////////////////////		parse_benchmark_def_statement()



std::pair<json_t, seq_t> parse_benchmark_def_statement(const seq_t& pos0){
	const auto pos = skip_whitespace(pos0);
	std::pair<bool, seq_t> pos2 = if_first(pos, keyword_t::k_benchmark_def);
	QUARK_ASSERT(pos2.first);

//	const auto name = parse_expression(pos2.second);
	const auto name_pos = parse_string_literal(skip_whitespace(pos2.second));
	const auto body = parse_statement_body(name_pos.second);

	const auto r = make_parser_node(
		location_t(pos.pos()),
		parse_tree_statement_opcode::k_benchmark_def,
		{
			name_pos.first,
			body.parse_tree
		}
	);
	return { r, body.pos };
}

QUARK_UNIT_TEST("", "parse_benchmark_def_statement()", "while(){}", ""){
	ut_verify(QUARK_POS,
		parse_benchmark_def_statement(seq_t(R"___(

			benchmark-def "Linear veq 0" {
				print(1234)
				return 2000
			}

		)___")).first,
		parse_json(seq_t(
			R"(
				[
					5,
					"benchmark-def",
					"Linear veq 0",
					[[40, "expression-statement", ["call", ["@", "print"], [["k", 1234, "^int"]]]], [56, "return", ["k", 2000, "^int"]]]
				]
			)"
		)).first
	);
}




//////////////////////////////////////////////////		parse_software_system_statement()

/*
	software-system-def: JSON
*/

std::pair<json_t, seq_t> parse_software_system_def_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto loc = location_t(start.pos());
	const auto ss_pos = if_first(start, keyword_t::k_software_system);
	if(ss_pos.first == false){
		throw_compiler_error(loc, "Syntax error.");
	}

	//??? Instead of parsing a static JSON literal, we could parse a Floyd expression that results in a JSON value = use variables etc.
	std::pair<json_t, seq_t> json_pos = parse_json(ss_pos.second);
	const auto r = make_parser_node(loc, parse_tree_statement_opcode::k_software_system_def, { json_pos.first } );
	return { r, json_pos.second };
}


//////////////////////////////////////////////////		parse_container_def()

/*
	container-def: JSON
*/

std::pair<json_t, seq_t> parse_container_def_statement(const seq_t& s){
	const auto start = skip_whitespace(s);
	const auto loc = location_t(start.pos());
	const auto ss_pos = if_first(start, keyword_t::k_container_def);
	if(ss_pos.first == false){
		throw_compiler_error(loc, "Syntax error.");
	}

	//??? Instead of parsing a static JSON literal, we could parse a Floyd expression that results in a JSON value = use variables etc.
	std::pair<json_t, seq_t> json_pos = parse_json(ss_pos.second);

	const auto r = make_parser_node(loc, parse_tree_statement_opcode::k_container_def, { json_pos.first } );
	return { r, json_pos.second };
}

}	// parser
}	//	floyd
