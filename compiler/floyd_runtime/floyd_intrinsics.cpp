//
//  floyd_intrinsics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_intrinsics.h"

#include "floyd_runtime.h"
#include "text_parser.h"

namespace floyd {


static std::string gen_to_string(value_backend_t& backend, rt_pod_t arg_value, type_t arg_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& types = backend.types;

	if(peek2(types, arg_type).is_typeid()){
		const auto value = from_runtime_value2(backend, arg_value, arg_type);
		const auto type2 = value.get_typeid_value();
		const auto type3 = peek2(types, type2);
		const auto a = type_to_compact_string(types, type3);
		return a;
	}
	else{
		const auto value = from_runtime_value2(backend, arg_value, arg_type);
		const auto a = to_compact_string2(backend.types, value);
		return a;
	}
}


void intrinsic__assert(runtime_t* frp, rt_pod_t arg){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	bool ok = (arg.bool_value & 0x01) == 0 ? false : true;
	if(!ok){
		on_print(frp, "Assertion failed.");
		throw assert_failed_exception();
//		quark::throw_runtime_error("Assertion failed.");
	}
}

rt_pod_t intrinsic__to_string(runtime_t* frp, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	return to_runtime_string2(backend, s);
}

rt_pod_t intrinsic__to_pretty_string(runtime_t* frp, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto& value2 = from_runtime_value2(backend, value, type0);
	const auto json = value_to_json(backend.types, value2);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return to_runtime_string2(backend, s);
}


rt_type_t intrinsic__typeof(runtime_t* frp, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(type0.check_invariant());
#endif
	return value_type;
}


/////////////////////////////////////////		update()


const rt_pod_t update_string(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);
#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type0).is_string());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
	QUARK_ASSERT(peek2(backend.types, type2).is_int());
#endif
	return update__string(backend, coll_value, key_value, value);
}

const rt_pod_t update_vector_carray_pod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_carray(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_vector_carray_nonpod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_carray(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_vector_hamt_pod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_hamt_pod(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_vector_hamt_nonpod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_int());
#endif

	return update_element__vector_hamt_nonpod(backend, coll_value, coll_type, key_value, value);
}

const rt_pod_t update_dict_cppmap_pod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif

	return update__dict_cppmap(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_dict_cppmap_nonpod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif

	return update__dict_cppmap(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_dict_hamt_pod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif
	return update__dict_hamt(backend, coll_value, coll_type, key_value, value);
}
const rt_pod_t update_dict_hamt_nonpod(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	const auto& type2 = lookup_type_ref(backend, value_type);

	QUARK_ASSERT(type0.check_invariant());
	QUARK_ASSERT(type1.check_invariant());
	QUARK_ASSERT(type2.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type1).is_string());
#endif
	return update__dict_hamt(backend, coll_value, coll_type, key_value, value);
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
			quark::throw_defective_request();
		}
		else if(json0.is_object()){
			quark::throw_defective_request();
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
		quark::throw_defective_request();
	}
}

//??? user function type overloading and create several different functions, depending on the DYN argument.

// let b = update(a, member, value)
rt_pod_t intrinsic__update(
	runtime_t* frp,

	rt_pod_t collection_value,
	rt_type_t collection_type,

	rt_pod_t key_value,
	rt_type_t key_type,

	rt_pod_t newvalue_value,
	rt_type_t newvalue_type
){
	auto& backend = get_backend(frp);

	const auto collection = rt_value_t(backend, type_t(collection_type), collection_value, rt_value_t::rc_mode::bump);
	const auto key = rt_value_t(backend, type_t(key_type), key_value, rt_value_t::rc_mode::bump);
	const auto newvalue = rt_value_t(backend, type_t(newvalue_type), newvalue_value, rt_value_t::rc_mode::bump);

	const auto result = update_element(backend, collection, key, newvalue);
	retain_value(backend, result._pod, result._type);
	return result._pod;
}



////////////////////////////////	size()


int64_t intrinsic__size_string(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, vec_type);
	QUARK_ASSERT(peek2(backend.types, type0).is_string());
#endif

	return vec.vector_carray_ptr->get_element_count();
}

