//
//  host_functions.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef host_functions_hpp
#define host_functions_hpp

#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include "ast.h"
#include "ast_value.h"

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct interpreter_t;


	std::pair<interpreter_t, value_t> host__print(const interpreter_t& vm, const std::vector<value_t>& args);

typedef std::pair<interpreter_t, value_t> (*HOST_FUNCTION_PTR)(const interpreter_t& vm, const std::vector<value_t>& args);

struct host_function_t {
	std::string _name;
	HOST_FUNCTION_PTR _function_ptr;
	typeid_t _function_type;
};

	extern const std::vector<host_function_t> k_host_functions;

}


#endif /* host_functions_hpp */
