//
//  floyd_runtime.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_runtime_hpp
#define floyd_runtime_hpp


#include <string>

struct json_t;

namespace floyd {


//////////////////////////////////////		runtime_handler_i

/*
	This is a callback from the interpreter (really the host functions run from the interpeter)
	that allows the interpreter to indirectly control the outside runtime that hosts the interpreter.
	FUTURE: Needs neater solution than this.
*/
struct runtime_handler_i {
	virtual ~runtime_handler_i(){};
	virtual void on_send(const std::string& process_id, const json_t& message) = 0;
};

}	//	floyd

#endif /* floyd_runtime_hpp */
