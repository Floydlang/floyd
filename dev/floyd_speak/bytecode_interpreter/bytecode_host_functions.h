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





struct bc_host_function_t {
	host_function_signature_t _signature;
	BC_HOST_FUNCTION_PTR _f;
};

std::map<int, bc_host_function_t> bc_get_host_functions();





}	//	floyd

#endif /* host_functions_hpp */