int64_t intrinsic__size_vector_carray(runtime_t* frp, rt_pod_t collection, rt_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_carray_ptr->get_element_count();
}
int64_t intrinsic__size_vector_hamt(runtime_t* frp, rt_pod_t collection, rt_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_hamt_ptr->get_element_count();
}
int64_t intrinsic__size_dict_cppmap(runtime_t* frp, rt_pod_t collection, rt_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_cppmap_ptr->size();
}
int64_t intrinsic__size_dict_hamt(runtime_t* frp, rt_pod_t collection, rt_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_hamt_ptr->size();
}
int64_t intrinsic__size_json(runtime_t* frp, rt_pod_t collection, rt_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;

	const auto& json = collection.json_ptr->get_json();

	if(json.is_object()){
		return json.get_object_size();
	}
	else if(json.is_array()){
		return json.get_array_size();
	}
	else if(json.is_string()){
		return json.get_string().size();
	}
	else{
		quark::throw_runtime_error("Calling size() on unsupported type of value.");
	}
}

int64_t intrinsic__size(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	//	BC and LLVM inlines size()
	quark::throw_feature_not_implemented_yet();
}


int64_t intrinsic__find(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, const rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);
	return find_vector_element(backend, coll_value, coll_type, value, value_type);
}

uint32_t intrinsic__exists(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
//	const auto& type1 = lookup_type_ref(backend, value_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());

	const bool result = exists_dict_value(backend, coll_value, coll_type, value, value_type);
	return result ? 1 : 0;
}

//??? all types are compile-time only.
rt_pod_t intrinsic__erase(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type, rt_pod_t key_value, rt_type_t key_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	const auto& type1 = lookup_type_ref(backend, key_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	QUARK_ASSERT(peek2(types, type1).is_string());
	return erase_dict_value(backend, coll_value, coll_type, key_value, key_type);
}

//??? We need to figure out the return type *again*, knowledge we have already in semast.
rt_pod_t intrinsic__get_keys(runtime_t* frp, rt_pod_t coll_value, rt_type_t coll_type){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, coll_type);
	QUARK_ASSERT(peek2(types, type0).is_dict());
	return get_keys(backend, coll_value, coll_type);
}






////////////////////////////////	push_back()


rt_pod_t intrinsic__push_back_string(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type, rt_pod_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__string(backend, vec, type_t(vec_type), element);
}

rt_pod_t intrinsic__push_back_carray_pod(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type, rt_pod_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_pod(backend, vec, type_t(vec_type), element);
}

rt_pod_t intrinsic__push_back_carray_nonpod(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type, rt_pod_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_nonpod(backend, vec, type_t(vec_type), element);
}

rt_pod_t intrinsic__push_back_hamt_pod(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type, rt_pod_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_pod(backend, vec, type_t(vec_type), element);
}

rt_pod_t intrinsic__push_back_hamt_nonpod(runtime_t* frp, rt_pod_t vec, rt_type_t vec_type, rt_pod_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_nonpod(backend, vec, type_t(vec_type), element);
}

rt_pod_t intrinsic__push_back(
	runtime_t* frp,

	rt_pod_t collection_value,
	rt_type_t collection_type,

	rt_pod_t newvalue_value,
	rt_type_t newvalue_type
){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	//	BC and LLVM inlines push_back()
	quark::throw_feature_not_implemented_yet();
}


//??? check type at compile time, not runtime.
// VECTOR subset(VECTOR s, int start, int end)
const rt_pod_t intrinsic__subset(runtime_t* frp, rt_pod_t elements_vec, rt_type_t elements_vec_type, uint64_t start, uint64_t end){
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
		quark::throw_defective_request();
	}
}

//??? check type at compile time, not runtime.
//	replace(VECTOR s, int start, int end, VECTOR new)
const rt_pod_t intrinsic__replace(runtime_t* frp, rt_pod_t elements_vec, rt_type_t elements_vec_type, uint64_t start, uint64_t end, rt_pod_t arg3_value, rt_type_t arg3_type){
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
		quark::throw_defective_request();
	}
}



/////////////////////////////////////////		map()
//	[R] map([E] elements, func R (E e, C context) f, C context)


rt_pod_t intrinsic__map__carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_type,
	rt_pod_t context_value,
	rt_type_t context_type,
	rt_type_t result_vec_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	QUARK_ASSERT(backend.check_invariant());

#if DEBUG
	const auto& f_type2 = lookup_type_ref(backend, f_type);
	const auto f_arg_types = peek2(types, f_type2).get_function_args(types);
