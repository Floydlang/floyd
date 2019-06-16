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
#include "ast_typeid.h"
#include "floyd_runtime.h"
#include "floyd_interpreter.h"


namespace floyd {



struct host_function_signature_t {
	function_id_t _function_id;
	floyd::typeid_t _function_type;
};

std::map<std::string, host_function_signature_t> get_host_function_signatures();


struct bc_host_function_t {
	host_function_signature_t _signature;
	std::string _name;
	BC_HOST_FUNCTION_PTR _f;
};

std::map<int, bc_host_function_t> get_host_functions();





}	//	floyd

#endif /* host_functions_hpp */
