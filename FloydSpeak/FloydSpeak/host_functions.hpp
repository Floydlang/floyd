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



	//////////////////////////////////////		host_function_signatures_t


	struct host_function_signatures_t {
		std::string _name;
		int _function_id;
		floyd::typeid_t _function_type;
	};

	std::vector<host_function_signatures_t> get_host_function_signatures();



}


#endif /* host_functions_hpp */
