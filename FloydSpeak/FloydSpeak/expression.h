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

#include "ast_value.h"
#include "pass2.h"


namespace floyd {
	struct ast_json_t;
	struct value_t;
	struct statement_t;
	struct expression_t;


	//	"+", "<=", "&&" etc.
	bool is_simple_expression__2(const std::string& op);
	json_t annotate_json(const json_t& j, const expression_t& e);





	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {

		private: struct expr_base_t {
			virtual ~expr_base_t(){};

			virtual ast_json_t expr_base__to_json() const{ return ast_json_t(); };
		};

		////////////////////////////////			literal_expr_t


		//### Not really "literals". "Terminal values" maybe better term?

		public: struct literal_expr_t : public expr_base_t {
			literal_expr_t(const value_t& value)
			:
				_value(value)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"k",
					value_to_ast_json(_value)._value,
					typeid_to_ast_json(_value.get_type())._value
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
				),
				std::make_shared<typeid_t>(value.get_type())
			};
		}

		public: static expression_t make_literal_null(){
			return make_literal(value_t::make_null());
		}
		public: static expression_t make_literal_int(const int i){
			return make_literal(value_t::make_int(i));
		}
		public: static expression_t make_literal_bool(const bool i){
			return make_literal(value_t::make_bool(i));
		}
		public: static expression_t make_literal_float(const float i){
			return make_literal(value_t::make_float(i));
		}
		public: static expression_t make_literal_string(const std::string& s){
			return make_literal(value_t::make_string(s));
		}

		inline bool is_literal() const{
			return dynamic_cast<const literal_expr_t*>(_expr.get()) != nullptr;
		}

		inline const value_t& get_literal() const{
			QUARK_ASSERT(is_literal())

			return dynamic_cast<const literal_expr_t*>(_expr.get())->_value;
		}



		////////////////////////////////			simple_expr__2_t


		public: struct simple_expr__2_t : public expr_base_t {
			simple_expr__2_t(
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

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{
					json_t::make_array({
						expression_type_to_token(_op),
						expression_to_json(*_left)._value,
						expression_to_json(*_right)._value
					})
				};
			}


			const expression_type _op;
			const std::shared_ptr<expression_t> _left;
			const std::shared_ptr<expression_t> _right;
		};

		public: static expression_t make_simple_expression__2(expression_type op, const expression_t& left, const expression_t& right, const std::shared_ptr<typeid_t>& annotated_type){
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
					),
					annotated_type
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
					),
					annotated_type
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
				throw std::exception();
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}

		public: const simple_expr__2_t* get_simple__2() const {
			return dynamic_cast<const simple_expr__2_t*>(_expr.get());
		}


		////////////////////////////////			unary_minus_expr_t


		public: struct unary_minus_expr_t : public expr_base_t {
			unary_minus_expr_t(
				const expression_t& expr
			)
			:
				_expr(std::make_shared<expression_t>(expr))
			{
			}


			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					expression_to_json(*_expr)._value
				})};
			}


			const std::shared_ptr<expression_t> _expr;
		};

		public: static expression_t make_unary_minus(const expression_t expr, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t{
				expression_type::k_arithmetic_unary_minus__1,
				std::make_shared<unary_minus_expr_t>(
					unary_minus_expr_t{ expr }
				),
				annotated_type
			};
		}

		public: const unary_minus_expr_t* get_unary_minus() const {
			return dynamic_cast<const unary_minus_expr_t*>(_expr.get());
		}



		////////////////////////////////			conditional_expr_t


		public: struct conditional_expr_t : public expr_base_t {
			conditional_expr_t(
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

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"?:",
					expression_to_json(*_condition)._value,
					expression_to_json(*_a)._value,
					expression_to_json(*_b)._value,
				})};
			}


			const std::shared_ptr<expression_t> _condition;
			const std::shared_ptr<expression_t> _a;
			const std::shared_ptr<expression_t> _b;
		};

		public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t{
				expression_type::k_conditional_operator3,
				std::make_shared<conditional_expr_t>(
					conditional_expr_t{ condition, a, b }
				),
				annotated_type
			};
		}

		public: const conditional_expr_t* get_conditional() const {
			return dynamic_cast<const conditional_expr_t*>(_expr.get());
		}


		////////////////////////////////			call_expr_t


		public: struct call_expr_t : public expr_base_t {
			call_expr_t(
				const expression_t& callee,
				std::vector<expression_t> args
			)
			:
				_callee(std::make_shared<expression_t>(callee)),
				_args(args)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"call",
					expression_to_json(*_callee)._value,
					expressions_to_json(_args)._value
				})};
			}


			const std::shared_ptr<expression_t> _callee;
			const std::vector<expression_t> _args;
		};

		public: static expression_t make_call(
			const expression_t& callee,
			const std::vector<expression_t>& args,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t{
				expression_type::k_call,
				std::make_shared<call_expr_t>(
					call_expr_t{ callee, args }
				),
				annotated_type
			};
		}

		public: const call_expr_t* get_call() const {
			return dynamic_cast<const call_expr_t*>(_expr.get());
		}


		////////////////////////////////			function_definition_expr_t


		public: struct function_definition_expr_t : public expr_base_t {
			function_definition_expr_t(const function_definition_t & def)
			:
				_def(def)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{function_def_to_ast_json(_def)};
			}


			const function_definition_t _def;
		};

		public: static expression_t make_function_definition(const function_definition_t& def){
			return expression_t{
				expression_type::k_define_function,
				std::make_shared<function_definition_expr_t>(
					function_definition_expr_t{ function_definition_t(def) }
				),
				std::make_shared<typeid_t>(get_function_type(def))
			};
		}

		public: const function_definition_expr_t* get_function_definition() const {
			return dynamic_cast<const function_definition_expr_t*>(_expr.get());
		}


		////////////////////////////////			variable_expr_t


		public: struct variable_expr_t : public expr_base_t {
			variable_expr_t(
				std::string variable
			)
			:
				_variable(variable)
			{
			}


			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({ "@", json_t(_variable) })};
			}


			const std::string _variable;
		};


		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_variable_expression(const std::string& variable, const std::shared_ptr<typeid_t>& annotated_type)
		{
			return expression_t{
				expression_type::k_variable,
				std::make_shared<variable_expr_t>(
					variable_expr_t{ variable }
				),
				annotated_type
			};
		}

		public: const variable_expr_t* get_variable() const {
			return dynamic_cast<const variable_expr_t*>(_expr.get());
		}


		////////////////////////////////			resolve_member_expr_t


		public: struct resolve_member_expr_t : public expr_base_t {
			resolve_member_expr_t(
				const expression_t& parent_address,
				std::string member_name
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_member_name(member_name)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
				 	"->",
				 	expression_to_json(*_parent_address)._value,
				 	json_t(_member_name)
				})};
			}


			const std::shared_ptr<expression_t> _parent_address;
			const std::string _member_name;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t{
				expression_type::k_resolve_member,
				std::make_shared<resolve_member_expr_t>(
					resolve_member_expr_t{ parent_address, member_name }
				),
				annotated_type
			};
		}

		public: const resolve_member_expr_t* get_resolve_member() const {
			return dynamic_cast<const resolve_member_expr_t*>(_expr.get());
		}


		////////////////////////////////			lookup_expr_t

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/

		public: struct lookup_expr_t : public expr_base_t {
			lookup_expr_t(
				const expression_t& parent_address,
				const expression_t& lookup_key
			)
			:
				_parent_address(std::make_shared<expression_t>(parent_address)),
				_lookup_key(std::make_shared<expression_t>(lookup_key))
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"[]",
					expression_to_json(*_parent_address)._value,
					expression_to_json(*_lookup_key)._value
				})};
			}


			const std::shared_ptr<expression_t> _parent_address;
			const std::shared_ptr<expression_t> _lookup_key;
		};

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t{
				expression_type::k_lookup_element,
				std::make_shared<lookup_expr_t>(
					lookup_expr_t{ parent_address, lookup_key }
				),
				annotated_type
			};
		}

		public: const lookup_expr_t* get_lookup() const {
			return dynamic_cast<const lookup_expr_t*>(_expr.get());
		}



		////////////////////////////////			vector_definition_exprt_t



		public: struct vector_definition_exprt_t : public expr_base_t {

			vector_definition_exprt_t(
				const typeid_t& element_type,
				const std::vector<expression_t>& elements
			)
			:
				_element_type(element_type),
				_elements(elements)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				return ast_json_t{json_t::make_array({
					"vector-def",
					typeid_to_ast_json(_element_type)._value,
					expressions_to_json(_elements)._value
				})};
			}


			const typeid_t _element_type;
			const std::vector<expression_t> _elements;
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
				),
				std::make_shared<typeid_t>(typeid_t::make_vector(element_type))
			};
		}

		//???rename vector_definition_expr!
		public: const vector_definition_exprt_t* get_vector_definition() const {
			return dynamic_cast<const vector_definition_exprt_t*>(_expr.get());
		}



		////////////////////////////////			dict_definition_exprt_t



		public: struct dict_definition_exprt_t : public expr_base_t {
			dict_definition_exprt_t(
				const typeid_t& value_type,
				const std::map<std::string, expression_t>& elements
			)
			:
				_value_type(value_type),
				_elements(elements)
			{
			}

			virtual ast_json_t expr_base__to_json() const {
				std::map<std::string, json_t> dict_contents;
				for(const auto e: _elements){
					const auto key = e.first;
					const auto value = e.second;
					const auto e2 = std::pair<std::string, json_t>{ key, expression_to_json(value)._value};
					dict_contents.insert(e2);
				}
				return ast_json_t{json_t::make_array({
					"dict-def",
					typeid_to_ast_json(_value_type)._value,
					dict_contents
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
				),
				std::make_shared<typeid_t>(typeid_t::make_dict(value_type))
			};
		}

		public: const dict_definition_exprt_t* get_dict_definition() const {
			return dynamic_cast<const dict_definition_exprt_t*>(_expr.get());
		}




		////////////////////////////////			expression_t-stuff



		public: bool check_invariant() const{
			//	QUARK_ASSERT(_debug.size() > 0);
			//	QUARK_ASSERT(_result_type._base_type != base_type::k_null && _result_type.check_invariant());
			QUARK_ASSERT(_annotated_type == nullptr || _annotated_type->check_invariant());
			return true;
		}
		public: bool operator==(const expression_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			if(_expr && other._expr){
				QUARK_ASSERT(false);
		/*
				const auto lhs = dynamic_cast<const function_definition_expr_t*>(_expr.get());
				const auto rhs = dynamic_cast<const function_definition_expr_t*>(other._expr.get());
				if(lhs && rhs && *lhs == *rhs){
					return true;
				}
		*/
				return false;
			}
			else if((_expr && !other._expr) || (!_expr && other._expr)){
				return false;
			}
			else{
				return
					(_operation == other._operation);
			}
		}
		public: expression_type get_operation() const{
			QUARK_ASSERT(check_invariant());

			return _operation;
		}
		public: const expr_base_t* get_expr() const{
			QUARK_ASSERT(check_invariant());
			return _expr.get();
		}



		public: typeid_t get_annotated_type() const {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_annotated_type != nullptr);

			return *_annotated_type;
		}
		public: std::shared_ptr<typeid_t> get_annotated_type2() const {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_annotated_type != nullptr);

			return _annotated_type;
		}

