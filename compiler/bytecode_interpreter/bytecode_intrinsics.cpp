//
//  bytecode_intrinsics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_intrinsics.h"

#include "json_support.h"

#include "text_parser.h"
#include "file_handling.h"
#include "floyd_corelib.h"
#include "ast_value.h"
#include "floyd_interpreter.h"
#include "floyd_runtime.h"
#include "value_features.h"


#include <algorithm>
#include <iostream>
#include <fstream>


namespace floyd {

static const bool k_trace = false;



static rt_value_t bc_intrinsic__assert(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_bool());

	const auto& value = args[0];
	bool ok = value.get_bool_value();
	if(!ok){
		vm._handler->on_print("Assertion failed.");
		throw assert_failed_exception();
//		quark::throw_runtime_error("Assertion failed.");
	}
	return rt_value_t::make_undefined();
}

//	string to_string(value_t)
static rt_value_t bc_intrinsic__to_string(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& value = args[0];
	const auto a = to_compact_string2(backend.types, rt_to_value(backend, value));
	return rt_value_t::make_string(backend, a);
}
static rt_value_t bc_intrinsic__to_pretty_string(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& value = args[0];
	const auto json = bcvalue_to_json(backend, value);
	const auto s = json_to_pretty_string(json, 0, pretty_t{ 80, 4 });
	return rt_value_t::make_string(backend, s);
}

static rt_value_t bc_intrinsic__typeof(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto& value = args[0];
	const auto type = value._type;
	const auto result = value_t::make_typeid_value(type);
	return value_to_rt(backend, result);
}







//////////////////////////////////////////		UPDATE



//??? move to value_features.h. Move to intrinsics for BC.
static rt_value_t update_element(interpreter_t& vm, const rt_value_t& obj1, const rt_value_t& lookup_key, const rt_value_t& new_value){
	QUARK_ASSERT(vm.check_invariant());

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));
	auto& backend = vm._backend;
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

static rt_value_t bc_intrinsic__update(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];
	return update_element(vm, obj1, lookup_key, new_value);
}

static rt_value_t bc_intrinsic__find(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	const auto obj = args[0];
	const auto wanted = args[1];
	const auto index = find_vector_element(backend, obj._pod, obj._type.get_data(), wanted._pod, wanted._type.get_data());
	return rt_value_t(backend, type_t::make_int(), make_runtime_int(index), rt_value_t::rc_mode::adopt);
}

//??? user function type overloading and create several different functions, depending on the DYN argument.


static rt_value_t bc_intrinsic__exists(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	const auto obj = args[0];
	const auto key = args[1];

	QUARK_ASSERT(peek2(backend.types, obj._type).is_dict());
	QUARK_ASSERT(peek2(backend.types, key._type).is_string());

	const bool result = exists_dict_value(backend, obj._pod, obj._type.get_data(), key._pod, key._type.get_data());
	return rt_value_t::make_bool(result);
}

static rt_value_t bc_intrinsic__erase(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	const auto obj = args[0];
	const auto key = args[1];

	QUARK_ASSERT(peek2(backend.types, obj._type).is_dict());
	QUARK_ASSERT(peek2(backend.types, key._type).is_string());

	const auto result = erase(backend, obj._pod, obj._type.get_data(), key._pod, key._type.get_data());
	return rt_value_t(backend, obj._type, result, rt_value_t::rc_mode::adopt);
}

static rt_value_t bc_intrinsic__get_keys(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto obj = args[0];
	const auto& obj_type_peek = peek2(backend.types, obj._type);
	QUARK_ASSERT(obj_type_peek.is_dict());

	const auto result = get_keys(backend, obj._pod, obj._type.get_data());
	return rt_value_t(backend, type_t::make_vector(backend.types, type_t::make_string()), result, rt_value_t::rc_mode::adopt);
}

/*
rt_value_t bc_intrinsic__push_back(interpreter_t& vm, const rt_value_t args[], int arg_count){
}
*/

