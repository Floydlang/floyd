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



	//////////////////////////////////////////////////		expression_t

	/*
		Immutable.
	*/
	struct expression_t {
		public: static expression_t make_constant(const value_t& value);

		//	Shortcuts were you don't need to make a value_t first.
		public: static expression_t make_constant(const bool i);
		public: static expression_t make_constant(const int i);
		public: static expression_t make_constant(const float f);
		public: static expression_t make_constant(const char s[]);
		public: static expression_t make_constant(const std::string& s);

		public: bool is_constant() const;
		public: const value_t& get_constant() const;

		public: enum class operation {
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
			k_resolve_member,

			k_lookup_element
		};

		public: static expression_t make_math2_operation(
			operation op,
			const expression_t& left,
			const expression_t& right
		);
		public: static expression_t make_conditional_operator(
			const expression_t& condition,
			const expression_t& a,
			const expression_t& b
		);

		public: static expression_t make_function_call(
			const type_identifier_t& function_name,
			const std::vector<expression_t>& inputs,
			const typeid_t& resolved_expression_type
		);

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.

			When compiler resolves this expression it may replace it with a resolve_
		*/
		public: static expression_t make_resolve_variable(
			const std::string& variable,
			const typeid_t& resolved_expression_type
		);

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const typeid_t& resolved_expression_type
		);

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const typeid_t& resolved_expression_type
		);

		public: bool check_invariant() const;

		public: bool operator==(const expression_t& other) const;

		private: expression_t(
			operation operation,
			const std::vector<expression_t>& expressions,
			const std::shared_ptr<value_t>& constant,
			const std::string& symbol,
			const typeid_t& resolved_expression_type
		);

		/*
			Returns pre-computed result of the expression - the type of value it represents.
			null if not resolved.
		*/
		public: typeid_t get_expression_type() const{
			QUARK_ASSERT(check_invariant());

			return _resolved_expression_type;
		}

		public: operation get_operation() const;
		public: const std::vector<expression_t>& get_expressions() const;
		public: const std::string& get_symbol() const;
		public: typeid_t get_resolved_expression_type() const;


		//////////////////////////		STATE
		private: std::string _debug;
		private: operation _operation;
		private: std::vector<expression_t> _expressions;
		private: std::shared_ptr<value_t> _constant;
		private: std::string _symbol;

		//	Tell what type of value this expression represents. Null if not yet defined.
		private: typeid_t _resolved_expression_type;
	};
	

	void trace(const expression_t& e);

	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	json_t expression_to_json(const expression_t& e);

	expression_t::operation string_to_math2_op(const std::string& op);

	bool is_math2_op(const std::string& op);

}	//	floyd_parser


#endif /* expressions_hpp */
