//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_expression.h"

#include "floyd_basics.h"
#include "parser2.h"
#include "json_support.h"
#include "ast_typeid.h"		//???	 remove -- instead use hardcoded strings.
#include "ast_typeid_helpers.h"
#include "parser_primitives.h"

namespace floyd {


using std::pair;
using std::string;
using std::vector;

using namespace parser2;



/////////////////////////////////		TO JSON


const std::map<eoperation, string> k_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_lookup, "[]" },

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


ast_json_t expr_to_json(const expr_t& e);

static ast_json_t op2_to_json(const location_t& location, eoperation op, const expr_t& expr0, const expr_t& expr1){
	const auto op_str = k_2_operator_to_string.at(op);


	return make_expression2(location, op_str, expr_to_json(expr0)._value, expr_to_json(expr1)._value);
}


ast_json_t expr_vector_to_json_array(const vector<expr_t>& v){
	vector<json_t> v2;
	for(const auto& e: v){
		v2.push_back(expr_to_json(e)._value);
	}
	return ast_json_t::make(v2);
}

ast_json_t expr_to_json(const expr_t& e){
	if(e._op == eoperation::k_0_number_constant){
		const auto value = *e._constant;
		if(value._type == constant_value_t::etype::k_bool){
			return make_expression2(e._location, "k", json_t(value._bool), typeid_to_ast_json(typeid_t::make_bool(), json_tags::k_tag_resolve_state)._value);
		}
		else if(value._type == constant_value_t::etype::k_int){
			return make_expression2(e._location, "k", json_t((double)value._int), typeid_to_ast_json(typeid_t::make_int(), json_tags::k_tag_resolve_state)._value);
		}
		else if(value._type == constant_value_t::etype::k_double){
			return make_expression2(e._location, "k", json_t(value._double), typeid_to_ast_json(typeid_t::make_double(), json_tags::k_tag_resolve_state)._value);
		}
		else if(value._type == constant_value_t::etype::k_string){
			//	 Use k_0_string_literal!
			return make_expression2(e._location, "k", json_t(value._string), typeid_to_ast_json(typeid_t::make_string(), json_tags::k_tag_resolve_state)._value);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else if(e._op == eoperation::k_0_resolve){
		return make_expression1(e._location, "@", json_t(e._identifier));
	}
	else if(e._op == eoperation::k_0_string_literal){
		const auto value = *e._constant;
		QUARK_ASSERT(value._type == constant_value_t::etype::k_string);
		return make_expression2(e._location, "k", json_t(value._string), typeid_to_ast_json(typeid_t::make_string(), json_tags::k_tag_resolve_state)._value);
	}
	else if(e._op == eoperation::k_x_member_access){
//		return ast_json_t{make_array_skip_nulls({ json_t("->"), json_t(), expr_to_json(e._exprs[0])._value, json_t(e._identifier) })};
		return make_expression2(e._location, "->", expr_to_json(e._exprs[0])._value, json_t(e._identifier));
	}
	else if(e._op == eoperation::k_2_lookup){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_add){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_subtract){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_multiply){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_divide){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_remainder){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller_or_equal){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger_or_equal){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_equal){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_nonequal){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_and){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_or){
		return op2_to_json(e._location, e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_3_conditional_operator){
		return make_expression3(
			e._location,
			"?:",
			expr_to_json(e._exprs[0])._value,
			expr_to_json(e._exprs[1])._value,
			expr_to_json(e._exprs[2])._value
		);
//		return ast_json_t{json_t::make_array({ json_t("?:"), expr_to_json(e._exprs[0])._value, expr_to_json(e._exprs[1])._value, expr_to_json(e._exprs[2])._value })};
	}
	else if(e._op == eoperation::k_n_call){
		vector<json_t> args;
		for(auto i = 0 ; i < e._exprs.size() - 1 ; i++){
			const auto& arg = expr_to_json(e._exprs[i + 1]);
			args.push_back(arg._value);
		}
		return make_expression2(e._location, "call", expr_to_json(e._exprs[0])._value, json_t(args));
	}
	else if(e._op == eoperation::k_1_unary_minus){
		return make_expression1(e._location, "unary_minus", expr_to_json(e._exprs[0])._value);
	}
	else if(e._op == eoperation::k_1_vector_definition){
		//??? what is _identifier?
		QUARK_ASSERT(e._identifier == "");

		return make_expression2(
			e._location,
			"construct-value",
			typeid_to_ast_json(typeid_t::make_vector(typeid_t::make_undefined()), json_tags::k_tag_resolve_state)._value,
			expr_vector_to_json_array(e._exprs)._value
		);
	}
	else if(e._op == eoperation::k_1_dict_definition){
		//??? what is _identifier?
		QUARK_ASSERT(e._identifier == "");

		return make_expression2(
			e._location,
			"construct-value",
			typeid_to_ast_json(typeid_t::make_dict(typeid_t::make_undefined()), json_tags::k_tag_resolve_state)._value,
			expr_vector_to_json_array(e._exprs)._value
		);

		//??? need to encode that this is a dict.
//		return ast_json_t{json_t::make_array({ json_t("construct-value"), "{string:null}", expr_vector_to_json_array(e._exprs)._value })};
	}
	else{
		QUARK_ASSERT(false)
		throw std::runtime_error("???");
	}
}

ast_json_t parse_expression_all(const seq_t& expression){
	const auto result = parse_expression_seq(expression);
/*
	if(!parser2::skip_expr_whitespace(result.second).empty()){
		throw std::runtime_error("All of expression not used");
	}
*/
	return result.first;
}

std::pair<ast_json_t, seq_t> parse_expression_seq(const seq_t& expression){
	const auto expr = parse_expression(expression);
	const auto json = expr_to_json(expr.first);
	return { json, expr.second };
}


}	//	floyd
