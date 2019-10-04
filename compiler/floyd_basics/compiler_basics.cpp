//
//  compiler_basics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_basics.h"

#include "text_parser.h"
#include "parser_primitives.h"

namespace floyd {


const location_t k_no_location(std::numeric_limits<std::size_t>::max());





config_t make_default_config(){
	return config_t { vector_backend::hamt, dict_backend::hamt, false } ;
}
 
compiler_settings_t make_default_compiler_settings(){
	return { make_default_config(), eoptimization_level::g_no_optimizations_enable_debugging };
}




std::vector<benchmark_result2_t> unpack_vec_benchmark_result2_t(types_t& type_interner, const value_t& value){
	QUARK_ASSERT(type_interner.check_invariant());
	QUARK_ASSERT(value.get_type() == make_vector(type_interner, make_benchmark_result2_t(type_interner)));

	std::vector<benchmark_result2_t> test_results;
	for(const auto& e: value.get_vector_value()){
		const auto& f = e.get_struct_value();

		const auto& test_id = f->_member_values[0].get_struct_value();
		const auto& result = f->_member_values[1].get_struct_value();

		const auto module = test_id->_member_values[0].get_string_value();
		const auto test = test_id->_member_values[1].get_string_value();

		const auto dur = result->_member_values[0].get_int_value();
		const auto more = result->_member_values[1].get_json();

		const auto r = benchmark_result2_t { benchmark_id_t { module, test }, benchmark_result_t { dur, more } };
		test_results.push_back(r);
	}
	return test_results;
}



bool is_floyd_literal(const type_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(type.is_void() || type.is_int() || type.is_double() || type.is_string() || type.is_bool() || type.is_typeid() || type.is_any() || type.is_json() || type.is_function()){
		return true;
	}
	else{
		return false;
	}
}

bool is_preinitliteral(const type_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(type.is_int() || type.is_double() || type.is_bool() || type.is_typeid() || type.is_any() || type.is_function()){
		return true;
	}
	else{
		return false;
	}
}


////////////////////////////////////////		main() init() and message handler




type_t make_process_init_type(types_t& interner, const type_t& t){
	return make_function3(interner, t, {}, epure::impure, return_dyn_type::none);
}

type_t make_process_message_handler_type(types_t& interner, const type_t& t){
	return make_function3(interner, t, { t, type_t::make_json() }, epure::impure, return_dyn_type::none);
}



//////////////////////////////////////		INTRINSICS




#define ANY_TYPE type_t::make_any()

static intrinsic_signature_t make_intrinsic(
	const std::string& name,
	floyd::type_t function_type
){
	return { name, function_type };
}


intrinsic_signature_t make_assert_signature(types_t& interner){
	return make_intrinsic("assert", make_function(interner, type_t::make_void(), { type_t::make_bool() }, epure::pure) );
}
intrinsic_signature_t make_to_string_signature(types_t& interner){
	return make_intrinsic("to_string", make_function(interner, type_t::make_string(), { ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_to_pretty_string_signature(types_t& interner){
	return make_intrinsic("to_pretty_string", make_function(interner, type_t::make_string(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_typeof_signature(types_t& interner){
	return make_intrinsic("typeof", make_function(interner, type_t::make_typeid(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_update_signature(types_t& interner){
	return make_intrinsic("update", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_size_signature(types_t& interner){
	return make_intrinsic("size", make_function(interner, type_t::make_int(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_find_signature(types_t& interner){
	return make_intrinsic("find", make_function(interner, type_t::make_int(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_exists_signature(types_t& interner){
	return make_intrinsic("exists", make_function(interner, type_t::make_bool(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_erase_signature(types_t& interner){
	return make_intrinsic("erase", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}
intrinsic_signature_t make_get_keys_signature(types_t& interner){
	return make_intrinsic("get_keys", make_function(interner, make_vector(interner, type_t::make_string()), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_push_back_signature(types_t& interner){
	return make_intrinsic("push_back", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_subset_signature(types_t& interner){
	return make_intrinsic("subset", make_function_dyn_return(interner, { ANY_TYPE, type_t::make_int(), type_t::make_int() }, epure::pure, return_dyn_type::arg0) );
}
intrinsic_signature_t make_replace_signature(types_t& interner){
	return make_intrinsic("replace", make_function_dyn_return(interner, { ANY_TYPE, type_t::make_int(), type_t::make_int(), ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

intrinsic_signature_t make_parse_json_script_signature(types_t& interner){
	return make_intrinsic("parse_json_script", make_function(interner, type_t::make_json(), { type_t::make_string() }, epure::pure) );
}

intrinsic_signature_t make_generate_json_script_signature(types_t& interner){
	return make_intrinsic("generate_json_script", make_function(interner, type_t::make_string(), {type_t::make_json() }, epure::pure) );
}


intrinsic_signature_t make_to_json_signature(types_t& interner){
	return make_intrinsic("to_json", make_function(interner, type_t::make_json(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_from_json_signature(types_t& interner){
	//	??? Tricky. How to we compute the return type from the input arguments?
	return make_intrinsic("from_json", make_function_dyn_return(interner, { type_t::make_json(), type_t::make_typeid() }, epure::pure, return_dyn_type::arg1_typeid_constant_type) );
}


intrinsic_signature_t make_get_json_type_signature(types_t& interner){
	return make_intrinsic("get_json_type", make_function(interner, type_t::make_int(), { type_t::make_json() }, epure::pure) );
}




//////////////////////////////////////		HIGHER-ORDER INTRINSICS



intrinsic_signature_t make_map_signature(types_t& interner){
	return make_intrinsic("map", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::vector_of_arg1func_return) );
}
type_t harden_map_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_signature(interner);

	const auto arg1_type = resolved_call_type.get_function_args(interner)[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type(interner);

	const auto arg2_type = resolved_call_type.get_function_args(interner)[1];
	if(arg2_type.is_function() == false){
		quark::throw_runtime_error("map() arg 2 must be a function.");
	}

	const auto r_type = arg2_type.get_function_return(interner);

	const auto context_type = resolved_call_type.get_function_args(interner)[2];

	const auto expected = make_function(interner,
		make_vector(interner, r_type),
		{
			make_vector(interner, e_type),
			make_function(interner, r_type, { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_func_type(types_t& interner, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());

	QUARK_ASSERT(f.get_function_args(interner).size() == 2);
	QUARK_ASSERT(f.get_function_args(interner)[0] == elements.get_vector_element_type(interner));
	QUARK_ASSERT(f.get_function_args(interner)[1] == context);
	return true;
}



//	string map_string(string s, func int(int e, C context) f, C context)
intrinsic_signature_t make_map_string_signature(types_t& interner){
	return make_intrinsic(
		"map_string",
		make_function(
			interner,
			type_t::make_string(),
			{
				type_t::make_string(),
				ANY_TYPE,
				ANY_TYPE
			},
			epure::pure
		)
	);
}

type_t harden_map_string_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_string_signature(interner);
	const auto context_type = resolved_call_type.get_function_args(interner)[2];

	const auto expected = make_function(
		interner,
		type_t::make_string(),
		{
			type_t::make_string(),
			make_function(interner, type_t::make_int(), { type_t::make_int(), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_string_func_type(types_t& interner, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(elements.is_string());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args(interner).size() == 2);
	QUARK_ASSERT(f.get_function_args(interner)[0] == type_t::make_int());
	QUARK_ASSERT(f.get_function_args(interner)[1] == context);
	return true;
}




//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
intrinsic_signature_t make_map_dag_signature(types_t& interner){
	return make_intrinsic("map_dag", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::vector_of_arg2func_return) );
}

type_t harden_map_dag_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_dag_signature(interner);

	const auto arg1_type = resolved_call_type.get_function_args(interner)[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map_dag() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type(interner);

	const auto arg3_type = resolved_call_type.get_function_args(interner)[2];
	if(arg3_type.is_function() == false){
		quark::throw_runtime_error("map_dag() arg 3 must be a function.");
	}

	const auto r_type = arg3_type.get_function_return(interner);

	const auto context_type = resolved_call_type.get_function_args(interner)[3];

	const auto expected = make_function(
		interner,
		make_vector(interner, r_type),
		{
			make_vector(interner, e_type),
			make_vector(interner, type_t::make_int()),
			make_function(interner, r_type, { e_type, make_vector(interner, r_type), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_dag_func_type(types_t& interner, const type_t& elements, const type_t& depends_on, const type_t& f, const type_t& context){
	//	Check topology.
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(depends_on == make_vector(interner, type_t::make_int()));
	QUARK_ASSERT(f.is_function() && f.get_function_args(interner).size () == 3);
	QUARK_ASSERT(f.get_function_args(interner)[0] == elements.get_vector_element_type(interner));
	QUARK_ASSERT(f.get_function_args(interner)[1] == make_vector(interner, f.get_function_return(interner)));
	QUARK_ASSERT(f.get_function_args(interner)[2] == context);
	return true;
}





//	[E] filter([E] elements, func bool (E e, C context) f, C context)
intrinsic_signature_t make_filter_signature(types_t& interner){
	return make_intrinsic("filter", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}
type_t harden_filter_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_filter_signature(interner);

	const auto arg1_type = resolved_call_type.get_function_args(interner)[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type(interner);

	const auto context_type = resolved_call_type.get_function_args(interner)[2];

	const auto expected = make_function(
		interner,
		make_vector(interner, e_type),
		{
			make_vector(interner, e_type),
			make_function(interner, type_t::make_bool(), { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_filter_func_type(types_t& interner, const type_t& elements, const type_t& f, const type_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args(interner).size() == 2);
	QUARK_ASSERT(f.get_function_args(interner)[0] == elements.get_vector_element_type(interner));
	QUARK_ASSERT(f.get_function_args(interner)[1] == context);
	return true;
}




intrinsic_signature_t make_reduce_signature(types_t& interner){
	return make_intrinsic("reduce", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg1) );
}

type_t harden_reduce_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto call_function_arg_type = resolved_call_type.get_function_args(interner);

	const auto elements_arg_type = call_function_arg_type[0];
	if(elements_arg_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = elements_arg_type.get_vector_element_type(interner);

	const auto r_type = call_function_arg_type[1];

	const auto context_type = call_function_arg_type[3];

	const auto expected = make_function(
		interner,
		r_type,
		{
			make_vector(interner, e_type),
			r_type,
			make_function(interner, r_type, { r_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_reduce_func_type(types_t& interner, const type_t& elements, const type_t& accumulator_init, const type_t& f, const type_t& context){
	QUARK_ASSERT(elements.check_invariant());
	QUARK_ASSERT(accumulator_init.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(context.check_invariant());

	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args(interner).size () == 3);

	QUARK_ASSERT(f.get_function_args(interner)[1] == elements.get_vector_element_type(interner));
	QUARK_ASSERT(f.get_function_args(interner)[0] == accumulator_init);

	return true;
}




//	[T] stable_sort([T] elements, func bool (T left, T right, C context) less, C context)
intrinsic_signature_t make_stable_sort_signature(types_t& interner){
	return make_intrinsic("stable_sort", make_function_dyn_return(interner, { ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, return_dyn_type::arg0) );
}

type_t harden_stable_sort_func_type(types_t& interner, const type_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_stable_sort_signature(interner);

	const auto arg1_type = resolved_call_type.get_function_args(interner)[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("stable_sort() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type(interner);

	const auto arg2_type = resolved_call_type.get_function_args(interner)[1];
	if(arg2_type.is_function() == false){
		quark::throw_runtime_error("stable_sort() arg 2 must be a function.");
	}

	const auto context_type = resolved_call_type.get_function_args(interner)[2];

	const auto expected = make_function(
		interner,
		arg1_type,
		{
			arg1_type,
			make_function(interner, type_t::make_bool(), { e_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_stable_sort_func_type(types_t& interner, const type_t& elements, const type_t& less, const type_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(less.is_function());
	QUARK_ASSERT(less.get_function_args(interner).size() == 3);

	const auto& e_type = elements.get_vector_element_type(interner);
	QUARK_ASSERT(less.get_function_return(interner) == type_t::make_bool());
	QUARK_ASSERT(less.get_function_args(interner)[0] == e_type);
	QUARK_ASSERT(less.get_function_args(interner)[1] == e_type);
	QUARK_ASSERT(less.get_function_args(interner)[2] == context);

	return true;
}





//////////////////////////////////////		IMPURE FUNCTIONS



intrinsic_signature_t make_print_signature(types_t& interner){
	return make_intrinsic("print", make_function(interner, type_t::make_void(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_send_signature(types_t& interner){
	return make_intrinsic("send", make_function(interner, type_t::make_void(), { type_t::make_string(), type_t::make_json() }, epure::impure) );
}



//////////////////////////////////////		BITWISE



intrinsic_signature_t make_bw_not_signature(types_t& interner){
	return make_intrinsic("bw_not", make_function(interner, type_t::make_int(), { type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_and_signature(types_t& interner){
	return make_intrinsic("bw_and", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_or_signature(types_t& interner){
	return make_intrinsic("bw_or", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_xor_signature(types_t& interner){
	return make_intrinsic("bw_xor", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_left_signature(types_t& interner){
	return make_intrinsic("bw_shift_left", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_signature(types_t& interner){
	return make_intrinsic("bw_shift_right", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_arithmetic_signature(types_t& interner){
	return make_intrinsic("bw_shift_right_arithmetic", make_function(interner, type_t::make_int(), { type_t::make_int(), type_t::make_int() }, epure::pure) );
}







std::string get_intrinsic_opcode(const intrinsic_signature_t& signature){
	return std::string() + "$" + signature.name;
}

intrinsic_signatures_t make_intrinsic_signatures(types_t& interner){
	intrinsic_signatures_t result;

	result.assert = make_assert_signature(interner);
	result.to_string = make_to_string_signature(interner);
	result.to_pretty_string = make_to_pretty_string_signature(interner);
	result.typeof_sign = make_typeof_signature(interner);

	result.update = make_update_signature(interner);
	result.size = make_size_signature(interner);
	result.find = make_find_signature(interner);
	result.exists = make_exists_signature(interner);
	result.erase = make_erase_signature(interner);
	result.get_keys = make_get_keys_signature(interner);
	result.push_back = make_push_back_signature(interner);
	result.subset = make_subset_signature(interner);
	result.replace = make_replace_signature(interner);

	result.parse_json_script = make_parse_json_script_signature(interner);
	result.generate_json_script = make_generate_json_script_signature(interner);
	result.to_json = make_to_json_signature(interner);
	result.from_json = make_from_json_signature(interner);

	result.get_json_type = make_get_json_type_signature(interner);


	result.map = make_map_signature(interner);
//	result.xxx = make_map_string_signature(interner);
	result.filter = make_filter_signature(interner);
	result.reduce = make_reduce_signature(interner);
	result.map_dag = make_map_dag_signature(interner);

	result.stable_sort = make_stable_sort_signature(interner);

	result.print = make_print_signature(interner);
	result.send = make_send_signature(interner);

	result.bw_not = make_bw_not_signature(interner);
	result.bw_and = make_bw_and_signature(interner);
	result.bw_or = make_bw_or_signature(interner);
	result.bw_xor = make_bw_xor_signature(interner);
	result.bw_shift_left = make_bw_shift_left_signature(interner);
	result.bw_shift_right = make_bw_shift_right_signature(interner);
	result.bw_shift_right_arithmetic = make_bw_shift_right_arithmetic_signature(interner);


	const std::vector<intrinsic_signature_t> vec = {
		make_assert_signature(interner),
		make_to_string_signature(interner),
		make_to_pretty_string_signature(interner),
		make_typeof_signature(interner),

		make_update_signature(interner),
		make_size_signature(interner),
		make_find_signature(interner),
		make_exists_signature(interner),
		make_erase_signature(interner),
		make_get_keys_signature(interner),
		make_push_back_signature(interner),
		make_subset_signature(interner),
		make_replace_signature(interner),

		make_parse_json_script_signature(interner),
		make_generate_json_script_signature(interner),
		make_to_json_signature(interner),
		make_from_json_signature(interner),

		make_get_json_type_signature(interner),

		make_map_signature(interner),
//		make_map_string_signature(interner),
		make_filter_signature(interner),
		make_reduce_signature(interner),
		make_map_dag_signature(interner),

		make_stable_sort_signature(interner),

		make_print_signature(interner),
		make_send_signature(interner),

		make_bw_not_signature(interner),
		make_bw_and_signature(interner),
		make_bw_or_signature(interner),
		make_bw_xor_signature(interner),
		make_bw_shift_left_signature(interner),
		make_bw_shift_right_signature(interner),
		make_bw_shift_right_arithmetic_signature(interner)
	};
	result.vec = vec;
	return result;
}





//////////////////////////////////////////////////		location_t





std::pair<json_t, location_t> unpack_loc(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;

		const auto elements = s.get_array();
		const std::vector<json_t> elements2 = { elements.begin() + 1, elements.end() };
		const auto statement = json_t::make_array(elements2);

		return { statement, source_offset };
	}
	else{
		return { s, k_no_location };
	}
}

location_t unpack_loc2(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;
		return source_offset;
	}
	else{
		return k_no_location;
	}
}



//////////////////////////////////////////////////		MACROS



void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}


//	Return one entry per source line PLUS one extra end-marker. int tells byte offset of files that maps to this line-start.
//	Never returns empty vector, at least 2 elements.
std::vector<int> make_location_lookup(const std::string& source){
	if(source.empty()){
		return { 0 };
	}
	else{
		std::vector<int> result;
		int char_index = 0;
		result.push_back(char_index);
		for(const auto& ch: source){
			char_index++;

			if(ch == '\n' || ch == '\r'){
				result.push_back(char_index);
			}
		}
		const auto back_char = source.back();
		if(back_char == '\r' || back_char == '\n'){
			return result;
		}
		else{
			int end_index = static_cast<int>(source.size());
			result.push_back(end_index);
			return result;
		}
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 3 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa\nbbb\n");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 4, 8 }));
}

const std::string cleanup_line_snippet(const std::string& s){
	const auto line1 = skip(seq_t(s), "\t ");
	const auto split = split_on_chars(line1, "\n\r");
	const auto line = split.size() > 0 ? split.front() : "";
	return line;
}
QUARK_TEST("", "cleanup_line_snippet()", "", ""){
	ut_verify(QUARK_POS, cleanup_line_snippet(" \tabc\n\a"), "abc");
}

location2_t make_loc2(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc, int line_index){
	const auto start = lookup[line_index];
	const auto end = lookup[line_index + 1];
	const auto line = cleanup_line_snippet(program.substr(start, end - start));
	const auto column = loc.offset - start;
	return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
}


location2_t find_loc_info(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc){
	QUARK_ASSERT(lookup.size() >= 2);
	QUARK_ASSERT(loc.offset <= lookup.back());

	int last_line_offset = lookup.back();
	QUARK_ASSERT(last_line_offset >= loc.offset);

	//	 EOF? Use program's last non-blank line.
	if(last_line_offset == loc.offset){
		auto line_index = static_cast<int>(lookup.size()) - 2;
		auto loc2 = make_loc2(program, lookup, file, loc, line_index);
		while(line_index >= 0 && loc2.line ==""){
			line_index--;
			loc2 = make_loc2(program, lookup, file, loc, line_index);
		}
		return loc2;
	}
	else{
		int line_index = 0;
		while(lookup[line_index + 1] <= loc.offset){
			line_index++;
			QUARK_ASSERT(line_index < lookup.size());
		}
		return make_loc2(program, lookup, file, loc, line_index);
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const std::vector<int> lookup = { 0, 1, 2, 18, 19, 21 };
	const std::string program = R"(

			{ let a = 10

		)";
	const auto r = find_loc_info(program, lookup, "path.txt", location_t(21));
}

location2_t find_source_line(const compilation_unit_t& cu, const location_t& loc){
	const auto program_lookup = make_location_lookup(cu.program_text);

	if(cu.prefix_source != ""){
		const auto corelib_lookup = make_location_lookup(cu.prefix_source);
		const auto corelib_end_offset = corelib_lookup.back();
		if(loc.offset < corelib_end_offset){
			return find_loc_info(cu.prefix_source, corelib_lookup, "corelib", loc);
		}
		else{
			const auto program_loc = location_t(loc.offset - corelib_end_offset);
			const auto loc2 = find_loc_info(cu.program_text, program_lookup, cu.source_file_path, program_loc);
			const auto result = location2_t(
				loc2.source_file_path,
				loc2.line_number,
				loc2.column,
				loc2.start,
				loc2.end,
				loc2.line,
				location_t(loc2.loc.offset + corelib_end_offset)
			);
			return result;
		}
	}
	else{
		return find_loc_info(cu.program_text, program_lookup, cu.source_file_path, loc);
	}
}



/*	const auto line_numbers2 = mapf<int>(
		statements_pos.line_numbers,
		[&](const auto& e){
			return e - pre_line_count;
		}
	);
*/



std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e){
	const auto loc2 = find_source_line(cu, e.location);
	const auto what1 = std::string(e.what());

	std::stringstream what2;
	const auto line_snippet = parser::reverse(parser::skip_whitespace(parser::reverse(loc2.line)));
	what2 << what1 << " Line: " << std::to_string(loc2.line_number + 1) << " \"" << line_snippet << "\"";
	if(loc2.source_file_path.empty() == false){
		what2 << " file: " << loc2.source_file_path;
	}

	return { loc2, what2.str() };
}




void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
}

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify(context, result.first, expected.first);
		ut_verify(context, result.second.str(), expected.second.str());
	}
}




////////////////////////////////	make_ast_node()



json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

QUARK_TEST("", "make_ast_node()", "", ""){
	const auto r = make_ast_node(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify(QUARK_POS, r, json_t::make_array({ 1234.0, "def-struct" }));
}




////////////////////////////////		ENCODE / DECODE LINK NAMES



static const std::string k_floyd_func_link_prefix = "floydf_";


link_name_t encode_floyd_func_link_name(const std::string& name){
	return link_name_t { k_floyd_func_link_prefix + name };
}
std::string decode_floyd_func_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.s.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}



static const std::string k_runtime_func_link_prefix = "floyd_runtime_";

link_name_t encode_runtime_func_link_name(const std::string& name){
	return link_name_t { k_runtime_func_link_prefix + name };
}

std::string decode_runtime_func_link_name(const link_name_t& name){
	const auto left = name.s.substr(0, k_runtime_func_link_prefix.size());
	const auto right = name.s.substr(k_runtime_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_runtime_func_link_prefix);
	return right;
}



static const std::string k_intrinsic_link_prefix = "floyd_intrinsic_";

link_name_t encode_intrinsic_link_name(const std::string& name){
	return link_name_t { k_intrinsic_link_prefix + name };
}
std::string decode_intrinsic_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_intrinsic_link_prefix.size());
	const auto right = name.s.substr(k_intrinsic_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_intrinsic_link_prefix);
	return right;
}


}	// floyd

