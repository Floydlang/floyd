//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass3_hpp
#define pass3_hpp

#include "ast.h"


struct json_t;

namespace floyd {

	/*
		Semantic Analysis -> SYMBOL TABLE + annotated AST
	*/
	floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2);
}
#endif /* pass3_hpp */


