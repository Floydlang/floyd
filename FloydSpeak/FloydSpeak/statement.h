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


	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
		Immutable
	*/
	struct statement_t {
		public: statement_t(const statement_t& other) = default;
		public: statement_t& operator=(const statement_t& other) = default;



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



		struct define_struct_statement_t {
			bool operator==(const define_struct_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			const std::string _name;
			const struct_definition_t _def;
		};
        public: statement_t(const define_struct_statement_t& value) :
			_def_struct(std::make_shared<define_struct_statement_t>(value))
		{
		}
		public: static statement_t make__define_struct_statement(const define_struct_statement_t& value){
			return statement_t(value);
		}


/*
		struct bind_or_assign_statement_t {
			enum mutable_mode {
				k_has_mutable_tag = 2,
				k_no_mutable_tag
			};

			bool operator==(const bind_or_assign_statement_t& other) const {
				return _new_variable_name == other._new_variable_name
					&& _bindtype == other._bindtype
					&& _expression == other._expression
					&& _bind_as_mutable_tag == other._bind_as_mutable_tag;
			}

			const std::string _new_variable_name;
			const typeid_t _bindtype;
			const expression_t _expression;
			const mutable_mode _bind_as_mutable_tag;
		};
        public: statement_t(const bind_or_assign_statement_t& value) :
			_bind_or_assign(std::make_shared<bind_or_assign_statement_t>(value))
		{
		}
		public: static statement_t make__bind_or_assign_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression, bind_or_assign_statement_t::mutable_mode bind_as_mutable_tag){
			return statement_t(bind_or_assign_statement_t{ new_variable_name, bindtype, expression, bind_as_mutable_tag });
		}
*/


		struct bind_local_t {
			enum mutable_mode {
				k_mutable = 2,
				k_immutable
			};

			bool operator==(const bind_local_t& other) const {
				return _new_variable_name == other._new_variable_name
					&& _bindtype == other._bindtype
					&& _expression == other._expression
					&& _locals_mutable_mode == other._locals_mutable_mode;
			}

			const std::string _new_variable_name;
			const typeid_t _bindtype;
			const expression_t _expression;
			const mutable_mode _locals_mutable_mode;
		};
        public: statement_t(const bind_local_t& value) :
			_bind_local(std::make_shared<bind_local_t>(value))
		{
		}
		public: static statement_t make__bind_local(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression, bind_local_t::mutable_mode locals_mutable_mode){
			return statement_t(bind_local_t{ new_variable_name, bindtype, expression, locals_mutable_mode });
		}



		struct store_local_t {
			bool operator==(const store_local_t& other) const {
				return _local_name == other._local_name
					&& _expression == other._expression;
			}

			const std::string _local_name;
			const expression_t _expression;
		};
        public: statement_t(const store_local_t& value) :
			_store_local(std::make_shared<store_local_t>(value))
		{
		}
		public: static statement_t make__store_local(const std::string& local_name, const expression_t& expression){
			return statement_t(store_local_t{ local_name, expression });
		}





		struct block_statement_t {
			bool operator==(const block_statement_t& other) const {
				return compare_shared_value_vectors(_statements, other._statements);
			}

			const std::vector<std::shared_ptr<statement_t>> _statements;
		};
        public: statement_t(const block_statement_t& value) :
			_block(std::make_shared<block_statement_t>(value))
		{
		}

		public: static statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements){
			return statement_t(block_statement_t{ statements });
		}



		struct ifelse_statement_t {
			bool operator==(const ifelse_statement_t& other) const {
				return
					_condition == other._condition
					&& _condition == other._condition
					&& compare_shared_value_vectors(_then_statements, other._then_statements)
					&& compare_shared_value_vectors(_else_statements, other._else_statements)
					;
			}

			const expression_t _condition;
			const std::vector<std::shared_ptr<statement_t>> _then_statements;
			const std::vector<std::shared_ptr<statement_t>> _else_statements;
		};
        public: statement_t(const ifelse_statement_t& value) :
			_if(std::make_shared<ifelse_statement_t>(value))
		{
		}
		public: static statement_t make__ifelse_statement(
			const expression_t& condition,
			std::vector<std::shared_ptr<statement_t>> then_statements,
			std::vector<std::shared_ptr<statement_t>> else_statements
		){
			return statement_t(ifelse_statement_t{ condition, then_statements, else_statements });
		}



		struct for_statement_t {
			bool operator==(const for_statement_t& other) const {
				return
					_iterator_name == other._iterator_name
					&& _start_expression == other._start_expression
					&& _end_expression == other._end_expression
					&& compare_shared_value_vectors(_body, other._body);
			}

			const std::string _iterator_name;
			const expression_t _start_expression;
			const expression_t _end_expression;
			const std::vector<std::shared_ptr<statement_t>> _body;
		};
        public: statement_t(const for_statement_t& value) :
			_for(std::make_shared<for_statement_t>(value))
		{
		}
		public: static statement_t make__for_statement(
			const std::string iterator_name,
			const expression_t& start_expression,
			const expression_t& end_expression,
			const std::vector<std::shared_ptr<statement_t>> body
		){
			return statement_t(for_statement_t{ iterator_name, start_expression, end_expression, body });
		}


		struct while_statement_t {
			bool operator==(const while_statement_t& other) const {
				return
					_condition == other._condition
					&& compare_shared_value_vectors(_body, other._body);
			}

			const expression_t _condition;
			const std::vector<std::shared_ptr<statement_t>> _body;
		};

        public: statement_t(const while_statement_t& value) :
			_while(std::make_shared<while_statement_t>(value))
		{
		}
		public: static statement_t make__while_statement(
			const expression_t& condition,
			const std::vector<std::shared_ptr<statement_t>> body
		){
			return statement_t(while_statement_t{ condition, body });
		}




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


		public: bool operator==(const statement_t& other) const {
			if(_return){
				return compare_shared_values(_return, other._return);
			}
			else if(_def_struct){
				return compare_shared_values(_def_struct, other._def_struct);
			}
/*
			else if(_bind_or_assign){
				return compare_shared_values(_bind_or_assign, other._bind_or_assign);
			}
*/
			else if(_bind_local){
				return compare_shared_values(_bind_local, other._bind_local);
			}
			else if(_store_local){
				return compare_shared_values(_store_local, other._store_local);
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
//			count = count + (_bind_or_assign != nullptr ? 1 : 0);
			count = count + (_bind_local != nullptr ? 1 : 0);
			count = count + (_store_local != nullptr ? 1 : 0);
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
/*
			else if(_bind_or_assign){
			}
*/
			else if(_bind_local){
			}
			else if(_store_local){
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

 		public: static bool is_annotated_deep(const std::vector<std::shared_ptr<statement_t>>& s){
			for(const auto e: s){
				if(e->is_annotated_deep() == false){
					return false;
				}
			}
			return true;
		}

		//??? save code all over by introducing non-return-block and return-block and use for all nested blocks/scopes.
		public: bool is_annotated_deep() const{
			QUARK_ASSERT(check_invariant());

			if(_return != nullptr){
				return _return->_expression.is_annotated_deep();
			}
			else if(_def_struct != nullptr){
				return true;
			}
/*
			else if(_bind_or_assign){
				return _bind_or_assign->_expression.is_annotated_deep();
			}
*/
			else if(_bind_local){
				return _bind_local->_expression.is_annotated_deep();
			}
			else if(_store_local){
				return _store_local->_expression.is_annotated_deep();
			}
			else if(_block){
				return is_annotated_deep(_block->_statements);
			}
			else if(_if){
				if(_if->_condition.is_annotated_deep()){
					return is_annotated_deep(_if->_then_statements) && is_annotated_deep(_if->_else_statements);
				}
				else{
					return false;
				}
			}
			else if(_for){
				if(_for->_start_expression.is_annotated_deep() && _for->_end_expression.is_annotated_deep()){
					return is_annotated_deep(_for->_body);
				}
				else{
					return false;
				}
			}
			else if(_while){
				if(_while->_condition.is_annotated_deep()){
					return is_annotated_deep(_while->_body);
				}
				else{
					return false;
				}
			}
			else if(_expression){
				return _expression->_expression.is_annotated_deep();
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}


		//	Only *one* of these are used for each instance.
		public: const std::shared_ptr<return_statement_t> _return;
		public: const std::shared_ptr<define_struct_statement_t> _def_struct;
//		public: const std::shared_ptr<bind_or_assign_statement_t> _bind_or_assign;
		public: const std::shared_ptr<bind_local_t> _bind_local;
		public: const std::shared_ptr<store_local_t> _store_local;
		public: const std::shared_ptr<block_statement_t> _block;
		public: const std::shared_ptr<ifelse_statement_t> _if;
		public: const std::shared_ptr<for_statement_t> _for;
		public: const std::shared_ptr<while_statement_t> _while;
		public: const std::shared_ptr<expression_statement_t> _expression;
	};




}	//	floyd


#endif /* parser_statement_hpp */
