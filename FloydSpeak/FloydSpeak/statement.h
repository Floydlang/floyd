//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_statement_hpp
#define parser_statement_hpp


#include "quark.h"
#include <vector>
#include <string>

#include "expression.h"


namespace floyd {
	struct statement_t;
	struct expression_t;




	//////////////////////////////////////		symbol_t

	/*
		This is an entry in the symbol table, kept for each environment/stack frame.
		When you make a local variable it gets an entry in symbol table, with a type and name but no value. Like a reservered slot.
		You can also add constants directly to the symbol table.


		# Host functions
		Host functions are added as constant values, with the propery _value_type (function signature) and _const_value (function value to that host function ID).


		# Functions
		These are stored as local variable reservations of correct function-signature-type. They are inited during execution, not const-values in symbol table. Function calls needs to evaluate callee expression.
		??? TODO: make functions const-values when possible.


		# Structs
		These are const-values in symbol table.

		struct pixel_t { int red; int green; int blue; }

		- needs to become a const variable "pixel_t" so print(pixel_t) etc works.
		- pixel_t variable =
			type: typeid_t = struct{ int red; int green; int blue; }
			const: value_t::typeid_value =
	*/

	struct symbol_t {
		enum type {
			immutable_local = 10,
			mutable_local
		};

		type _symbol_type;
		floyd::typeid_t _value_type;
		floyd::value_t _const_value;



		private: symbol_t(type symbol_type, const floyd::typeid_t& value_type, const floyd::value_t& const_value) :
			_symbol_type(symbol_type),
			_value_type(value_type),
			_const_value(const_value)
		{
		}

		public: floyd::typeid_t get_type() const {
			return _value_type;
		}

		public: static symbol_t make_immutable_local(const floyd::typeid_t value_type){
			return symbol_t{ type::immutable_local, value_type, {} };
		}

		public: static symbol_t make_mutable_local(const floyd::typeid_t value_type){
			return symbol_t{ type::mutable_local, value_type, {} };
		}

		public: static symbol_t make_constant(const floyd::value_t& value){
			return symbol_t{ type::immutable_local, value.get_type(), value };
		}

		public: static symbol_t make_type(const floyd::typeid_t& t){
			return symbol_t{
				type::immutable_local,
				floyd::typeid_t::make_typeid(),
				floyd::value_t::make_typeid_value(t)
			};
		}
	};



	//////////////////////////////////////		body_t



	struct body_t {
		body_t(){
		}

//		body_t(const body_t& oth) = default;
//		body_t& operator=(const body_t& oth) = default;


		body_t(const std::vector<std::shared_ptr<statement_t>>& s) :
			_statements(s),
			_symbols{}
		{
		}

		body_t(const std::vector<std::shared_ptr<statement_t>>& statements, const std::vector<std::pair<std::string, symbol_t>>& symbols) :
			_statements(statements),
			_symbols(symbols)
		{
		}
		bool check_types_resolved() const;

		bool check_invariant() const;

		std::vector<std::shared_ptr<statement_t>> _statements;
		std::vector<std::pair<std::string, symbol_t>> _symbols;
	};

	inline bool operator==(const body_t& lhs, const body_t& rhs){
		return compare_shared_value_vectors(lhs._statements, rhs._statements);
	}




	
	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
		Immutable
	*/
	struct statement_t {
		public: statement_t(const statement_t& other) = default;
		public: statement_t& operator=(const statement_t& other) = default;


		//////////////////////////////////////		return_statement_t


		struct return_statement_t {
			bool operator==(const return_statement_t& other) const {
				return _expression == other._expression;
			}

			const expression_t _expression;
		};
        public: statement_t(const return_statement_t& value) :
			_return(std::make_shared<return_statement_t>(value))
		{
		}
		public: static statement_t make__return_statement(const expression_t& expression){
			return statement_t(return_statement_t{ expression });
		}


		//////////////////////////////////////		define_struct_statement_t


		struct define_struct_statement_t {
			bool operator==(const define_struct_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			const std::string _name;
			const std::shared_ptr<const struct_definition_t> _def;
		};
        public: statement_t(const define_struct_statement_t& value) :
			_def_struct(std::make_shared<define_struct_statement_t>(value))
		{
		}
		public: static statement_t make__define_struct_statement(const define_struct_statement_t& value){
			return statement_t(value);
		}


		//////////////////////////////////////		define_function_statement_t


		struct define_function_statement_t {
			bool operator==(const define_function_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			const std::string _name;
			const std::shared_ptr<const function_definition_t> _def;
		};
        public: statement_t(const define_function_statement_t& value) :
			_def_function(std::make_shared<define_function_statement_t>(value))
		{
		}
		public: static statement_t make__define_function_statement(const define_function_statement_t& value){
			return statement_t(value);
		}


		//////////////////////////////////////		bind_local_t


		struct bind_local_t {
			enum mutable_mode {
				k_mutable = 2,
				k_immutable
			};