#endif
	const auto e_type = peek2(types, type_t(elements_vec_type)).get_vector_element_type(types);

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(
		func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc
		|| func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc
	);
	const auto f = reinterpret_cast<MAP_F>(func_link.f);

	const auto count = elements_vec.vector_carray_ptr->get_element_count();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, type_t(result_vec_type));
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_carray_ptr->get_element_ptr()[i];

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t(backend, type_t(e_type), element, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(context_type), context_value, rt_value_t::rc_mode::bump)
			};
			const auto a = call_thunk(frp, rt_value_t(backend, type_t(f_type), f_value, rt_value_t::rc_mode::bump), f_args, 2);
			QUARK_ASSERT(a.check_invariant());
			retain_value(backend, a._pod, a._type);

			result_vec.vector_carray_ptr->get_element_ptr()[i] = a._pod;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			const auto a = (*f)(frp, element, context_value);
			result_vec.vector_carray_ptr->get_element_ptr()[i] = a;
		}
		else{
			quark::throw_defective_request();
		}
	}
	return result_vec;
}

struct f_env_t {
	runtime_t runtime;

	type_t e_type;
	type_t context_type;
	type_t f_type;
	rt_pod_t f_value;
	const func_link_t* func_link;
};

static rt_pod_t map_f_thunk(runtime_t* frp, rt_pod_t e_value, rt_pod_t context_value){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(backend.check_invariant());

	const auto& env = *(const f_env_t*)frp;

	const rt_value_t f_args[] = {
		rt_value_t(backend, env.e_type, e_value, rt_value_t::rc_mode::bump),
		rt_value_t(backend, env.context_type, context_value, rt_value_t::rc_mode::bump)
	};
	const auto a = call_thunk(frp, rt_value_t(backend, env.f_type, env.f_value, rt_value_t::rc_mode::bump), f_args, 2);
	QUARK_ASSERT(a.check_invariant());
	retain_value(backend, a._pod, a._type);

	return a._pod;
}

rt_pod_t intrinsic__map__hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_type,
	rt_pod_t context_value,
	rt_type_t context_type,
	rt_type_t vec_r_type
){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(backend.check_invariant());
	const auto& types = backend.types;

const auto& f_type2 = lookup_type_ref(backend, f_type);
#if DEBUG
	const auto f_arg_types = peek2(types, f_type2).get_function_args(types);
#endif
	const auto e_type = peek2(types, type_t(elements_vec_type)).get_vector_element_type(types);

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(
		func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc
		|| func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc
	);
	const auto f0 = reinterpret_cast<MAP_F>(func_link.f);

	f_env_t env = { *frp, e_type, type_t(context_type), f_type2, f_value, &func_link };
	const auto f = func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc ? map_f_thunk : f0;

	const auto count = elements_vec.vector_hamt_ptr->get_element_count();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, type_t(vec_r_type));
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_hamt_ptr->load_element(i);
		const auto a = (*f)((runtime_t*)&env, element, context_value);
		result_vec.vector_hamt_ptr->store_mutate(i, a);
	}
	return result_vec;
}

rt_pod_t intrinsic__map(runtime_t* frp, rt_pod_t elements_vec, rt_type_t elements_vec_type, rt_pod_t f_value, rt_type_t f_type, rt_pod_t context_value, rt_type_t context_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(backend.check_invariant());

	const auto& types = backend.types;

	const auto vec_e_type = type_t(elements_vec_type);
	QUARK_ASSERT(vec_e_type.is_string() == false);

	const auto& r_type = peek2(types, type_t(f_type)).get_function_return(types);
	const auto vec_r_type = type_t::make_vector(types, r_type);

	if(is_vector_carray(backend.types, backend.config, vec_e_type)){
		return intrinsic__map__carray(frp, elements_vec, elements_vec_type, f_value, f_type, context_value, context_type, vec_r_type.get_data());
	}
	else if(is_vector_hamt(backend.types, backend.config, vec_e_type)){
		return intrinsic__map__hamt(frp, elements_vec, elements_vec_type, f_value, f_type, context_value, context_type, vec_r_type.get_data());
	}
	else{
		quark::throw_defective_request();
	}
}



/////////////////////////////////////////		map_dag()



// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)


//	Input dependencies are specified for as 1... many integers per E, in order. [-1] or [a, -1] or [a, b, -1 ] etc.
//
//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)

