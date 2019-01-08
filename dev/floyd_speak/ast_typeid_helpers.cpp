//
//  typeid_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-07.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "ast_typeid_helpers.h"

#include "quark.h"
#include "floyd_basics.h"
#include "ast_typeid.h"



namespace floyd {

using std::vector;


ast_json_t struct_definition_to_ast_json(const struct_definition_t& v){
	QUARK_ASSERT(v.check_invariant());

	return ast_json_t{json_t::make_array({
		members_to_json(v._members)
	})};
}


ast_json_t protocol_definition_to_ast_json(const protocol_definition_t& v){
	QUARK_ASSERT(v.check_invariant());

	return ast_json_t{json_t::make_array({
		members_to_json(v._members)
	})};
}



ast_json_t typeid_to_ast_json(const typeid_t& t, json_tags tags){
	QUARK_ASSERT(t.check_invariant());

	const auto b = t.get_base_type();
	const auto basetype_str = base_type_to_string(b);
	const auto basetype_str_tagged = tags == json_tags::k_tag_resolve_state ? (std::string() + tag_resolved_type_char + basetype_str) : basetype_str;

	if(false
		|| b == base_type::k_internal_undefined
		|| b == base_type::k_internal_dynamic

		|| b == base_type::k_void
		|| b == base_type::k_bool
		|| b == base_type::k_int
		|| b == base_type::k_double
		|| b == base_type::k_string
		|| b == base_type::k_json_value
		|| b == base_type::k_typeid
	){
		return ast_json_t{basetype_str_tagged};
	}
	else if(b == base_type::k_struct){
		const auto struct_def = t.get_struct();
		return ast_json_t{json_t::make_array({
			json_t(basetype_str_tagged),
			struct_definition_to_ast_json(struct_def)._value
		})};
	}
	else if(b == base_type::k_protocol){
		const auto protocol_def = t.get_protocol();
		return ast_json_t{json_t::make_array({
			json_t(basetype_str_tagged),
			protocol_definition_to_ast_json(protocol_def)._value
		})};
	}
	else if(b == base_type::k_vector){
		const auto d = t.get_vector_element_type();
		return ast_json_t{json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d, tags)._value
		})};
	}
	else if(b == base_type::k_dict){
		const auto d = t.get_dict_value_type();
		return ast_json_t{json_t::make_array({
			json_t(basetype_str),
			typeid_to_ast_json(d, tags)._value
		})};
	}
	else if(b == base_type::k_function){
		return ast_json_t{json_t::make_array({
			basetype_str,
			typeid_to_ast_json(t.get_function_return(), tags)._value,
			typeids_to_json_array(t.get_function_args())
		})};
	}
	else if(b == base_type::k_internal_unresolved_type_identifier){
		return ast_json_t{ std::string() + std::string(1, tag_unresolved_type_char) + t.get_unresolved_type_identifier() };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


typeid_t typeid_from_ast_json(const ast_json_t& t2){
	QUARK_ASSERT(t2._value.check_invariant());

	const auto t = t2._value;
	if(t.is_string()){
		const auto s0 = t.get_string();
		auto s = std::string(s0.begin() + 1, s0.end());

		if(s0.front() == tag_resolved_type_char){
			if(s == ""){
				return typeid_t::make_undefined();
			}
			else if(s == keyword_t::k_internal_undefined){
				return typeid_t::make_undefined();
			}
			else if(s == keyword_t::k_internal_dynamic){
				return typeid_t::make_internal_dynamic();
			}
			else if(s == keyword_t::k_void){
				return typeid_t::make_void();
			}
			else if(s == keyword_t::k_bool){
				return typeid_t::make_bool();
			}
			else if(s == keyword_t::k_int){
				return typeid_t::make_int();
			}
			else if(s == keyword_t::k_double){
				return typeid_t::make_double();
			}
			else if(s == keyword_t::k_string){
				return typeid_t::make_string();
			}
			else if(s == keyword_t::k_typeid){
				return typeid_t::make_typeid();
			}
			else if(s == keyword_t::k_json_value){
				return typeid_t::make_json_value();
			}
			else{
				throw std::exception();
			}
		}
		else if(s0.front() == tag_unresolved_type_char){
			return typeid_t::make_unresolved_type_identifier(s);
		}
		else{
			throw std::exception();
		}
	}
	else if(t.is_array()){
		const auto a = t.get_array();
		const auto s = a[0].get_string();
/*
		if(s == "typeid"){
			const auto t3 = typeid_from_ast_json(ast_json_t{a[1]});
			return typeid_t::make_typeid(t3);
		}
		else
*/
		if(s == keyword_t::k_struct){
			const auto struct_def_array = a[1].get_array();
			const auto member_array = struct_def_array[0].get_array();

			const vector<member_t> struct_members = members_from_json(member_array);
			return typeid_t::make_struct2(struct_members);
		}
		if(s == keyword_t::k_protocol){
			const auto protocol_def_array = a[1].get_array();
			const auto member_array = protocol_def_array[0].get_array();

			const vector<member_t> protocol_members = members_from_json(member_array);
			return typeid_t::make_protocol(
				std::make_shared<protocol_definition_t>(protocol_definition_t(protocol_members))
			);
		}
		else if(s == "vector"){
			const auto element_type = typeid_from_ast_json(ast_json_t{a[1]});
			return typeid_t::make_vector(element_type);
		}
		else if(s == "dict"){
			const auto value_type = typeid_from_ast_json(ast_json_t{a[1]});
			return typeid_t::make_dict(value_type);
		}
		else if(s == "function"){
			const auto ret_type = typeid_from_ast_json(ast_json_t{a[1]});
			const auto arg_types_array = a[2].get_array();
			const vector<typeid_t> arg_types = typeids_from_json_array(arg_types_array);
			return typeid_t::make_function(ret_type, arg_types);
		}
		else if(s == "**unknown-identifier**"){
			QUARK_ASSERT(false);
			throw std::exception();
		}
		else {
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else{
		throw std::runtime_error("Invalid typeid-json.");
	}
}





std::vector<json_t> typeids_to_json_array(const std::vector<typeid_t>& m){
	vector<json_t> r;
	for(const auto& a: m){
		r.push_back(typeid_to_ast_json(a, json_tags::k_tag_resolve_state)._value);
	}
	return r;
}
std::vector<typeid_t> typeids_from_json_array(const std::vector<json_t>& m){
	vector<typeid_t> r;
	for(const auto& a: m){
		r.push_back(typeid_from_ast_json(ast_json_t{a}));
	}
	return r;
}



json_t members_to_json(const std::vector<member_t>& members){
	std::vector<json_t> r;
	for(const auto& i: members){
		const auto member = make_object({
			{ "type", typeid_to_ast_json(i._type, json_tags::k_tag_resolve_state)._value },
			{ "name", json_t(i._name) }
		});
		r.push_back(json_t(member));
	}
	return r;
}

std::vector<member_t> members_from_json(const json_t& members){
	QUARK_ASSERT(members.check_invariant());

	std::vector<member_t> r;
	for(const auto& i: members.get_array()){
		const auto m = member_t(
			typeid_from_ast_json(ast_json_t{i.get_object_element("type")}),
			i.get_object_element("name").get_string()
		);

		r.push_back(m);
	}
	return r;
}


}