/*
		public: expression_t annotate(const typeid_t& annotated_type) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(annotated_type.check_invariant());

			const auto result = expression_t(_operation, _expr, std::make_shared<typeid_t>(annotated_type));

			QUARK_ASSERT(result.check_invariant());
			QUARK_ASSERT(result.is_annotated_deep());
			return result;
		}
*/

		public: bool has_builtin_type() const {
			QUARK_ASSERT(check_invariant());

			if(false
			|| _operation == expression_type::k_literal
			|| _operation == expression_type::k_define_function
			|| _operation == expression_type::k_vector_definition
			|| _operation == expression_type::k_dict_definition
			){
				return true;
			}
			else{
				return false;
			}
		}

/*
		//	Notice that some expression types has built-in types. ??? refactor this.
		public: expression_t annotate_type(const typeid_t& type) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(type.check_invariant());

			if(_operation == expression_type::k_literal){
				return expression_t(_operation, literal_expr_t(get_literal()->_value), type);
			}
			else if(_operation == expression_type::k_literal){
			}
			else if(_operation == expression_type::k_define_function){
			}
			else if(_operation == expression_type::k_vector_definition){
			}
			else if(_operation == expression_type::k_dict_definition){
			}
			else{
				return expression_t(_operation, _expr, std::make_shared<typeid_t>(type));
			}
		}
*/

		public: bool is_annotated_shallow() const{
			QUARK_ASSERT(check_invariant());

			return _annotated_type != nullptr;
		}

		public: bool is_annotated_deep() const{
			QUARK_ASSERT(check_invariant());
			if(_annotated_type == nullptr){
				return false;
			}

			//	Annotated OK, now check input expressions too.
			else{
				const auto op = get_operation();

				if(op == expression_type::k_literal){
					return true;
				}
				else if(op == expression_type::k_resolve_member){
					const auto e = *get_resolve_member();
					return e._parent_address->is_annotated_deep();
				}
				else if(op == expression_type::k_lookup_element){
					const auto e = *get_lookup();
					return e._parent_address->is_annotated_deep() && e._lookup_key;
				}
				else if(op == expression_type::k_variable){
					return true;
				}
				else if(op == expression_type::k_call){
					const auto e = *get_call();
					if(e._callee->is_annotated_deep() == false){
						return false;
					}
					else{
						for(const auto a: e._args){
							if(a.is_annotated_deep() == false){
								return false;
							}
						}
						return true;
					}
				}

				else if(op == expression_type::k_define_function){
					//??? check functions statements too.
					const auto e = *get_function_definition();
					return true;
				}

				else if(op == expression_type::k_vector_definition){
					const auto e = *get_vector_definition();
					for(const auto a: e._elements){
						if(a.is_annotated_deep() == false){
							return false;
						}
					}
					return true;
				}

				else if(op == expression_type::k_dict_definition){
					const auto e = *get_dict_definition();
					for(const auto kv: e._elements){
						if(kv.second.is_annotated_deep() == false){
							return false;
						}
					}
					return true;
				}

				//	This can be desugared at compile time.
				else if(op == expression_type::k_arithmetic_unary_minus__1){
					const auto e = *get_unary_minus();
					return e._expr->is_annotated_deep();
				}

				//	Special-case since it uses 3 expressions & uses shortcut evaluation.
				else if(op == expression_type::k_conditional_operator3){
					const auto e = *get_conditional();
					return e._condition->is_annotated_deep() && e._a->is_annotated_deep() && e._b->is_annotated_deep();
				}
				else if (false
					|| op == expression_type::k_comparison_smaller_or_equal__2
					|| op == expression_type::k_comparison_smaller__2
					|| op == expression_type::k_comparison_larger_or_equal__2
					|| op == expression_type::k_comparison_larger__2

					|| op == expression_type::k_logical_equal__2
					|| op == expression_type::k_logical_nonequal__2
				){
					const auto e = *get_simple__2();
					return e._left->is_annotated_deep() && e._right->is_annotated_deep();
				}
				else if (false
					|| op == expression_type::k_arithmetic_add__2
					|| op == expression_type::k_arithmetic_subtract__2
					|| op == expression_type::k_arithmetic_multiply__2
					|| op == expression_type::k_arithmetic_divide__2
					|| op == expression_type::k_arithmetic_remainder__2

					|| op == expression_type::k_logical_and__2
					|| op == expression_type::k_logical_or__2
				){
					const auto e = *get_simple__2();
					return e._left->is_annotated_deep() && e._right->is_annotated_deep();
				}
				else{
					QUARK_ASSERT(false);
					throw std::exception();
				}
			}

		}



		//////////////////////////		INTERNALS


		public: expression_t(
			const expression_type operation,
			const std::shared_ptr<const expr_base_t>& expr,
			const std::shared_ptr<typeid_t> annotated_type
		)
		:
		#if DEBUG
			_debug(""),
		#endif
			_operation(operation),
			_expr(expr),
			_annotated_type(annotated_type)
		{
			QUARK_ASSERT(expr != nullptr);
			QUARK_ASSERT(annotated_type == nullptr || annotated_type->check_invariant());

		#if DEBUG
			_debug = expression_to_json_string(*this);
		#endif

			QUARK_ASSERT(check_invariant());
		}


		//??? make const
		//////////////////////////		STATE
#if DEBUG
		private: std::string _debug;
#endif
		private: expression_type _operation;
		private: std::shared_ptr<const expr_base_t> _expr;
		private: std::shared_ptr<typeid_t> _annotated_type;
	};


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

	inline bool operator==(const expression_t::call_expr_t& lhs, const expression_t::call_expr_t& rhs){
		return
			lhs._callee == rhs._callee
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

	inline bool is_simple_expression__2(const std::string& op){
		return
			op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
			|| op == "<=" || op == "<" || op == ">=" || op == ">"
			|| op == "==" || op == "!=" || op == "&&" || op == "||";
	}


}	//	floyd


#endif /* expressions_hpp */
