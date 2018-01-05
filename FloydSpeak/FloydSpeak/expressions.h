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
#include "floyd_basics.h"

#include "parser_ast.h"
#include "parser_value.h"

struct json_t;

namespace floyd_ast {
	struct value_t;
	struct statement_t;
	struct expression_t;
	struct typeid_t;

	json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e);
	json_t expression_to_json(const expression_t& e);
	json_t expressions_to_json(const std::vector<expression_t> v);

	//	"+", "<=", "&&" etc.
	bool is_simple_expression__2(const std::string& op);



	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {

		private: struct expr_base_t {
			public: virtual ~expr_base_t(){};

			public: virtual typeid_t get_result_type() const = 0;
			public: virtual json_t expr_base__to_json() const{ return json_t(); };
		};


		////////////////////////////////			make_literal()


		public: struct literal_expr_t : public expr_base_t {
			public: virtual ~literal_expr_t(){};

			public: literal_expr_t(const value_t& value) 
			:
				_value(value)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _value.get_type();
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({ "k", value_to_json(_value), typeid_to_json(_value.get_type()) });
			}


			const value_t _value;
		};

		public: static expression_t make_literal(const value_t& value)
		{
			return expression_t{
				floyd_basics::expression_type::k_literal,
				std::make_shared<literal_expr_t>(
					literal_expr_t{ value }
				)
			};
		}

		public: static expression_t make_literal_null();
		public: static expression_t make_literal_int(const int i);
		public: static expression_t make_literal_bool(const bool i);
		public: static expression_t make_literal_float(const float i);
		public: static expression_t make_literal_string(const std::string& s);

		public: bool is_literal() const;
		public: const value_t& get_literal() const;


		////////////////////////////////			make_simple_expression__2()


		public: struct simple_expr__2_t : public expr_base_t {
			public: virtual ~simple_expr__2_t(){};

			public: simple_expr__2_t(
				floyd_basics::expression_type op,
				const expression_t& left,
				const expression_t& right,
				const typeid_t result_type
			)
			:
				_op(op),
				_left(std::make_shared<expression_t>(left)),
				_right(std::make_shared<expression_t>(right)),
				_result_type(result_type)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result_type;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({
					floyd_basics::expression_type_to_token(_op),
					expression_to_json(*_left),
					expression_to_json(*_right),
					typeid_to_json(get_result_type())
				});
			}


			const floyd_basics::expression_type _op;
			const std::shared_ptr<expression_t> _left;
			const std::shared_ptr<expression_t> _right;
			const typeid_t _result_type;
		};

		public: static expression_t make_simple_expression__2(floyd_basics::expression_type op, const expression_t& left, const expression_t& right){
			if(
				op == floyd_basics::expression_type::k_arithmetic_add__2
				|| op == floyd_basics::expression_type::k_arithmetic_subtract__2
				|| op == floyd_basics::expression_type::k_arithmetic_multiply__2
				|| op == floyd_basics::expression_type::k_arithmetic_divide__2
				|| op == floyd_basics::expression_type::k_arithmetic_remainder__2
			)
			{
				return expression_t{
					op,
					std::make_shared<simple_expr__2_t>(
						simple_expr__2_t{ op, left, right, left.get_result_type() }
					)
				};
			}
			else if(
				op == floyd_basics::expression_type::k_comparison_smaller_or_equal__2
				|| op == floyd_basics::expression_type::k_comparison_smaller__2
				|| op == floyd_basics::expression_type::k_comparison_larger_or_equal__2
				|| op == floyd_basics::expression_type::k_comparison_larger__2

				|| op == floyd_basics::expression_type::k_logical_equal__2
				|| op == floyd_basics::expression_type::k_logical_nonequal__2
				|| op == floyd_basics::expression_type::k_logical_and__2
				|| op == floyd_basics::expression_type::k_logical_or__2
		//		|| op == floyd_basics::expression_type::k_logical_negate
			)
			{
				return expression_t{
					op,
					std::make_shared<simple_expr__2_t>(
						simple_expr__2_t{ op, left, right, typeid_t::make_bool() }
					)
				};
			}
			else if(
				op == floyd_basics::expression_type::k_literal
				|| op == floyd_basics::expression_type::k_arithmetic_unary_minus__1
				|| op == floyd_basics::expression_type::k_conditional_operator3
				|| op == floyd_basics::expression_type::k_call
				|| op == floyd_basics::expression_type::k_variable
				|| op == floyd_basics::expression_type::k_resolve_member
				|| op == floyd_basics::expression_type::k_lookup_element)
			{
				QUARK_ASSERT(false);
			}
			else{
				QUARK_ASSERT(false);
			}
		}

		public: const simple_expr__2_t* get_simple__2() const {
			return dynamic_cast<const simple_expr__2_t*>(_expr.get());
		}


		////////////////////////////////			make_unary_minus()


		public: struct unary_minus_expr_t : public expr_base_t {
			public: virtual ~unary_minus_expr_t(){};

			public: unary_minus_expr_t(
				const expression_t& expr
			)
			:
				_expr(std::make_shared<expression_t>(expr))
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _expr->get_result_type();
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({
					expression_to_json(*_expr)
				});
			}


			const std::shared_ptr<expression_t> _expr;
		};

		public: static expression_t make_unary_minus(const expression_t expr){
			return expression_t{
				floyd_basics::expression_type::k_arithmetic_unary_minus__1,
				std::make_shared<unary_minus_expr_t>(
					unary_minus_expr_t{ expr }
				)
			};
		}

		public: const unary_minus_expr_t* get_unary_minus() const {
			return dynamic_cast<const unary_minus_expr_t*>(_expr.get());
		}



		////////////////////////////////			make_conditional_operator()


		public: struct conditional_expr_t : public expr_base_t {
			public: virtual ~conditional_expr_t(){};

			public: conditional_expr_t(
				const expression_t& condition,
				const expression_t& a,
				const expression_t& b,
				const typeid_t result_type
			)
			:
				_condition(std::make_shared<expression_t>(condition)),
				_a(std::make_shared<expression_t>(a)),
				_b(std::make_shared<expression_t>(b)),
				_result_type(result_type)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result_type;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({
					expression_to_json(*_condition),
					expression_to_json(*_a),
					expression_to_json(*_b),
					typeid_to_json(get_result_type())
				});
			}


			const std::shared_ptr<expression_t> _condition;
			const std::shared_ptr<expression_t> _a;
			const std::shared_ptr<expression_t> _b;
			const typeid_t _result_type;
		};

		public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b){
			return expression_t{
				floyd_basics::expression_type::k_conditional_operator3,
				std::make_shared<conditional_expr_t>(
					conditional_expr_t{ condition, a, b, a.get_result_type() }
				)
			};
		}

		public: const conditional_expr_t* get_conditional() const {
			return dynamic_cast<const conditional_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_function_call()


		public: struct function_call_expr_t : public expr_base_t {
			public: virtual ~function_call_expr_t(){};

			public: function_call_expr_t(
				const expression_t& function,
				std::vector<expression_t> args,
				typeid_t result
			)
			:
				_function(std::make_shared<expression_t>(function)),
				_args(args),
				_result(result)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({
					"call",
					expression_to_json(*_function),
					expressions_to_json(_args),
					typeid_to_json(_result)
				});
			}


			const std::shared_ptr<expression_t> _function;
			const std::vector<expression_t> _args;
			const typeid_t _result;
		};

		public: static expression_t make_function_call(
			const expression_t& function,
			const std::vector<expression_t>& args,
			const typeid_t& result
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_call,
				std::make_shared<function_call_expr_t>(
					function_call_expr_t{ function, args, result }
				)
			};
		}

		public: const function_call_expr_t* get_function_call() const {
			return dynamic_cast<const function_call_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_function_definition()


		public: struct function_definition_expr_t : public expr_base_t {
			public: virtual ~function_definition_expr_t(){};

			public: function_definition_expr_t(const function_definition_t & def)
			:
				_def(def)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _def._return_type;
			}

			public: virtual json_t expr_base__to_json() const {
				return _def.to_json();
			}


			const function_definition_t _def;
		};

		public: static expression_t make_function_definition(const function_definition_t& def){
			return expression_t{
				floyd_basics::expression_type::k_define_function,
				std::make_shared<function_definition_expr_t>(
					function_definition_expr_t{ function_definition_t(def) }
				)
			};
		}

		public: const function_definition_expr_t* get_function_definition() const {
			return dynamic_cast<const function_definition_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_variable_expression()


		public: struct variable_expr_t : public expr_base_t {
			public: virtual ~variable_expr_t(){};

			public: variable_expr_t(
				std::string variable,
				typeid_t result
			)
			:
				_variable(variable),
				_result(result)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({ "@", json_t(_variable), typeid_to_json(_result) });
			}


			std::string _variable;
			typeid_t _result;
		};

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_variable_expression(
			const std::string& variable,
			const typeid_t& result
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_variable,
				std::make_shared<variable_expr_t>(
					variable_expr_t{ variable, result }
				)
			};
		}

		public: const variable_expr_t* get_variable() const {
			return dynamic_cast<const variable_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_resolve_member()


		public: struct resolve_member_expr_t : public expr_base_t {
			public: virtual ~resolve_member_expr_t(){};

			public: resolve_member_expr_t(
				const expression_t& parent_address,
				std::string member_name,
				typeid_t result
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_member_name(member_name),
				_result(result)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({ "->", expression_to_json(*_parent_address), json_t(_member_name) });
			}


			std::shared_ptr<expression_t> _parent_address;
			std::string _member_name;
			typeid_t _result;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const typeid_t& result
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_resolve_member,
				std::make_shared<resolve_member_expr_t>(
					resolve_member_expr_t{ parent_address, member_name, result }
				)
			};
		}

		public: const resolve_member_expr_t* get_resolve_member() const {
			return dynamic_cast<const resolve_member_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_lookup()

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/

		public: struct lookup_expr_t : public expr_base_t {
			public: virtual ~lookup_expr_t(){};

			public: lookup_expr_t(
				const expression_t& parent_address,
				const expression_t& lookup_key,
				typeid_t result
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_lookup_key(std::make_shared<expression_t>(lookup_key)),
				_result(result)
			{
			}

			public: virtual typeid_t get_result_type() const{
				return _result;
			}

			public: virtual json_t expr_base__to_json() const {
				return json_t::make_array({ "[-]", expression_to_json(*_parent_address), expression_to_json(*_lookup_key), typeid_to_json(_result) });
			}


			std::shared_ptr<expression_t> _parent_address;
			std::shared_ptr<expression_t> _lookup_key;
			typeid_t _result;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const typeid_t result
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_resolve_member,
				std::make_shared<lookup_expr_t>(
					lookup_expr_t{ parent_address, lookup_key, result }
				)
			};
		}

		public: const lookup_expr_t* get_lookup() const {
			return dynamic_cast<const lookup_expr_t*>(_expr.get());
		}



		////////////////////////////////			OTHER


		public: bool check_invariant() const;

		public: bool operator==(const expression_t& other) const;

		/*
			Returns pre-computed result of the expression - the type of value it represents.
			null if not resolved.
		*/
		public: typeid_t get_result_type() const{
			QUARK_ASSERT(check_invariant());

			return _expr->get_result_type();
		}

		public: floyd_basics::expression_type get_operation() const;

		public: const expr_base_t* get_expr() const{
			return _expr.get();
		}


		//////////////////////////		INTERNALS


		private: expression_t(
			const floyd_basics::expression_type operation,
			const std::shared_ptr<const expr_base_t>& expr
		);


		//////////////////////////		STATE
		private: std::string _debug;
		private: floyd_basics::expression_type _operation;
		private: std::shared_ptr<const expr_base_t> _expr;
	};



	void trace(const expression_t& e);

	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	json_t expression_to_json(const expression_t& e);

	json_t expressions_to_json(const std::vector<expression_t> v);




	inline bool operator==(const expression_t::literal_expr_t& lhs, const expression_t::literal_expr_t& rhs){
		return lhs._value == rhs._value;
	}

	inline bool operator==(const expression_t::variable_expr_t& lhs, const expression_t::variable_expr_t& rhs){
		return lhs._variable == rhs._variable
			&& lhs._result == rhs._result;
	}

	inline bool operator==(const expression_t::resolve_member_expr_t& lhs, const expression_t::resolve_member_expr_t& rhs){
		return compare_shared_values(lhs._parent_address, rhs._parent_address)
			&& lhs._member_name == rhs._member_name
			&& lhs._result == rhs._result;
	}

	inline bool operator==(const expression_t::function_definition_expr_t& lhs, const expression_t::function_definition_expr_t& rhs){
		return lhs._def == rhs._def;
	}

	inline bool operator==(const expression_t::function_call_expr_t& lhs, const expression_t::function_call_expr_t& rhs){
		return
			lhs._function == rhs._function
			&& lhs._args == rhs._args
			&& lhs._result == rhs._result;
	}

}	//	floyd_ast


#endif /* expressions_hpp */