struct dep_t {
	int64_t incomplete_count;
	std::vector<int64_t> depends_in_element_index;
};

#if 0
static rt_value_t bc_intrinsic__map_dag2(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_map_dag_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& dependencies = args[1];
	const auto& f = args[2];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto& r_type = f_type_peek.get_function_return(backend.types);

	const auto& context = args[3];

	if(
		e_type == f_type_peek.get_function_args(backend.types)[0]
		&& r_type == peek2(backend.types, f_type_peek.get_function_args(backend.types)[1]).get_vector_element_type(backend.types)
	){
	}
	else {
		quark::throw_runtime_error("R map_dag([E] elements, R init_value, R (R acc, E element) f");
	}

	const auto elements2 = get_vector_elements(backend, elements);
	const auto dependencies2 = get_vector_elements(backend, dependencies);


	immer::vector<rt_value_t> complete(elements2.size(), rt_value_t());

	std::vector<dep_t> element_dependencies(elements2.size(), dep_t{ 0, {} });
	{
		const auto element_count = static_cast<int64_t>(elements2.size());
		const auto dep_index_count = static_cast<int64_t>(dependencies2.size());
		int element_index = 0;
		int dep_index = 0;
		while(element_index < element_count){
			std::vector<int64_t> deps;
			const auto& e = dependencies2[dep_index];
			auto e_int = e.get_int_value();
			while(e_int != -1){
				deps.push_back(e_int);

				dep_index++;
				QUARK_ASSERT(dep_index < dep_index_count);

				e_int = dependencies2[dep_index].get_int_value();
			}
			QUARK_ASSERT(element_index < element_count);
			element_dependencies[element_index] = dep_t{ static_cast<int64_t>(deps.size()), deps };
			element_index++;
		}
		QUARK_ASSERT(element_index == element_count);
		QUARK_ASSERT(dep_index == dep_index_count);
	}

	auto elements_todo = elements2.size();
	while(elements_todo > 0){

		//	Make list of elements that should be processed this pass.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
			const auto rc = element_dependencies[i].incomplete_count;
			if(rc == 0){
				pass_ids.push_back(i);
				element_dependencies[i].incomplete_count = -1;
			}
		}

		if(pass_ids.empty()){
			quark::throw_runtime_error("map_dag() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2[element_index];

			immer::vector<rt_value_t> ready_elements;
			for(const auto& dep_e: element_dependencies[element_index].depends_in_element_index){
				const auto& ready = complete[dep_e];
				ready_elements = ready_elements.push_back(ready);
			}
			const auto ready_elements2 = make_vector_value(backend, r_type, ready_elements);
			const rt_value_t f_args[] = { e, ready_elements2, context };

			const auto result1 = call_function_bc(vm, f, f_args, 3);

			//	Decrement incomplete_count for every element that specifies *element_index* as a input dependency.
			for(auto& x: element_dependencies){
				const auto it = std::find(x.depends_in_element_index.begin(), x.depends_in_element_index.end(), element_index);
				if(it != x.depends_in_element_index.end()){
					x.incomplete_count--;
				}
			}

			//	Copy value to output.
			complete = complete.set(element_index, result1);
			elements_todo--;
		}
	}

	const auto result = make_vector_value(backend, r_type, complete);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	return result;
}
#endif


