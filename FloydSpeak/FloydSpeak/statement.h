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
#include <map>

#include "ast.h"
#include "expression.h"
#include "utils.h"

struct json_t;

namespace floyd {
	struct statement_t;
	struct expression_t;


	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return _expression == other._expression;
		}

		const expression_t _expression;
	};

	struct define_struct_statement_t {
		bool operator==(const define_struct_statement_t& other) const {
			return _name == other._name && _def == other._def;
		}

		const std::string _name;
		const struct_definition_t _def;
	};

	//	FYI: We don't know until runtime if a = 10 is a deduced bind or a mutation.
	struct bind_or_assign_statement_t {
		bool operator==(const bind_or_assign_statement_t& other) const {
			return _new_variable_name == other._new_variable_name
				&& _bindtype == other._bindtype
				&& _expression == other._expression
				&& _bind_as_mutable_tag == other._bind_as_mutable_tag;
		}

		const std::string _new_variable_name;
		const typeid_t _bindtype;
		const expression_t _expression;
		const bool _bind_as_mutable_tag;
	};

	struct block_statement_t {
		bool operator==(const block_statement_t& other) const {
			return compare_shared_value_vectors(_statements, other._statements);
		}

		const std::vector<std::shared_ptr<statement_t>> _statements;
	};

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

	struct while_statement_t {
		bool operator==(const while_statement_t& other) const {
			return
				_condition == other._condition
				&& compare_shared_value_vectors(_body, other._body);
		}

		const expression_t _condition;
		const std::vector<std::shared_ptr<statement_t>> _body;
	};

	struct expression_statement_t {
		bool operator==(const expression_statement_t& other) const {
			return _expression == other._expression;
		}

		const expression_t _expression;
	};



	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
		Immutable
	*/
	struct statement_t {
		public: statement_t(const statement_t& other) = default;
		public: statement_t& operator=(const statement_t& other) = default;
		public: bool check_invariant() const;

		//??? Remove constructor, make static member makers.

        public: statement_t(const return_statement_t& value) :
			_return(std::make_shared<return_statement_t>(value))
		{
		}
        public: statement_t(const define_struct_statement_t& value) :
			_def_struct(std::make_shared<define_struct_statement_t>(value))
		{
		}
        public: statement_t(const bind_or_assign_statement_t& value) :
			_bind_or_assign(std::make_shared<bind_or_assign_statement_t>(value))
		{
		}
        public: statement_t(const block_statement_t& value) :
			_block(std::make_shared<block_statement_t>(value))
		{
		}
        public: statement_t(const ifelse_statement_t& value) :
			_if(std::make_shared<ifelse_statement_t>(value))
		{
		}
        public: statement_t(const for_statement_t& value) :
			_for(std::make_shared<for_statement_t>(value))
		{
		}
        public: statement_t(const while_statement_t& value) :
			_while(std::make_shared<while_statement_t>(value))
		{
		}
        public: statement_t(const expression_statement_t& value) :
			_expression(std::make_shared<expression_statement_t>(value))
		{
		}

		public: bool operator==(const statement_t& other) const {
			if(_return){
				return other._return && *_return == *other._return;
			}
			else if(_bind_or_assign){
				return other._bind_or_assign && *_bind_or_assign == *other._bind_or_assign;
			}
			else if(_block){
				return other._block && *_block == *other._block;
			}
			else if(_for){
				return other._for && *_for == *other._for;
			}
			else if(_while){
				return other._while && *_while == *other._while;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		//	Only *one* of these are used for each instance.
		public: const std::shared_ptr<return_statement_t> _return;
		public: const std::shared_ptr<define_struct_statement_t> _def_struct;
		public: const std::shared_ptr<bind_or_assign_statement_t> _bind_or_assign;
		public: const std::shared_ptr<block_statement_t> _block;
		public: const std::shared_ptr<ifelse_statement_t> _if;
		public: const std::shared_ptr<for_statement_t> _for;
		public: const std::shared_ptr<while_statement_t> _while;
		public: const std::shared_ptr<expression_statement_t> _expression;
	};


	//////////////////////////////////////		Makers



	statement_t make__return_statement(const return_statement_t& value);
	statement_t make__return_statement(const expression_t& expression);
	statement_t make__define_struct_statement(const define_struct_statement_t& value);
	statement_t make__bind_or_assign_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression, bool bind_as_mutable_tag);
	statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements);
	statement_t make__ifelse_statement(
		const expression_t& condition,
		std::vector<std::shared_ptr<statement_t>> then_statements,
		std::vector<std::shared_ptr<statement_t>> else_statements
	);
	statement_t make__for_statement(
		const std::string iterator_name,
		const expression_t& start_expression,
		const expression_t& end_expression,
		const std::vector<std::shared_ptr<statement_t>> body
	);
	statement_t make__while_statement(const expression_t& condition, const std::vector<std::shared_ptr<statement_t>> body);

	statement_t make__expression_statement(const expression_t& expression);

	ast_json_t statement_to_json(const statement_t& e);
	ast_json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e);



	inline statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}

	inline statement_t make__return_statement(const expression_t& expression){
		return statement_t(return_statement_t{ expression });
	}

	inline statement_t make__define_struct_statement(const define_struct_statement_t& value){
		return statement_t(value);
	}

	inline statement_t make__bind_or_assign_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression, bool bind_as_mutable_tag){
		return statement_t(bind_or_assign_statement_t{ new_variable_name, bindtype, expression, bind_as_mutable_tag });
	}
	inline statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements){
		return statement_t(block_statement_t{ statements });
	}

	inline statement_t make__ifelse_statement(
		const expression_t& condition,
		std::vector<std::shared_ptr<statement_t>> then_statements,
		std::vector<std::shared_ptr<statement_t>> else_statements
	){
		return statement_t(ifelse_statement_t{ condition, then_statements, else_statements });
	}


	inline statement_t make__for_statement(
		const std::string iterator_name,
		const expression_t& start_expression,
		const expression_t& end_expression,
		const std::vector<std::shared_ptr<statement_t>> body
	){
		return statement_t(for_statement_t{ iterator_name, start_expression, end_expression, body });
	}

	inline statement_t make__while_statement(
		const expression_t& condition,
		const std::vector<std::shared_ptr<statement_t>> body
	){
		return statement_t(while_statement_t{ condition, body });
	}

	inline statement_t make__expression_statement(const expression_t& expression){
		return statement_t(expression_statement_t{ expression });
	}

	inline bool statement_t::check_invariant() const {
		int count = 0;
		count = count + (_return != nullptr ? 1 : 0);
		count = count + (_def_struct != nullptr ? 1 : 0);
		count = count + (_bind_or_assign != nullptr ? 1 : 0);
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
		else if(_bind_or_assign){
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

	inline std::vector<json_t> statements_shared_to_json(const std::vector<std::shared_ptr<statement_t>>& a){
		std::vector<json_t> result;
		for(const auto& e: a){
			result.push_back(statement_to_json(*e)._value);
		}
		return result;
	}

	inline ast_json_t statement_to_json(const statement_t& e){
		QUARK_ASSERT(e.check_invariant());

		if(e._return){
			return ast_json_t{json_t::make_array({
				json_t(keyword_t::k_return),
				expression_to_json(e._return->_expression)._value
			})};
		}
		else if(e._def_struct){
			return ast_json_t{json_t::make_array({
				json_t("def-struct"),
				json_t(e._def_struct->_name),
				struct_definition_to_ast_json(e._def_struct->_def)._value
			})};
		}
		else if(e._bind_or_assign){
			const auto meta = (e._bind_or_assign->_bind_as_mutable_tag) ? (json_t::make_object({std::pair<std::string,json_t>{"mutable", true}})) : json_t();

			return ast_json_t{make_array_skip_nulls({
				json_t("bind"),
				e._bind_or_assign->_new_variable_name,
				typeid_to_ast_json(e._bind_or_assign->_bindtype)._value,
				expression_to_json(e._bind_or_assign->_expression)._value,
				meta
			})};
		}
		else if(e._block){
			return ast_json_t{json_t::make_array({
				json_t("block"),
				json_t::make_array(statements_shared_to_json(e._block->_statements))
			})};
		}
		else if(e._if){
			return ast_json_t{json_t::make_array({
				json_t(keyword_t::k_if),
				expression_to_json(e._if->_condition)._value,
				json_t::make_array(statements_shared_to_json(e._if->_then_statements)),
				json_t::make_array(statements_shared_to_json(e._if->_else_statements))
			})};
		}
		else if(e._for){
			return ast_json_t{json_t::make_array({
				json_t(keyword_t::k_for),
				json_t("open_range"),
				expression_to_json(e._for->_start_expression)._value,
				expression_to_json(e._for->_end_expression)._value,
				statements_to_json(e._for->_body)._value
			})};
		}
		else if(e._while){
			return ast_json_t{json_t::make_array({
				json_t(keyword_t::k_while),
				expression_to_json(e._while->_condition)._value,
				statements_to_json(e._while->_body)._value
			})};
		}
		else if(e._expression){
			return ast_json_t{json_t::make_array({
				json_t("expression-statement"),
				expression_to_json(e._expression->_expression)._value
			})};
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}

	inline ast_json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e){
		std::vector<json_t> statements;
		for(const auto& i: e){
			statements.push_back(statement_to_json(*i)._value);
		}
		return ast_json_t{json_t::make_array(statements)};
	}

}	//	floyd


#endif /* parser_statement_hpp */
