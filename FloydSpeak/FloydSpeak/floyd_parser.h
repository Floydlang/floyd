//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"
#include <string>

struct json_value_t;

namespace floyd_parser {
	std::pair<json_value_t, std::string> read_statements_into_scope_def(const json_value_t& scope_def2, const std::string& s);

	json_value_t program_to_ast(const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
