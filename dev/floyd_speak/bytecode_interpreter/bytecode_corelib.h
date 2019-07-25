//
//  bytecode_corelib_hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef bytecode_corelib_hpp
#define bytecode_corelib_hpp

/*
	Corelib provides functions like get_time_of_day(), read_text_file() etc.
*/

#include "quark.h"

#include <string>
#include <map>
#include "floyd_runtime.h"
#include "floyd_interpreter.h"


namespace floyd {


//	Create lookup from function id -> C function pointer.

std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_filelib_calls();



}	//	floyd

#endif /* bytecode_corelib_hpp */
