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







/////////////////////////////////////////		update()


const runtime_value_t update_string(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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

const runtime_value_t update_vector_carray_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_vector_carray_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_vector_hamt_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_vector_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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

const runtime_value_t update_dict_cppmap_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_dict_cppmap_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_dict_hamt_pod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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
const runtime_value_t update_dict_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type, runtime_value_t value, runtime_type_t value_type){
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



////////////////////////////////	size()


int64_t unified_intrinsic__size_string(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type){
	auto& backend = get_backend(frp);

#if DEBUG
	const auto& type0 = lookup_type_ref(backend, vec_type);
	QUARK_ASSERT(peek2(backend.types, type0).is_string());
#endif

	return vec.vector_carray_ptr->get_element_count();
}

int64_t unified_intrinsic__size_vector_carray(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_carray_ptr->get_element_count();
}
int64_t unified_intrinsic__size_vector_hamt(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.vector_hamt_ptr->get_element_count();
}
int64_t unified_intrinsic__size_dict_cppmap(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_cppmap_ptr->size();
}
int64_t unified_intrinsic__size_dict_hamt(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
	auto& backend = get_backend(frp);
	(void)backend;
	return collection.dict_hamt_ptr->size();
}
int64_t unified_intrinsic__size_json(floyd_runtime_t* frp, runtime_value_t collection, runtime_type_t collection_type){
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

int64_t unified_intrinsic__size(floyd_runtime_t* frp, runtime_value_t coll_value, runtime_type_t coll_type){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	//	BC and LLVM inlines size()
	NOT_IMPLEMENTED_YET();
}




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












////////////////////////////////	push_back()


runtime_value_t unified_intrinsic__push_back_string(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__string(backend, vec, type_t(vec_type), element);
}

runtime_value_t unified_intrinsic__push_back_carray_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_pod(backend, vec, type_t(vec_type), element);
}

runtime_value_t unified_intrinsic__push_back_carray_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__carray_nonpod(backend, vec, type_t(vec_type), element);
}

runtime_value_t unified_intrinsic__push_back_hamt_pod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_pod(backend, vec, type_t(vec_type), element);
}

runtime_value_t unified_intrinsic__push_back_hamt_nonpod(floyd_runtime_t* frp, runtime_value_t vec, runtime_type_t vec_type, runtime_value_t element){
	auto& backend = get_backend(frp);

	return push_back_vector_element__hamt_nonpod(backend, vec, type_t(vec_type), element);
}

runtime_value_t unified_intrinsic__push_back(
	floyd_runtime_t* frp,

	runtime_value_t collection_value,
	runtime_type_t collection_type,

	runtime_value_t newvalue_value,
	runtime_type_t newvalue_type
){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	//	BC and LLVM inlines push_back()
	NOT_IMPLEMENTED_YET();
}


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



/////////////////////////////////////////		map()
//	[R] map([E] elements, func R (E e, C context) f, C context)



//??? Use C++ template to generate these two functions.

runtime_value_t unified_intrinsic__map__carray(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context_value, runtime_type_t context_type, runtime_type_t result_vec_type){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;

	QUARK_ASSERT(backend.check_invariant());

#if DEBUG
	const auto& type1 = lookup_type_ref(backend, f_type);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
//	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

//	const auto e_type = peek2(types, type0).get_vector_element_type(types);
	const auto f_arg_types = peek2(types, type1).get_function_args(types);
#endif

	const auto& func_link = lookup_func_link_required(backend, f_value);
	QUARK_ASSERT(func_link.machine == func_link_t::emachine::k_native || func_link.machine == func_link_t::emachine::k_native2);
	const auto f = reinterpret_cast<MAP_F>(func_link.f);

	const auto count = elements_vec.vector_carray_ptr->get_element_count();
	auto result_vec = alloc_vector_carray(backend.heap, count, count, type_t(result_vec_type));
	for(int i = 0 ; i < count ; i++){
		const auto a = (*f)(frp, elements_vec.vector_carray_ptr->get_element_ptr()[i], context_value);
		result_vec.vector_carray_ptr->get_element_ptr()[i] = a;
	}
	return result_vec;
}

