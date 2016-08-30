//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_expression.h"

#include "text_parser.h"
#include "statements.h"
#include "parser_value.h"
#include "parse_function_def.h"
#include "parser_ast.h"
#include "parser_primitives.h"
#include "parser2.h"

namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;




struct parse_helper : public maker<expression_t> {
	private: static const std::map<eoperation, string> _2_operator_to_string;

	private: static string make_2op(string lhs, string op, string rhs){
		return make3(quote(op), lhs, rhs);
	}

	private: static string make2(string s0, string s1){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << "]";
		return ss.str();
	}

	private: static string make3(string s0, string s1, string s2){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << ", " << s2 << "]";
		return ss.str();
	}


	public: virtual const expression_t maker__make_identifier(const std::string& s) const{
		return expression_t::make_resolve_variable(s);
	}

	public: virtual const expression_t maker__make1(const eoperation op, const expression_t& expr) const{
		if(op == eoperation::k_1_logical_not){
			return expression_t::make_math_operation1(math_operation1_expr_t::operation::negate, expr);
		}
/*
		else if(op == eoperation::k_1_load){
			return expr;
		}
*/
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const expression_t maker__make2(const eoperation op, const expression_t& lhs, const expression_t& rhs) const{
//		const auto op_str = _2_operator_to_string.at(op);
//		return make3(quote(op_str), lhs, rhs);

		if(op == eoperation::k_2_looup){
			return expression_t::make_lookup(lhs, rhs);
		}
		else if(op == eoperation::k_2_add){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_add, lhs, rhs);
		}
		else if(op == eoperation::k_2_subtract){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_subtract, lhs, rhs);
		}
		else if(op == eoperation::k_2_multiply){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_multiply, lhs, rhs);
		}
		else if(op == eoperation::k_2_divide){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_divide, lhs, rhs);
		}
		else if(op == eoperation::k_2_remainder){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_remainder, lhs, rhs);
		}

		else if(op == eoperation::k_2_smaller_or_equal){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_smaller_or_equal, lhs, rhs);
		}
		else if(op == eoperation::k_2_smaller){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_smaller, lhs, rhs);
		}
		else if(op == eoperation::k_2_larger_or_equal){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_larger_or_equal, lhs, rhs);
		}
		else if(op == eoperation::k_2_larger){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_larger, lhs, rhs);
		}


		else if(op == eoperation::k_2_logical_equal){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_logical_equal, lhs, rhs);
		}
		else if(op == eoperation::k_2_logical_nonequal){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_logical_nonequal, lhs, rhs);
		}
		else if(op == eoperation::k_2_logical_and){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_logical_and, lhs, rhs);
		}
		else if(op == eoperation::k_2_logical_or){
			return expression_t::make_math_operation2(math_operation2_expr_t::operation::k_logical_or, lhs, rhs);
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const expression_t maker__make3(const eoperation op, const expression_t& e1, const expression_t& e2, const expression_t& e3) const{
		if(op == eoperation::k_3_conditional_operator){
			return expression_t::make_conditional_operator(e1, e2, e3);
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const expression_t maker__call(const expression_t& f, const std::vector<expression_t>& args) const{
		if(f._resolve_variable){
			return expression_t::make_function_call(type_identifier_t::make(f._resolve_variable->_variable_name), args);
		}
		else{
			throw std::runtime_error("??? function names must be constant identifiers right now. Broken");
		}
	}

	public: virtual const expression_t maker__member_access(const expression_t& address, const std::string& member_name) const{
		return expression_t::make_resolve_member(make_shared<expression_t>(address), member_name);
	}

	public: virtual const expression_t maker__make_constant(const constant_value_t& value) const{
		if(value._type == constant_value_t::etype::k_bool){
			return expression_t::make_constant(value._bool);
		}
		else if(value._type == constant_value_t::etype::k_int){
			return expression_t::make_constant(value._int);
		}
		else if(value._type == constant_value_t::etype::k_float){
			return expression_t::make_constant(value._float);
		}
		else if(value._type == constant_value_t::etype::k_string){
			return expression_t::make_constant(value._string);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
};

const std::map<eoperation, string> parse_helper::_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_looup, "[-]" },

	{ eoperation::k_2_add, "+" },
	{ eoperation::k_2_subtract, "-" },
	{ eoperation::k_2_multiply, "*" },
	{ eoperation::k_2_divide, "/" },
	{ eoperation::k_2_remainder, "%" },

	{ eoperation::k_2_smaller_or_equal, "<=" },
	{ eoperation::k_2_smaller, "<" },
	{ eoperation::k_2_larger_or_equal, ">=" },
	{ eoperation::k_2_larger, ">" },

	{ eoperation::k_2_logical_equal, "==" },
	{ eoperation::k_2_logical_nonequal, "!=" },
	{ eoperation::k_2_logical_and, "&&" },
	{ eoperation::k_2_logical_or, "||" },
};


std::pair<expression_t, seq_t> parse_expression(const seq_t& expression){
	parse_helper helper;
	return parse_expression(helper, expression);
}

expression_t parse_expression(std::string expression){
	parse_helper helper;

	const auto result = parse_expression(helper, seq_t(expression));
	if(!skip_whitespace(result.second).empty()){
		throw std::runtime_error("All of expression not used");
	}
	return result.first;
}



//////////////////////////////////////////////////		test rig



ast_t make_test_ast(){
	ast_t result;
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "log", make_log_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "log2", make_log2_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "f", make_log_function(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_function_type(result._global_scope->_types_collector, "return5", make_return5(result._global_scope)));

	result._global_scope = result._global_scope->set_types(define_struct_type(result._global_scope->_types_collector, "test_struct0", make_struct0(result._global_scope)));
	result._global_scope = result._global_scope->set_types(define_struct_type(result._global_scope->_types_collector, "test_struct1", make_struct1(result._global_scope)));
	return result;
}

QUARK_UNIT_TESTQ("make_test_ast()", ""){
	auto a = make_test_ast();
	QUARK_TEST_VERIFY(*resolve_struct_type(a._global_scope->_types_collector, "test_struct0") == *make_struct0(a._global_scope));
	QUARK_TEST_VERIFY(*resolve_struct_type(a._global_scope->_types_collector, "test_struct1") == *make_struct1(a._global_scope));
}



}	//	floyd_parser
