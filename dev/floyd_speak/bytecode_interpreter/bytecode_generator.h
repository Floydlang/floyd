//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_gen_h
#define bytecode_gen_h

/*
	Converts an AST into Floyd-specific byte code, ready to execute with the Floyd byte code interpreter.
*/

#include "quark.h"

namespace floyd {
struct semantic_ast_t;
struct bc_program_t;


//////////////////////////		generate_bytecode()

/*
	Compiles the ast to Floyd byte code.
*/
bc_program_t generate_bytecode(const semantic_ast_t& ast);


} //	floyd

#endif /* bytecode_gen_h */
