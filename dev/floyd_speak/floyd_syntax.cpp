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
		throw std::exception();
	}
}


//////////////////////////////////////		base_type_to_string(base_type)


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





//	WARNING: Make sure all accessed constants have already been initialized!
static const std::map<eoperation, std::string> k_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_lookup, expression_opcode_t::k_lookup_element },

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

std::string k_2_operator_to_string__func(eoperation op){
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

	{ expression_type::k_literal, expression_opcode_t::k_literal },

	{ expression_type::k_arithmetic_unary_minus__1, expression_opcode_t::k_unary_minus },

	{ expression_type::k_conditional_operator3, expression_opcode_t::k_conditional_operator },
	{ expression_type::k_call, expression_opcode_t::k_call },

	{ expression_type::k_load, expression_opcode_t::k_load },
	{ expression_type::k_load2, expression_opcode_t::k_load2 },
	{ expression_type::k_resolve_member, expression_opcode_t::k_resolve_member },

	{ expression_type::k_lookup_element, expression_opcode_t::k_lookup_element },

	{ expression_type::k_struct_def, expression_opcode_t::k_struct_def },
	{ expression_type::k_function_def, expression_opcode_t::k_function_def },
	{ expression_type::k_value_constructor, expression_opcode_t::k_value_constructor }
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
