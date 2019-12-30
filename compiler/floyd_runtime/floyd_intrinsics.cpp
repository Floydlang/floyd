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


//??? user function type overloading and create several different functions, depending on the DYN argument.


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

//	SIZE

int64_t unified_intrinsic__find(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);
	return find_vector_element(backend, coll_value, coll_type, value, value_type);
}

uint32_t unified_intrinsic__exists(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
//	const auto& type1 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());

	const bool result = exists_dict_value(backend, coll_value, coll_type, value, value_type);
	return result ? 1 : 0;
}

//??? all types are compile-time only.
runtime_value_t unified_intrinsic__erase(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	QUARK_ASSERT(peek2(types, type1).is_string());
	return erase_dict_value(backend, coll_value, coll_type, key_value, key_type);
}

//??? We need to figure out the return type *again*, knowledge we have already in semast.
runtime_value_t unified_intrinsic__get_keys(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	return get_keys(backend, coll_value, coll_type);
}

//	PUSHBACK

//??? check type at compile time, not runtime.
// VECTOR subset(VECTOR s, int start, int end)
const runtime_value_t unified_intrinsic__subset(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(peek2(backend.types, type0).is_string()){
		return subset_vector_range__string(backend, elements_vec, elements_vec_type, start, end);
	}
	else if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return subset_vector_range__carray(backend, elements_vec, elements_vec_type, start, end);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return subset_vector_range__hamt(backend, elements_vec, elements_vec_type, start, end);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}

//??? check type at compile time, not runtime.
//	replace(VECTOR s, int start, int end, VECTOR new)
const runtime_value_t unified_intrinsic__replace(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, uint64_t start, uint64_t end, runtime_value_t arg3_value, runtime_type_t arg3_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	const auto& type3 = lookup_type_ref(backend, arg3_type);

	QUARK_ASSERT(type3 == type0);

	if(peek2(backend.types, type0).is_string()){
		return replace_vector_range__string(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return replace_vector_range__carray(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return replace_vector_range__hamt(backend, elements_vec, elements_vec_type, start, end, arg3_value, arg3_type);
	}
	else{
		//	No other types allowed.
		UNSUPPORTED();
	}
}



/////////////////////////////////////////		JSON



int64_t unified_intrinsic__get_json_type(floyd_runtime_t* frp, runtime_value_t json0){
	auto& backend = get_backend(frp);
	(void)backend;
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto result = get_json_type(json);
	return result;
}

runtime_value_t unified_intrinsic__generate_json_script(floyd_runtime_t* frp, runtime_value_t json0){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const std::string s = json_to_compact_string(json);
	return to_runtime_string2(backend, s);
}

runtime_value_t unified_intrinsic__parse_json_script(floyd_runtime_t* frp, runtime_value_t string_s0){
	auto& backend = get_backend(frp);

	const auto string_s = from_runtime_string2(backend, string_s0);

	std::pair<json_t, seq_t> result0 = parse_json(seq_t(string_s));
	return alloc_json(backend.heap, result0.first);
}

runtime_value_t unified_intrinsic__to_json(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto value0 = from_runtime_value2(backend, value, type0);
	const auto j = value_to_json(backend.types, value0);
	return alloc_json(backend.heap, j);
}

runtime_value_t unified_intrinsic__from_json(floyd_runtime_t* frp, runtime_value_t json0, runtime_type_t target_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto& target_type2 = lookup_type_ref(backend, target_type);

	const auto result = unflatten_json_to_specific_type(backend.types, json, target_type2);
	return to_runtime_value2(backend, result);
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









/////////////////////////////////////////		PURE BITWISE

/*
 |OPERATOR		| EXPLANATION
 |:---	|:---
 | int bw\_not(int v)		| inverts all bits in the integer v.
 | int bw\_and(int a, int b)		| and:s each bit in a with the corresponding bit in b
 | int bw\_or(int a, int b)		| or:s each bit in a with the corresponding bit in b
 | int bw\_xor(int a, int b)		| xor:s each bit in a with the corresponding bit in b
 | int bw\_shift\_left(int v, int count)		| shifts the bits in v left, the number of bits specified by count. New bits are set to 0.
 | int bw\_shift\_right(int v, int count)		| shifts the bits in v right, the number of bits specified by count. New bits are set to 0.
 | int bw\_shift\_right\_arithmetic(int v, int count)		| shifts the bits in v right, the number of bits specified by count. New bits are copied from bit 63, which sign-extends the number		| it doesn't lose its negativeness.

*/

runtime_value_t unified_intrinsic__bw_not(floyd_runtime_t* frp, runtime_value_t v){
	auto& backend = get_backend(frp);

	const int64_t result = ~v.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_and(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
	auto& backend = get_backend(frp);

	const int64_t result = a.int_value & b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_or(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
	auto& backend = get_backend(frp);

	const int64_t result = a.int_value | b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_xor(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
	auto& backend = get_backend(frp);

	const int64_t result = a.int_value ^ b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_left(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
	auto& backend = get_backend(frp);

	const int64_t result = v.int_value << count.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_right(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
	auto& backend = get_backend(frp);

	const int64_t result = v.int_value >> count.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_right_arithmetic(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
	auto& backend = get_backend(frp);

	const int64_t result = v.int_value >> count.int_value;
	return runtime_value_t { .int_value = result };
}





} // floyd
