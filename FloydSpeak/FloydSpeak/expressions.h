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

#include "parser_ast.h"
#include "parser_value.h"

struct json_t;

namespace floyd_parser {
	struct expression_t;
	struct value_t;
	struct scope_def_t;
	struct type_identifier_t;


	struct math_operation2_expr_t;
	struct conditional_operator_expr_t;
	struct function_call_expr_t;
	struct resolve_variable_expr_t;

	struct resolve_member_expr_t;
	struct resolve_member_expr_t;
	struct lookup_element_expr_t;


	//////////////////////////////////////////////////		expression_t


	struct expression_t {
		public: static expression_t make_nop();

		public: static expression_t make_constant(const value_t& value);

		//	Shortcuts were you don't need to make a value_t first.
		public: static expression_t make_constant(const bool i);
		public: static expression_t make_constant(const int i);
		public: static expression_t make_constant(const float f);
		public: static expression_t make_constant(const char s[]);
		public: static expression_t make_constant(const std::string& s);


		public: enum class math2_operation {
			k_add = 10,
			k_subtract,
			k_multiply,
			k_divide,
			k_remainder,

			k_smaller_or_equal,
			k_smaller,
			k_larger_or_equal,
			k_larger,

			k_logical_equal,
			k_logical_nonequal,
			k_logical_and,
			k_logical_or,

			k_math1_negate
		};

		public: static expression_t make_math_operation2(
			math2_operation op,
			const expression_t& left,
			const expression_t& right
		);

		public: static expression_t make_conditional_operator(
			const expression_t& condition,
			const expression_t& a,
			const expression_t& b
		);

		public: static expression_t make_function_call(
			const type_identifier_t& function,
			const std::vector<expression_t>& inputs,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		/*_
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.

			When compiler resolves this expression it may replace it with a resolve_
		*/
		public: static expression_t make_resolve_variable(
			const std::string& variable,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		/*
			Specifies a member variable of a struct.
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
		public: std::shared_ptr<value_t> _constant;
		public: std::shared_ptr<math_operation2_expr_t> _math2;
		public: std::shared_ptr<conditional_operator_expr_t> _conditional_operator;
		public: std::shared_ptr<function_call_expr_t> _call;

		public: std::shared_ptr<resolve_variable_expr_t> _resolve_variable;
		public: std::shared_ptr<resolve_member_expr_t> _resolve_member;
		public: std::shared_ptr<lookup_element_expr_t> _lookup_element;

		//	Tell what type of value this expression represents. Null if not yet defined.
		public: std::shared_ptr<const type_def_t> _resolved_expression_type;
	};
	



	//////////////////////////////////////////////////		math_operation2_expr_t


	struct math_operation2_expr_t {
		bool operator==(const math_operation2_expr_t& other) const;

		const expression_t::math2_operation _operation;
		const expression_t _left;
		const expression_t _right;
	};


	//////////////////////////////////////////////////		conditional_operator_expr_t


	struct conditional_operator_expr_t {
		bool operator==(const conditional_operator_expr_t& other) const;

		const expression_t _condition;
		const expression_t _a;
		const expression_t _b;
	};


	//////////////////////////////////////////////////		function_call_expr_t


	struct function_call_expr_t {
		bool operator==(const function_call_expr_t& other) const{
			return _function == other._function && _inputs == other._inputs;
		}

		public: function_call_expr_t(const type_identifier_t& function, const std::vector<expression_t>& inputs) :
			_function(function),
			_inputs(inputs)
		{
		}

		const type_identifier_t _function;
		const std::vector<expression_t> _inputs;
	};


	//////////////////////////////////////////////////		resolve_variable_expr_t


	struct resolve_variable_expr_t {
		bool operator==(const resolve_variable_expr_t& other) const;

		const std::string _variable_name;
	};


	//////////////////////////////////////////////////		resolve_member_expr_t


	struct resolve_member_expr_t {
		bool operator==(const resolve_member_expr_t& other) const;

		expression_t _parent_address;
		const std::string _member_name;
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
