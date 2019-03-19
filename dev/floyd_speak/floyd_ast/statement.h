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
//#include "../parts/xcode-libcxx-xcode9/variant"	//	https://github.com/youknowone/xcode-libcxx
#include <variant>

#include "expression.h"


namespace floyd {
	struct statement_t;
	struct expression_t;


	//////////////////////////////////////		symbol_t

	/*
		This is an entry in the symbol table, kept for each environment/stack frame.
		When you make a local variable it gets an entry in symbol table, with a type and name but no value. Like a reservered slot.
		You can also add constants directly to the symbol table.


		# Functions
		These are stored as local variable reservations of correct function-signature-type. They are inited
		during execution, not const-values in symbol table. Function calls needs to evaluate callee expression.
		??? TODO: make functions const-values when possible.


		# Structs
		These are const-values in symbol table -- the *type* of the struct that is.

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


		bool operator==(const symbol_t& other) const {
			return true
				&& _symbol_type == other._symbol_type
				&& _value_type == other._value_type
				&& _const_value == other._const_value
				;
		}

		public: bool check_invariant() const {
			QUARK_ASSERT(_const_value.is_undefined() || _const_value.get_type() == _value_type);
			return true;
		}

		private: symbol_t(type symbol_type, const floyd::typeid_t& value_type, const floyd::value_t& const_value) :
			_symbol_type(symbol_type),
			_value_type(value_type),
			_const_value(const_value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: floyd::typeid_t get_type() const {
			QUARK_ASSERT(check_invariant());

			return _value_type;
		}

		public: static symbol_t make_immutable_local(const floyd::typeid_t& value_type){
			return symbol_t{ type::immutable_local, value_type, {} };
		}

		public: static symbol_t make_mutable_local(const floyd::typeid_t& value_type){
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



	//////////////////////////////////////		symbol_table_t


	struct symbol_table_t {
		bool operator==(const symbol_table_t& other) const {
			return _symbols == other._symbols;
		}


		public: std::vector<std::pair<std::string, floyd::symbol_t>> _symbols;
	};

	int add_constant_literal(symbol_table_t& symbols, const std::string& name, const floyd::value_t& value);
	int add_temp(symbol_table_t& symbols, const std::string& name, const floyd::typeid_t& value_type);


	//////////////////////////////////////		body_t


	struct body_t {
		body_t(){
		}

//		body_t(const body_t& oth) = default;
//		body_t& operator=(const body_t& oth) = default;


		body_t(const std::vector<statement_t>& s) :
			_statements(s),
			_symbol_table{}
		{
		}

		body_t(const std::vector<statement_t>& statements, const symbol_table_t& symbols) :
			_statements(statements),
			_symbol_table(symbols)
		{
		}
		bool check_types_resolved() const;

		bool check_invariant() const;


		////////////////////		STATE
		std::vector<statement_t> _statements;
		symbol_table_t _symbol_table;
	};

	static inline bool operator==(const body_t& lhs, const body_t& rhs){
		return
			lhs._statements == rhs._statements
			&& lhs._symbol_table == rhs._symbol_table;
	}


	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
		Immutable
	*/
	struct statement_t {

		//////////////////////////////////////		return_statement_t



		struct return_statement_t {
			bool operator==(const return_statement_t& other) const {
				return _expression == other._expression;
			}

			expression_t _expression;
		};
		public: static statement_t make__return_statement(const location_t& location, const expression_t& expression){
			return statement_t{ .location = location, ._contents = {return_statement_t{expression}} };
		}


		//////////////////////////////////////		define_struct_statement_t


		struct define_struct_statement_t {
			bool operator==(const define_struct_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			std::string _name;
			std::shared_ptr<const struct_definition_t> _def;
		};
		public: static statement_t make__define_struct_statement(const location_t& location, const define_struct_statement_t& value){
			return statement_t{ .location = location, ._contents = {define_struct_statement_t{value}} };
		}


