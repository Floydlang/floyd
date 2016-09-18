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