//	assert(subset("abc", 1, 3) == "bc");
static rt_value_t bc_intrinsic__subset(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[2]._type).is_int());

	const auto obj = args[0];

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();

	const auto result = subset_vector_range(backend, obj._pod, obj._type.get_data(), start, end);
	return rt_value_t(backend, obj._type, result, rt_value_t::rc_mode::adopt);
}


//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
static rt_value_t bc_intrinsic__replace(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[2]._type).is_int());

	const auto obj = args[0];

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	const auto replacement = args[3];
	const auto result = replace_vector_range(backend, obj._pod, obj._type.get_data(), start, end, replacement._pod, replacement._type.get_data());
	return rt_value_t(backend, obj._type, result, rt_value_t::rc_mode::adopt);
}

/*
	Reads json from a text string, returning an unpacked json.
*/
static rt_value_t bc_intrinsic__parse_json_script(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_string());

	const auto s = args[0].get_string_value(backend);
	std::pair<json_t, seq_t> result = parse_json(seq_t(s));
	const auto json = rt_value_t::make_json(backend, result.first);
	return json;
}

static rt_value_t bc_intrinsic__generate_json_script(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_json());

	const auto value0 = args[0].get_json();
	const auto s = json_to_compact_string(value0);
	return rt_value_t::make_string(backend, s);
}


static rt_value_t bc_intrinsic__to_json(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	const auto value = args[0];
	const auto value2 = rt_to_value(backend, args[0]);

	const auto j = value_to_json(backend.types, value2);
	value_t result = value_t::make_json(j);

	return value_to_rt(backend, result);
}

static rt_value_t bc_intrinsic__from_json(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_json());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_typeid());

	const auto json = args[0].get_json();
	const auto target_type = args[1].get_typeid_value();

	types_t temp = backend.types;
	const auto result = unflatten_json_to_specific_type(temp, json, target_type);
	return value_to_rt(backend, result);
}


static rt_value_t bc_intrinsic__get_json_type(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_json());

	const auto json = args[0].get_json();
	return rt_value_t::make_int(get_json_type(json));
}



/////////////////////////////////////////		PURE -- MAP()



//	[R] map([E] elements, func R (E e, C context) f, C context)
static rt_value_t bc_intrinsic__map(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_map_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;

//	const auto e_type = peek2(backend.types, args[0]._type).get_vector_element_type(backend.types);
	const auto f = args[1];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto f_arg_types = f_type_peek.get_function_args(backend.types);
	const auto r_type = f_type_peek.get_function_return(backend.types);

	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, args[0]);
	immer::vector<rt_value_t> vec2;
	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		QUARK_ASSERT(result1.check_invariant());
		vec2 = vec2.push_back(result1);
	}

	const auto result = make_vector_value(backend, r_type, vec2);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}
	QUARK_ASSERT(result.check_invariant());
	return result;
}


/////////////////////////////////////////		PURE -- map_string()


//	string map_string(string s, func string(string e, C context) f, C context)
static rt_value_t bc_intrinsic__map_string(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_map_string_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;
	const auto f = args[1];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto f_arg_types = f_type_peek.get_function_args(backend.types);
//	const auto r_type = f_type_peek.get_function_return(backend.types);
	const auto& context = args[2];

	const auto input_vec = args[0].get_string_value(backend);
	std::string vec2;
	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { rt_value_t::make_int(e), context };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		QUARK_ASSERT(peek2(backend.types, result1._type).is_int());
		const int64_t ch = result1.get_int_value();
		vec2.push_back(static_cast<char>(ch));
	}

	const auto result = rt_value_t::make_string(backend, vec2);

if(k_trace && false){
	const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
	QUARK_TRACE(json_to_pretty_string(debug));
}

	return result;
}



/////////////////////////////////////////		PURE -- map_dag()



