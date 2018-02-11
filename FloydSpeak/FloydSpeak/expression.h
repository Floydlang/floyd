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

#include "ast.h"
#include "parser_value.h"


namespace floyd {
	struct ast_json_t;
	struct value_t;
	struct statement_t;
	struct expression_t;


	ast_json_t expression_to_json(const expression_t& e);
	ast_json_t expressions_to_json(const std::vector<expression_t> v);

	//	"+", "<=", "&&" etc.
	bool is_simple_expression__2(const std::string& op);



	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {

		private: struct expr_base_t {
			public: virtual ~expr_base_t(){};

			public: virtual ast_json_t expr_base__to_json() const{ return ast_json_t(); };
		};


		////////////////////////////////			make_literal()
		//### Not really "literals". "Terminal values" maybe better term?

		public: struct literal_expr_t : public expr_base_t {
			public: virtual ~literal_expr_t(){};

			public: literal_expr_t(const value_t& value) 
			:
				_value(value)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"k",
					value_to_normalized_json(_value)._value,
					typeid_to_normalized_json(_value.get_type())._value
				})};
			}


			const value_t _value;
		};

		public: static expression_t make_literal(const value_t& value)
		{
			return expression_t{
				expression_type::k_literal,
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
				expression_type op,
				const expression_t& left,
				const expression_t& right
			)
			:
				_op(op),
				_left(std::make_shared<expression_t>(left)),
				_right(std::make_shared<expression_t>(right))
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					expression_type_to_token(_op),
					expression_to_json(*_left)._value,
					expression_to_json(*_right)._value
				})};
			}


			const expression_type _op;
			const std::shared_ptr<expression_t> _left;
			const std::shared_ptr<expression_t> _right;
		};

		public: static expression_t make_simple_expression__2(expression_type op, const expression_t& left, const expression_t& right){
			if(
				op == expression_type::k_arithmetic_add__2
				|| op == expression_type::k_arithmetic_subtract__2
				|| op == expression_type::k_arithmetic_multiply__2
				|| op == expression_type::k_arithmetic_divide__2
				|| op == expression_type::k_arithmetic_remainder__2
			)
			{
				return expression_t{
					op,
					std::make_shared<simple_expr__2_t>(
						simple_expr__2_t{ op, left, right }
					)
				};
			}
			else if(
				op == expression_type::k_comparison_smaller_or_equal__2
				|| op == expression_type::k_comparison_smaller__2
				|| op == expression_type::k_comparison_larger_or_equal__2
				|| op == expression_type::k_comparison_larger__2

				|| op == expression_type::k_logical_equal__2
				|| op == expression_type::k_logical_nonequal__2
				|| op == expression_type::k_logical_and__2
				|| op == expression_type::k_logical_or__2
		//		|| op == expression_type::k_logical_negate
			)
			{
				return expression_t{
					op,
					std::make_shared<simple_expr__2_t>(
						simple_expr__2_t{ op, left, right }
					)
				};
			}
			else if(
				op == expression_type::k_literal
				|| op == expression_type::k_arithmetic_unary_minus__1
				|| op == expression_type::k_conditional_operator3
				|| op == expression_type::k_call
				|| op == expression_type::k_variable
				|| op == expression_type::k_resolve_member
				|| op == expression_type::k_lookup_element
				|| op == expression_type::k_define_function)
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


			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					expression_to_json(*_expr)._value
				})};
			}


			const std::shared_ptr<expression_t> _expr;
		};

		public: static expression_t make_unary_minus(const expression_t expr){
			return expression_t{
				expression_type::k_arithmetic_unary_minus__1,
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
				const expression_t& b
			)
			:
				_condition(std::make_shared<expression_t>(condition)),
				_a(std::make_shared<expression_t>(a)),
				_b(std::make_shared<expression_t>(b))
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					expression_to_json(*_condition)._value,
					expression_to_json(*_a)._value,
					expression_to_json(*_b)._value,
				})};
			}


			const std::shared_ptr<expression_t> _condition;
			const std::shared_ptr<expression_t> _a;
			const std::shared_ptr<expression_t> _b;
		};

		public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b){
			return expression_t{
				expression_type::k_conditional_operator3,
				std::make_shared<conditional_expr_t>(
					conditional_expr_t{ condition, a, b }
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
				std::vector<expression_t> args
			)
			:
				_function(std::make_shared<expression_t>(function)),
				_args(args)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"call",
					expression_to_json(*_function)._value,
					expressions_to_json(_args)._value
				})};
			}


			const std::shared_ptr<expression_t> _function;
			const std::vector<expression_t> _args;
		};

		public: static expression_t make_function_call(
			const expression_t& function,
			const std::vector<expression_t>& args
		)
		{
			return expression_t{
				expression_type::k_call,
				std::make_shared<function_call_expr_t>(
					function_call_expr_t{ function, args }
				)
			};
		}

		public: const function_call_expr_t* get_function_call() const {
			return dynamic_cast<const function_call_expr_t*>(_expr.get());
		}


		////////////////////////////////			make_function_definition()

