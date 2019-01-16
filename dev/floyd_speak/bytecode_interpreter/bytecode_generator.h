//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_gen_h
#define bytecode_gen_h

#include "quark.h"

#include "bytecode_interpreter.h"


namespace floyd {
	struct semantic_ast_t;

	//////////////////////////		generate_bytecode()

	bc_program_t generate_bytecode(const semantic_ast_t& ast);


} //	floyd


#endif /* bytecode_gen_h */
