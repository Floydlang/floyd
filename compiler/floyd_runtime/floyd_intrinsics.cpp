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

void unified_intrinsic__assert(floyd_runtime_t* frp, runtime_value_t arg){
	auto& backend = get_backend(frp);
	(void)backend;

	bool ok = (arg.bool_value & 0x01) == 0 ? false : true;
	if(!ok){
		on_print(frp, "Assertion failed.");
		throw assert_failed_exception();
//		quark::throw_runtime_error("Assertion failed.");
	}
}




runtime_value_t unified_intrinsic__to_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	return to_runtime_string2(backend, s);
}


runtime_value_t unified_intrinsic__to_pretty_string(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto& value2 = from_runtime_value2(backend, value, type0);
	const auto json = value_to_json(backend.types, value2);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string2(backend, s);
}


runtime_type_t unified_intrinsic__typeof(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return value_type;
}



static rt_value_t update_element(value_backend_t& backend, const rt_value_t& obj1, const rt_value_t& lookup_key, const rt_value_t& new_value){
	QUARK_ASSERT(backend.check_invariant());

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	const auto& types = backend.types;

	const auto& obj1_peek = peek2(types, obj1._type);
	const auto& key_peek = peek2(types, lookup_key._type);
	const auto& new_value_peek = peek2(types, new_value._type);

	if(obj1_peek.is_string()){
		if(key_peek.is_int() == false){
			quark::throw_runtime_error("String lookup using integer index only.");
		}
		else{
			if(new_value_peek.is_int() == false){
				quark::throw_runtime_error("Update element must be a character in an int.");
			}
			else{
				return rt_value_t(backend, obj1._type, update__string(backend, obj1._pod, lookup_key._pod, new_value._pod), rt_value_t::rc_mode::adopt);
			}
		}
	}
	else if(obj1_peek.is_json()){
		const auto json0 = obj1.get_json();
		if(json0.is_array()){
			QUARK_ASSERT(false);
			throw std::exception();
		}
		else if(json0.is_object()){
			QUARK_ASSERT(false);
			throw std::exception();
		}
		else{
			quark::throw_runtime_error("Can only update string, vector, dict or struct.");
		}
	}
	else if(obj1_peek.is_vector()){
		const auto element_type = obj1_peek.get_vector_element_type(types);
		if(key_peek.is_int() == false){
			quark::throw_runtime_error("Vector lookup using integer index only.");
		}
		else if(element_type != new_value._type){
			quark::throw_runtime_error("Update element must match vector type.");
		}
		else{
			return rt_value_t(backend, obj1._type, update_element__vector(backend, obj1._pod, obj1_peek.get_data(), lookup_key._pod, new_value._pod), rt_value_t::rc_mode::adopt);
		}
	}
	else if(obj1_peek.is_dict()){
		if(key_peek.is_string() == false){
			quark::throw_runtime_error("Dict lookup using string key only.");
		}
		else{
			const auto value_type = obj1_peek.get_dict_value_type(types);
			if(value_type != new_value._type){
				quark::throw_runtime_error("Update element must match dict value type.");
			}
			else{
				return rt_value_t(backend, obj1._type, update_dict(backend, obj1._pod, obj1_peek.get_data(), lookup_key._pod, new_value._pod), rt_value_t::rc_mode::adopt);
			}
		}
	}
	else if(obj1_peek.is_struct()){
		if(key_peek.is_string() == false){
			quark::throw_runtime_error("You must specify structure member using string.");
		}
		else{
			//??? Instruction should keep the member index, not the name of the member!
			const auto& struct_def = obj1_peek.get_struct(types);
			const auto key = from_runtime_string2(backend, lookup_key._pod);
			int member_index = find_struct_member_index(struct_def, key);
			if(member_index == -1){
				quark::throw_runtime_error("Unknown member.");
			}

//			trace_heap(backend.heap);

			const auto result = rt_value_t(backend, obj1._type, update_struct_member(backend, obj1._pod, obj1._type, member_index, new_value._pod, new_value_peek), rt_value_t::rc_mode::adopt);

//			trace_heap(backend.heap);

			return result;
		}
	}
	else {
		UNSUPPORTED();
		throw std::exception();
	}
}

// let b = update(a, member, value)
runtime_value_t unified_intrinsic__update(
	floyd_runtime_t* frp,

	runtime_value_t collection_value,
	runtime_type_t collection_type,

	runtime_value_t key_value,
	runtime_type_t key_type,

	runtime_value_t newvalue_value,
	runtime_type_t newvalue_type
){
	auto& backend = get_backend(frp);

	const auto collection = rt_value_t(backend, type_t(collection_type), collection_value, rt_value_t::rc_mode::bump);
	const auto key = rt_value_t(backend, type_t(key_type), key_value, rt_value_t::rc_mode::bump);
	const auto newvalue = rt_value_t(backend, type_t(newvalue_type), newvalue_value, rt_value_t::rc_mode::bump);

	const auto result = update_element(backend, collection, key, newvalue);
	retain_value(backend, result._pod, result._type);
	return result._pod;
}







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
