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
#include "types.h"
#include "compiler_basics.h"
#include "ast_value.h"

struct json_t;

namespace floyd {

struct value_t;


////////////////////////////////////////		assert_failed_exception


class assert_failed_exception : public std::runtime_error {
	public: assert_failed_exception() :
		std::runtime_error("Assertion failed.")
	{
	}
};


value_t unflatten_json_to_specific_type(types_t& types, const json_t& v, const type_t& target_type);



}	//	floyd

#endif /* floyd_runtime_hpp */
