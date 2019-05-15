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
#include "floyd_interpreter.h"

class TDirEntry;

namespace floyd {

/*
enum class host_function_id {
	jsonvalue_to_value = 1020
};
*/

value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type);

extern const std::string k_builtin_types_and_constants;


struct host_function_record_t {
	std::string _name;
	HOST_FUNCTION_PTR _f;

	int _function_id;

	floyd::typeid_t _function_type;
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


typeid_t make__fsentry_t__type();
typeid_t make__fsentry_info_t__type();
typeid_t make__fs_environment_t__type();


/*
	struct sha1_t {
		string ascii40
	}
*/
typeid_t make__sha1_t__type();

/*
	struct binary_t {
		string bytes
	}
*/
typeid_t make__binary_t__type();



bool is_valid_absolute_dir_path(const std::string& s);
std::vector<value_t> directory_entries_to_values(const std::vector<TDirEntry>& v);

value_t impl__get_fsentry_info(const std::string& path);


}	//	floyd

#endif /* host_functions_hpp */
