//
//  expressions.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef expressions_hpp
#define expressions_hpp

#include "ast_json.h"
#include "ast_value.h"

#include "quark.h"

#include <vector>
#include <string>
#include <variant>


namespace floyd {
	struct value_t;
	struct statement_t;
	struct expression_t;

	const int k_no_host_function_id = 0;


	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	json_t expression_to_json(const expression_t& e);

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


		location_t _location;
		std::string _definition_name;
		typeid_t _function_type;
		std::vector<member_t> _args;
		std::shared_ptr<body_t> _body;
		int _host_function_id;
	};

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);
	json_t function_def_to_ast_json(const function_definition_t& v);
	function_definition_t json_to_function_def(const json_t& p);
	const typeid_t& get_function_type(const function_definition_t& f);


	json_t function_def_expression_to_ast_json(const function_definition_t& v);






	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {

		////////////////////////////////		literal_exp_t

		struct literal_exp_t {
			public: bool operator==(const literal_exp_t& other){ return value == other.value; }

			value_t value;
		};
		public: static expression_t make_literal(const value_t& value){
			return expression_t({ literal_exp_t{ value } }, std::make_shared<typeid_t>(value.get_type()));
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

		const value_t& get_literal() const{
			return std::get<literal_exp_t>(_contents).value;
		}

/*
		////////////////////////////////		simple_expression__2_t

		//??? Split into arithmetic vs comparison.
		struct simple_expression__2_t {
			expression_type op;
			std::shared_ptr<expression_t> lhs;
			std::shared_ptr<expression_t> rhs;
		};

		public: static expression_t make_simple_expression__2(expression_type op, const expression_t& lhs, const expression_t& rhs, const std::shared_ptr<typeid_t>& annotated_type){
			QUARK_ASSERT(is_arithmetic_expression(op) || is_comparison_expression(op));

			return expression_t({ simple_expression__2_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, annotated_type);
		}
*/
		////////////////////////////////		arithmetic_t

		//??? Split into arithmetic vs comparison.
		struct arithmetic_t {
			expression_type op;
			std::shared_ptr<expression_t> lhs;
			std::shared_ptr<expression_t> rhs;
		};

		public: static expression_t make_arithmetic(expression_type op, const expression_t& lhs, const expression_t& rhs, const std::shared_ptr<typeid_t>& annotated_type){
			QUARK_ASSERT(is_arithmetic_expression(op));

			return expression_t({ arithmetic_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, annotated_type);
		}


		////////////////////////////////		comparison_t


		struct comparison_t {
			expression_type op;
			std::shared_ptr<expression_t> lhs;
			std::shared_ptr<expression_t> rhs;
		};

		public: static expression_t make_comparison(expression_type op, const expression_t& lhs, const expression_t& rhs, const std::shared_ptr<typeid_t>& annotated_type){
			QUARK_ASSERT(is_comparison_expression(op));

			return expression_t({ comparison_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, annotated_type);
		}


		////////////////////////////////		unary_minus_t

		struct unary_minus_t {
			std::shared_ptr<expression_t> expr;
		};

		public: static expression_t make_unary_minus(const expression_t expr, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t({ unary_minus_t{ std::make_shared<expression_t>(expr) } }, annotated_type);
		}


		////////////////////////////////		conditional_t

		struct conditional_t {
			std::shared_ptr<expression_t> condition;
			std::shared_ptr<expression_t> a;
			std::shared_ptr<expression_t> b;
		};

		public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t(
				{ conditional_t{ std::make_shared<expression_t>(condition), std::make_shared<expression_t>(a), std::make_shared<expression_t>(b) } },
				annotated_type
			);
		}


		////////////////////////////////		call_t

		struct call_t {
			std::shared_ptr<expression_t> callee;
			std::vector<expression_t> args;
		};

		public: static expression_t make_call(
			const expression_t& callee,
			const std::vector<expression_t>& args,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t({ call_t{ std::make_shared<expression_t>(callee), args } }, annotated_type);
		}


		////////////////////////////////		struct_definition_expr_t

		struct struct_definition_expr_t {
			std::shared_ptr<const struct_definition_t> def;
		};

		public: static expression_t make_struct_definition(const std::shared_ptr<const struct_definition_t>& def){
			return expression_t({ struct_definition_expr_t{ def } }, std::make_shared<typeid_t>(typeid_t::make_struct1(def)));
		}


		////////////////////////////////		function_definition_expr_t

		struct function_definition_expr_t {
			std::shared_ptr<const function_definition_t> def;
		};

		public: static expression_t make_function_definition(const std::shared_ptr<const function_definition_t>& def){
			return expression_t({ function_definition_expr_t{ def } }, std::make_shared<typeid_t>(def->_function_type));
		}


		////////////////////////////////		load_t

		struct load_t {
			std::string variable_name;
		};

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_load(const std::string& variable, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t({ load_t{ variable } }, annotated_type);
		}


		////////////////////////////////		load2_t

		struct load2_t {
			variable_address_t address;
		};

		public: static expression_t make_load2(const variable_address_t& address, const std::shared_ptr<typeid_t>& annotated_type){
			return expression_t({ load2_t{ address } }, annotated_type);
		}


		////////////////////////////////		resolve_member_t

		struct resolve_member_t {
			std::shared_ptr<expression_t> parent_address;
			std::string member_name;
		};

		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const std::shared_ptr<typeid_t>& annotated_type
		){
			return expression_t({ resolve_member_t{ std::make_shared<expression_t>(parent_address), member_name } }, annotated_type);
		}


		////////////////////////////////		lookup_t

		struct lookup_t {
			std::shared_ptr<expression_t> parent_address;
			std::shared_ptr<expression_t> lookup_key;
		};

		//	Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const std::shared_ptr<typeid_t>& annotated_type
		)
		{
			return expression_t({ lookup_t{ std::make_shared<expression_t>(parent_address), std::make_shared<expression_t>(lookup_key) } }, annotated_type);
		}



		////////////////////////////////		value_constructor_t

		struct value_constructor_t {
			typeid_t value_type;
			std::vector<expression_t> elements;
		};

		public: static expression_t make_construct_value_expr(
			const typeid_t& value_type,
			const std::vector<expression_t>& args
		)
		{
			return expression_t({ value_constructor_t{ value_type, args } }, std::make_shared<typeid_t>(value_type));
		}


		////////////////////////////////			expression_t-stuff



		public: bool check_invariant() const{
			//	QUARK_ASSERT(_debug.size() > 0);
			//	QUARK_ASSERT(_result_type._base_type != base_type::k_internal_undefined && _result_type.check_invariant());
			QUARK_ASSERT(_output_type == nullptr || _output_type->check_invariant());
			return true;
		}

		public: bool operator==(const expression_t& other) const{
 			QUARK_ASSERT(false);
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return true
				&& _debug == other._debug
				&& location == other.location
//				&& _contents == other._contents
				&& compare_shared_values(_output_type, other._output_type)
				;
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
				|| std::holds_alternative<literal_exp_t>(_contents)
				|| std::holds_alternative<struct_definition_expr_t>(_contents)
				|| std::holds_alternative<function_definition_expr_t>(_contents)
				|| std::holds_alternative<value_constructor_t>(_contents)
				;
		}


		//??? Change this to a free function
		public: static bool check_types_resolved(const std::vector<expression_t>& expressions);

		//??? Change this to a free function
		public: bool check_types_resolved() const;


		//////////////////////////		INTERNALS


		typedef std::variant<
			literal_exp_t,
			arithmetic_t,
			comparison_t,
			unary_minus_t,
			conditional_t,
			call_t,
			struct_definition_expr_t,
			function_definition_expr_t,
			load_t,
			load2_t,
			resolve_member_t,
			lookup_t,
			value_constructor_t
		> expression_variant_t;

		private: expression_t(const expression_variant_t& contents, const std::shared_ptr<typeid_t>& output_type) :
			_debug(""),
			location(k_no_location),
			_contents(contents),
			_output_type(output_type)
		{
		#if DEBUG
			_debug = expression_to_json_string(*this);
		#endif
		}

		//////////////////////////		STATE
#if DEBUG
		public: std::string _debug;
#endif
		public: location_t location;
		public: expression_variant_t _contents;
		public: std::shared_ptr<typeid_t> _output_type;
	};



}	//	floyd


#endif /* expressions_hpp */