//??? Update 1 element in a big hamt will copy the entire hamt, inc RC on all elements in hamt2. This is not needed since most of hamt is shared. Cheaper if we build in RC for leaf in the hamt itself.
//??? Use batching to speed up hamt creation. Add 32 nodes at a time. Also faster read iteration.
runtime_value_t unified_intrinsic__map__hamt(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context_value, runtime_type_t context_type, runtime_type_t result_vec_type){
	auto& backend = get_backend(frp);
	QUARK_ASSERT(backend.check_invariant());
	const auto& types = backend.types;

#if DEBUG
	const auto& type1 = lookup_type_ref(backend, f_type);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
//	QUARK_ASSERT(check_map_func_type(type0, type1, type2));

//	const auto e_type = peek2(types, type0).get_vector_element_type(types);
	const auto f_arg_types = peek2(types, type1).get_function_args(types);
#endif

	const auto& func_link = lookup_func_link_required(backend, f_value);
	QUARK_ASSERT(func_link.machine == func_link_t::emachine::k_native || func_link.machine == func_link_t::emachine::k_native2);
	const auto f = reinterpret_cast<MAP_F>(func_link.f);

	const auto count = elements_vec.vector_hamt_ptr->get_element_count();
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, type_t(result_vec_type));
	for(int i = 0 ; i < count ; i++){
		const auto& element = elements_vec.vector_hamt_ptr->load_element(i);
		const auto a = (*f)(frp, element, context_value);
		result_vec.vector_hamt_ptr->store_mutate(i, a);
	}
	return result_vec;
}

