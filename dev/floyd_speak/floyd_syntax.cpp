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




//////////////////////////////////////////////////		base_type


string base_type_to_string(const base_type t){
	if(t == base_type::k_internal_undefined){
		return keyword_t::k_internal_undefined;
	}
	else if(t == base_type::k_internal_dynamic){
		return keyword_t::k_internal_dynamic;
	}

	else if(t == base_type::k_void){
		return keyword_t::k_void;
	}
	else if(t == base_type::k_bool){
		return keyword_t::k_bool;
	}
	else if(t == base_type::k_int){
		return keyword_t::k_int;
	}
	else if(t == base_type::k_double){
		return keyword_t::k_double;
	}
	else if(t == base_type::k_string){
		return keyword_t::k_string;
	}
	else if(t == base_type::k_json_value){
		return keyword_t::k_json_value;
	}

	else if(t == base_type::k_typeid){
		return keyword_t::k_typeid;
	}

	else if(t == base_type::k_struct){
		return keyword_t::k_struct;
	}
	else if(t == base_type::k_protocol){
		return keyword_t::k_protocol;
	}
	else if(t == base_type::k_vector){
		return "vector";
	}
	else if(t == base_type::k_dict){
		return "dict";
	}
	else if(t == base_type::k_function){
		return "fun";
	}
	else if(t == base_type::k_internal_unresolved_type_identifier){
		return "**unknown-identifier**";
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_undefined) == "**undef**");
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_dynamic) == "**dyn**");
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_void) == keyword_t::k_void);
}

QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_bool) == keyword_t::k_bool);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_int) == keyword_t::k_int);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_double) == keyword_t::k_double);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_string) == keyword_t::k_string);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_json_value) == keyword_t::k_json_value);
}

QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_typeid) == "typeid");
}

QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_struct) == keyword_t::k_struct);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_protocol) == keyword_t::k_protocol);
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_vector) == "vector");
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_dict) == "dict");
}
QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_function) == "fun");
}

QUARK_UNIT_TEST("", "base_type_to_string(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_string(base_type::k_internal_unresolved_type_identifier) == "**unknown-identifier**");
}

void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected){
	ut_verify(
		context,
		base_type_to_string(result),
		base_type_to_string(expected)
	);
}




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


//	WARNING: Make sure all accessed constants have already been initialized!
static std::map<expression_type, string> operation_to_string_lookup = {
	{ expression_type::k_arithmetic_add__2, "+" },
	{ expression_type::k_arithmetic_subtract__2, "-" },
	{ expression_type::k_arithmetic_multiply__2, "*" },
	{ expression_type::k_arithmetic_divide__2, "/" },
	{ expression_type::k_arithmetic_remainder__2, "%" },

	{ expression_type::k_comparison_smaller_or_equal__2, "<=" },
	{ expression_type::k_comparison_smaller__2, "<" },
	{ expression_type::k_comparison_larger_or_equal__2, ">=" },
	{ expression_type::k_comparison_larger__2, ">" },

	{ expression_type::k_logical_equal__2, "==" },
	{ expression_type::k_logical_nonequal__2, "!=" },
	{ expression_type::k_logical_and__2, "&&" },
	{ expression_type::k_logical_or__2, "||" },
//	{ expression_type::k_logical_not, "!" },

	{ expression_type::k_literal, "k" },

	{ expression_type::k_arithmetic_unary_minus__1, "unary-minus" },

	{ expression_type::k_conditional_operator3, "?:" },
	{ expression_type::k_call, "call" },

	{ expression_type::k_load, "@" },
	{ expression_type::k_load2, "@i" },
	{ expression_type::k_resolve_member, "->" },

	{ expression_type::k_lookup_element, "[]" },

	{ expression_type::k_struct_def, "def-struct" },
	{ expression_type::k_function_def, "def-func" },
	{ expression_type::k_value_constructor, "construct-value" }
};







std::map<string, expression_type> make_reverse(const std::map<expression_type, string>& m){
	std::map<string, expression_type> temp;
	for(const auto& e: m){
		temp[e.second] = e.first;
	}
	return temp;
}

static std::map<string, expression_type> string_to_operation_lookup = make_reverse(operation_to_string_lookup);




string expression_type_to_token(const expression_type& op){
	const auto r = operation_to_string_lookup.find(op);
	QUARK_ASSERT(r != operation_to_string_lookup.end());
	return r->second;
}

expression_type token_to_expression_type(const string& op){
	const auto r = string_to_operation_lookup.find(op);
	QUARK_ASSERT(r != string_to_operation_lookup.end());
	return r->second;
}



//??? split this file into syntax.h and ast.h


}
