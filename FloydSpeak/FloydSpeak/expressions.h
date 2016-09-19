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

	struct json_value_t;

/*
	ABOUT ADDRESSING AND CHAINS

	Each value are refered to via an address which is a construct like this: { scope + member }.
	Encoded as { value_t scope, string member_name } or { value_t scope int _member_offset }.

	This address can specifify any value in the system:

	- To a local variable in the function-body-scope
	- a function argument in the function-scope
	- a global variable as a value in the global scope
	- point to a struct + which member.


	LOAD is performed via an address.
	STORE is performed via an addres
	CALL is made on a function_scope


		PROBLEM: How to resolve a complex address expression tree into something you can read a value from (or store a value to or call as a function etc.
		We don't have any value we can return from each expression in tree.
		Alternatives:

		A) Have dedicated expression types:
			struct_member_address_t { expression_t _parent_address, struct_def* _def, shared_ptr<struct_instance_t> _instance, string _member_name; }
			collection_lookup { vector_def* _def, shared_ptr<vector_instance_t> _instance, value_t _key };

		B)	Have value_t of type struct_member_spec_t { string member_name, value_t} so a value_t can point to a specific member variable.
		C)	parse address in special function that resolves the expression and keeps the actual address on the side. Address can be raw C++ pointer.


	CHAINS
	"hello.test.a[10 + x].next.last.get_ptr().title"

	call						"call"
	resolve_variable			"@"
	resolve_member				"->"
	lookup						"[-]"

	!!! AST DOES NOT GENERATE LOADs, ONLY IDENTIFIER, FOR EXAMPLE.


		a = my_global_int;
		["bind", "<int>", "a", ["@", "my_global_int"]]

		"my_global.next"
		["->", ["@", "my_global"], "next"]

		c = my_global_obj.all[3].f(10).prev;
*/

namespace floyd_parser {
	struct expression_t;
	struct value_t;
	struct scope_def_t;
	struct type_identifier_t;


	//////////////////////////////////////////////////		constant expression
	
	//	Has no type, uses value_t.
	

	//////////////////////////////////////////////////		math_operation1_expr_t


	struct math_operation1_expr_t {
		bool operator==(const math_operation1_expr_t& other) const;


		enum operation {
			negate = 20
		};

		const operation _operation;
		const std::shared_ptr<expression_t> _input;
	};


	//////////////////////////////////////////////////		math_operation2_expr_t


	struct math_operation2_expr_t {
		bool operator==(const math_operation2_expr_t& other) const;


		enum operation {
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
			k_logical_or
		};

		const operation _operation;
		const std::shared_ptr<expression_t> _left;
		const std::shared_ptr<expression_t> _right;
	};


	//////////////////////////////////////////////////		conditional_operator_expr_t


	struct conditional_operator_expr_t {
		bool operator==(const conditional_operator_expr_t& other) const;


		const std::shared_ptr<expression_t> _condition;
		const std::shared_ptr<expression_t> _a;
		const std::shared_ptr<expression_t> _b;
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


	//////////////////////////////////////////////////		load_expr_t

	/*
		Supports reading a named variable, like "int a = 10; print(a);"

		The address must resolve to a resolve_member_expr_t to be able to execute LOAD.
	*/
	struct load_expr_t {
		bool operator==(const load_expr_t& other) const;


		std::shared_ptr<expression_t> _address;
	};


	//////////////////////////////////////////////////		resolve_variable_expr_t



	/*
		Specify free variables.
		It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.

		When compiler resolves this expression it may replace it with a resolve_
	*/
	struct resolve_variable_expr_t {
		bool operator==(const resolve_variable_expr_t& other) const;

		const std::string _variable_name;
	};

	//////////////////////////////////////////////////		resolve_member_expr_t



	/*
		Specifies a member variable of a struct.
	*/
	struct resolve_member_expr_t {
		bool operator==(const resolve_member_expr_t& other) const;

		std::shared_ptr<expression_t> _parent_address;
		const std::string _member_name;
	};


	//////////////////////////////////////////////////		lookup_element_expr_t

	/*
		Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
	*/
	struct lookup_element_expr_t {
		bool operator==(const lookup_element_expr_t& other) const;

		std::shared_ptr<expression_t> _parent_address;
		std::shared_ptr<expression_t> _lookup_key;
	};



	//////////////////////////////////////////////////		expression_t


	struct expression_t {
		public: static expression_t make_constant(const value_t& value);

		//	Shortcuts were you don't need to make a value_t first.
		public: static expression_t make_constant(const bool i);
		public: static expression_t make_constant(const int i);
		public: static expression_t make_constant(const float f);
		public: static expression_t make_constant(const char s[]);
		public: static expression_t make_constant(const std::string& s);


		public: static expression_t make_math_operation1(
			math_operation1_expr_t::operation op,
			const expression_t& input
		);
		public: static expression_t make_math_operation2(
			math_operation2_expr_t::operation op,
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


		public: static expression_t make_load(
			const expression_t& address_expression,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		public: static expression_t make_resolve_variable(
			const std::string& variable,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

		//??? Why shared_ptr?
		public: static expression_t make_resolve_member(
			const std::shared_ptr<expression_t>& parent_address,
			const std::string& member_name,
			const std::shared_ptr<const type_def_t>& resolved_expression_type
		);

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
			Only one of there are used at any time.
		*/
		public: std::shared_ptr<value_t> _constant;
		public: std::shared_ptr<math_operation1_expr_t> _math1;
		public: std::shared_ptr<math_operation2_expr_t> _math2;
		public: std::shared_ptr<conditional_operator_expr_t> _conditional_operator;
		public: std::shared_ptr<function_call_expr_t> _call;
		public: std::shared_ptr<load_expr_t> _load;

		public: std::shared_ptr<resolve_variable_expr_t> _resolve_variable;
		public: std::shared_ptr<resolve_member_expr_t> _resolve_member;
		public: std::shared_ptr<lookup_element_expr_t> _lookup_element;

		//	Tell what type of value this expression represents. Null if not yet defined.
		public: std::shared_ptr<const type_def_t> _resolved_expression_type;
	};


	//////////////////////////////////////////////////		visit()


/*
	struct visit_expression_i {
		public: virtual ~visit_expression_i(){};

		public: virtual expression_t visit_expression_i__on_constant(const value_t& value) const = 0;
		public: virtual expression_t visit_expression_i__on_math1(const math_operation1_expr_t& e) const = 0;
		public: virtual expression_t visit_expression_i__on_math2(const math_operation2_expr_t& e) const = 0;
		public: virtual expression_t visit_expression_i__on_call(const function_call_expr_t& e) const = 0;
		public: virtual expression_t visit_expression_i__on_load(const load_expr_t& e) const = 0;

		public: virtual expression_t visit_expression_i__on_resolve_variable(const resolve_variable_expr_t& e) const = 0;
		public: virtual expression_t visit_expression_i__on_resolve_member(const resolve_member_expr_t& e) const = 0;
		public: virtual expression_t visit_expression_i__on_lookup_element(const lookup_element_expr_t& e) const = 0;
	};

	expression_t visit(const visit_expression_i& visitor, const expression_t& e);
*/
	//////////////////////////////////////////////////		trace()


	void trace(const expression_t& e);

	json_value_t expression_to_json(const expression_t& e);

}	//	floyd_parser


#endif /* expressions_hpp */