runtime_value_t unified_intrinsic__map(floyd_runtime_t* frp, runtime_value_t elements_vec, runtime_type_t elements_vec_type, runtime_value_t f_value, runtime_type_t f_type, runtime_value_t context_value, runtime_type_t context_type, runtime_type_t result_vec_type){
	auto& backend = get_backend(frp);
//	const auto& types = backend.types;

	const auto vec_type = type_t(elements_vec_type);

	QUARK_ASSERT(backend.check_invariant());

	if(vec_type.is_string()){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(is_vector_carray(backend.types, backend.config, vec_type)){
		return unified_intrinsic__map__carray(frp, elements_vec, elements_vec_type, f_value, f_type, context_value, context_type, result_vec_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, vec_type)){
		return unified_intrinsic__map__hamt(frp, elements_vec, elements_vec_type, f_value, f_type, context_value, context_type, result_vec_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



/////////////////////////////////////////		map_dag()



// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)


runtime_value_t unified_intrinsic__map_dag__carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);
	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
#if DEBUG
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
#endif
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_carray(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_carray(make_vector(r_type)));

	const auto return_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

	const auto elements2 = elements.vector_carray_ptr;
	const auto parents2 = parents.vector_carray_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<runtime_value_t> complete(elements2->get_element_count(), runtime_value_t());

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
			std::vector<runtime_value_t> solved_deps;
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

			auto solved_deps2 = alloc_vector_carray(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_carray_ptr->store(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

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
	auto result_vec = alloc_vector_carray(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_carray_ptr->store(i, complete[i]);
	}

	return result_vec;
}

runtime_value_t unified_intrinsic__map_dag__hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;
	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, depends_on_vec_type);
	const auto& type2 = lookup_type_ref(backend, f_value_type);
//	QUARK_ASSERT(check_map_dag_func_type(type0, type1, type2, lookup_type_ref(backend, context_type)));

	const auto& elements = elements_vec;
#if DEBUG
	const auto& e_type = peek2(types, type0).get_vector_element_type(types);
#endif
	const auto& parents = depends_on_vec;
	const auto& f = f_value;
	const auto& r_type = peek2(types, type2).get_function_return(types);

	QUARK_ASSERT(e_type == peek2(types, type2).get_function_args(types)[0] && r_type == peek2(types, peek2(types, type2).get_function_args(types)[1]).get_vector_element_type(types));

//	QUARK_ASSERT(is_vector_hamt(make_vector(e_type)));
//	QUARK_ASSERT(is_vector_hamt(make_vector(r_type)));

	const auto return_type = type_t::make_vector(types, r_type);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<map_dag_F>(func_link.f);

	const auto elements2 = elements.vector_hamt_ptr;
	const auto parents2 = parents.vector_hamt_ptr;

	if(elements2->get_element_count() != parents2->get_element_count()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2->get_element_count();
	std::vector<int> rcs(elements2->get_element_count(), 0);

	std::vector<runtime_value_t> complete(elements2->get_element_count(), runtime_value_t());

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
			std::vector<runtime_value_t> solved_deps;
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

			auto solved_deps2 = alloc_vector_hamt(backend.heap, solved_deps.size(), solved_deps.size(), return_type);
			for(int i = 0 ; i < solved_deps.size() ; i++){
				solved_deps2.vector_hamt_ptr->store_mutate(i, solved_deps[i]);
			}
			runtime_value_t solved_deps3 = solved_deps2;

			const auto result1 = (*f2)(frp, e, solved_deps3, context);

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
	auto result_vec = alloc_vector_hamt(backend.heap, count, count, return_type);
	for(int i = 0 ; i < count ; i++){
//		retain_value(r, complete[i], r_type);
		result_vec.vector_hamt_ptr->store_mutate(i, complete[i]);
	}

	return result_vec;
}

// ??? check type at compile time, not runtime.
// [R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
runtime_value_t unified_intrinsic__map_dag(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t depends_on_vec,
	runtime_type_t depends_on_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__map_dag__carray(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__map_dag__hamt(frp, elements_vec, elements_vec_type, depends_on_vec, depends_on_vec_type, f_value, f_value_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


/////////////////////////////////////////		filter()



runtime_value_t unified_intrinsic__filter_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
	const auto& return_type = type0;

//	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_type(backend, type_t(elements_vec_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(backend.types, e_element_itype)){
				retain_value(backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_carray(backend.heap, count2, count2, return_type);

	if(count2 > 0){
		//	Count > 0 required to get address to first element in acc.
		copy_elements(result_vec.vector_carray_ptr->get_element_ptr(), &acc[0], count2);
	}
	return result_vec;
}

runtime_value_t unified_intrinsic__filter_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context,
	runtime_type_t context_type)
{
	auto& backend = get_backend(frp);

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_type);
	const auto& return_type = type0;

//	QUARK_ASSERT(check_filter_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<FILTER_F>(func_link.f);

	auto count = vec.get_element_count();

	const auto e_element_itype = lookup_vector_element_type(backend, type_t(elements_vec_type));

	std::vector<runtime_value_t> acc;
	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto keep = (*f)(frp, element_value, context);
		if(keep.bool_value != 0){
			acc.push_back(element_value);

			if(is_rc_value(backend.types, e_element_itype)){
				retain_value(backend, element_value, e_element_itype);
			}
		}
		else{
		}
	}

	const auto count2 = acc.size();
	auto result_vec = alloc_vector_hamt(backend.heap, &acc[0], count2, return_type);
	return result_vec;
}

//??? check type at compile time, not runtime.
//	[E] filter([E] elements, func bool (E e, C context) f, C context)
runtime_value_t unified_intrinsic__filter(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t arg2_value,
	runtime_type_t arg2_type)
{
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__filter_carray(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__filter_hamt(frp, elements_vec, elements_vec_type, f_value, f_value_type, arg2_value, arg2_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


/////////////////////////////////////////		reduce()



runtime_value_t unified_intrinsic__reduce_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, init_value_type);
//	const auto& type2 = lookup_type_ref(backend, f_type);

//	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_carray_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.get_element_ptr()[i];
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, acc, type_t(init_value_type));
		}
		acc = acc2;
	}
	return acc;
}

runtime_value_t unified_intrinsic__reduce_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, init_value_type);
//	const auto& type2 = lookup_type_ref(backend, f_type);

//	QUARK_ASSERT(check_reduce_func_type(type0, type1, type2, lookup_type_ref(backend, f_type)));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type)));

	const auto& vec = *elements_vec.vector_hamt_ptr;
	const auto& init = init_value;

	const auto& func_link = lookup_func_link_required(backend, f_value);
	const auto f = reinterpret_cast<REDUCE_F>(func_link.f);

	auto count = vec.get_element_count();
	runtime_value_t acc = init;
	retain_value(backend, acc, type_t(init_value_type));

	for(int i = 0 ; i < count ; i++){
		const auto element_value = vec.load_element(i);
		const auto acc2 = (*f)(frp, acc, element_value, context);

		if(is_rc_value(backend.types, type_t(init_value_type))){
			release_value(backend, acc, type_t(init_value_type));
		}
		acc = acc2;
	}
	return acc;
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)

