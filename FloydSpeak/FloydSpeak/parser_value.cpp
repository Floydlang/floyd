//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "parser_statement.h"


namespace floyd_parser {



void trace(const value_t& e){
	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}





}	//	floyd_parser