		//////////////////////////////////////		define_protocol_statement_t


		struct define_protocol_statement_t {
			bool operator==(const define_protocol_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			std::string _name;
			std::shared_ptr<const protocol_definition_t> _def;
		};
		public: static statement_t make__define_protocol_statement(const location_t& location, const define_protocol_statement_t& value){
			return statement_t{ .location = location, ._contents = {define_protocol_statement_t{value}} };
		}


		//////////////////////////////////////		define_function_statement_t


		struct define_function_statement_t {
			bool operator==(const define_function_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			std::string _name;
			std::shared_ptr<const function_definition_t> _def;
		};
		public: static statement_t make__define_function_statement(const location_t& location, const define_function_statement_t& value){
			return statement_t{ .location = location, ._contents = {define_function_statement_t{value}} };
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

			std::string _new_local_name;
			typeid_t _bindtype;
			expression_t _expression;
			mutable_mode _locals_mutable_mode;
		};
		public: static statement_t make__bind_local(const location_t& location, const std::string& new_local_name, const typeid_t& bindtype, const expression_t& expression, bind_local_t::mutable_mode locals_mutable_mode){
			return statement_t{ .location = location, ._contents = {bind_local_t{ new_local_name, bindtype, expression, locals_mutable_mode}} };
		}


		//////////////////////////////////////		store_t


		struct store_t {
			bool operator==(const store_t& other) const {
				return _local_name == other._local_name
					&& _expression == other._expression;
			}

			std::string _local_name;
			expression_t _expression;
		};
		public: static statement_t make__store(const location_t& location, const std::string& local_name, const expression_t& expression){
			return statement_t{ .location = location, ._contents = {store_t{ local_name, expression}} };
		}


		//////////////////////////////////////		store2_t

		//	Resolved store-operation.
		struct store2_t {
			bool operator==(const store2_t& other) const {
				return _dest_variable == other._dest_variable
					&& _expression == other._expression;
			}

			variable_address_t _dest_variable;
			expression_t _expression;
		};

		public: static statement_t make__store2(const location_t& location, const variable_address_t& dest_variable, const expression_t& expression){
			return statement_t{ .location = location, ._contents = {store2_t{ dest_variable, expression}} };
		}


		//////////////////////////////////////		block_statement_t


		struct block_statement_t {
			bool operator==(const block_statement_t& other) const {
				return _body == other._body;
			}

			body_t _body;
		};

		public: static statement_t make__block_statement(const location_t& location, const body_t& body){
			return statement_t{ .location = location, ._contents = {block_statement_t{ body}} };
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

			expression_t _condition;
			body_t _then_body;
			body_t _else_body;
		};
		public: static statement_t make__ifelse_statement(
			const location_t& location,
			const expression_t& condition,
			const body_t& then_body,
			const body_t& else_body
		){
			return statement_t{ .location = location, ._contents = {ifelse_statement_t{ condition, then_body, else_body}} };
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

			std::string _iterator_name;
			expression_t _start_expression;
			expression_t _end_expression;
			body_t _body;
			range_type _range_type;
		};
		public: static statement_t make__for_statement(
			const location_t& location,
			const std::string iterator_name,
			const expression_t& start_expression,
			const expression_t& end_expression,
			const body_t& body,
			for_statement_t::range_type range_type
		){
			return statement_t{ .location = location, ._contents = {for_statement_t{ iterator_name, start_expression, end_expression, body, range_type }} };
		}


		//////////////////////////////////////		software_system_statement_t


		struct software_system_statement_t {
			bool operator==(const software_system_statement_t& other) const {
				return _json_data == other._json_data;
			}

			json_t _json_data;
		};

		public: static statement_t make__software_system_statement(
			const location_t& location,
			json_t json_data
		){
			return statement_t{ .location = location, ._contents = {software_system_statement_t{ json_data }} };
		}


		//////////////////////////////////////		container_def_statement_t


