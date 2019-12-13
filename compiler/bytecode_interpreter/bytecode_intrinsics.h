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
	Provides built-in intrinsics: these are operators built into the language itself.
*/

#include "quark.h"

#include <map>
#include "bytecode_interpreter.h"


namespace floyd {

struct types_t;


std::vector<std::pair<intrinsic_signature_t, BC_NATIVE_FUNCTION_PTR>> bc_get_intrinsics_internal(types_t& types);

//	Create lookup from function id -> C function pointer.
std::map<module_symbol_t, BC_NATIVE_FUNCTION_PTR> bc_get_intrinsics(types_t& types);


}	//	floyd

#endif /* bytecode_intrinsics_h */