//??? check type at compile time, not runtime.
runtime_value_t unified_intrinsic__reduce(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t init_value,
	runtime_type_t init_value_type,
	runtime_value_t f_value,
	runtime_type_t f_type,
	runtime_value_t context,
	runtime_type_t context_type
){
	auto& backend = get_backend(frp);

	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__reduce_carray(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__reduce_hamt(frp, elements_vec, elements_vec_type, init_value, init_value_type, f_value, f_type, context, context_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


/////////////////////////////////////////		stable_sort()



typedef uint8_t (*stable_sort_F)(floyd_runtime_t* frp, runtime_value_t left_value, runtime_value_t right_value, runtime_value_t context_value);

runtime_value_t unified_intrinsic__stable_sort_carray(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_value_type);

//	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_carray(types, backend.config, type_t(elements_vec_type)));

	const auto& elements = elements_vec;
	const auto& f = f_value;
	const auto& context = context_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<stable_sort_F>(func_link.f);

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

		floyd_runtime_t* frp;
		runtime_value_t context;
		stable_sort_F f;
	};

	const sort_functor_r sort_functor { frp, context, f2 };

	auto mutate_inplace_elements = elements2.get_vector_value();
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);
	return to_runtime_value2(backend, value_t::make_vector_value(types, peek2(types, type0).get_vector_element_type(types), mutate_inplace_elements));
}

runtime_value_t unified_intrinsic__stable_sort_hamt(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

	const auto& types = backend.types;

	const auto& type0 = lookup_type_ref(backend, elements_vec_type);
//	const auto& type1 = lookup_type_ref(backend, f_value_type);
//	const auto& type2 = lookup_type_ref(backend, context_value_type);

//	QUARK_ASSERT(check_stable_sort_func_type(type0, type1, type2));
	QUARK_ASSERT(is_vector_hamt(types, backend.config, type_t(elements_vec_type)));

	const auto& elements = elements_vec;
	const auto& f = f_value;
	const auto& context = context_value;

	const auto elements2 = from_runtime_value2(backend, elements, type0);

	const auto& func_link = lookup_func_link_required(backend, f);
	const auto f2 = reinterpret_cast<stable_sort_F>(func_link.f);

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

		floyd_runtime_t* frp;
		runtime_value_t context;
		stable_sort_F f;
	};

	const sort_functor_r sort_functor { frp, context, f2 };

	auto mutate_inplace_elements = elements2.get_vector_value();
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

	//??? optimize
	const auto result = to_runtime_value2(backend, value_t::make_vector_value(types, peek2(types, type0).get_vector_element_type(types), mutate_inplace_elements));
	return result;
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)

//??? check type at compile time, not runtime.
runtime_value_t unified_intrinsic__stable_sort(
	floyd_runtime_t* frp,
	runtime_value_t elements_vec,
	runtime_type_t elements_vec_type,
	runtime_value_t f_value,
	runtime_type_t f_value_type,
	runtime_value_t context_value,
	runtime_type_t context_value_type
){
	auto& backend = get_backend(frp);

//	const auto& type0 = lookup_type_ref(r.backend, elements_vec_type);
	if(is_vector_carray(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__stable_sort_carray(frp, elements_vec, elements_vec_type, f_value, f_value_type, context_value, context_value_type);
	}
	else if(is_vector_hamt(backend.types, backend.config, type_t(elements_vec_type))){
		return unified_intrinsic__stable_sort_hamt(frp, elements_vec, elements_vec_type, f_value, f_value_type, context_value, context_value_type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


/////////////////////////////////////////		JSON



int64_t unified_intrinsic__get_json_type(floyd_runtime_t* frp, runtime_value_t json0){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif
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

runtime_value_t unified_intrinsic__bw_not(floyd_runtime_t* frp, runtime_value_t v){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = ~v.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_and(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value & b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_or(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value | b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_xor(floyd_runtime_t* frp, runtime_value_t a, runtime_value_t b){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = a.int_value ^ b.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_left(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value << count.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_right(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value >> count.int_value;
	return runtime_value_t { .int_value = result };
}
runtime_value_t unified_intrinsic__bw_shift_right_arithmetic(floyd_runtime_t* frp, runtime_value_t v, runtime_value_t count){
#if DEBUG
	auto& backend = get_backend(frp);
	(void)backend;
#endif

	const int64_t result = v.int_value >> count.int_value;
	return runtime_value_t { .int_value = result };
}





} // floyd