//??? make better
		public: struct function_definition_expr_t : public expr_base_t {
			public: virtual ~function_definition_expr_t(){};

			public: function_definition_expr_t(const function_definition_t & def)
			:
				_def(def)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{_def.to_json()};
			}


			const function_definition_t _def;
		};

		public: static expression_t make_function_definition(const function_definition_t& def){
			return expression_t{
				expression_type::k_define_function,
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
				std::string variable
			)
			:
				_variable(variable)
			{
			}


			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({ "@", json_t(_variable) })};
			}


			std::string _variable;
		};

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_variable_expression(
			const std::string& variable
		)
		{
			return expression_t{
				expression_type::k_variable,
				std::make_shared<variable_expr_t>(
					variable_expr_t{ variable }
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
				std::string member_name
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_member_name(member_name)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
				 	"->",
				 	expression_to_json(*_parent_address)._value,
				 	json_t(_member_name)
				})};
			}


			std::shared_ptr<expression_t> _parent_address;
			std::string _member_name;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name
		)
		{
			return expression_t{
				expression_type::k_resolve_member,
				std::make_shared<resolve_member_expr_t>(
					resolve_member_expr_t{ parent_address, member_name }
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
				const expression_t& lookup_key
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_lookup_key(std::make_shared<expression_t>(lookup_key))
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"[]",
					expression_to_json(*_parent_address)._value,
					expression_to_json(*_lookup_key)._value
				})};
			}


			std::shared_ptr<expression_t> _parent_address;
			std::shared_ptr<expression_t> _lookup_key;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key
		)
		{
			return expression_t{
				expression_type::k_lookup_element,
				std::make_shared<lookup_expr_t>(
					lookup_expr_t{ parent_address, lookup_key }
				)
			};
		}

		public: const lookup_expr_t* get_lookup() const {
			return dynamic_cast<const lookup_expr_t*>(_expr.get());
		}



		////////////////////////////////			make_vector_definition()



		public: struct vector_definition_exprt_t : public expr_base_t {
			public: virtual ~vector_definition_exprt_t(){};

			public: vector_definition_exprt_t(
				const typeid_t& element_type,
				const std::vector<expression_t>& elements
			)
			:
				_element_type(element_type),
				_elements(elements)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"vector-def",
					typeid_to_normalized_json(_element_type)._value,
					expressions_to_json(_elements)._value
				})};
			}


			const typeid_t _element_type;
			std::vector<expression_t> _elements;
		};

		public: static expression_t make_vector_definition(
			const typeid_t& element_type,
			const std::vector<expression_t>& elements
		)
		{
			return expression_t{
				expression_type::k_vector_definition,
				std::make_shared<vector_definition_exprt_t>(
					vector_definition_exprt_t{ element_type, elements }
				)
			};
		}

		public: const vector_definition_exprt_t* get_vector_definition() const {
			return dynamic_cast<const vector_definition_exprt_t*>(_expr.get());
		}



		////////////////////////////////			make_dict_definition()



		public: struct dict_definition_exprt_t : public expr_base_t {
			public: virtual ~dict_definition_exprt_t(){};

			public: dict_definition_exprt_t(
				const typeid_t& value_type,
				const std::map<std::string, expression_t>& elements
			)
			:
				_value_type(value_type),
				_elements(elements)
			{
			}

			public: virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"dict-def",
					typeid_to_normalized_json(_value_type)._value
//???					expressions_to_json(_elements)
				})};
			}


			const typeid_t _value_type;
			const std::map<std::string, expression_t> _elements;
		};

		public: static expression_t make_dict_definition(
			const typeid_t& value_type,
			const std::map<std::string, expression_t>& elements
		)
		{
			return expression_t{
				expression_type::k_dict_definition,
				std::make_shared<dict_definition_exprt_t>(
					dict_definition_exprt_t{ value_type, elements }
				)
			};
		}

		public: const dict_definition_exprt_t* get_dict_definition() const {
			return dynamic_cast<const dict_definition_exprt_t*>(_expr.get());
		}






		////////////////////////////////			OTHER


		public: bool check_invariant() const;

		public: bool operator==(const expression_t& other) const;


		public: expression_type get_operation() const;

		public: const expr_base_t* get_expr() const{
			return _expr.get();
		}


		//////////////////////////		INTERNALS


		private: expression_t(
			const expression_type operation,
			const std::shared_ptr<const expr_base_t>& expr
		);

//??? make const
		//////////////////////////		STATE
		private: std::string _debug;
		private: expression_type _operation;
		private: std::shared_ptr<const expr_base_t> _expr;
	};


	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	ast_json_t expression_to_json(const expression_t& e);

	ast_json_t expressions_to_json(const std::vector<expression_t> v);




	inline bool operator==(const expression_t::literal_expr_t& lhs, const expression_t::literal_expr_t& rhs){
		return lhs._value == rhs._value;
	}

	inline bool operator==(const expression_t::variable_expr_t& lhs, const expression_t::variable_expr_t& rhs){
		return lhs._variable == rhs._variable;
	}

	inline bool operator==(const expression_t::resolve_member_expr_t& lhs, const expression_t::resolve_member_expr_t& rhs){
		return compare_shared_values(lhs._parent_address, rhs._parent_address)
			&& lhs._member_name == rhs._member_name;
	}

	inline bool operator==(const expression_t::function_definition_expr_t& lhs, const expression_t::function_definition_expr_t& rhs){
		return lhs._def == rhs._def;
	}

	inline bool operator==(const expression_t::function_call_expr_t& lhs, const expression_t::function_call_expr_t& rhs){
		return
			lhs._function == rhs._function
			&& lhs._args == rhs._args;
	}
	inline bool operator==(const expression_t::vector_definition_exprt_t& lhs, const expression_t::vector_definition_exprt_t& rhs){
		return
			lhs._element_type == rhs._element_type
			&& lhs._elements == rhs._elements;
	}
	inline bool operator==(const expression_t::dict_definition_exprt_t& lhs, const expression_t::dict_definition_exprt_t& rhs){
		return
			lhs._value_type == rhs._value_type
			&& lhs._elements == rhs._elements;
	}

}	//	floyd


#endif /* expressions_hpp */
