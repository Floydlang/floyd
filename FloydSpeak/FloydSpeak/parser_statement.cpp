//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_statement.hpp"



#include "parser_expression.hpp"


namespace floyd_parser {


	statement_t make__bind_statement(const bind_statement_t& value){
		return statement_t(value);
	}

	statement_t make__bind_statement(const std::string& identifier, const expression_t& e){
		return statement_t(bind_statement_t{identifier, std::make_shared<expression_t>(e)});
	}

	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}

}	//	floyd_parser
