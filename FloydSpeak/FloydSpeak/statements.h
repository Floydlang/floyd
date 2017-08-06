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
			_return_statement(std::make_shared<return_statement_t>(value))
		{
		}

		public: bool operator==(const statement_t& other) const {
			if(_return_statement){
				return other._return_statement && *_return_statement == *other._return_statement;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

 //       public: std::shared_ptr<bind_statement_t> _bind_statement;
		public: std::shared_ptr<return_statement_t> _return_statement;
	};


	//////////////////////////////////////		Makers



	statement_t make__return_statement(const return_statement_t& value);
	statement_t make__return_statement(const expression_t& expression);

	void trace(const statement_t& s);
	json_t statement_to_json(const statement_t& e);

}	//	floyd_parser


#endif /* parser_statement_hpp */
