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
	return typeid_t::make_function(t, { t, typeid_t::make_json_value() }, epure::impure);
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
			return value_t::make_json_value(v);
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









#define VOID typeid_t::make_void()
#define DYN typeid_t::make_any()

corecall_signature_t make_assert_signature(){
	return corecall_signature_t{ "assert", 1001, typeid_t::make_function(VOID, { typeid_t::make_bool() }, epure::pure) };
}
corecall_signature_t make_to_string_signature(){
	return corecall_signature_t{ "to_string", 1002, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure) };
}

corecall_signature_t make_to_pretty_string_signature(){
	return corecall_signature_t{ "to_pretty_string", 1003, typeid_t::make_function(typeid_t::make_string(), { DYN }, epure::pure) };
}
corecall_signature_t make_typeof_signature(){
	return corecall_signature_t{ "typeof", 1004, typeid_t::make_function(typeid_t::make_typeid(), { DYN }, epure::pure) };
}


corecall_signature_t make_update_signature(){
	return corecall_signature_t{ "update", 1006, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_size_signature(){
	return corecall_signature_t{ "size", 1007, typeid_t::make_function(typeid_t::make_int(), { DYN }, epure::pure) };
}


corecall_signature_t make_find_signature(){
	return corecall_signature_t{ "find", 1008, typeid_t::make_function(typeid_t::make_int(), { DYN, DYN }, epure::pure) };
}
corecall_signature_t make_exists_signature(){
	return corecall_signature_t{ "exists", 1009, typeid_t::make_function(typeid_t::make_bool(), { DYN, DYN }, epure::pure) };
}

corecall_signature_t make_erase_signature(){
	return corecall_signature_t{ "erase", 1010, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0) };
}


corecall_signature_t make_push_back_signature(){
	return corecall_signature_t{ "push_back", 1011, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_subset_signature(){
	return corecall_signature_t{ "subset", 1012, typeid_t::make_function_dyn_return({ DYN, typeid_t::make_int(), typeid_t::make_int()}, epure::pure, typeid_t::return_dyn_type::arg0) };
}
corecall_signature_t make_replace_signature(){
	return corecall_signature_t{ "replace", 1013, typeid_t::make_function_dyn_return({ DYN, typeid_t::make_int(), typeid_t::make_int(), DYN }, epure::pure, typeid_t::return_dyn_type::arg0) };
}

corecall_signature_t make_script_to_jsonvalue_signature(){
	return corecall_signature_t{ "script_to_jsonvalue", 1017, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_string()}, epure::pure) };
}

corecall_signature_t make_jsonvalue_to_script_signature(){
	return corecall_signature_t{ "jsonvalue_to_script", 1018, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json_value()}, epure::pure) };
}


corecall_signature_t make_value_to_jsonvalue_signature(){
	return corecall_signature_t{ "value_to_jsonvalue", 1019, typeid_t::make_function(typeid_t::make_json_value(), { DYN }, epure::pure) };
}


corecall_signature_t make_jsonvalue_to_value_signature(){
	//	??? Tricky. How to we compute the return type from the input arguments?
	return corecall_signature_t{ "jsonvalue_to_value", 1020, typeid_t::make_function_dyn_return({ typeid_t::make_json_value(), typeid_t::make_typeid() }, epure::pure, typeid_t::return_dyn_type::arg1_typeid_constant_type) };
}


corecall_signature_t make_get_json_type_signature(){
	return corecall_signature_t{ "get_json_type", 1021, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_json_value()}, epure::pure) };
}


corecall_signature_t make_calc_string_sha1_signature(){
	return corecall_signature_t{ "calc_string_sha1", 1031, typeid_t::make_function(make__sha1_t__type(), { typeid_t::make_string() }, epure::pure) };
}


corecall_signature_t make_calc_binary_sha1_signature(){
	return corecall_signature_t{ "calc_binary_sha1", 1032, typeid_t::make_function(make__sha1_t__type(), { make__binary_t__type() }, epure::pure) };
}


corecall_signature_t make_map_signature(){
	return corecall_signature_t{ "map", 1033, typeid_t::make_function_dyn_return({ DYN, DYN}, epure::pure, typeid_t::return_dyn_type::vector_of_arg1func_return) };
}

corecall_signature_t make_map_string_signature(){
	return corecall_signature_t{
		"map_string",
		1034,
		typeid_t::make_function(
			typeid_t::make_string(),
			{
				typeid_t::make_string(),
				typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_string() }, epure::pure)
			},
			epure::pure
		)
	};
}

corecall_signature_t make_filter_signature(){
	return corecall_signature_t{ "filter", 1036, typeid_t::make_function_dyn_return({ DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg0) };
}
corecall_signature_t make_reduce_signature(){
	return corecall_signature_t{ "reduce", 1035, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::arg1) };
}
corecall_signature_t make_supermap_signature(){
	return corecall_signature_t{ "supermap", 1037, typeid_t::make_function_dyn_return({ DYN, DYN, DYN }, epure::pure, typeid_t::return_dyn_type::vector_of_arg2func_return) };
}


corecall_signature_t make_print_signature(){
	return corecall_signature_t{ "print", 1000, typeid_t::make_function(VOID, { DYN }, epure::pure) };
}
corecall_signature_t make_send_signature(){
	return corecall_signature_t{ "send", 1022, typeid_t::make_function(VOID, { typeid_t::make_string(), typeid_t::make_json_value() }, epure::impure) };
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
		make_push_back_signature(),

		make_subset_signature(),
		make_replace_signature(),


		make_script_to_jsonvalue_signature(),
		make_jsonvalue_to_script_signature(),
		make_value_to_jsonvalue_signature(),

		make_jsonvalue_to_value_signature(),

		make_get_json_type_signature(),

		make_calc_string_sha1_signature(),
		make_calc_binary_sha1_signature(),

		make_map_signature(),
		make_map_string_signature(),
		make_filter_signature(),
		make_reduce_signature(),
		make_supermap_signature(),

		make_print_signature(),
		make_send_signature()
	};
	return result;
}


std::vector<corecall_signature_t> get_corecall_signatures(){
	static const auto a = get_host_function_records();

	return a;
}


}	//	floyd
