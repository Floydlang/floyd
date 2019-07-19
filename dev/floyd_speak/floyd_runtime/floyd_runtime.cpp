//
//  floyd_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_runtime.h"


#include "ast_value.h"
#include "floyd_filelib.h"

#include "quark.h"


namespace floyd {


typeid_t make_process_init_type(const typeid_t& t){
	return typeid_t::make_function(t, {}, epure::impure);
}

typeid_t make_process_message_handler_type(const typeid_t& t){
	return typeid_t::make_function(t, { t, typeid_t::make_json() }, epure::impure);
}


//??? remove usage of value_t
value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type){
	QUARK_ASSERT(v.check_invariant());


	struct visitor_t {
		const typeid_t& target_type;
		const json_t& v;

		value_t operator()(const typeid_t::undefined_t& e) const{
			quark::throw_runtime_error("Invalid json schema, found null - unsupported by Floyd.");
		}
		value_t operator()(const typeid_t::any_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}

		value_t operator()(const typeid_t::void_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		value_t operator()(const typeid_t::bool_t& e) const{
			if(v.is_true()){
				return value_t::make_bool(true);
			}
			else if(v.is_false()){
				return value_t::make_bool(false);
			}
			else{
				quark::throw_runtime_error("Invalid json schema, expected true or false.");
			}
		}
		value_t operator()(const typeid_t::int_t& e) const{
			if(v.is_number()){
				return value_t::make_int((int)v.get_number());
			}
			else{
				quark::throw_runtime_error("Invalid json schema, expected number.");
			}
		}
		value_t operator()(const typeid_t::double_t& e) const{
			if(v.is_number()){
				return value_t::make_double((double)v.get_number());
			}
			else{
				quark::throw_runtime_error("Invalid json schema, expected number.");
			}
		}
		value_t operator()(const typeid_t::string_t& e) const{
			if(v.is_string()){
				return value_t::make_string(v.get_string());
			}
			else{
				quark::throw_runtime_error("Invalid json schema, expected string.");
			}
		}

		value_t operator()(const typeid_t::json_type_t& e) const{
			return value_t::make_json(v);
		}
		value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto typeid_value = typeid_from_ast_json(v);
			return value_t::make_typeid_value(typeid_value);
		}

		value_t operator()(const typeid_t::struct_t& e) const{
			if(v.is_object()){
				const auto& struct_def = *e._struct_def;
				std::vector<value_t> members2;
				for(const auto& member: struct_def._members){
					const auto member_value0 = v.get_object_element(member._name);
					const auto member_value1 = unflatten_json_to_specific_type(member_value0, member._type);
					members2.push_back(member_value1);
				}
				const auto result = value_t::make_struct_value(target_type, members2);
				return result;
			}
			else{
				quark::throw_runtime_error("Invalid json schema for Floyd struct, expected JSON object.");
			}
		}
		value_t operator()(const typeid_t::vector_t& e) const{
			if(v.is_array()){
				const auto target_element_type = target_type.get_vector_element_type();
				std::vector<value_t> elements2;
				for(int i = 0 ; i < v.get_array_size() ; i++){
					const auto member_value0 = v.get_array_n(i);
					const auto member_value1 = unflatten_json_to_specific_type(member_value0, target_element_type);
					elements2.push_back(member_value1);
				}
				const auto result = value_t::make_vector_value(target_element_type, elements2);
				return result;
			}
			else{
				quark::throw_runtime_error("Invalid json schema for Floyd vector, expected JSON array.");
			}
		}
		value_t operator()(const typeid_t::dict_t& e) const{
			if(v.is_object()){
				const auto value_type = target_type.get_dict_value_type();
				const auto source_obj = v.get_object();
				std::map<std::string, value_t> obj2;
				for(const auto& member: source_obj){
					const auto member_name = member.first;
					const auto member_value0 = member.second;
					const auto member_value1 = unflatten_json_to_specific_type(member_value0, value_type);
					obj2[member_name] = member_value1;
				}
				const auto result = value_t::make_dict_value(value_type, obj2);
				return result;
			}
			else{
				quark::throw_runtime_error("Invalid json schema, expected JSON object.");
			}
		}
		value_t operator()(const typeid_t::function_t& e) const{
			quark::throw_runtime_error("Invalid json schema, cannot unflatten functions.");
		}
		value_t operator()(const typeid_t::unresolved_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	};
	return std::visit(visitor_t{ target_type, v}, target_type._contents);
}




#define ANY_TYPE typeid_t::make_any()


corecall_signature_t make_assert_signature(){
	return { "assert", 1001, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_bool() }, epure::pure) };
}
corecall_signature_t make_to_string_signature(){
	return { "to_string", 1002, typeid_t::make_function(typeid_t::make_string(), { ANY_TYPE }, epure::pure) };
}