//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
static rt_value_t bc_intrinsic__map_dag(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_map_dag_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
//	const auto& e_type = elements._type.get_vector_element_type();
	const auto& parents = args[1];
	const auto& f = args[2];
	const auto f_type_peek = peek2(backend.types, f._type);
	const auto& r_type = f_type_peek.get_function_return(backend.types);
	const auto& context = args[3];

	const auto elements2 = get_vector_elements(backend, elements);
	const auto parents2 = get_vector_elements(backend, parents);

	if(elements2.size() != parents2.size()) {
		quark::throw_runtime_error("map_dag() requires elements and parents be the same count.");
	}

	auto elements_todo = elements2.size();
	std::vector<int> rcs(elements2.size(), 0);

	immer::vector<rt_value_t> complete(elements2.size(), rt_value_t());

	for(const auto& e: parents2){
		const auto parent_index = e.get_int_value();

		const auto count = static_cast<int64_t>(elements2.size());
		QUARK_ASSERT(parent_index >= -1);
		QUARK_ASSERT(parent_index < count);

		if(parent_index != -1){
			rcs[parent_index]++;
		}
	}

	while(elements_todo > 0){
		std::vector<int> pass_ids;
		for(int i = 0 ; i < elements2.size() ; i++){
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
			const auto& e = elements2[element_index];

			//	Make list of the element's inputs -- the must all be complete now.
			immer::vector<rt_value_t> solved_deps;
			for(int element_index2 = 0 ; element_index2 < parents2.size() ; element_index2++){
				const auto& p = parents2[element_index2];
				const auto parent_index = p.get_int_value();
				if(parent_index == element_index){
					QUARK_ASSERT(element_index2 != -1);
					QUARK_ASSERT(element_index2 >= -1 && element_index2 < elements2.size());
					QUARK_ASSERT(rcs[element_index2] == -1);
					QUARK_ASSERT(complete[element_index2]._type.is_undefined() == false);
					const auto& solved = complete[element_index2];
					solved_deps = solved_deps.push_back(solved);
				}
			}

			const rt_value_t f_args[] = { e, make_vector_value(backend, r_type, solved_deps), context };
			const auto result1 = call_function_bc(vm, f, f_args, 3);

			const auto parent_index = parents2[element_index].get_int_value();
			if(parent_index != -1){
				rcs[parent_index]--;
			}
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





/////////////////////////////////////////		PURE -- map_dag2()

//	Input dependencies are specified for as 1... many integers per E, in order. [-1] or [a, -1] or [a, b, -1 ] etc.
//
//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)

struct dep_t {
	int64_t incomplete_count;
	std::vector<int64_t> depends_in_element_index;
};


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




/////////////////////////////////////////		PURE -- filter()



//	[E] filter([E] elements, func bool (E e, C context) f, C context)
static rt_value_t bc_intrinsic__filter(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_filter_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& f = args[1];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, elements);
	immer::vector<rt_value_t> vec2;

	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 2);
		QUARK_ASSERT(peek2(backend.types, result1._type).is_bool());

		if(result1.get_bool_value()){
			vec2 = vec2.push_back(e);
		}
	}

	const auto result = make_vector_value(backend, e_type, vec2);

	if(k_trace && false){
		const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	return result;
}



/////////////////////////////////////////		PURE -- reduce()



//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
static rt_value_t bc_intrinsic__reduce(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 4);
//	QUARK_ASSERT(check_reduce_func_type(args[0]._type, args[1]._type, args[2]._type, args[3]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& init = args[1];
	const auto& f = args[2];
	const auto& context = args[3];
	const auto input_vec = get_vector_elements(backend, elements);

	rt_value_t acc = init;
	for(const auto& e: input_vec){
		const rt_value_t f_args[] = { acc, e, context };
		const auto result1 = call_function_bc(vm, f, f_args, 3);
		acc = result1;
	}

	const auto result = acc;

if(k_trace && false){
	const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
	QUARK_TRACE(json_to_pretty_string(debug));
}

	return result;
}




/////////////////////////////////////////		PURE -- stable_sort()


