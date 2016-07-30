//
//  parse_expression.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_expression_hpp
#define parse_expression_hpp

#include "quark.h"
#include <vector>
#include <string>

#include "parser_types.h"
#include "parser_value.hpp"


namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct ast_t;

	//////////////////////////////////////////////////		function_def_expr_t


	struct function_def_expr_t {
		bool check_invariant() const {
			QUARK_ASSERT(_def);
			QUARK_ASSERT(_def->check_invariant());
			return true;
		}

		bool operator==(const function_def_expr_t& other) const{
			return *_def == *other._def;
		}

		std::shared_ptr<function_def_t> _def;
	};



	//////////////////////////////////////////////////		struct_def_expr_t


	struct struct_def_expr_t {
		bool check_invariant() const {
			QUARK_ASSERT(_def);
			QUARK_ASSERT(_def->check_invariant());
			return true;
		}

		bool operator==(const struct_def_expr_t& other) const{
			return *_def == *other._def;
		}

		const std::shared_ptr<struct_def_t> _def;
	};



	//////////////////////////////////////////////////		math_operation2_expr_t


	struct math_operation2_expr_t {
		bool operator==(const math_operation2_expr_t& other) const;


		enum operation {
			add = 10,
			subtract,
			multiply,
			divide
		};

		const operation _operation;

		const std::shared_ptr<expression_t> _left;
		const std::shared_ptr<expression_t> _right;
	};


	//////////////////////////////////////////////////		math_operation1_expr_t


	struct math_operation1_expr_t {
		bool operator==(const math_operation1_expr_t& other) const;


		enum operation {
			negate = 20
		};

		const operation _operation;

		const std::shared_ptr<expression_t> _input;
	};


	//////////////////////////////////////////////////		function_call_expr_t


	struct function_call_expr_t {
		bool operator==(const function_call_expr_t& other) const{
			return _function_name == other._function_name && _inputs == other._inputs;
		}


		const std::string _function_name;
		const std::vector<std::shared_ptr<expression_t>> _inputs;
	};


	//////////////////////////////////////////////////		variable_read_expr_t


	struct variable_read_expr_t {
		bool operator==(const variable_read_expr_t& other) const{
			return _variable_name == other._variable_name ;
		}


		const std::string _variable_name;
	};


	//////////////////////////////////////////////////		expression_t


	struct expression_t {
		public: bool check_invariant() const{
			return true;
		}

		expression_t() :
			_nop_expr(true)
		{
			QUARK_ASSERT(false);
		}

		expression_t(const function_def_expr_t& value) :
			_function_def_expr(std::make_shared<const function_def_expr_t>(value))
		{
		}

		expression_t(const struct_def_expr_t& value) :
			_struct_def_expr(std::make_shared<const struct_def_expr_t>(value))
		{
		}


		expression_t(const value_t& value) :
			_constant(std::make_shared<value_t>(value))
		{
			QUARK_ASSERT(value.check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		expression_t(const math_operation2_expr_t& value) :
			_math_operation2_expr(std::make_shared<math_operation2_expr_t>(value))
		{
			QUARK_ASSERT(check_invariant());
		}
		expression_t(const math_operation1_expr_t& value) :
			_math_operation1_expr(std::make_shared<math_operation1_expr_t>(value))
		{
			QUARK_ASSERT(check_invariant());
		}

		expression_t(const function_call_expr_t& value) :
			_call_function_expr(std::make_shared<function_call_expr_t>(value))
		{
			QUARK_ASSERT(check_invariant());
		}

		expression_t(const variable_read_expr_t& value) :
			_variable_read_expr(std::make_shared<variable_read_expr_t>(value))
		{
			QUARK_ASSERT(check_invariant());
		}

		bool operator==(const expression_t& other) const {
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			if(_function_def_expr){
				return other._function_def_expr && *_function_def_expr == *other._function_def_expr;
			}
			else if(_constant){
				return other._constant && *_constant == *other._constant;
			}
			else if(_math_operation2_expr){
				return other._math_operation2_expr && *_math_operation2_expr == *other._math_operation2_expr;
			}
			else if(_math_operation1_expr){
				return other._math_operation1_expr && *_math_operation1_expr == *other._math_operation1_expr;
			}
			else if(_call_function_expr){
				return other._call_function_expr && *_call_function_expr == *other._call_function_expr;
			}
			else if(_variable_read_expr){
				return other._variable_read_expr && *_variable_read_expr == *other._variable_read_expr;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		//////////////////////////		STATE

		/*
			Only one of there are used at any time.
		*/
		std::shared_ptr<const function_def_expr_t> _function_def_expr;
		std::shared_ptr<const struct_def_expr_t> _struct_def_expr;
		std::shared_ptr<value_t> _constant;
		std::shared_ptr<math_operation2_expr_t> _math_operation2_expr;
		std::shared_ptr<math_operation1_expr_t> _math_operation1_expr;
		std::shared_ptr<function_call_expr_t> _call_function_expr;
		std::shared_ptr<variable_read_expr_t> _variable_read_expr;
		bool _nop_expr = false;
	};


	//////////////////////////////////////////////////		inlines


	inline bool math_operation2_expr_t::operator==(const math_operation2_expr_t& other) const {
		return _operation == other._operation && *_left == *other._left && *_right == *other._right;
	}
	inline bool math_operation1_expr_t::operator==(const math_operation1_expr_t& other) const {
		return _operation == other._operation && *_input == *other._input;
	}


	//////////////////////////////////////////////////		make specific expressions


	inline expression_t make__function_def_expr(const function_def_expr_t& value){
		return expression_t(value);
	}

	inline expression_t make__struct_def_expr(const struct_def_expr_t& value){
		return expression_t(value);
	}

	inline expression_t make_constant(const value_t& value){
		return expression_t(value);
	}

	inline expression_t make_math_operation2_expr(const math_operation2_expr_t& value){
		return expression_t(value);
	}

	inline expression_t make_math_operation2_expr(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right){
		return expression_t(math_operation2_expr_t{op, std::make_shared<expression_t>(left), std::make_shared<expression_t>(right)});
	}

	inline expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input){
		return expression_t(math_operation1_expr_t{op, std::make_shared<expression_t>(input) });
	}

	std::shared_ptr<const function_def_expr_t> makie_function_def_expr_t(const function_def_t& f);
	std::shared_ptr<const struct_def_expr_t> makie_struct_def_expr_t(const struct_def_t& f);



	//////////////////////////////////////////////////		trace()


	void trace(const function_def_expr_t& e);
	void trace(const struct_def_expr_t& e);
	void trace(const math_operation2_expr_t& e);
	void trace(const math_operation1_expr_t& e);
	void trace(const function_call_expr_t& e);
	void trace(const function_call_expr_t& e);
	void trace(const variable_read_expr_t& e);
	void trace(const expression_t& e);


	/*
		Parses the expression string
		Checks syntax
		Validates that called functions exists and has correct type.
		Validates that accessed variables exists and has correct types.

		No optimization or evalution of any constant expressions etc.

		Example input:
			0
			3
			(3)
			(1 + 2) * 3
			"test"
			"test number: " +

			x
			x + y

			f()
			f(10, 122)

			(my_fun1("hello, 3) + 4) * my_fun2(10))
	*/
	expression_t parse_expression(const parser_i& parser, std::string expression);

	ast_t make_test_ast();

}


#endif /* parse_expression_hpp */
