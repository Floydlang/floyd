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

namespace floyd_ast {
	struct value_t;



	bool is_simple_expression__2(const std::string& op);


	//////////////////////////////////////////////////		expression_t

	/*
		Immutable.
	*/
	struct expression_t {
		public: static expression_t make_constant_value(const value_t& value);

		public: static expression_t make_constant_null();
		public: static expression_t make_constant_int(const int i);
		public: static expression_t make_constant_bool(const bool i);
		public: static expression_t make_constant_float(const float i);
		public: static expression_t make_constant_string(const std::string& s);

		public: bool is_constant() const;
		public: const value_t& get_constant() const;

		public: static expression_t make_simple_expression__2(
			floyd_basics::expression_type op,
			const expression_t& left,
			const expression_t& right
		);
		public: static expression_t make_unary_minus(const expression_t& expr);
		public: static expression_t make_conditional_operator(
			const expression_t& condition,
			const expression_t& a,
			const expression_t& b
		);

		public: static expression_t make_function_call(
			const expression_t& function,
			const std::vector<expression_t>& args,
			const typeid_t& result
		);

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_variable_expression(
			const std::string& variable,
			const typeid_t& result
		);

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const typeid_t& result
		);

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const typeid_t& result
		);

		public: bool check_invariant() const;

		public: bool operator==(const expression_t& other) const;

		/*
			Returns pre-computed result of the expression - the type of value it represents.
			null if not resolved.
		*/
		public: typeid_t get_expression_type() const{
			QUARK_ASSERT(check_invariant());

			return _result_type;
		}

		public: floyd_basics::expression_type get_operation() const;
		public: const std::vector<expression_t>& get_expressions() const;
		public: const std::string& get_symbol() const;
		public: typeid_t get_result_type() const;


		//////////////////////////		INTERNALS


		private: expression_t(
			floyd_basics::expression_type operation,
			const std::vector<expression_t>& expressions,
			const std::shared_ptr<value_t>& constant,
			const std::string& symbol,
			const typeid_t& result_type
		);


		//////////////////////////		STATE
		private: std::string _debug;
		private: floyd_basics::expression_type _operation;
		private: std::vector<expression_t> _expressions;
		private: std::shared_ptr<value_t> _constant;
		private: std::string _symbol;

		//	Tell what type of value this expression represents. Null if not yet defined.
		private: typeid_t _result_type;
	};
	

	void trace(const expression_t& e);

	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	json_t expression_to_json(const expression_t& e);

}	//	floyd_ast


#endif /* expressions_hpp */
