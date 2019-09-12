//
//  floyd_runtime.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_runtime_hpp
#define floyd_runtime_hpp


#include <string>
#include "typeid.h"
#include "compiler_basics.h"
#include "ast_value.h"

struct json_t;

namespace floyd {

struct value_t;


//////////////////////////////////////		runtime_handler_i

/*
	This is a callback from the running Floyd process.
	that allows the interpreter to indirectly control the outside runtime that hosts the interpreter.
	FUTURE: Needs neater solution than this.
*/
struct runtime_handler_i {
	virtual ~runtime_handler_i(){};
	virtual void on_send(const std::string& process_id, const json_t& message) = 0;
};


value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type);



}	//	floyd

#endif /* floyd_runtime_hpp */