//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)
static rt_value_t bc_intrinsic__stable_sort(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 3);
//	QUARK_ASSERT(check_stable_sort_func_type(args[0]._type, args[1]._type, args[2]._type));

	auto& backend = vm._backend;
	const auto& elements = args[0];
	const auto& f = args[1];
	const auto& e_type = peek2(backend.types, elements._type).get_vector_element_type(backend.types);
	const auto& context = args[2];

	const auto input_vec = get_vector_elements(backend, elements);
	std::vector<rt_value_t> mutate_inplace_elements(input_vec.begin(), input_vec.end());

	struct sort_functor_r {
		bool operator() (const rt_value_t &a, const rt_value_t &b) {
			const rt_value_t f_args[] = { a, b, context };
			const auto result1 = call_function_bc(vm, f, f_args, 3);
			QUARK_ASSERT(peek2(backend.types, result1._type).is_bool());
			return result1.get_bool_value();
		}

		interpreter_t& vm;
		rt_value_t context;
		rt_value_t f;
		const value_backend_t& backend;
	};

	const sort_functor_r sort_functor { vm, context, f, backend };
	std::stable_sort(mutate_inplace_elements.begin(), mutate_inplace_elements.end(), sort_functor);

	const auto mutate_inplace_elements2 = immer::vector<rt_value_t>(mutate_inplace_elements.begin(), mutate_inplace_elements.end());
	const auto result = make_vector_value(backend, e_type, mutate_inplace_elements2);

if(k_trace && false){
	const auto debug = value_and_type_to_json(backend.types, rt_to_value(backend, result));
	QUARK_TRACE(json_to_pretty_string(debug));
}

	return result;
}




/////////////////////////////////////////		IMPURE -- MISC




//	print = impure!
//	Records all output to interpreter
static rt_value_t bc_intrinsic__print(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
//	QUARK_ASSERT(args[0]._type.is_string());

	auto& backend = vm._backend;
	if(false) trace_types(backend.types);

	const auto& value = args[0];
	const auto s = to_compact_string2(backend.types, rt_to_value(backend, value));
//	printf("%s", s.c_str());
	vm._handler->on_print(s);

//	const auto lines = split_on_chars(seq_t(s), "\n");
//	vm._print_output = concat(vm._print_output, lines);

	return rt_value_t::make_undefined();
}

static rt_value_t bc_intrinsic__send(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_string());

	const auto& dest_process_id = args[0].get_string_value(backend);
	const auto& message = args[1];

//	QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");

	vm._handler->on_send(dest_process_id, get_rt_value(backend, message), message._type);

	return rt_value_t::make_undefined();
}

static rt_value_t bc_intrinsic__exit(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

//	QUARK_TRACE_SS("send(\"" << process_id << "\"," << json_to_pretty_string(message_json) <<")");

	vm._handler->on_exit();

	return rt_value_t::make_undefined();
}



/////////////////////////////////////////		PURE BITWISE



