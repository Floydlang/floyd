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

#include "parser_ast.h"
#include "expressions.h"

struct json_t;

namespace floyd_parser {
	struct statement_t;
	struct expression_t;


	//////////////////////////////////////		return_statement_t


	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return _expression == other._expression;
		}

		expression_t _expression;
	};


	//////////////////////////////////////		bind_statement_t


	struct bind_statement_t {
		bool operator==(const bind_statement_t& other) const {
			return _new_variable_name == other._new_variable_name && _bindtype == other._bindtype && _expression == other._expression;
		}

		std::string _new_variable_name;
		typeid_t _bindtype;
		expression_t _expression;
	};



	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.

		- bind
		- return
	*/
	struct statement_t {
		public: statement_t(const statement_t& other) = default;
		public: statement_t& operator=(const statement_t& other) = default;
		public: bool check_invariant() const;

        public: statement_t(const return_statement_t& value) :
			_return(std::make_shared<return_statement_t>(value))
		{
		}
        public: statement_t(const bind_statement_t& value) :
			_bind(std::make_shared<bind_statement_t>(value))
		{
		}

		public: bool operator==(const statement_t& other) const {
			if(_return){
				return other._return && *_return == *other._return;
			}
			else if(_bind){
				return other._bind && *_bind == *other._bind;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		public: std::shared_ptr<return_statement_t> _return;
		public: std::shared_ptr<bind_statement_t> _bind;
	};


	//////////////////////////////////////		Makers



	statement_t make__return_statement(const return_statement_t& value);
	statement_t make__return_statement(const expression_t& expression);

	statement_t make__bind_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression);

	void trace(const statement_t& s);
	json_t statement_to_json(const statement_t& e);

}	//	floyd_parser


#endif /* parser_statement_hpp */