rt_pod_t intrinsic__map_dag__carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	rt_type_t depends_on_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& f_value_type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, f_value_type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, f_value_type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, f_value_type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, f_value_type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_carray(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_carray(make_vector(r_type)));

	const auto vec_r_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_by_pod_required(backend, f);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

	const auto elements2 = elements.vector_carray_ptr;
	const auto parents2 = parents.vector_carray_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<rt_pod_t> complete(elements2->get_element_count(), rt_pod_t());

	for(int i = 0 ; i < parents2->get_element_count() ; i++){
		const auto& e = parents2->load_element(i);
		const auto parent_index = e.int_value;

		const auto count = static_cast<int64_t>(elements2->get_element_count());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		//	Find all elements that are free to process -- they are not blocked on a depenency.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2->get_element_count() ; i++){
			const auto rc = rcs[i];
			if(rc == 0){
				pass_ids.push_back(i);
				rcs[i] = -1;
			}
		}
		if(pass_ids.empty()){
			quark::throw_runtime_error("map_dag() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2->load_element(element_index);

			//	Make list of the element's inputs -- they must all be complete now.
			std::vector<rt_pod_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2->get_element_count() ; element_index2++){
				const auto& p = parents2->load_element(element_index2);
				const auto parent_index = p.int_value;
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2->get_element_count());
					QUARK_ASSERT(rcs[element_index2] == -1);

					const auto& solved = complete[element_index2];
					solved_deps.push_back(solved);
				}
			}

			auto solved_deps2 = alloc_vector_carray(backend.heap, solved_deps.size(), solved_deps.size(), vec_r_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_carray_ptr->store(i, solved_deps[i]);
			}
			rt_pod_t solved_deps3 = solved_deps2;

			rt_pod_t result1 = make_uninitialized_magic();

			// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
			if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
				const rt_value_t f_args[] = {
					rt_value_t(backend, type_t(e_type), e, rt_value_t::rc_mode::bump),
					rt_value_t(backend, type_t(vec_r_type), solved_deps3, rt_value_t::rc_mode::bump),
					rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
				};
				const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 3);
				QUARK_ASSERT(a.check_invariant());
				retain_value(backend, a._pod, a._type);

				result1 = a._pod;
			}
			else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
				result1 = (*f2)(frp, e, solved_deps3, context);
			}
			else{
				quark::throw_defective_request();
			}

			//	Warning: optimization trick.
			//	Release just the vec, **not the elements**. The elements are aliases for complete-vector.
			if(dec_rc(solved_deps2.vector_carray_ptr->alloc) == 0){
				dispose_vector_carray(solved_deps2);
			}

			const auto parent_index = parents2->load_element(element_index).int_value;
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete[element_index] = result1;
			elements_todo--;
		}
	}

	//??? No need to copy all elements -- could store them directly into the VEC_T.
	const auto count = complete.size();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, vec_r_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_carray_ptr->store(i, complete[i]);
	}

	return result_vec;
}

rt_pod_t intrinsic__map_dag__hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	rt_type_t depends_on_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& f_value_type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, f_value_type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, f_value_type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, f_value_type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, f_value_type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_hamt(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_hamt(make_vector(r_type)));

	const auto vec_r_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_by_pod_required(backend, f);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

	const auto elements2 = elements.vector_hamt_ptr;
	const auto parents2 = parents.vector_hamt_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<rt_pod_t> complete(elements2->get_element_count(), rt_pod_t());

	for(int i = 0 ; i < parents2->get_element_count() ; i++){
		const auto& e = parents2->load_element(i);
		const auto parent_index = e.int_value;

		//??? remove cast? static_cast<int64_t>()
		const auto count = static_cast<int64_t>(elements2->get_element_count());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		//	Find all elements that are free to process -- they are not blocked on a depenency.
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2->get_element_count() ; i++){
			const auto rc = rcs[i];
			if(rc == 0){
				pass_ids.push_back(i);
				rcs[i] = -1;
			}
		}
		if(pass_ids.empty()){
			quark::throw_runtime_error("map_dag() dependency cycle error.");
		}

		for(const auto element_index: pass_ids){
			const auto& e = elements2->load_element(element_index);

			//	Make list of the element's inputs -- they must all be complete now.
			std::vector<rt_pod_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2->get_element_count() ; element_index2++){
				const auto& p = parents2->load_element(element_index2);
				const auto parent_index = p.int_value;
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2->get_element_count());
					QUARK_ASSERT(rcs[element_index2] == -1);

					const auto& solved = complete[element_index2];
					solved_deps.push_back(solved);
				}
			}

			auto solved_deps2 = alloc_vector_hamt(backend.heap, solved_deps.size(), solved_deps.size(), vec_r_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_hamt_ptr->store_mutate(i, solved_deps[i]);
			}
			rt_pod_t solved_deps3 = solved_deps2;

			// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
			rt_pod_t result1 = make_uninitialized_magic();
			if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
				const rt_value_t f_args[] = {
					rt_value_t(backend, type_t(e_type), e, rt_value_t::rc_mode::bump),
					rt_value_t(backend, type_t(vec_r_type), solved_deps3, rt_value_t::rc_mode::bump),
					rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
				};
				const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 3);
				QUARK_ASSERT(a.check_invariant());
				retain_value(backend, a._pod, a._type);

				result1 = a._pod;
			}
			else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
				result1 = (*f2)(frp, e, solved_deps3, context);
			}
			else{
				quark::throw_defective_request();
			}

			//	Warning: optimization trick.
			//	Release just the vec, **not the elements**. The elements are aliases for complete-vector.
			if(dec_rc(solved_deps2.vector_hamt_ptr->alloc) == 0){
				dispose_vector_hamt(solved_deps2);
			}

			const auto parent_index = parents2->load_element(element_index).int_value;
			if(parent_index != -1){
				rcs[parent_index]--;
			}
			complete[element_index] = result1;
			elements_todo--;
		}
	}

	//??? No need to copy all elements -- could store them directly into the VEC_T.
	const auto count = complete.size();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, vec_r_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_hamt_ptr->store_mutate(i, complete[i]);
	}

	return result_vec;
}

