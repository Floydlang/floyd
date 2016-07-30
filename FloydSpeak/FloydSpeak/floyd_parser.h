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

#include "parser_primitives.h"

namespace floyd_parser {
	struct expression_t;
	struct statement_t;
	struct function_def_expr_t;
	struct value_t;



	ast_t program_to_ast(const ast_t& init, const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
