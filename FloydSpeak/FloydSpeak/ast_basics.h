//
//  ast_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_basics_hpp
#define ast_basics_hpp


#include <vector>
#include "json_support.h"


namespace floyd {

	//////////////////////////////////////		ast_json_t


	struct ast_json_t {
		json_t _value;
	};



	//////////////////////////////////////		variable_address_t


	struct variable_address_t {
		public: variable_address_t() :
			_parent_steps(-1),
			_index(-1)
		{
		}

		public: static variable_address_t make_variable_address(int parent_steps, int index){
			return variable_address_t(parent_steps, index);
		}
		private: variable_address_t(int parent_steps, int index) :
			_parent_steps(parent_steps),
			_index(index)
		{
		}

		/*
			0: current stack frame
			1: previous stack frame
			2: previous-previous stack frame
			-1: global stack frame
		*/
		public: const int _parent_steps;
		public: const int _index;
	};

	inline bool operator==(const variable_address_t& lhs, const variable_address_t& rhs){
		return lhs._parent_steps == rhs._parent_steps && lhs._index == rhs._index;
	}


	//////////////////////////////////////		expression_type



	//??? Split and categories better. Logic vs equality vs math.

	//	Number at end of name tells number of input expressions operation has.
	enum class expression_type {

		//	c99: a + b			token: "+"
		k_arithmetic_add__2 = 10,

		//	c99: a - b			token: "-"
		k_arithmetic_subtract__2,

		//	c99: a * b			token: "*"
		k_arithmetic_multiply__2,

		//	c99: a / b			token: "/"
		k_arithmetic_divide__2,

		//	c99: a % b			token: "%"
		k_arithmetic_remainder__2,


		//	c99: a <= b			token: "<="
		k_comparison_smaller_or_equal__2,

		//	c99: a < b			token: "<"
		k_comparison_smaller__2,

		//	c99: a >= b			token: ">="
		k_comparison_larger_or_equal__2,

		//	c99: a > b			token: ">"
		k_comparison_larger__2,


		//	c99: a == b			token: "=="
		k_logical_equal__2,

		//	c99: a != b			token: "!="
		k_logical_nonequal__2,


		//	c99: a && b			token: "&&"
		k_logical_and__2,

		//	c99: a || b			token: "||"
		k_logical_or__2,

		//	c99: !a				token: "!"
//			k_logical_not,

		//	c99: 13				token: "k"
		k_literal,

		//	c99: -a				token: "unary_minus"
		k_arithmetic_unary_minus__1,

		//	c99: cond ? a : b	token: "?:"
		k_conditional_operator3,

		//	c99: a(b, c)		token: "call"
		k_call,

		//	c99: a				token: "@"
		k_load,

		//	c99: a				token: "@i"
		k_load2,

		//	c99: a.b			token: "->"
		k_resolve_member,

		//	c99: a[b]			token: "[]"
		k_lookup_element,

		k_define_struct,

		//???	use k_literal for function values?
		k_define_function,

		k_construct_value,
	};

	expression_type token_to_expression_type(const std::string& op);
	std::string expression_type_to_token(const expression_type& op);

}

#endif /* ast_basics_hpp */
