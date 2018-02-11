//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

#include "ast.h"

namespace floyd {
	struct ast_json_t;

	/*
		Input is an array of statements from parser.
		A function has its own list of statements.
	*/
	const std::vector<std::shared_ptr<statement_t> > parser_statements_to_ast(const ast_json_t& p);

	floyd::ast_t run_pass2(const ast_json_t& parse_tree);
}
#endif /* pass2_hpp */
