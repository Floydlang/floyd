//
//  bytecode_corecalls_h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_corecalls_h
#define bytecode_corecalls_h

/*
	Provides built-in corecalls: these are operators built into the language itself.
*/

#include "quark.h"

#include <map>
#include "bytecode_interpreter.h"


namespace floyd {


//	Create lookup from function id -> C function pointer.
std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_corecalls();


}	//	floyd

#endif /* bytecode_corecalls_h */