// ??? check type at compile time, not runtime.
// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
rt_pod_t intrinsic__map_dag(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t depends_on_vec,
	rt_type_t depends_on_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__map_dag__carray(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__map_dag__hamt(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else{
		quark::throw_defective_request();
	}
}


/////////////////////////////////////////		filter()

//	[E] filter([E] elements, func bool (E e, C context) f, C context)


rt_pod_t intrinsic__filter_carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	const auto& vec_e_type = lookup_type_ref(backend, elements_vec_type);
	const auto e_type = peek2(types, vec_e_type).get_vector_element_type(types);
	const auto& f_value_type2 = lookup_type_ref(backend, f_value_type);

//	QUARK_ASSERT(check_filter_func_type(vec_e_type, type1, type2));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_type(backend, type_t(elements_vec_type));

	std::vector<rt_pod_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto value = vec.get_element_ptr()[i];

		rt_pod_t keep = make_uninitialized_magic();

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t(backend, type_t(e_type), value, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
			};
			const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 2);
			QUARK_ASSERT(a.check_invariant());
			retain_value(backend, a._pod, a._type);

			keep = a._pod;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			keep = (*f)(frp, value, context);
		}
		else{
			quark::throw_defective_request();
		}

		if(keep.bool_value != 0){
			acc.push_back(value);

			if(is_rc_value(backend.types, e_element_itype)){
				retain_value(backend, value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_carray(backend.heap, count2, count2, vec_e_type);

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec.vector_carray_ptr->get_element_ptr(), &acc[0], count2);
	}
	return result_vec;
}

rt_pod_t intrinsic__filter_hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context,
	rt_type_t context_type)
{
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	const auto& vec_e_type = lookup_type_ref(backend, elements_vec_type);
	const auto e_type = peek2(types, vec_e_type).get_vector_element_type(types);
	const auto& f_value_type2 = lookup_type_ref(backend, f_value_type);

//	QUARK_ASSERT(check_filter_func_type(vec_e_type, type1, type2));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	std::vector<rt_pod_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto value = vec.load_element(i);

		rt_pod_t keep = make_uninitialized_magic();

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t(backend, type_t(e_type), value, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
			};
			const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 2);
			QUARK_ASSERT(a.check_invariant());
			retain_value(backend, a._pod, a._type);

			keep = a._pod;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			keep = (*f)(frp, value, context);
		}
		else{
			quark::throw_defective_request();
		}

		if(keep.bool_value != 0){
			acc.push_back(value);

			if(is_rc_value(backend.types, e_type)){
				retain_value(backend, value, e_type);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_hamt(backend.heap, &acc[0], count2, vec_e_type);
	return result_vec;
}

//??? check type at compile time, not runtime.
rt_pod_t intrinsic__filter(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t arg2_value,
	rt_type_t arg2_type)
{
	auto& backend = get_backend(frp);

	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__filter_carray(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__filter_hamt(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else{
		quark::throw_defective_request();
	}
}


/////////////////////////////////////////		reduce()

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)


rt_pod_t intrinsic__reduce_carray(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t init_value,
	rt_type_t init_value_type,
	rt_pod_t f_value,
	rt_type_t f_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	const auto& vec_e_type = lookup_type_ref(backend, elements_vec_type);
	const auto& r_type = lookup_type_ref(backend, init_value_type);
	const auto& f_value_type2 = lookup_type_ref(backend, f_type);
	const auto e_type = peek2(types, vec_e_type).get_vector_element_type(types);

//	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	rt_pod_t accumulator = init;
	retain_value(backend, accumulator, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto value = vec.get_element_ptr()[i];

		rt_pod_t accumulator2 = make_uninitialized_magic();

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t(backend, type_t(r_type), accumulator, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(e_type), value, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
			};
			const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 3);
			QUARK_ASSERT(a.check_invariant());
			retain_value(backend, a._pod, a._type);

			accumulator2 = a._pod;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			accumulator2 = (*f)(frp, accumulator, value, context);
		}
		else{
			quark::throw_defective_request();
		}

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, accumulator, type_t(init_value_type));
		}
		accumulator = accumulator2;
	}
	return accumulator;
}

