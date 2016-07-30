//
//  parser_statement.hpp
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

#include "parser_types.h"
#include "parser_primitives.h"
#include "parser_expression.hpp"


namespace floyd_parser {
	struct statement_t;
	struct expression_t;


	//////////////////////////////////////		bind_statement_t


	struct bind_statement_t {
		bool operator==(const bind_statement_t& other) const {
			return _identifier == other._identifier && _expression == other._expression;
		}

		std::string _identifier;
		std::shared_ptr<expression_t> _expression;
	};


	//////////////////////////////////////		return_statement_t


	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return *_expression == *other._expression;
		}

		std::shared_ptr<expression_t> _expression;
	};


	statement_t makie_return_statement(const expression_t& expression);


	//////////////////////////////////////		statement_t


	struct statement_t {
		bool check_invariant() const;

		statement_t(const bind_statement_t& value) :
			_bind_statement(std::make_shared<bind_statement_t>(value))
		{
		}

		statement_t(const return_statement_t& value) :
			_return_statement(std::make_shared<return_statement_t>(value))
		{
		}

		bool operator==(const statement_t& other) const {
			if(_bind_statement){
				return other._bind_statement && *_bind_statement == *other._bind_statement;
			}
			else if(_return_statement){
				return other._return_statement && *_return_statement == *other._return_statement;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		const std::shared_ptr<bind_statement_t> _bind_statement;
		const std::shared_ptr<return_statement_t> _return_statement;
	};


	//////////////////////////////////////		Makers


	statement_t make__bind_statement(const bind_statement_t& value);
	statement_t make__bind_statement(const std::string& identifier, const expression_t& e);
	statement_t make__return_statement(const return_statement_t& value);

	void trace(const statement_t& s);


	/*
		"int a = 10;"
		"float b = 0.3;"
		"int c = a + b;"
		"int b = f(a);"
		"string hello = f(a) + \"_suffix\";";

		...can contain trailing whitespace.
	*/
	std::pair<statement_t, std::string> parse_assignment_statement(const ast_t& ast, const std::string& s);

	/*
		Global function definitions:

			#1
			int test_func1(){ return 100; };

			#2
			string test_func2(int a, float b){ return "sdf" };

		Global struct definitions:

			struct my_image {
				int width;
				int height;
			};

			struct my_sprite {
				string name;
				my_image image;
			};

		FUTURE
		- Define data structures (also in local scopes).
		- Add support for global constants.
		- Assign global constants


		Global constants:

			float my_global1 = 3.1415f + ;
			my_sprite my_test_sprite =
	*/
	std::pair<statement_t, std::string> read_statement(const ast_t& ast, const std::string& pos);



}	//	floyd_parser


#endif /* parser_statement_hpp */