		struct container_def_statement_t {
			bool operator==(const container_def_statement_t& other) const {
				return _json_data == other._json_data;
			}

			json_t _json_data;
		};

		public: static statement_t make__container_def_statement(
			const location_t& location,
			json_t json_data
		){
			return statement_t{ .location = location, ._contents = {container_def_statement_t{ json_data }} };
		}


		//////////////////////////////////////		while_statement_t


		struct while_statement_t {
			bool operator==(const while_statement_t& other) const {
				return
					_condition == other._condition
					&& _body == other._body;
			}

			expression_t _condition;
			body_t _body;
		};

		public: static statement_t make__while_statement(
			const location_t& location,
			const expression_t& condition,
			const body_t& body
		){
			return statement_t{ .location = location, ._contents = {while_statement_t{ condition, body }} };
		}


		//////////////////////////////////////		expression_statement_t


		struct expression_statement_t {
			bool operator==(const expression_statement_t& other) const {
				return _expression == other._expression;
			}

			expression_t _expression;
		};
		public: static statement_t make__expression_statement(const location_t& location, const expression_t& expression){
			return statement_t{ .location = location, ._contents = {expression_statement_t{ expression }} };
		}


		//////////////////////////////////////		statement_t


		bool check_invariant() const {
			return true;
		}

		//??? make into free function
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

			struct visitor_t {
				bool operator()(const return_statement_t& s) const{
					return s._expression.check_types_resolved();
				}
				bool operator()(const define_struct_statement_t& s) const{
					return s._def->check_types_resolved();
				}
				bool operator()(const define_protocol_statement_t& s) const{
					return s._def->check_types_resolved();
				}
				bool operator()(const define_function_statement_t& s) const{
					return s._def->check_types_resolved();
				}

				bool operator()(const bind_local_t& s) const{
					return true
						&& s._bindtype.check_types_resolved()
						&& s._expression.check_types_resolved()
						;
				}
				bool operator()(const store_t& s) const{
					return s._expression.check_types_resolved();
				}
				bool operator()(const store2_t& s) const{
					return s._expression.check_types_resolved();
				}
				bool operator()(const block_statement_t& s) const{
					return s._body.check_types_resolved();
				}

				bool operator()(const ifelse_statement_t& s) const{
					return true
						&& s._condition.check_types_resolved()
						&& s._then_body.check_types_resolved()
						&& s._else_body.check_types_resolved()
						;
				}
				bool operator()(const for_statement_t& s) const{
					return true
						&& s._start_expression.check_types_resolved()
						&& s._end_expression.check_types_resolved()
						&& s._body.check_types_resolved()
						;
				}
				bool operator()(const while_statement_t& s) const{
					return true
						&& s._condition.check_types_resolved()
						&& s._body.check_types_resolved()
						;
				}

				bool operator()(const expression_statement_t& s) const{
					return s._expression.check_types_resolved();
				}
				bool operator()(const software_system_statement_t& s) const{
					return true;
				}
				bool operator()(const container_def_statement_t& s) const{
					return true;
				}
			};

			return std::visit(visitor_t{}, _contents);
		}



		//////////////////////////////////////		STATE


		location_t location;
		std::variant<
			return_statement_t,
			define_struct_statement_t,
			define_protocol_statement_t,
			define_function_statement_t,
			bind_local_t,
			store_t,
			store2_t,
			block_statement_t,
			ifelse_statement_t,
			for_statement_t,
			while_statement_t,
			expression_statement_t,
			software_system_statement_t,
			container_def_statement_t
		> _contents;
	};

	static bool operator==(const statement_t& lhs, const statement_t& rhs){
		return lhs.location == rhs.location && lhs._contents == rhs._contents;
	}

	inline bool body_t::check_types_resolved() const{
		for(const auto& e: _statements){
			if(e.check_types_resolved() == false){
				return false;
			}
		}
		for(const auto& s: _symbol_table._symbols){
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