rt_pod_t intrinsic__reduce_hamt(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t init_value,
	rt_type_t init_value_type,
	rt_pod_t f_value,
	rt_type_t f_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	const auto& vec_e_type = lookup_type_ref(backend, elements_vec_type);
	const auto& r_type = lookup_type_ref(backend, init_value_type);
	const auto& f_value_type2 = lookup_type_ref(backend, f_type);
	const auto e_type = peek2(types, vec_e_type).get_vector_element_type(types);

//	QUARK_ASSERT(check_reduce_func_type(type0, r_type, f_value_type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	rt_pod_t accumulator = init;
	retain_value(backend, accumulator, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto value = vec.load_element(i);

		rt_pod_t accumulator2 = make_uninitialized_magic();

		// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
		if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
			const rt_value_t f_args[] = {
				rt_value_t(backend, type_t(r_type), accumulator, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(e_type), value, rt_value_t::rc_mode::bump),
				rt_value_t(backend, type_t(context_type), context, rt_value_t::rc_mode::bump)
			};
			const auto a = call_thunk(frp, rt_value_t(backend, f_value_type2, f_value, rt_value_t::rc_mode::bump), f_args, 3);
			QUARK_ASSERT(a.check_invariant());
			retain_value(backend, a._pod, a._type);

			accumulator2 = a._pod;
		}
		else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
			accumulator2 = (*f)(frp, accumulator, value, context);
		}
		else{
			quark::throw_defective_request();
		}

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, accumulator, type_t(init_value_type));
		}
		accumulator = accumulator2;
	}
	return accumulator;
}

//??? check type at compile time, not runtime.
rt_pod_t intrinsic__reduce(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t init_value,
	rt_type_t init_value_type,
	rt_pod_t f_value,
	rt_type_t f_type,
	rt_pod_t context,
	rt_type_t context_type
){
	auto& backend = get_backend(frp);

	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__reduce_carray(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return intrinsic__reduce_hamt(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else{
		quark::throw_defective_request();
	}
}


/////////////////////////////////////////		stable_sort()

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)

//??? check type at compile time, not runtime.
rt_pod_t intrinsic__stable_sort(
	runtime_t* frp,
	rt_pod_t elements_vec,
	rt_type_t elements_vec_type,
	rt_pod_t f_value,
	rt_type_t f_value_type,
	rt_pod_t context_value,
	rt_type_t context_value_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
	}
	else{
		quark::throw_defective_request();
	}

	const auto& vec_t_type = lookup_type_ref(backend, elements_vec_type);
	const auto t_type = vec_t_type.get_vector_element_type(types);
	const auto& f_value_type2 = lookup_type_ref(backend, f_value_type);

	const auto& func_link = lookup_func_link_by_pod_required(backend, f_value);
	QUARK_ASSERT(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc || func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc);
	const auto f2 = reinterpret_cast<stable_sort_F>(func_link.f);

	//??? Expensive deep-copy of all elements!
	const auto elements2 = from_runtime_value2(backend, elements_vec, vec_t_type);

	// ??? This thunking must be moved of inner loop. Use ffi to make bridge for k_native__floydcc?
	if(func_link.execution_model == func_link_t::eexecution_model::k_bytecode__floydcc){
		struct sort_functor_r {
			bool operator() (const value_t &a, const value_t &b) {
				auto& backend = get_backend(frp);

				const auto left = to_runtime_value2(backend, a);
				const auto right = to_runtime_value2(backend, b);

				const rt_value_t f_args[] = {
					rt_value_t(backend, t_type, left, rt_value_t::rc_mode::bump),
					rt_value_t(backend, t_type, right, rt_value_t::rc_mode::bump),
					rt_value_t(backend, context_type, context, rt_value_t::rc_mode::bump)
				};
				const auto result = call_thunk(frp, rt_value_t(backend, f_type, f_value, rt_value_t::rc_mode::bump), f_args, 3);
				QUARK_ASSERT(result.check_invariant());
				return result.get_bool_value();
			}

			runtime_t* frp;

			rt_pod_t f_value;
			type_t f_type;

			type_t t_type;

			rt_pod_t context;
			type_t context_type;
		};
		const sort_functor_r sort_functor { frp, f_value, f_value_type2, t_type, context_value, type_t(context_value_type) };

		auto mutate_inplace_elements = elements2.get_vector_value();
		std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

		//??? optimize
		const auto result = to_runtime_value2(backend, value_t::make_vector_value(types, t_type, mutate_inplace_elements));
		return result;
	}
	else if(func_link.execution_model == func_link_t::eexecution_model::k_native__floydcc){
		struct sort_functor_r {
			bool operator() (const value_t &a, const value_t &b) {
				auto& backend = get_backend(frp);

				const auto left = to_runtime_value2(backend, a);
				const auto right = to_runtime_value2(backend, b);
				uint8_t result = (*f)(frp, left, right, context);

				release_value(backend, left, a.get_type());
				release_value(backend, right, b.get_type());

				return result == 1 ? true : false;
			}

			runtime_t* frp;
			rt_pod_t context;
			stable_sort_F f;
		};
		const sort_functor_r sort_functor { frp, context_value, f2 };

		auto mutate_inplace_elements = elements2.get_vector_value();
		std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

		//??? optimize
		const auto result = to_runtime_value2(backend, value_t::make_vector_value(types, t_type, mutate_inplace_elements));
		return result;
	}
	else{
		quark::throw_defective_request();
	}
}