corecall_signature_t make_to_pretty_string_signature(){
	return { "to_pretty_string", 1003, typeid_t::make_function(typeid_t::make_string(), { ANY_TYPE }, epure::pure) };
}
corecall_signature_t make_typeof_signature(){
	return { "typeof", 1004, typeid_t::make_function(typeid_t::make_typeid(), { ANY_TYPE }, epure::pure) };
}


corecall_signature_t make_update_signature(){
	return { "update", 1006, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_size_signature(){
	return { "size", 1007, typeid_t::make_function(typeid_t::make_int(), { ANY_TYPE }, epure::pure) };
}


corecall_signature_t make_find_signature(){
	return { "find", 1008, typeid_t::make_function(typeid_t::make_int(), { ANY_TYPE, ANY_TYPE }, epure::pure) };
}
corecall_signature_t make_exists_signature(){
	return { "exists", 1009, typeid_t::make_function(typeid_t::make_bool(), { ANY_TYPE, ANY_TYPE }, epure::pure) };
}

corecall_signature_t make_erase_signature(){
	return { "erase", 1010, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}
corecall_signature_t make_get_keys_signature(){
	return { "get_keys", 1014, typeid_t::make_function(typeid_t::make_vector(typeid_t::make_string()), { ANY_TYPE }, epure::pure) };
}


corecall_signature_t make_push_back_signature(){
	return { "push_back", 1011, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_subset_signature(){
	return { "subset", 1012, typeid_t::make_function_dyn_return({ ANY_TYPE, typeid_t::make_int(), typeid_t::make_int() }, epure::pure, typeid_t::return_dyn_type::arg0) };
}
corecall_signature_t make_replace_signature(){
	return { "replace", 1013, typeid_t::make_function_dyn_return({ ANY_TYPE, typeid_t::make_int(), typeid_t::make_int(), ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_parse_json_script_signature(){
	return { "parse_json_script", 1017, typeid_t::make_function(typeid_t::make_json(), { typeid_t::make_string() }, epure::pure) };
}

corecall_signature_t make_generate_json_script_signature(){
	return { "generate_json_script", 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json() }, epure::pure) };
}


corecall_signature_t make_to_json_signature(){
	return { "to_json", 1019, typeid_t::make_function(typeid_t::make_json(), { ANY_TYPE }, epure::pure) };
}


corecall_signature_t make_from_json_signature(){
	//	??? Tricky. How to we compute the return type from the input arguments?
	return { "from_json", 1020, typeid_t::make_function_dyn_return({ typeid_t::make_json(), typeid_t::make_typeid() }, epure::pure, typeid_t::return_dyn_type::arg1_typeid_constant_type) };
}


corecall_signature_t make_get_json_type_signature(){
	return { "get_json_type", 1021, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_json() }, epure::pure) };
}




//////////////////////////////////////		HIGHER-ORDER FUNCTIONS



corecall_signature_t make_map_signature(){
	return { "map", 1033, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::vector_of_arg1func_return) };
}
typeid_t harden_map_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto arg2_type = resolved_call_type.get_function_args()[1];
	if(arg2_type.is_function() == false){
		quark::throw_runtime_error("map() arg 2 must be a function.");
	}

	const auto r_type = arg2_type.get_function_return();

	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(r_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_function(r_type, { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());

	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}




corecall_signature_t make_map_string_signature(){
	return {
		"map_string",
		1034,
		typeid_t::make_function(
			typeid_t::make_string(),
			{
				typeid_t::make_string(),
				ANY_TYPE,
				ANY_TYPE
			},
			epure::pure
		)
	};
}

typeid_t harden_map_string_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_string_signature();
	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_string(),
		{
			typeid_t::make_string(),
			typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_string(), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_string_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_string());
	QUARK_ASSERT(f.is_function());

	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == typeid_t::make_string());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}




//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
corecall_signature_t make_map_dag_signature(){
	return { "map_dag", 1037, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::vector_of_arg2func_return) };
}

typeid_t harden_map_dag_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_dag_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map_dag() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto arg3_type = resolved_call_type.get_function_args()[2];
	if(arg3_type.is_function() == false){
		quark::throw_runtime_error("map_dag() arg 3 must be a function.");
	}

	const auto r_type = arg3_type.get_function_return();

	const auto context_type = resolved_call_type.get_function_args()[3];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(r_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_vector(typeid_t::make_int()),
			typeid_t::make_function(r_type, { e_type, typeid_t::make_vector(r_type), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_dag_func_type(const typeid_t& elements, const typeid_t& depends_on, const typeid_t& f, const typeid_t& context){
	//	Check topology.
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(depends_on == typeid_t::make_vector(typeid_t::make_int()));
	QUARK_ASSERT(f.is_function() && f.get_function_args().size () == 3);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == typeid_t::make_vector(f.get_function_return()));
	QUARK_ASSERT(f.get_function_args()[2] == context);
	return true;
}





//	[E] filter([E] elements, func bool (E e, C context) f, C context)
corecall_signature_t make_filter_signature(){
	return { "filter", 1036, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}
typeid_t harden_filter_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_filter_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(e_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_function(typeid_t::make_bool(), { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_filter_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}




corecall_signature_t make_reduce_signature(){
	return { "reduce", 1035, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg1) };
}

typeid_t harden_reduce_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto call_function_arg_type = resolved_call_type.get_function_args();

	const auto elements_arg_type = call_function_arg_type[0];
	if(elements_arg_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = elements_arg_type.get_vector_element_type();

	const auto r_type = call_function_arg_type[1];

	const auto context_type = call_function_arg_type[3];

	const auto expected = typeid_t::make_function(
		r_type,
		{
			typeid_t::make_vector(e_type),
			r_type,
			typeid_t::make_function(r_type, { r_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_reduce_func_type(const typeid_t& elements, const typeid_t& accumulator_init, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.check_invariant());
	QUARK_ASSERT(accumulator_init.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(context.check_invariant());

	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args().size () == 3);

	QUARK_ASSERT(f.get_function_args()[1] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[0] == accumulator_init);

	return true;
}





corecall_signature_t make_stable_sort_signature(){
	return { "stable_sort", 1038, typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) };
}




//////////////////////////////////////		IMPURE FUNCTIONS



corecall_signature_t make_print_signature(){
	return { "print", 1000, typeid_t::make_function(typeid_t::make_void(), { ANY_TYPE }, epure::pure) };
}
corecall_signature_t make_send_signature(){
	return { "send", 1022, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string(), typeid_t::make_json() }, epure::impure) };
}



//////////////////////////////////////		BITWISE



corecall_signature_t make_bw_not_signature(){
	return { "bw_not", 1040, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_and_signature(){
	return { "bw_and", 1041, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_or_signature(){
	return { "bw_or", 1042, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_xor_signature(){
	return { "bw_xor", 1043, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_shift_left_signature(){
	return { "bw_shift_left", 1044, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_shift_right_signature(){
	return { "bw_shift_right", 1045, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}
corecall_signature_t make_bw_shift_right_arithmetic_signature(){
	return { "bw_shift_right_arithmetic", 1046, typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) };
}







std::string get_opcode(const corecall_signature_t& signature){
	return std::string() + "$" + signature.name;
}

static std::vector<corecall_signature_t> get_host_function_records(){
	const std::vector<corecall_signature_t> result = {
		make_assert_signature(),
		make_to_string_signature(),
		make_to_pretty_string_signature(),
		make_typeof_signature(),

		make_update_signature(),
		make_size_signature(),
		make_find_signature(),
		make_exists_signature(),
		make_erase_signature(),
		make_get_keys_signature(),
		make_push_back_signature(),
		make_subset_signature(),
		make_replace_signature(),

		make_parse_json_script_signature(),
		make_generate_json_script_signature(),
		make_to_json_signature(),
		make_from_json_signature(),

		make_get_json_type_signature(),

		make_map_signature(),
		make_map_string_signature(),
		make_filter_signature(),
		make_reduce_signature(),
		make_map_dag_signature(),

		make_stable_sort_signature(),

		make_print_signature(),
		make_send_signature(),

		make_bw_not_signature(),
		make_bw_and_signature(),
		make_bw_or_signature(),
		make_bw_xor_signature(),
		make_bw_shift_left_signature(),
		make_bw_shift_right_signature(),
		make_bw_shift_right_arithmetic_signature()
	};
	return result;
}

std::vector<corecall_signature_t> get_corecall_signatures(){
	static const auto a = get_host_function_records();

	return a;
}


}	//	floyd
