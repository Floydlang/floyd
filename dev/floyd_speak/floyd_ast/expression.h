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
#include "ast_json.h"

#include "ast_value.h"


namespace floyd {
	struct ast_json_t;
	struct value_t;
	struct statement_t;
	struct expression_t;

	const int k_no_host_function_id = 0;


	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	ast_json_t expression_to_json(const expression_t& e);

	std::string expression_to_json_string(const expression_t& e);



	//////////////////////////////////////////////////		function_definition_t


	struct function_definition_t {
		public: bool check_invariant() const {
			if(_host_function_id != k_no_host_function_id){
				QUARK_ASSERT(!_body);
			}
			else{
				QUARK_ASSERT(_body);
			}
			return true;
		}

		public: bool check_types_resolved() const;


		const location_t _location;
		const typeid_t _function_type;
		const std::vector<member_t> _args;
		const std::shared_ptr<body_t> _body;
		const int _host_function_id;
	};

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);
	ast_json_t function_def_to_ast_json(const function_definition_t& v);
	const typeid_t& get_function_type(const function_definition_t& f);



	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {

		public: static expression_t make_literal(const value_t& value){
			return expression_t(
				expression_type::k_literal,
				{ },
				std::make_shared<typeid_t>(value.get_type()),
				value,
				{},
				{},
				"",
				{}
			);
		}

		public: static expression_t make_literal_undefined(){
			return make_literal(value_t::make_undefined());
		}
		public: static expression_t make_literal_internal_dynamic(){
			return make_literal(value_t::make_internal_dynamic());
		}
		public: static expression_t make_literal_void(){
			return make_literal(value_t::make_void());
		}
		public: static expression_t make_literal_int(const int i){
			return make_literal(value_t::make_int(i));
		}
		public: static expression_t make_literal_bool(const bool i){
			return make_literal(value_t::make_bool(i));
		}
		public: static expression_t make_literal_double(const double i){
			return make_literal(value_t::make_double(i));
		}
		public: static expression_t make_literal_string(const std::string& s){
			return make_literal(value_t::make_string(s));
		}

		inline bool is_literal() const{
			return _operation == expression_type::k_literal;
		}

		inline const value_t& get_literal() const{
			QUARK_ASSERT(is_literal())

			return _value;
		}

		//??? Split into arithmetic vs comparison.

		public: static expression_t make_simple_expression__2(expression_type op, const expression_t& left, const expression_t& right, const std::shared_ptr<typeid_t>& annotated_type){
			if(is_arithmetic_expression(op) || is_comparison_expression(op)){
				return expression_t(
					op,
					{ left, right },
					annotated_type,
					{},
					{},
					{},
					"",
				{}
				);
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
		}

		public: static expression_t make_unary_minus(const expression_t expr, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t(
				expression_type::k_arithmetic_unary_minus__1,
				{ expr },
				annotated_type,
				{},
				{},
				{},
				"",
				{}
			);
		}

		public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t(
				expression_type::k_conditional_operator3,
				{ condition, a, b },
				annotated_type,
				{},
				{},
				{},
				"",
				{}
			);
		}

		public: static expression_t make_call(
			const expression_t& callee,
			const std::vector<expression_t>& args,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			std::vector<expression_t> e = { callee };
			e.insert(e.end(), args.begin(), args.end());

			return expression_t(
				expression_type::k_call,
				e,
				annotated_type,
				{},
				{},
				{},
				"",
				{}
			);
		}

		public: static expression_t make_struct_definition(const std::shared_ptr<const struct_definition_t>& def){
			return expression_t(
				expression_type::k_struct_def,
				{},
				std::make_shared<typeid_t>(typeid_t::make_struct1(def)),
				{},
				def,
				{},
				"",
				{}
			);
		}

		public: static expression_t make_function_definition(const std::shared_ptr<const function_definition_t>& def){
			return expression_t(
				expression_type::k_function_def,
				{},
				std::make_shared<typeid_t>(def->_function_type),
				{},
				{},
				def,
				"",
				{}
			);
		}

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_load(const std::string& variable, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t(
				expression_type::k_load,
				{},
				annotated_type,
				{},
				{},
				{},
				variable,
				{}
			);
		}

		public: static expression_t make_load2(const variable_address_t& address, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t(
				expression_type::k_load2,
				{},
				annotated_type,
				{},
				{},
				{},
				"",
				address
			);
		}

		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const std::shared_ptr<typeid_t>& annotated_type
		){
			return expression_t(
				expression_type::k_resolve_member,
				{parent_address},
				annotated_type,
				{},
				{},
				{},
				member_name,
				{}
			);
		}

		//	Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t(
				expression_type::k_lookup_element,
				{ parent_address, lookup_key },
				annotated_type,
				{},
				{},
				{},
				"",
				{}
			);
		}

		public: static expression_t make_construct_value_expr(
			const typeid_t& value_type,
			const std::vector<expression_t>& args
		)
		{
			return expression_t(
				expression_type::k_value_constructor,
				args,
				std::make_shared<typeid_t>(value_type),
				{},
				{},
				{},
				"",
				{}
			);
		}


		////////////////////////////////			expression_t-stuff


		public: bool check_invariant() const{
			//	QUARK_ASSERT(_debug.size() > 0);
			//	QUARK_ASSERT(_result_type._base_type != base_type::k_internal_undefined && _result_type.check_invariant());
			QUARK_ASSERT(_output_type == nullptr || _output_type->check_invariant());
			return true;
		}
		public: bool operator==(const expression_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return true
				&& _operation == other._operation
				&& compare_shared_values(_output_type, other._output_type)
				&& _input_exprs == other._input_exprs
				&& _value == other._value

				&& compare_shared_values(_struct_def, other._struct_def)
				&& compare_shared_values(_function_def, other._function_def)

				&& _variable_name == other._variable_name
				&& _address == other._address
				;
		}
		public: expression_type get_operation() const{
			QUARK_ASSERT(check_invariant());

			return _operation;
		}

		public: typeid_t get_output_type() const {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(_output_type != nullptr);

			return *_output_type;
		}

		public: bool is_annotated_shallow() const{
			QUARK_ASSERT(check_invariant());

			return _output_type != nullptr;
		}

		public: bool has_builtin_type() const {
			QUARK_ASSERT(check_invariant());

			return false
				|| _operation == expression_type::k_literal
				|| _operation == expression_type::k_struct_def
				|| _operation == expression_type::k_function_def
				|| _operation == expression_type::k_value_constructor
				;
		}


		public: bool check_types_resolved() const{
			QUARK_ASSERT(check_invariant());

			if(_output_type == nullptr || _output_type->check_types_resolved() == false){
				return false;
			}

			for(const auto& a: _input_exprs){
				if(a.check_types_resolved() == false){
					return false;
				}
			}

			return true;
		}


		//////////////////////////		INTERNALS


		public: expression_t(
			const expression_type operation,
			const std::vector<expression_t>& input_exprs,
			const std::shared_ptr<typeid_t>& output_type,
			const value_t& value,
			const std::shared_ptr<const struct_definition_t>& struct_def,
			const std::shared_ptr<const function_definition_t>& function_def,
			const std::string& variable_name,
			const variable_address_t& address
		)
		:
		#if DEBUG
			_debug(""),
		#endif
			_operation(operation),
			_input_exprs(input_exprs),
			_output_type(output_type),
			_value(value),
			_struct_def(struct_def),
			_function_def(function_def),
			_variable_name(variable_name),
			_address(address)
		{
			QUARK_ASSERT(output_type == nullptr || output_type->check_invariant());

		#if DEBUG
			_debug = expression_to_json_string(*this);
		#endif

			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////		STATE
#if DEBUG
		private: std::string _debug;
#endif
		public: expression_type _operation;
		public: std::shared_ptr<typeid_t> _output_type;

		public: std::vector<expression_t> _input_exprs;
		public: value_t _value;
		public: std::shared_ptr<const struct_definition_t> _struct_def;
		public: std::shared_ptr<const function_definition_t> _function_def;
		public: std::string _variable_name;
		public: variable_address_t _address;
	};



}	//	floyd


#endif /* expressions_hpp */
