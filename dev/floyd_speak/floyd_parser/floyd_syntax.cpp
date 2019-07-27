//
//  floyd_syntax.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_syntax.h"

#include "quark.h"
#include <map>

using std::string;

namespace floyd {


//	WARNING: Make sure all accessed constants have already been initialized!
static const std::map<syntax_expression_type, std::string> k_2_operator_to_string{
//	{ syntax_expression_type::k_x_member_access, "->" },

	{ syntax_expression_type::k_2_lookup, "[]" },

	{ syntax_expression_type::k_2_add, "+" },
	{ syntax_expression_type::k_2_subtract, "-" },
	{ syntax_expression_type::k_2_multiply, "*" },
	{ syntax_expression_type::k_2_divide, "/" },
	{ syntax_expression_type::k_2_remainder, "%" },

	{ syntax_expression_type::k_2_smaller_or_equal, "<=" },
	{ syntax_expression_type::k_2_smaller, "<" },
	{ syntax_expression_type::k_2_larger_or_equal, ">=" },
	{ syntax_expression_type::k_2_larger, ">" },

	{ syntax_expression_type::k_2_logical_equal, "==" },
	{ syntax_expression_type::k_2_logical_nonequal, "!=" },
	{ syntax_expression_type::k_2_logical_and, "&&" },
	{ syntax_expression_type::k_2_logical_or, "||" },
};

std::string k_2_operator_to_string__func(syntax_expression_type op){
	return k_2_operator_to_string.at(op);
}


}
