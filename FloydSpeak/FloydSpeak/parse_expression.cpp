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

#include "json_support.h"

namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;



using namespace parser2;

struct parse_helper : public maker<expression_t> {
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

/*
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
*/



/////////////////////////////////		TO JSON


const std::map<eoperation, string> k_2_operator_to_string{
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

template<typename EXPRESSION>
struct json_helper : public maker<EXPRESSION> {
	public: virtual const EXPRESSION maker__make_identifier(const std::string& s) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("@"), json_value_t(), json_value_t(s) });
	}
	public: virtual const EXPRESSION maker__make1(const eoperation op, const EXPRESSION& expr) const{
		if(op == eoperation::k_1_logical_not){
			return json_value_t::make_array_skip_nulls({ json_value_t("neg"), json_value_t(), expr });
		}
		else if(op == eoperation::k_1_load){
			return json_value_t::make_array_skip_nulls({ json_value_t("load"), json_value_t(), expr });
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const EXPRESSION maker__make2(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const{
		const auto op_str = k_2_operator_to_string.at(op);
		return json_value_t::make_array2({ json_value_t(op_str), lhs, rhs });
	}
	public: virtual const EXPRESSION maker__make3(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const{
		if(op == eoperation::k_3_conditional_operator){
			return json_value_t::make_array2({ json_value_t("?:"), e1, e2, e3 });
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	public: virtual const EXPRESSION maker__call(const EXPRESSION& f, const std::vector<EXPRESSION>& args) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("call"), json_value_t(f), json_value_t(), args });
	}

	public: virtual const EXPRESSION maker__member_access(const EXPRESSION& address, const std::string& member_name) const{
		return json_value_t::make_array_skip_nulls({ json_value_t("->"), json_value_t(), address, json_value_t(member_name) });
	}

	public: virtual const EXPRESSION maker__make_constant(const constant_value_t& value) const{
		if(value._type == constant_value_t::etype::k_bool){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t(value._bool), json_value_t("<bool>") });
		}
		else if(value._type == constant_value_t::etype::k_int){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t((double)value._int), json_value_t("<int>") });
		}
		else if(value._type == constant_value_t::etype::k_float){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t(value._float), json_value_t("<float>") });
		}
		else if(value._type == constant_value_t::etype::k_string){
			return json_value_t::make_array_skip_nulls({ json_value_t("k"), json_value_t(value._string), json_value_t("<string>") });
		}
		else{
			QUARK_ASSERT(false);
		}
	}
};

json_value_t parse_expression_all(std::string expression){
	const auto result = parse_expression_seq(seq_t(expression));
	if(!parser2::skip_whitespace(result.second).empty()){
		throw std::runtime_error("All of expression not used");
	}
	return result.first;
}

std::pair<json_value_t, seq_t> parse_expression_seq(const seq_t& expression){
	json_helper<json_value_t> helper;
	return parse_expression_template<json_value_t>(helper, expression);
}



}	//	floyd_parser
