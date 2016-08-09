//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

#include "parser_primitives.h"


//??? Make this a separate parse pass, that resolves symbols and take decisions.
/*
	Pass2:Procsses parse-tree
	- Collect types and symbols-
	- Resolve types and symbols using compile-time scopes.
	- Verifies all expression and statement semantics
	- Inject automatic functions, like default constructors.
	- Perform basic evaluation simplifications / optimizations.
*/
floyd_parser::ast_t pass2(const floyd_parser::ast_t& ast1);


#endif /* pass2_hpp */