static rt_value_t bc_intrinsic__bw_not(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto result = ~a;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_and(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto b = args[1].get_int_value();
	const auto result = a & b;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_or(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto b = args[1].get_int_value();
	const auto result = a | b;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_xor(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto b = args[1].get_int_value();
	const auto result = a ^ b;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_shift_left(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto count = args[1].get_int_value();
	const auto result = a << count;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_shift_right(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const uint64_t a = args[0].get_int_value();
	const uint64_t count = args[1].get_int_value();
	const auto result = a >> count;
	return rt_value_t::make_int(result);
}
static rt_value_t bc_intrinsic__bw_shift_right_arithmetic(interpreter_t& vm, const rt_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);

	auto& backend = vm._backend;
	QUARK_ASSERT(peek2(backend.types, args[0]._type).is_int());
	QUARK_ASSERT(peek2(backend.types, args[1]._type).is_int());

	const auto a = args[0].get_int_value();
	const auto count = args[1].get_int_value();
	const auto result = a >> count;
	return rt_value_t::make_int(result);
}






/////////////////////////////////////////		REGISTRY




std::vector<std::pair<intrinsic_signature_t, BC_NATIVE_FUNCTION_PTR>> bc_get_intrinsics_internal(types_t& types){
	QUARK_ASSERT(types.check_invariant());

	std::vector<std::pair<intrinsic_signature_t, BC_NATIVE_FUNCTION_PTR>> log;

	log.push_back({ make_assert_signature(types), bc_intrinsic__assert });
	log.push_back({ make_to_string_signature(types), bc_intrinsic__to_string });
	log.push_back({ make_to_pretty_string_signature(types), bc_intrinsic__to_pretty_string });
	log.push_back({ make_typeof_signature(types), bc_intrinsic__typeof });

	log.push_back({ make_update_signature(types), bc_intrinsic__update });

	//	size(types) is translated to bc_opcode::k_get_size_vector_w_external_elements(types) etc.

	log.push_back({ make_find_signature(types), bc_intrinsic__find });
	log.push_back({ make_exists_signature(types), bc_intrinsic__exists });
	log.push_back({ make_erase_signature(types), bc_intrinsic__erase });
	log.push_back({ make_get_keys_signature(types), bc_intrinsic__get_keys });

	//	push_back(types) is translated to bc_opcode::k_pushback_vector_w_inplace_elements(types) etc.

	log.push_back({ make_subset_signature(types), bc_intrinsic__subset });
	log.push_back({ make_replace_signature(types), bc_intrinsic__replace });


	log.push_back({ make_parse_json_script_signature(types), bc_intrinsic__parse_json_script });
	log.push_back({ make_generate_json_script_signature(types), bc_intrinsic__generate_json_script });
	log.push_back({ make_to_json_signature(types), bc_intrinsic__to_json });

	log.push_back({ make_from_json_signature(types), bc_intrinsic__from_json });

	log.push_back({ make_get_json_type_signature(types), bc_intrinsic__get_json_type });

	log.push_back({ make_map_signature(types), bc_intrinsic__map });
//	log.push_back({ make_map_string_signature(types), bc_intrinsic__map_string });
	log.push_back({ make_filter_signature(types), bc_intrinsic__filter });
	log.push_back({ make_reduce_signature(types), bc_intrinsic__reduce });
	log.push_back({ make_map_dag_signature(types), bc_intrinsic__map_dag });

	log.push_back({ make_stable_sort_signature(types), bc_intrinsic__stable_sort });


	log.push_back({ make_print_signature(types), bc_intrinsic__print });
	log.push_back({ make_send_signature(types), bc_intrinsic__send });
	log.push_back({ make_exit_signature(types), bc_intrinsic__exit });


	log.push_back({ make_bw_not_signature(types), bc_intrinsic__bw_not });
	log.push_back({ make_bw_and_signature(types), bc_intrinsic__bw_and });
	log.push_back({ make_bw_or_signature(types), bc_intrinsic__bw_or });
	log.push_back({ make_bw_xor_signature(types), bc_intrinsic__bw_xor });
	log.push_back({ make_bw_shift_left_signature(types), bc_intrinsic__bw_shift_left });
	log.push_back({ make_bw_shift_right_signature(types), bc_intrinsic__bw_shift_right });
	log.push_back({ make_bw_shift_right_arithmetic_signature(types), bc_intrinsic__bw_shift_right_arithmetic });

	return log;
}

std::map<module_symbol_t, BC_NATIVE_FUNCTION_PTR> bc_get_intrinsics(types_t& types){
	QUARK_ASSERT(types.check_invariant());

	const auto a  = bc_get_intrinsics_internal(types);
	std::map<module_symbol_t, BC_NATIVE_FUNCTION_PTR> result;
	for(const auto& e: a){
		result.insert({ module_symbol_t(e.first.name), e.second });
	}
	return result;
}


}	// floyd
