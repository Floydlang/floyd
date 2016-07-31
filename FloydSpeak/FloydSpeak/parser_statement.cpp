//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_statement.hpp"



#include "parser_types.h"
#include "parser_expression.hpp"
#include "parser_function.h"
#include "parser_struct.h"


namespace floyd_parser {

	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;


	statement_t make__bind_statement(const bind_statement_t& value){
		return statement_t(value);
	}

	statement_t make__bind_statement(const std::string& identifier, const expression_t& e){
		return statement_t(bind_statement_t{identifier, std::make_shared<expression_t>(e)});
	}

	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}


	statement_t makie_return_statement(const expression_t& expression){
		return statement_t(return_statement_t{make_shared<expression_t>(expression)});
	}



	void trace(const statement_t& s){
		if(s._bind_statement){
			std::string t = "bind_statement_t: \"" + s._bind_statement->_identifier + "\"";
			QUARK_SCOPED_TRACE(t);
			trace(*s._bind_statement->_expression);
		}

		else if(s._define_struct){
			QUARK_SCOPED_TRACE("define_struct_statement_t: \"" + s._define_struct->_type_identifier);
			trace(s._define_struct->_struct_def);
		}
		else if(s._define_function){
			QUARK_SCOPED_TRACE("define_function_statement_t: \"" + s._define_function->_type_identifier);
			trace(s._define_function->_function_def);
		}

		else if(s._return_statement){
			QUARK_SCOPED_TRACE("return_statement_t");
			trace(*s._return_statement->_expression);
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		statement_t



	bool statement_t::check_invariant() const {
		if(_bind_statement){
			QUARK_ASSERT(_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_struct){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_function){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_return_statement){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(_return_statement);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}






pair<return_statement_t, string> parse_return_statement(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_return_statement()");
	QUARK_ASSERT(s.size() >= string("return").size());

	QUARK_ASSERT(peek_string(s, "return"));

	const auto token_pos = read_until(s, whitespace_chars);
	const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
	const auto expression1 = parse_expression(ast, expression_pos.first);
//			const auto expression2 = evaluate3(local_scope, expression1);
	const auto statement = return_statement_t{ make_shared<expression_t>(expression1) };

	//	Skip trailing ";".
	const auto pos = skip_whitespace(expression_pos.second.substr(1));
	return pair<return_statement_t, string>(statement, pos);
}


QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto t = make_shared<expression_t>(make_constant(value_t(0)));
	
	QUARK_TEST_VERIFY((
		parse_return_statement({}, "return 0;") == pair<return_statement_t, string>(return_statement_t{t}, "")
	));
}

QUARK_UNIT_TESTQ("parse_return_statement()", ""){
	const auto t = make_shared<expression_t>(make_constant(value_t(123)));
	
	QUARK_TEST_VERIFY((
		parse_return_statement({}, "return \t123\t;\t\nxyz}") == pair<return_statement_t, string>(return_statement_t{t}, "xyz}")
	));
}







/*
	Returns
		[bind_statement_t]
			[expression_t]
*/

pair<statement_t, string> parse_assignment_statement(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_assignment_statement()");
	QUARK_ASSERT(ast.check_invariant());

	const auto token_pos = read_until(s, whitespace_chars);
	const auto type = make_type_identifier(token_pos.first);
	QUARK_ASSERT(ast.parser_i__is_known_type(read_until(s, whitespace_chars).first));

	const auto variable_pos = read_until(skip_whitespace(token_pos.second), whitespace_chars + "=");
	const auto equal_rest = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until(skip_whitespace(equal_rest), ";");

	const auto expression = parse_expression(ast, expression_pos.first);

	const auto statement = make__bind_statement(variable_pos.first, expression);
	trace(statement);

	//	Skip trailing ";".
	return { statement, expression_pos.second.substr(1) };
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "int"){
	const auto a = parse_assignment_statement(ast_t(), "int a = 10; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "a");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(10));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "float"){
	const auto a = parse_assignment_statement(make_test_ast(), "float b = 0.3; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "b");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_constant == value_t(0.3f));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "function call"){
	const auto ast = make_test_ast();
	const auto a = parse_assignment_statement(ast, "float test = log(\"hello\");\n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression->_call_function_expr->_inputs[0]->_constant ==value_t("hello"));
	QUARK_TEST_VERIFY(a.second == "\n");
}





}	//	floyd_parser
