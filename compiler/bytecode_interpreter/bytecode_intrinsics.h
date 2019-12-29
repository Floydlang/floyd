//
//  bytecode_intrinsics_h
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_intrinsics_h
#define bytecode_intrinsics_h

/*
	Provides bytecode implementation of the language built-in intrinsics.
	Intrinsics are operators built into the language itself.
*/

#include "bytecode_interpreter.h"
#include "quark.h"


namespace floyd {

struct types_t;

std::vector<func_link_t> bc_get_intrinsics(types_t& types);


}	//	floyd

#endif /* bytecode_intrinsics_h */
