//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_expression.h"

#include "parser2.h"
#include "json_support.h"
#include "floyd_basics.h"

namespace floyd {


using std::pair;
using std::string;
using std::vector;

using namespace parser2;



/////////////////////////////////		TO JSON


const std::map<eoperation, string> k_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_looup, "[]" },

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


json_t expr_to_json(const expr_t& e);

static json_t op2_to_json(eoperation op, const expr_t& expr0, const expr_t& expr1){
	const auto op_str = k_2_operator_to_string.at(op);
	return json_t::make_array({ json_t(op_str), expr_to_json(expr0), expr_to_json(expr1) });
}


json_t expr_vector_to_json_array(const vector<expr_t>& v){
	vector<json_t> v2;
	for(const auto e: v){
		v2.push_back(expr_to_json(e));
	}
	return v2;
}

json_t expr_to_json(const expr_t& e){
	if(e._op == eoperation::k_0_number_constant){
		const auto value = *e._constant;
		if(value._type == constant_value_t::etype::k_bool){
			return make_array_skip_nulls({ json_t("k"), json_t(value._bool), json_t(keyword_t::k_bool) });
		}
		else if(value._type == constant_value_t::etype::k_int){
			return make_array_skip_nulls({ json_t("k"), json_t((double)value._int), json_t(keyword_t::k_int) });
		}
		else if(value._type == constant_value_t::etype::k_float){
			return make_array_skip_nulls({ json_t("k"), json_t(value._float), json_t(keyword_t::k_float) });
		}
		else if(value._type == constant_value_t::etype::k_string){
			//	 Use k_0_string_literal!
			return make_array_skip_nulls({ json_t("k"), json_t(value._string), json_t(keyword_t::k_string) });
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._op == eoperation::k_0_resolve){
		return make_array_skip_nulls({ json_t("@"), json_t(), json_t(e._identifier) });
	}
	else if(e._op == eoperation::k_0_string_literal){
		const auto value = *e._constant;
		QUARK_ASSERT(value._type == constant_value_t::etype::k_string);
		return make_array_skip_nulls({ json_t("k"), json_t(value._string), json_t(keyword_t::k_string) });
	}
	else if(e._op == eoperation::k_x_member_access){
		return make_array_skip_nulls({ json_t("->"), json_t(), expr_to_json(e._exprs[0]), json_t(e._identifier) });
	}
	else if(e._op == eoperation::k_2_looup){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_add){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_subtract){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_multiply){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_divide){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_remainder){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller_or_equal){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger_or_equal){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_equal){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_nonequal){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_and){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_or){
		return op2_to_json(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_3_conditional_operator){
		return json_t::make_array({ json_t("?:"), expr_to_json(e._exprs[0]), expr_to_json(e._exprs[1]), expr_to_json(e._exprs[2]) });
	}
	else if(e._op == eoperation::k_n_call){
		vector<json_t> args;
		for(auto i = 0 ; i < e._exprs.size() - 1 ; i++){
			const auto& arg = expr_to_json(e._exprs[i + 1]);
			args.push_back(arg);
		}
		return make_array_skip_nulls({ json_t("call"), expr_to_json(e._exprs[0]), json_t(), args });
	}
	else if(e._op == eoperation::k_1_unary_minus){
		return make_array_skip_nulls({ json_t("unary_minus"), json_t(), expr_to_json(e._exprs[0]) });
	}
	else if(e._op == eoperation::k_1_vector_definition){
		return json_t::make_array({ json_t("vector-def"), e._identifier, expr_vector_to_json_array(e._exprs) });
	}
	else if(e._op == eoperation::k_1_dict_definition){
		return json_t::make_array({ json_t("dict-def"), e._identifier, expr_vector_to_json_array(e._exprs) });
	}
	else{
		QUARK_ASSERT(false)
		return "";
	}
}

json_t parse_expression_all(const seq_t& expression){
	const auto result = parse_expression_seq(expression);
	if(!parser2::skip_expr_whitespace(result.second).empty()){
		throw std::runtime_error("All of expression not used");
	}
	return result.first;
}

std::pair<json_t, seq_t> parse_expression_seq(const seq_t& expression){
	const auto expr = parse_expression(expression);
	const auto json = expr_to_json(expr.first);
	return { json, expr.second };
}


}	//	floyd
