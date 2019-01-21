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
#include <map>
#include "ast_typeid.h"
#include "floyd_interpreter.h"

namespace floyd {

	extern const std::string k_builtin_types_and_constants;

	typedef typeid_t (*HOST_FUNCTION__CALC_RETURN_TYPE)(const std::vector<typeid_t>& args);


	struct host_function_record_t {
		std::string _name;
		HOST_FUNCTION_PTR _f;

		int _function_id;

		floyd::typeid_t _function_type;

		//	Set to non-nullptr to override _function_type.get_return_type() depending on caller's argument types.
		//	Use make_internal_dynamic() as return value and at least *one* input argument.
		HOST_FUNCTION__CALC_RETURN_TYPE _dynamic_return_type;
	};

	std::vector<host_function_record_t> get_host_function_records();



	struct host_function_signature_t {
		int _function_id;
		floyd::typeid_t _function_type;
	};

	std::map<std::string, host_function_signature_t> get_host_function_signatures();


	struct host_function_t {
		host_function_signature_t _signature;
		std::string _name;
		HOST_FUNCTION_PTR _f;
	};

	std::map<int, host_function_t> get_host_functions();


	typeid_t get_host_function_return_type(const std::string& function_name, const std::vector<typeid_t>& args);



	typeid_t make__fsentry_t__type();
	typeid_t make__fsentry_info_t__type();
	typeid_t make__fs_environment_t__type();

}

#endif /* host_functions_hpp */
