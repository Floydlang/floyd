//
//  floyd_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_runtime.h"


#include "quark.h"



namespace floyd {

typeid_t make_process_init_type(const typeid_t& t){
	return typeid_t::make_function(t, {}, epure::impure);
}

typeid_t make_process_message_handler_type(const typeid_t& t){
	return typeid_t::make_function(t, { t, typeid_t::make_json_value() }, epure::impure);
}


}	//	floyd
