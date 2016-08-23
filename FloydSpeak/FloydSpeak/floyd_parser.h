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

#include "parser_statement.h"
#include "parser_ast.h"

#include <string>

namespace floyd_parser {
	struct scope_def_t;


	std::pair<scope_ref_t, std::string> read_statements_into_scope_def(const ast_t& ast, const scope_ref_t scope_def, const std::string& s);

	ast_t program_to_ast(const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
