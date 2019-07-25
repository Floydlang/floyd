//
//  host_functions.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef host_functions_hpp
#define host_functions_hpp

/*
	Provides built-in corecalls: these are operators built into the language itself.
*/

#include "quark.h"

#include <string>
#include <map>
#include "floyd_runtime.h"
#include "floyd_interpreter.h"


namespace floyd {


//	Create lookup from function id -> C function pointer.
std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_corecalls();


}	//	floyd

#endif /* host_functions_hpp */
