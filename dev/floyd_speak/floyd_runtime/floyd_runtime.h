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
#include "ast_typeid.h"
#include "compiler_basics.h"

struct json_t;

namespace floyd {

struct value_t;


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


inline typeid_t get_main_signature_arg_impure(){
	return typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_vector(typeid_t::make_string()) }, epure::impure);
}

inline typeid_t get_main_signature_no_arg_impure(){
 	return typeid_t::make_function(typeid_t::make_int(), { }, epure::impure);
}


inline typeid_t get_main_signature_arg_pure(){
	return typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_vector(typeid_t::make_string()) }, epure::pure);
}

inline typeid_t get_main_signature_no_arg_pure(){
	return typeid_t::make_function(typeid_t::make_int(), { }, epure::pure);
}

//	T x_init() impure
typeid_t make_process_init_type(const typeid_t& t);

//	T x(T state, json_value message) impure
typeid_t make_process_message_handler_type(const typeid_t& t);





value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type);





//////////////////////////////////////		HOST FUNCTIONS




struct host_function_signature_t {
	function_id_t _function_id;
	floyd::typeid_t _function_type;
};

std::map<std::string, host_function_signature_t> get_host_function_signatures();



}	//	floyd

#endif /* floyd_runtime_hpp */
