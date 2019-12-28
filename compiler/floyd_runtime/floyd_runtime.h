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
#include "value_backend.h"

struct json_t;

namespace floyd {

struct value_t;
struct floyd_runtime_t;
struct symbol_table_t;


////////////////////////////////////////		assert_failed_exception


class assert_failed_exception : public std::runtime_error {
	public: assert_failed_exception() :
		std::runtime_error("Assertion failed.")
	{
	}
};


value_t unflatten_json_to_specific_type(types_t& types, const json_t& v, const type_t& target_type);



//////////////////////////////////////		runtime_handler_i


struct runtime_handler_i {
	virtual ~runtime_handler_i(){};
	virtual void on_print(const std::string& s) = 0;
};



//////////////////////////////////////		runtime_process_i


struct runtime_process_i {
	virtual ~runtime_process_i(){};
	virtual void runtime_process__on_print(const std::string& s) = 0;
	virtual void runtime_process__on_send_message(const std::string& dest_process_id, const runtime_value_t& message, const type_t& message_type) = 0;
	virtual void runtime_process__on_exit_process() = 0;
};




//////////////////////////////////////		floyd_runtime_t



/*
	The Floyd runtime doesn't use global variables at all. Not even for memory heaps etc.
	Instead it passes around an invisible argumen to all functions, called Floyd Runtime Ptr (FRP).
*/

struct floyd_runtime_t {
	bool check_invariant() const {
		QUARK_ASSERT(backend != nullptr && backend->check_invariant());
		QUARK_ASSERT(global_symbols != nullptr);
//		QUARK_ASSERT(handler != nullptr);
		return true;
	}

	value_backend_t* backend;
	symbol_table_t* global_symbols;
	runtime_process_i* handler;
};

inline value_backend_t& get_backend(floyd_runtime_t& runtime){
	QUARK_ASSERT(runtime.check_invariant());

	return *runtime.backend;
}

inline symbol_table_t& get_global_symbols(floyd_runtime_t& runtime){
	QUARK_ASSERT(runtime.check_invariant());

	return *runtime.global_symbols;
}

inline void on_print(floyd_runtime_t& runtime, const std::string& s){
	QUARK_ASSERT(runtime.check_invariant());

	runtime.handler->runtime_process__on_print(s);
}

inline void send_message2(floyd_runtime_t& runtime, const std::string& dest_process_id, const runtime_value_t& message, const type_t& message_type){
	QUARK_ASSERT(runtime.check_invariant());
	QUARK_ASSERT(dest_process_id.empty() == false);
	QUARK_ASSERT(message_type.check_invariant());

	runtime.handler->runtime_process__on_send_message(dest_process_id, message, message_type);
}

inline void on_exit_process(floyd_runtime_t& runtime){
	QUARK_ASSERT(runtime.check_invariant());

	runtime.handler->runtime_process__on_exit_process();
}



}	//	floyd

#endif /* floyd_runtime_hpp */