			bool operator==(const bind_local_t& other) const {
				return _new_local_name == other._new_local_name
					&& _bindtype == other._bindtype
					&& _expression == other._expression
					&& _locals_mutable_mode == other._locals_mutable_mode;
			}

			const std::string _new_local_name;
			const typeid_t _bindtype;
			const expression_t _expression;
			const mutable_mode _locals_mutable_mode;
		};
        public: statement_t(const bind_local_t& value) :
			_bind_local(std::make_shared<bind_local_t>(value))
		{
		}
		public: static statement_t make__bind_local(const std::string& new_local_name, const typeid_t& bindtype, const expression_t& expression, bind_local_t::mutable_mode locals_mutable_mode){
			return statement_t(bind_local_t{ new_local_name, bindtype, expression, locals_mutable_mode });
		}


		//////////////////////////////////////		store_t


		struct store_t {
			bool operator==(const store_t& other) const {
				return _local_name == other._local_name
					&& _expression == other._expression;
			}

			const std::string _local_name;
			const expression_t _expression;
		};
        public: statement_t(const store_t& value) :
			_store(std::make_shared<store_t>(value))
		{
		}
		public: static statement_t make__store(const std::string& local_name, const expression_t& expression){
			return statement_t(store_t{ local_name, expression });
		}


		//////////////////////////////////////		store2_t


		struct store2_t {
			bool operator==(const store2_t& other) const {
				return _dest_variable == other._dest_variable
					&& _expression == other._expression;
			}

			const variable_address_t _dest_variable;
			const expression_t _expression;
		};

        public: statement_t(const store2_t& value) :
			_store2(std::make_shared<store2_t>(value))
		{
		}
		public: static statement_t make__store2(const variable_address_t& dest_variable, const expression_t& expression){
			return statement_t(store2_t{ dest_variable, expression });
		}


		//////////////////////////////////////		block_statement_t


		struct block_statement_t {
			bool operator==(const block_statement_t& other) const {
				return _body == other._body;
			}

			const body_t _body;
		};
        public: statement_t(const block_statement_t& value) :
			_block(std::make_shared<block_statement_t>(value))
		{
		}

		public: static statement_t make__block_statement(const body_t& body){
			return statement_t(block_statement_t{ body });
		}


		//////////////////////////////////////		ifelse_statement_t


		struct ifelse_statement_t {
			bool operator==(const ifelse_statement_t& other) const {
				return
					_condition == other._condition
					&& _condition == other._condition
					&& _then_body == other._then_body
					&& _else_body == other._else_body
					;
			}

			const expression_t _condition;
			const body_t _then_body;
			const body_t _else_body;
		};
        public: statement_t(const ifelse_statement_t& value) :
			_if(std::make_shared<ifelse_statement_t>(value))
		{
		}
		public: static statement_t make__ifelse_statement(
			const expression_t& condition,
			const body_t& then_body,
			const body_t& else_body
		){
			return statement_t(ifelse_statement_t{ condition, then_body, else_body });
		}


		//////////////////////////////////////		for_statement_t


		struct for_statement_t {
			enum range_type {
				k_open_range,	//	..<
				k_closed_range	//	...
			};

			bool operator==(const for_statement_t& other) const {
				return
					_iterator_name == other._iterator_name
					&& _start_expression == other._start_expression
					&& _end_expression == other._end_expression
					&& _body == other._body
					&& _range_type == other._range_type
					;
			}

			const std::string _iterator_name;
			const expression_t _start_expression;
			const expression_t _end_expression;
			const body_t _body;
			const range_type _range_type;
		};
        public: statement_t(const for_statement_t& value) :
			_for(std::make_shared<for_statement_t>(value))
		{
		}
		public: static statement_t make__for_statement(
			const std::string iterator_name,
			const expression_t& start_expression,
			const expression_t& end_expression,
			const body_t& body,
			for_statement_t::range_type range_type
		){
			return statement_t(for_statement_t{ iterator_name, start_expression, end_expression, body, range_type });
		}


		//////////////////////////////////////		while_statement_t


		struct while_statement_t {
			bool operator==(const while_statement_t& other) const {
				return
					_condition == other._condition
					&& _body == other._body;
			}

			const expression_t _condition;
			const body_t _body;
		};

        public: statement_t(const while_statement_t& value) :
			_while(std::make_shared<while_statement_t>(value))
		{
		}
		public: static statement_t make__while_statement(
			const expression_t& condition,
			const body_t& body
		){
			return statement_t(while_statement_t{ condition, body });
		}


		//////////////////////////////////////		expression_statement_t


		struct expression_statement_t {
			bool operator==(const expression_statement_t& other) const {
				return _expression == other._expression;
			}

			const expression_t _expression;
		};
        public: statement_t(const expression_statement_t& value) :
			_expression(std::make_shared<expression_statement_t>(value))
		{
		}
		public: static statement_t make__expression_statement(const expression_t& expression){
			return statement_t(expression_statement_t{ expression });
		}


		//////////////////////////////////////		statement_t