/////////////////////////////////////////		JSON



int64_t intrinsic__get_json_type(runtime_t* frp, rt_pod_t json0){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto result = get_json_type(json);
	return result;
}

rt_pod_t intrinsic__generate_json_script(runtime_t* frp, rt_pod_t json0){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const std::string s = json_to_compact_string(json);
	return to_runtime_string2(backend, s);
}

rt_pod_t intrinsic__parse_json_script(runtime_t* frp, rt_pod_t string_s0){
	auto& backend = get_backend(frp);

	const auto string_s = from_runtime_string2(backend, string_s0);

	std::pair<json_t, seq_t> result0 = parse_json(seq_t(string_s));
	return alloc_json(backend.heap, result0.first);
}

rt_pod_t intrinsic__to_json(runtime_t* frp, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, value_type);
	const auto value0 = from_runtime_value2(backend, value, type0);
	const auto j = value_to_json(backend.types, value0);
	return alloc_json(backend.heap, j);
}

rt_pod_t intrinsic__from_json(runtime_t* frp, rt_pod_t json0, rt_type_t target_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(json0.json_ptr != nullptr);

	const auto& json = json0.json_ptr->get_json();
	const auto& target_type2 = lookup_type_ref(backend, target_type);

	const auto result = unflatten_json_to_specific_type(backend.types, json, target_type2);
	return to_runtime_value2(backend, result);
}



/////////////////////////////////////////		print()


void intrinsic__print(runtime_t* frp, rt_pod_t value, rt_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	on_print(frp, s);
}


void intrinsic__send(runtime_t* frp, rt_pod_t dest_process_id0, rt_pod_t message, rt_type_t message_type){
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

void intrinsic__exit(runtime_t* frp){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

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

rt_pod_t intrinsic__bw_not(runtime_t* frp, rt_pod_t v){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = ~v.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_and(runtime_t* frp, rt_pod_t a, rt_pod_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value & b.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_or(runtime_t* frp, rt_pod_t a, rt_pod_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value | b.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_xor(runtime_t* frp, rt_pod_t a, rt_pod_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value ^ b.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_shift_left(runtime_t* frp, rt_pod_t v, rt_pod_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value << count.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_shift_right(runtime_t* frp, rt_pod_t v, rt_pod_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value >> count.int_value;
	return rt_pod_t { .int_value = result };
}

rt_pod_t intrinsic__bw_shift_right_arithmetic(runtime_t* frp, rt_pod_t v, rt_pod_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value >> count.int_value;
	return rt_pod_t { .int_value = result };
}


} // floyd
