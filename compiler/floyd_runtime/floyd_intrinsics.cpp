//
//  floyd_intrinsics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_intrinsics.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_codegen_basics.h"
#include "value_features.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "utils.h"


namespace floyd {


/////////////////////////////////////////		print()

void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	on_print(frp, s);
}





void unified_intrinsic__send(floyd_runtime_t* frp, runtime_value_t dest_process_id0, runtime_value_t message, runtime_type_t message_type){
	auto& backend = get_backend(frp);

	const auto& dest_process_id = from_runtime_string2(backend, dest_process_id0);
	const auto& message_type2 = lookup_type_ref(backend, message_type);

/*
	if(k_trace_process_messaging){
		const auto& message2 = from_runtime_value2(backend, message, message_type2);
		const auto j = value_and_type_to_json(backend.types, message2);
		QUARK_TRACE_SS("send(\"" << dest_process_id << "\"," << json_to_pretty_string(j) <<")");
	}
*/

//	r._handler->on_send(process_id, message_json);
	send_message2(frp, dest_process_id, message, message_type2);
}

void unified_intrinsic__exit(floyd_runtime_t* frp){
	auto& backend = get_backend(frp);
	(void)backend;

//???	const auto& process_id = from_runtime_string2(backend, process_id0);

	on_exit_process(frp);
}



} // floyd
