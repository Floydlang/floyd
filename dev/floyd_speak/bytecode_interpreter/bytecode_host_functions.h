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
	Host functions = built-in functions callable from the Floyd program. print(), assert(), size() etc.
*/

#include "quark.h"

#include <string>
#include <map>
#include "floyd_runtime.h"
#include "floyd_interpreter.h"


namespace floyd {





//	Create lookup from function id -> C function pointer.
std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_corecalls();

std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_filelib_calls();





}	//	floyd

#endif /* host_functions_hpp */
