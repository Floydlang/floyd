//
//  floyd_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-17.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_runtime.h"


#include "ast_value.h"

#include "quark.h"


namespace floyd {



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
	};
	return std::visit(visitor_t{ target_type, v}, target_type._contents);
}





}	//	floyd
