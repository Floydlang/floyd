	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass3.h"

#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "parser_primitives.h"

namespace floyd {
using namespace std;


floyd::ast_t run_pass3(const quark::trace_context_t& tracer, const floyd::ast_t& ast_pass2){
	QUARK_ASSERT(ast_pass2.check_invariant());
	return ast_pass2;
}



}	//	floyd

