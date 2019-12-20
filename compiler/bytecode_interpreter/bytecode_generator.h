//
//  parser_evaluator.h
//  Floyd
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
Compiles the ast to Floyd byte code, ready to interpret.

The result is a ready-to-run program that has a global scope (with symbols and instructions) a bunch of functions (with their own symbols and instructions), which types are use and some global config, like container settings.

The byte code scopes are flattened: they don't have sub-scopes. This is flattened out during code gen. All instructions and symbols of a function (or global scope) are in *one* long list.
*/
bc_program_t generate_bytecode(const semantic_ast_t& ast);


} //	floyd

#endif /* bytecode_gen_h */