		public: bool operator==(const statement_t& other) const {
			if(_return){
				return compare_shared_values(_return, other._return);
			}
			else if(_def_struct){
				return compare_shared_values(_def_struct, other._def_struct);
			}
			else if(_def_function){
				return compare_shared_values(_def_function, other._def_function);
			}
			else if(_bind_local){
				return compare_shared_values(_bind_local, other._bind_local);
			}
			else if(_store){
				return compare_shared_values(_store, other._store);
			}
			else if(_store2){
				return compare_shared_values(_store2, other._store2);
			}
			else if(_block){
				return compare_shared_values(_block, other._block);
			}
			else if(_if){
				return compare_shared_values(_if, other._if);
			}
			else if(_for){
				return compare_shared_values(_for, other._for);
			}
			else if(_while){
				return compare_shared_values(_while, other._while);
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		bool check_invariant() const {
			int count = 0;
			count = count + (_return != nullptr ? 1 : 0);
			count = count + (_def_struct != nullptr ? 1 : 0);
			count = count + (_def_function != nullptr ? 1 : 0);
			count = count + (_bind_local != nullptr ? 1 : 0);
			count = count + (_store != nullptr ? 1 : 0);
			count = count + (_store2 != nullptr ? 1 : 0);
			count = count + (_block != nullptr ? 1 : 0);
			count = count + (_if != nullptr ? 1 : 0);
			count = count + (_for != nullptr ? 1 : 0);
			count = count + (_while != nullptr ? 1 : 0);
			count = count + (_expression != nullptr ? 1 : 0);
			QUARK_ASSERT(count == 1);

			if(_return != nullptr){
			}
			else if(_def_struct != nullptr){
			}
			else if(_def_function != nullptr){
			}
			else if(_bind_local){
			}
			else if(_store){
			}
			else if(_store2){
			}
			else if(_block){
			}
			else if(_if){
			}
			else if(_for){
			}
			else if(_while){
			}
			else if(_expression){
			}
			else{
				QUARK_ASSERT(false);
			}
			return true;
		}

 		public: static bool check_types_resolved(const std::vector<std::shared_ptr<statement_t>>& s){
			for(const auto& e: s){
				if(e->check_types_resolved() == false){
					return false;
				}
			}
			return true;
		}

		public: bool check_types_resolved() const{
			QUARK_ASSERT(check_invariant());

			if(_return != nullptr){
				return _return->_expression.check_types_resolved();
			}
			else if(_def_struct != nullptr){
				return _def_struct->_def->check_types_resolved();
			}
			else if(_def_function != nullptr){
				return _def_function->_def->check_types_resolved();
			}
			else if(_bind_local){
				return true
					&& _bind_local->_bindtype.check_types_resolved()
					&& _bind_local->_expression.check_types_resolved()
					;
			}
			else if(_store){
				return _store->_expression.check_types_resolved();
			}
			else if(_store2){
				return _store2->_expression.check_types_resolved();
			}
			else if(_block){
				return _block->_body.check_types_resolved();
			}
			else if(_if){
				return true
					&& _if->_condition.check_types_resolved()
					&& _if->_then_body.check_types_resolved()
					&& _if->_else_body.check_types_resolved()
					;
			}
			else if(_for){
				return true
					&& _for->_start_expression.check_types_resolved()
					&& _for->_end_expression.check_types_resolved()
					&& _for->_body.check_types_resolved()
					;
			}
			else if(_while){
				return true
					&& _while->_condition.check_types_resolved()
					&& _while->_body.check_types_resolved()
					;
			}
			else if(_expression){
				return _expression->_expression.check_types_resolved();
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}


		//////////////////////////////////////		STATE


		//	Only *one* of these are used for each instance.
		public: const std::shared_ptr<return_statement_t> _return;
		public: const std::shared_ptr<define_struct_statement_t> _def_struct;
		public: const std::shared_ptr<define_function_statement_t> _def_function;
		public: const std::shared_ptr<bind_local_t> _bind_local;
		public: const std::shared_ptr<store_t> _store;
		public: const std::shared_ptr<store2_t> _store2;
		public: const std::shared_ptr<block_statement_t> _block;
		public: const std::shared_ptr<ifelse_statement_t> _if;
		public: const std::shared_ptr<for_statement_t> _for;
		public: const std::shared_ptr<while_statement_t> _while;
		public: const std::shared_ptr<expression_statement_t> _expression;
	};

	inline bool body_t::check_types_resolved() const{
		for(const auto& e: _statements){
			if(e->check_types_resolved() == false){
				return false;
			}
		}
		for(const auto& s: _symbols){
			if(s.first != "**undef**" && s.second._value_type.check_types_resolved() == false){
				return false;
			}
			if(s.first != "**undef**" && s.second._const_value.is_undefined() == false && s.second._const_value.get_type().check_types_resolved() == false){
				return false;
			}
		}
		return true;
	}



}	//	floyd


#endif /* parser_statement_hpp */
