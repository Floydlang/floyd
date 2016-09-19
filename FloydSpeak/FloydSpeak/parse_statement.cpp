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
#include "json_support.h"

namespace floyd_parser {
	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;


	/*
		["return", EXPRESSION ]
	*/
	pair<json_value_t, string> parse_return_statement(const string& s){
		QUARK_SCOPED_TRACE("parse_return_statement()");
		QUARK_ASSERT(s.size() >= string("return").size());

		QUARK_ASSERT(peek_string(s, "return"));

		const auto token_pos = read_until(s, whitespace_chars);
		const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
		const auto expression1 = parse_expression_all(expression_pos.first);
//		const auto statement = return_statement_t{ make_shared<expression_t>(expression1) };
		const auto statement = json_value_t::make_array2({ json_value_t("return"), expression1 });
		//	Skip trailing ";".
		const auto pos = skip_whitespace(expression_pos.second.substr(1));
		return pair<json_value_t, string>(statement, pos);
	}

#if false
	QUARK_UNIT_TESTQ("parse_return_statement()", ""){
		const auto t = make_shared<expression_t>(expression_t::make_constant(0));
		
		QUARK_TEST_VERIFY((
			parse_return_statement("return 0;") == pair<return_statement_t, string>(return_statement_t{t}, "")
		));
	}

	QUARK_UNIT_TESTQ("parse_return_statement()", ""){
		const auto t = make_shared<expression_t>(expression_t::make_constant(123));
		
		QUARK_TEST_VERIFY((
			parse_return_statement("return \t123\t;\t\nxyz}") == pair<return_statement_t, string>(return_statement_t{t}, "xyz}")
		));
	}
#endif


	/*
		"int a = 13";
		"int b = f("hello");"
		"bool a = is_hello("hello")";

		Returns
			[bind_statement_t]
				[expression_t]
	*/

	pair<json_value_t, string> parse_assignment_statement(const string& s){
		QUARK_SCOPED_TRACE("parse_assignment_statement()");

		const auto token_pos = read_until(s, whitespace_chars);
		const auto type = token_pos.first;

		const auto variable_pos = read_until(skip_whitespace(token_pos.second), whitespace_chars + "=");
		const auto equal_rest = read_required_char(skip_whitespace(variable_pos.second), '=');
		const auto expression_pos = read_until(skip_whitespace(equal_rest), ";");

		const auto expression = parse_expression_all(expression_pos.first);

//		const auto statement = make__bind_statement(type, variable_pos.first, expression);
//		trace(statement);
		const auto statement = json_value_t::make_array2({ "bind", "<" + type + ">", variable_pos.first, expression });

		//	Skip trailing ";".
		return { statement, expression_pos.second.substr(1) };
	}

#if false
	QUARK_UNIT_TESTQ("parse_assignment_statement", "bool true"){
		const auto a = parse_assignment_statement("bool bb = true; \n");
		QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "bb");
		QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(true));
		QUARK_TEST_VERIFY(a.second == " \n");
	}

	QUARK_UNIT_TESTQ("parse_assignment_statement", "bool false"){
		const auto a = parse_assignment_statement("bool bb = false; \n");
		QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "bb");
		QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(false));
		QUARK_TEST_VERIFY(a.second == " \n");
	}

	QUARK_UNIT_TESTQ("parse_assignment_statement", "int"){
		const auto a = parse_assignment_statement("int a = 10; \n");
		QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "a");
		QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(10));
		QUARK_TEST_VERIFY(a.second == " \n");
	}

	QUARK_UNIT_TESTQ("parse_assignment_statement", "float"){
		const auto a = parse_assignment_statement("float b = 0.3; \n");
		QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "b");
		QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(0.3f));
		QUARK_TEST_VERIFY(a.second == " \n");
	}

	QUARK_UNIT_TESTQ("parse_assignment_statement", "function call"){
		const auto a = parse_assignment_statement("float test = log(\"hello\");\n");
		QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "test");
		QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call->_function.to_string() == "log");
		QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call->_inputs.size() == 1);
		QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_call->_inputs[0]._constant ==value_t("hello"));
		QUARK_TEST_VERIFY(a.second == "\n");
	}
#endif

}	//	floyd_parser
