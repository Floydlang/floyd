//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

#include "parser_ast.h"


/*
	Pass2:
	Non-lossy. Use to validate sematics, generate compiler errors.
	Inserts resolved links

	- Collect types and symbols-
	- Resolve types and symbols using compile-time scopes.
	- Verifies all expression and statement semantics
	- Generate compiler errors.
*/
floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1);


/*
	Optimizes expressions, lossy.
	- Inject automatic functions, like default constructors.
	- Perform basic evaluation simplifications / optimizations.
*/
floyd_parser::ast_t run_pass3(const floyd_parser::ast_t& ast1);


#endif /* pass2_hpp */
