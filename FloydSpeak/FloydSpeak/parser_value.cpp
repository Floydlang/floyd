//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.hpp"

#include "parser_statement.hpp"


namespace floyd_parser {

	//////////////////////////////////////////////////		function_body_t



	bool function_body_t::operator==(const function_body_t& other) const{
		return _statements == other._statements;
	}








void trace(const function_body_t& body){
	QUARK_SCOPED_TRACE("function_body_t");
//	trace_vec<statement_t>("Statements:", body._statements);
}


void trace(const value_t& e){
	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}





}	//	floyd_parser
