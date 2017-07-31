//
//  expressions.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef expressions_hpp
#define expressions_hpp

#include "quark.h"
#include <vector>
#include <string>

#include "parser_value.h"

struct json_t;

namespace floyd_parser {
	struct expression_t;
	struct value_t;
	struct scope_def_t;
	struct type_identifier_t;


	struct math_operation2_expr_t;

	struct lookup_element_expr_t;


	//////////////////////////////////////////////////		expression_t


	struct expression_t {
		public: static expression_t make_nop();
		public: bool is_nop() const;

		public: static expression_t make_constant(const value_t& value);

		//	Shortcuts were you don't need to make a value_t first.
		public: static expression_t make_constant(const bool i);
		public: static expression_t make_constant(const int i);
		public: static expression_t make_constant(const float f);
		public: static expression_t make_constant(const char s[]);
		public: static expression_t make_constant(const std::string& s);

		public: bool is_constant() const;
		public: value_t get_constant() const;

		public: enum class math2_operation {
			k_math2_add = 10,
			k_math2_subtract,
			k_math2_multiply,
			k_math2_divide,
			k_math2_remainder,

			k_math2_smaller_or_equal,
			k_math2_smaller,
			k_math2_larger_or_equal,
			k_math2_larger,

			k_logical_equal,
			k_logical_nonequal,
			k_logical_and,
			k_logical_or,
			k_logical_negate,

			k_constant,

			k_conditional_operator3,
			k_call,

			k_resolve_variable,
			k_resolve_member
		};

		public: static expression_t make_math_operation2(
			math2_operation op,
			const expression_t& left,
			const expression_t& right
		){
			return make_math_operation2(op, { left, right }, {}, {});
		}
		public: static expression_t make_math_operation2(
			math2_operation op,
			const std::vector<expression_t>& expressions,
			const std::shared_ptr<value_t>& constant,
			const std::string& symbol
		);

		public: static expression_t make_conditional_operator(
			const expression_t& condition,
			const expression_t& a,
			const expression_t& b
		);

		public: static expression_t make_function_call(
			const type_identifier_t& function_name,
			const std::vector<expression_t>& inputs,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.

			When compiler resolves this expression it may replace it with a resolve_
		*/
		public: static expression_t make_resolve_variable(
			const std::string& variable,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);


		public: bool check_invariant() const;

		/*
			Returns pre-computed result of the expression - the type of value it represents.
			null if not resolved.
		*/
		public: std::shared_ptr<const type_def_t> get_expression_type() const{
			QUARK_ASSERT(check_invariant());

			return _resolved_expression_type;
		}

		public: bool operator==(const expression_t& other) const;
		private: expression_t(){
			// Invariant is broken here - expression type is setup.
		}


		//////////////////////////		STATE
		public: std::string _debug_aaaaaaaaaaaaaaaaaaaaaaa;

		/*
			Only ONE of there are used at any time.
		*/
		public: std::shared_ptr<math_operation2_expr_t> _math2;

		public: std::shared_ptr<lookup_element_expr_t> _lookup_element;


		//	Tell what type of value this expression represents. Null if not yet defined.
		public: std::shared_ptr<const type_def_t> _resolved_expression_type;
	};
	



	//////////////////////////////////////////////////		math_operation2_expr_t


	struct math_operation2_expr_t {
		bool operator==(const math_operation2_expr_t& other) const;

		const expression_t::math2_operation _operation;
		const std::vector<expression_t> _expressions;
		const std::shared_ptr<value_t> _constant;
		const std::string _symbol;
	};



	//////////////////////////////////////////////////		lookup_element_expr_t


	struct lookup_element_expr_t {
		bool operator==(const lookup_element_expr_t& other) const;

		expression_t _parent_address;
		expression_t _lookup_key;
	};



	//////////////////////////////////////////////////		trace()


	void trace(const expression_t& e);

	json_t expression_to_json(const expression_t& e);

	expression_t::math2_operation string_to_math2_op(const std::string& op);

	bool is_math2_op(const std::string& op);

}	//	floyd_parser


#endif /* expressions_hpp */
