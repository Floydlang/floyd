//
//  type_interner.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "type_interner.h"


#include "utils.h"
#include "ast_helpers.h"
#include "json_support.h"
#include "format_table.h"
#include "compiler_basics.h"
#include "parser_primitives.h"

#include <limits.h>

namespace floyd {



static itype_t lookup_itype_from_index_it(const type_interner_t& interner, size_t type_index){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type_index >= 0 && type_index < interner.interned2.size());

	return lookup_itype_from_index(interner, static_cast<type_lookup_index_t>(type_index));
}



std::vector<itype_t> get_member_types(const std::vector<member_itype_t>& m){
	std::vector<itype_t> r;
	for(const auto& a: m){
		r.push_back(a._type);
	}
	return r;
}



json_t members_to_json(const type_interner_t& interner, const std::vector<member_itype_t>& members){
	std::vector<json_t> r;
	for(const auto& i: members){
		const auto member = make_object({
			{ "type", itype_to_json(interner, i._type) },
			{ "name", json_t(i._name) }
		});
		r.push_back(json_t(member));
	}
	return r;
}

std::vector<member_itype_t> members_from_json(type_interner_t& interner, const json_t& members){
	QUARK_ASSERT(members.check_invariant());

	std::vector<member_itype_t> r;
	for(const auto& i: members.get_array()){
		const auto m = member_itype_t {
			itype_from_json(interner, i.get_object_element("type")),
			i.get_object_element("name").get_string()
		};

		r.push_back(m);
	}
	return r;
}


itype_variant_t get_itype_variant(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_undefined()){
		return undefined_t {};
	}
	else if(type.is_any()){
		return any_t {};
	}
	else if(type.is_void()){
		return void_t {};
	}


	else if(type.is_bool()){
		return bool_t {};
	}
	else if(type.is_int()){
		return int_t {};
	}
	else if(type.is_double()){
		return double_t {};
	}
	else if(type.is_string()){
		return string_t {};
	}
	else if(type.is_json()){
		return json_type_t {};
	}
	else if(type.is_typeid()){
		return typeid_type_t {};
	}

	else if(type.is_struct()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return struct_t { info.struct_def };
	}
	else if(type.is_vector()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return vector_t { info.child_types };
	}
	else if(type.is_dict()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return dict_t { info.child_types };
	}
	else if(type.is_function()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return function_t { info.child_types };
	}
	else if(type.is_symbol_ref()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return symbol_ref_t { info.identifier_str };
	}
	else if(type.is_named_type()){
		const auto& info = lookup_typeinfo_from_itype(interner, type);
		return named_type_t { info.child_types[0] };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}




//////////////////////////////////////////////////		itype_t



itype_t itype_t::make_struct2(type_interner_t& interner, const std::vector<member_itype_t>& members){
	return itype_t::make_struct(interner, struct_def_itype_t{ members });
}


itype_t itype_t::get_vector_element_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_vector());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.child_types[0];
}
itype_t itype_t::get_dict_value_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_dict());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.child_types[0];
}




struct_def_itype_t itype_t::get_struct(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(is_struct());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.struct_def;
}



itype_t itype_t::get_function_return(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.child_types[0];
}

std::vector<itype_t> itype_t::get_function_args(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return std::vector<itype_t>(info.child_types.begin() + 1, info.child_types.end());
}

return_dyn_type itype_t::get_function_dyn_return_type(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.func_return_dyn_type;
}

epure itype_t::get_function_pure(const type_interner_t& interner) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_function());
	QUARK_ASSERT(interner.check_invariant());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.func_pure;
}



std::string itype_t::get_symbol_ref(const type_interner_t& interner) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_symbol_ref());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.identifier_str;
}


type_tag_t itype_t::get_named_type(const type_interner_t& interner) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(is_named_type());

	const auto& info = lookup_typeinfo_from_itype(interner, *this);
	return info.optional_tag;
}








int find_struct_member_index(const struct_def_itype_t& def, const std::string& name){
	int index = 0;
	while(index < def._members.size() && def._members[index]._name != name){
		index++;
	}
	if(index == def._members.size()){
		return -1;
	}
	else{
		return index;
	}
}





//////////////////////////////////////////////////		type_interner_t



static type_node_t make_entry(const base_type& bt){
	return type_node_t{
		make_empty_type_tag(),
		bt,
		std::vector<itype_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
}

type_interner_t::type_interner_t(){
	//	Order is designed to match up the interned2[] with base_type indexes.
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_any));
	interned2.push_back(make_entry(base_type::k_void));


	interned2.push_back(make_entry(base_type::k_bool));
	interned2.push_back(make_entry(base_type::k_int));
	interned2.push_back(make_entry(base_type::k_double));
	interned2.push_back(make_entry(base_type::k_string));
	interned2.push_back(make_entry(base_type::k_json));

	interned2.push_back(make_entry(base_type::k_typeid));

	//	These are complex types and are undefined. We need them to take up space in the interned2-vector.
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));
	interned2.push_back(make_entry(base_type::k_undefined));

	interned2.push_back(make_entry(base_type::k_symbol_ref));
	interned2.push_back(make_entry(base_type::k_undefined));

	QUARK_ASSERT(check_invariant());
}

bool type_interner_t::check_invariant() const {
	QUARK_ASSERT(interned2.size() < INT_MAX);

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_undefined] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_any] == make_entry(base_type::k_any));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_void] == make_entry(base_type::k_void));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_bool] == make_entry(base_type::k_bool));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_int] == make_entry(base_type::k_int));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_double] == make_entry(base_type::k_double));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_string] == make_entry(base_type::k_string));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_json] == make_entry(base_type::k_json));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_typeid] == make_entry(base_type::k_typeid));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_struct] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_vector] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_dict] == make_entry(base_type::k_undefined));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_function] == make_entry(base_type::k_undefined));

	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_symbol_ref] == make_entry(base_type::k_symbol_ref));
	QUARK_ASSERT(interned2[(type_lookup_index_t)base_type::k_named_type] == make_entry(base_type::k_undefined));


	//??? Add other common combinations, like vectors with each atomic type, dict with each atomic
	//	type. This allows us to hardcoded their itype indexes.
	return true;
}



static itype_t lookup_node(const type_interner_t& interner, const type_node_t& node){
	QUARK_ASSERT(interner.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e == node; }
	);

	if(it != interner.interned2.end()){
		return lookup_itype_from_index_it(interner, it - interner.interned2.begin());
	}
	else{
		throw std::exception();
//		return itype_t::make_undefined();
	}
}



static itype_t intern_node(type_interner_t& interner, const type_node_t& node){
	QUARK_ASSERT(interner.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e == node; }
	);

	if(it != interner.interned2.end()){
		return lookup_itype_from_index_it(interner, it - interner.interned2.begin());
	}

	//	New type, store it.
	else{

		//	All child type are guaranteed to have itypes already since those are specified using itypes_t:s.
		interner.interned2.push_back(node);
		return lookup_itype_from_index_it(interner, interner.interned2.size() - 1);

/*
		if(type.bt == base_type::k_undefined){
			return itype_t::make_undefined();
		}
		else if(type.bt == base_type::k_any){
			return itype_t::make_any();
		}

		else if(type.bt == base_type::k_void){
			return itype_t::make_void();
		}

		else if(type.bt == base_type::k_bool){
			return itype_t::make_bool();
		}



		else if(type.bt == base_type::k_int){
			return itype_t::make_int();
		}
		else if(type.bt == base_type::k_double){
			return itype_t::make_double();
		}
		else if(type.bt == base_type::k_string){
			return itype_t::make_string();
		}
		else if(type.bt == base_type::k_json_type){
			return itype_t::make_json();
		}
		else if(type.bt == base_type::k_typeid_t){
			return itype_t::make_typeid();
		}

		else if(type.bt == base_type::k_struct){
			const auto& struct_def = type.get_struct();
			std::vector<member_t> members2;
			for(const auto& m: struct_def._members){
				members2.push_back(
					member_t(
						lookup_typeid_from_itype(interner, intern_anonymous_type(interner, m._type)),
						m._name)
				);
			}
			const auto type2 = typeid_t::make_struct2(members2);
			interner.interned2.push_back(type_node_t{ tag, type2 });

			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_struct(lookup_index);
		}
		else if(type.bt == base_type::k_vector){
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type2 = lookup_typeid_from_itype(interner, intern_anonymous_type(interner, e._parts[0]));
			const auto type2 = typeid_t::make_vector(element_type2);

			interner.interned2.push_back(type_node_t{ tag, type2 });

			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_vector(lookup_index, element_type2.get_base_type());
		}

		else if(type.bt == base_type::k_dict){
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type2 = lookup_typeid_from_itype(interner,intern_anonymous_type(interner, e._parts[0]));
			const auto type2 = typeid_t::make_dict(element_type2);

			interner.interned2.push_back(type_node_t{ tag, type2 });

			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_dict(lookup_index, element_type2.get_base_type());
		}

		else if(type.bt == base_type::k_function){
			const auto ret = type.get_function_return();
			const auto args = type.get_function_args();
			const auto pure = type.get_function_pure();
			const auto dyn_return_type = type.get_function_dyn_return_type();

			const auto ret2 = lookup_typeid_from_itype(interner, intern_anonymous_type(interner, ret));
			std::vector<typeid_t> args2;
			for(const auto& m: args){
				args2.push_back(lookup_typeid_from_itype(interner, intern_anonymous_type(interner, m)));
			}
			const auto type2 = typeid_t::make_function3(ret2, args2, pure, dyn_return_type);

			interner.interned2.push_back(type_node_t{ tag, type2 });

			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_function(lookup_index);
		}

		else if(type.bt == base_type::k_idenfitier){
			const auto identifier = type.get_identifier();
			QUARK_ASSERT(is_type_tag(identifier));

			const auto type2 = type;
			interner.interned2.push_back(type_node_t{ tag, type2 });

			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_identifier(lookup_index);
		}
*/

	}
}


itype_t new_tagged_type(type_interner_t& interner, const type_tag_t& tag){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());

	const auto itype = itype_t::make_undefined();
	const auto result = new_tagged_type(interner, tag, itype);
	return result;
}

itype_t new_tagged_type(type_interner_t& interner, const type_tag_t& tag, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(false) trace_type_interner(interner);

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e.optional_tag == tag; }
	);
	if(it != interner.interned2.end()){
		throw std::exception();
	}

	const auto node = type_node_t{
		tag,
		base_type::k_named_type,
		{ type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};

	QUARK_ASSERT(node.child_types.size() == 1);

	//	Can't use intern_node() since we have a tag.
	interner.interned2.push_back(node);
	return lookup_itype_from_index_it(interner, interner.interned2.size() - 1);
}

itype_t update_tagged_type(type_interner_t& interner, const itype_t& named, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(named.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& node = lookup_typeinfo_from_itype(interner, named);
	QUARK_ASSERT(node.bt == base_type::k_named_type);
	QUARK_ASSERT(node.child_types.size() == 1);

	node.child_types = { type };

	//	Returns a new itype for the named tag, so it contains the updated byte_type info.
	return lookup_itype_from_index(interner, named.get_lookup_index());
}

itype_t get_tagged_type2(const type_interner_t& interner, const type_tag_t& tag){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(tag.check_invariant());

	const auto it = std::find_if(
		interner.interned2.begin(),
		interner.interned2.end(),
		[&](const auto& e){ return e.optional_tag == tag; }
	);
	if(it == interner.interned2.end()){
		throw std::exception();
	}
	QUARK_ASSERT(it->child_types.size() == 1);
	return lookup_itype_from_index_it(interner, it - interner.interned2.begin());
}

/*
static typeid_t to_typeid(const type_interner_t& interner, const itype_t& type, int peek_depth){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto peeked = peek_depth > 0 ? peek(interner, type) : type;
	auto& node = lookup_typeinfo_from_itype(interner, peeked);

	const auto peek_depth2 = peek_depth - 1;

	if(node.bt == base_type::k_undefined){
		return typeid_t::make_undefined();
	}
	else if(node.bt == base_type::k_any){
		return typeid_t::make_any();
	}

	else if(node.bt == base_type::k_void){
		return typeid_t::make_void();
	}


	else if(node.bt == base_type::k_bool){
		return typeid_t::make_bool();
	}
	else if(node.bt == base_type::k_int){
		return typeid_t::make_int();
	}
	else if(node.bt == base_type::k_double){
		return typeid_t::make_double();
	}
	else if(node.bt == base_type::k_string){
		return typeid_t::make_string();
	}
	else if(node.bt == base_type::k_json){
		return typeid_t::make_json();
	}
	else if(node.bt == base_type::k_typeid){
		return typeid_t::make_typeid();
	}


	else if(node.bt == base_type::k_struct){
		const auto& struct_def = node.struct_def;
		const auto members2 = mapf<member_t>(struct_def._members, [&](const auto& m) { return member_t{ to_typeid(interner, m._type, peek_depth2), m._name }; });
		return typeid_t::make_struct2(members2);
	}
	else if(node.bt == base_type::k_vector){
		QUARK_ASSERT(node.child_types.size() == 1);

		return typeid_t::make_vector(to_typeid(interner, node.child_types[0], peek_depth2));
	}
	else if(node.bt == base_type::k_dict){
		QUARK_ASSERT(node.child_types.size() == 1);

		return typeid_t::make_dict(to_typeid(interner, node.child_types[0], peek_depth2));
	}
	else if(node.bt == base_type::k_function){
		const auto e = mapf<typeid_t>(node.child_types, [&](const auto& m) { return to_typeid(interner, m, peek_depth2); });
		return typeid_t::make_function3(e[0], std::vector<typeid_t>(e.begin() + 1, e.end()), node.func_pure, node.func_return_dyn_type);
	}
	else if(node.bt == base_type::k_identifier){
		return typeid_t::make_identifier(node.identifier_str);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
*/

#if 0
static typeid_t flatten_type_description_deep_internal(const type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const type_interner_t& interner;
		const typeid_t& type;


		typeid_t operator()(const typeid_t::undefined_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::any_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::void_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::bool_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::int_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::double_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::string_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::json_type_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::typeid_type_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::struct_t& e) const{
			const auto& struct_def = type.get_struct();
			std::vector<member_t> members2;
			for(const auto& m: struct_def._members){
				members2.push_back(
					member_t(
						flatten_type_description_deep(interner, lookup_itype_from_typeid(interner, m._type)),
						m._name
					)
				);
			}
			const auto type2 = typeid_t::make_struct2(members2);
			return type2;
		}
		typeid_t operator()(const typeid_t::vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type2 = flatten_type_description_deep(
				interner,
				lookup_itype_from_typeid(interner, e._parts[0])
			);
			const auto type2 = typeid_t::make_vector(element_type2);
			return type2;

		}
		typeid_t operator()(const typeid_t::dict_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type2 = flatten_type_description_deep(
				interner,
				lookup_itype_from_typeid(interner, e._parts[0])
			);
			const auto type2 = typeid_t::make_dict(element_type2);
			return type2;
		}
		typeid_t operator()(const typeid_t::function_t& e) const{
			const auto ret = type.get_function_return();
			const auto args = type.get_function_args();
			const auto pure = type.get_function_pure();
			const auto dyn_return_type = type.get_function_dyn_return_type();

			const auto ret2 = flatten_type_description_deep_internal(interner, ret);
			const std::vector<typeid_t> args2 = mapf<typeid_t>(args, [&interner = interner](const auto& e){ return flatten_type_description_deep_internal(interner, e); }); 
			const auto type2 = typeid_t::make_function3(ret2, args2, pure, dyn_return_type);
			return type2;
		}
		typeid_t operator()(const typeid_t::identifier_t& e) const {
			const auto identifier = type.get_identifier();
			QUARK_ASSERT(identifier != "");

			QUARK_ASSERT(is_type_tag(identifier));
			const auto tag = unpack_type_tag(identifier);

			//	Find the itype the current itype refers to using its name.
			const auto it = std::find_if(
				interner.interned2.begin(),
				interner.interned2.end(),
				[&tag](const auto& m){ return m.optional_tag == tag; }
			);
			if(it == interner.interned2.end()){
				throw std::exception();
			}

			return flatten_type_description_deep_internal(interner, it->type);
		}
	};

	const auto result = std::visit(visitor_t{ interner, type }, type._contents);

	QUARK_ASSERT(interner.check_invariant());
	return result;
}
typeid_t flatten_type_description_deep(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& result = lookup_typeinfo_from_itype(interner, type);
	return flatten_type_description_deep_internal(interner, result.type);
}
#endif






itype_t peek(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(false) trace_type_interner(interner);

	const auto& info = lookup_typeinfo_from_itype(interner, type);
	if(info.bt == base_type::k_named_type){
		QUARK_ASSERT(info.child_types.size() == 1);
		const auto dest = info.child_types[0];

		//	Support many linked tags.
		return peek(interner, dest);
	}
	else{
		return type;
	}
}

itype_t refresh_itype(const type_interner_t& interner, const itype_t& type){
	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	return lookup_itype_from_index(interner, type.get_lookup_index());
}


#if 0
//	Adds type. Also interns any child types, as needed.
//	Child types guaranteed to have lower itype indexes.
//	Attempts to resolve all identifer-types by looking up tagged types.
//	Tag type can be empty = anonymous type.
static itype_t intern_anonymous_type_internal(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		type_interner_t& interner;
		const typeid_t& type;

		itype_t operator()(const typeid_t::undefined_t& e) const{
			return itype_t::make_undefined();
		}
		itype_t operator()(const typeid_t::any_t& e) const{
			return itype_t::make_any();
		}

		itype_t operator()(const typeid_t::void_t& e) const{
			return itype_t::make_void();
		}
		itype_t operator()(const typeid_t::bool_t& e) const{
			return itype_t::make_bool();
		}
		itype_t operator()(const typeid_t::int_t& e) const{
			return itype_t::make_int();
		}
		itype_t operator()(const typeid_t::double_t& e) const{
			return itype_t::make_double();
		}
		itype_t operator()(const typeid_t::string_t& e) const{
			return itype_t::make_string();
		}

		itype_t operator()(const typeid_t::json_type_t& e) const{
			return itype_t::make_json();
		}
		itype_t operator()(const typeid_t::typeid_type_t& e) const{
			return itype_t::make_typeid();
		}

		itype_t operator()(const typeid_t::struct_t& e) const{
			const auto& struct_def = type.get_struct();
			std::vector<member_itype_t> members2;
			for(const auto& m: struct_def._members){
				members2.push_back(
					member_itype_t{ intern_anonymous_type(interner, m._type), m._name }
				);
			}
			return make_struct(interner, struct_def_itype_t { members2 } );
		}
		itype_t operator()(const typeid_t::vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_anonymous_type(interner, e._parts[0]);
			return make_vector(interner, element_type);
		}
		itype_t operator()(const typeid_t::dict_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_anonymous_type(interner, e._parts[0]);
			return make_dict(interner, element_type);
		}
		itype_t operator()(const typeid_t::function_t& e) const{
			const auto ret = type.get_function_return();
			const auto args = type.get_function_args();
			const auto pure = type.get_function_pure();
			const auto dyn_return_type = type.get_function_dyn_return_type();

			const auto ret2 = intern_anonymous_type(interner, ret);
			const auto args2 = mapf<itype_t>(args, [&](const auto& e) { return intern_anonymous_type(interner, e); });
			return make_function3(interner, ret2, args2, pure, dyn_return_type);
		}
		itype_t operator()(const typeid_t::identifier_t& e) const {
			const auto identifier = type.get_identifier();
			QUARK_ASSERT(is_type_tag(identifier));
			return make_identifier(interner, identifier);
		}
	};
	const auto result = std::visit(visitor_t{ interner, type }, type._contents);

	QUARK_ASSERT(interner.check_invariant());
	return result;
}

itype_t intern_anonymous_type(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(false){
		QUARK_SCOPED_TRACE("intern_anonymous_type()");
		{
			QUARK_SCOPED_TRACE("before");
			trace_type_interner(interner);
		}
		const auto itype = intern_anonymous_type_internal(interner, type);
		{
			QUARK_SCOPED_TRACE("after");
			trace_type_interner(interner);
		}
		return itype;
	}
	else{
		return intern_anonymous_type_internal(interner, type);
	}
}
#endif


#if 0
QUARK_TEST("type_interner_t", "", "", ""){
	type_interner_t a;
	if(true) trace_type_interner(a);

	const auto find = lookup_itype_from_typeid(a, typeid_t::make_undefined());
	QUARK_ASSERT(find.is_undefined());
}


//??? How to update named type's type with new subtype and still guarantee named-type has
//	bigger index? Subtype may introduce new itypes.

QUARK_TEST("type_interner_t", "new_tagged_type()", "", ""){
	type_interner_t a;
	const auto r = new_tagged_type(a, type_tag_t{{ "a" }});
	if(true) trace_type_interner(a);

	const auto find = lookup_itype_from_tagged_type(a, type_tag_t{{ "a" }});
	const auto find2 = lookup_typeinfo_from_itype(a, find);
//	QUARK_UT_VERIFY(find2.second == typeid_t::make_vector(typeid_t::make_identifier("hello")) );

//	const auto r2 = intern_type_with_name(a, "a", typeid_t::make_vector(typeid_t::make_string()));
//	QUARK_UT_VERIFY(find == typeid_t::make_vector(typeid_t::make_string()) );
}


QUARK_TEST("type_interner_t", "new_tagged_type()", "Nested types", "Nested types get their own itypes"){
	type_interner_t a;
	const auto r = new_tagged_type(a, type_tag_t{{ "a" }});
	if(true) trace_type_interner(a);

//	QUARK_UT_VERIFY(lookup_typeinfo_from_itype(a, lookup_itype_from_tagged_type(a, type_tag_t{{ "a" }})).second == typeid_t::make_vector(typeid_t::make_dict(typeid_t::make_double())) );

	QUARK_UT_VERIFY(
		lookup_itype_from_typeid(a, typeid_t::make_vector(typeid_t::make_dict(typeid_t::make_double())))
		== itype_t::assemble2(15, base_type::k_vector, base_type::k_dict)
	);
	QUARK_UT_VERIFY(
		lookup_itype_from_typeid(a, typeid_t::make_dict(typeid_t::make_double()))
		== itype_t::assemble2(14, base_type::k_dict, base_type::k_double)
	);
}
#endif



#if 0
itype_t lookup_itype_from_typeid(const type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

/*
	const auto it = find_type(interner, type);
	if(it != interner.interned2.end()){
		const auto index = it - interner.interned2.begin();
		const auto lookup_index = static_cast<int32_t>(index);
		return make_itype_from_parts(lookup_index, it->type);
	}
*/
	throw std::exception();
}
#endif

itype_t lookup_itype_from_index(const type_interner_t& interner, type_lookup_index_t type_index){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type_index >= 0 && type_index < interner.interned2.size());

	const auto& node = interner.interned2[type_index];

	if(node.bt == base_type::k_struct){
		return itype_t::make_struct(type_index);
	}
	else if(node.bt == base_type::k_vector){
		const auto element_bt = node.child_types[0].get_base_type();
		return itype_t::make_vector(type_index, element_bt);
	}
	else if(node.bt == base_type::k_dict){
		const auto value_bt = node.child_types[0].get_base_type();
		return itype_t::make_dict(type_index, value_bt);
	}
	else if(node.bt == base_type::k_function){
		return itype_t::make_function(type_index);
	}
	else if(node.bt == base_type::k_symbol_ref){
		return itype_t::assemble2(type_index, base_type::k_symbol_ref, base_type::k_undefined);
	}
	else if(node.bt == base_type::k_named_type){
		return itype_t::assemble2(type_index, base_type::k_named_type, base_type::k_undefined);
	}
	else{
		const auto bt = node.bt;
		return itype_t::assemble2((type_lookup_index_t)type_index, bt, base_type::k_undefined);
	}
}

itype_t lookup_itype_from_tagged_type(const type_interner_t& interner, const type_tag_t& tag){
	QUARK_ASSERT(interner.check_invariant());

	if(tag.lexical_path.empty()){
		throw std::exception();
	}
	else{
		const auto it = std::find_if(
			interner.interned2.begin(),
			interner.interned2.end(),
			[&](const auto& e){ return e.optional_tag == tag; }
		);
		if(it == interner.interned2.end()){
			throw std::exception();
		}

		return lookup_itype_from_index_it(interner, it - interner.interned2.begin());
	}
}





void trace_type_interner(const type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	{
		QUARK_SCOPED_TRACE("ITYPES");

		{
			QUARK_SCOPED_TRACE("ITYPES");
			std::vector<std::vector<std::string>> matrix;
			for(auto i = 0 ; i < interner.interned2.size() ; i++){
				const auto& itype = lookup_itype_from_index(interner, i);

				const auto& e = interner.interned2[i];
				const auto contents = e.bt == base_type::k_named_type
					? std::to_string(e.child_types[0].get_lookup_index())
					: itype_to_compact_string(interner, itype, resolve_named_types::dont_resolve);

				const auto line = std::vector<std::string>{
					std::to_string(i),
					pack_type_tag(e.optional_tag),
					base_type_to_opcode(e.bt),
					contents,
				};
				matrix.push_back(line);
			}

			const auto result = generate_table_type1({ "itype_t", "name-tag", "base_type", "contents" }, matrix);
			QUARK_TRACE(result);
		}
	}

/*
	{
		QUARK_SCOPED_TRACE("NAMED TYPES");
		for(auto i = 0 ; i < interner.name_lookup.size() ; i++){
			const auto& e = interner.name_lookup[i];
			const auto t = lookup_type(interner, e.second);
			QUARK_TRACE_SS(
				"name_index: " << i
				<< "\t name: \"" << e.first.path << "\""
				<< " itype_data: " << e.second.get_data()
				<< " itype_lookup_index: " << e.second.get_lookup_index()
				<< " typeid: " << typeid_to_compact_string(t)
			);
		}
	}
*/
}

#if 0
QUARK_TEST("type_interner_t", "type_interner_t()", "Check that built in types work with lookup_itype_from_typeid()", ""){
	const type_interner_t a;
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_undefined()) == itype_t::make_undefined());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_any()) == itype_t::make_any());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_void()) == itype_t::make_void());

	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_bool()) == itype_t::make_bool());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_int()) == itype_t::make_int());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_double()) == itype_t::make_double());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_string()) == itype_t::make_string());
	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_json()) == itype_t::make_json());

	QUARK_UT_VERIFY(lookup_itype_from_typeid(a, typeid_t::make_typeid()) == itype_t::make_typeid());
}

QUARK_TEST("type_interner_t", "type_interner_t()", "Check that built in types work with lookup_typeid_from_itype()", ""){
	const type_interner_t a;
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_undefined()) == typeid_t::make_undefined());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_any()) == typeid_t::make_any());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_void()) == typeid_t::make_void());

	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_bool()) == typeid_t::make_bool());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_int()) == typeid_t::make_int());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_double()) == typeid_t::make_double());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_string()) == typeid_t::make_string());
	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_json()) == typeid_t::make_json());

	QUARK_UT_VERIFY(lookup_typeid_from_itype(a, itype_t::make_typeid()) == typeid_t::make_typeid());
}
#endif



#if 0
static type_tag_t xxx_make_type_tag(const std::string& identifier){
//	const auto id = a.scope_id_generator++;
	const auto id = 1;

	//??? TOOD: user proper hiearchy of lexical scopes, not a flat list of generated ID:s. This requires a name--string in each lexical_scope_t.

	const auto b = std::string() + "lexical_scope" + std::to_string(id);

	return type_tag_t { { b, identifier } };
}

static itype_t xxx_make_benchmark_result_t(){
	const auto x = itype_t::make_struct2(
		{
			member_itype_t{ itype_t::make_int(), "dur" },
			member_itype_t{ itype_t::make_json(), "more" }
		}
	);
	return x;
}

static itype_t xxx_make_benchmark_function_t(){
//	return itype_t::make_function(itype_t::make_vector(make_benchmark_result_t()), {}, epure::pure);

	const auto tag = pack_type_tag(xxx_make_type_tag("benchmark_result_t"));
	return itype_t::make_function(itype_t::make_identifier(tag), {}, epure::pure);
}

static itype_t xxx_make_benchmark_def_t(){
	const auto x = itype_t::make_struct2( {
		member_t{ itype_t::make_string(), "name" },
		member_t{ xxx_make_benchmark_function_t(), "f" }
	} );
	return x;
}

QUARK_TEST("type_interner_t", "type_interner_t()", "Check that built in types work with lookup_typeid_from_itype()", ""){
	type_interner_t a;

	const auto benchmark_result_itype = intern_anonymous_type(a, xxx_make_benchmark_result_t());
	new_tagged_type(a, xxx_make_type_tag("/benchmark_result_t"), benchmark_result_itype);

	const auto benchmark_def_itype = intern_anonymous_type(a, xxx_make_benchmark_def_t());
	new_tagged_type(a, xxx_make_type_tag("/benchmark_def_t"), benchmark_def_itype);
//???	to_asttype(resolve_and_intern_itype_from_typeid(a, k_no_location, benchmark_registry_type)),
}
#endif






json_t itype_to_json_shallow(const itype_t& itype){
	const auto s = std::string("itype:") + std::to_string(itype.get_data());
	return json_t(s);
}

static json_t struct_definition_to_json(const type_interner_t& interner, const struct_def_itype_t& v){
	QUARK_ASSERT(v.check_invariant());

	return json_t::make_array({
		members_to_json(interner, v._members)
	});
}

static std::vector<json_t> typeids_to_json_array(const type_interner_t& interner, const std::vector<typeid_t>& m){
	std::vector<json_t> r;
	for(const auto& a: m){
		r.push_back(itype_to_json(interner, a));
	}
	return r;
}
static std::vector<typeid_t> typeids_from_json_array(type_interner_t& interner, const std::vector<json_t>& m){
	std::vector<typeid_t> r;
	for(const auto& a: m){
		r.push_back(itype_from_json(interner, a));
	}
	return r;
}

json_t itype_to_json(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto bt = type.get_base_type();
	const auto basetype_str = base_type_to_opcode(bt);

	struct visitor_t {
		const type_interner_t& interner;
		const itype_t& type;
		const std::string basetype_str;


		json_t operator()(const undefined_t& e) const{
			return basetype_str;
		}
		json_t operator()(const any_t& e) const{
			return basetype_str;
		}

		json_t operator()(const void_t& e) const{
			return basetype_str;
		}
		json_t operator()(const bool_t& e) const{
			return basetype_str;
		}
		json_t operator()(const int_t& e) const{
			return basetype_str;
		}
		json_t operator()(const double_t& e) const{
			return basetype_str;
		}
		json_t operator()(const string_t& e) const{
			return basetype_str;
		}

		json_t operator()(const json_type_t& e) const{
			return basetype_str;
		}
		json_t operator()(const typeid_type_t& e) const{
			return basetype_str;
		}

		json_t operator()(const struct_t& e) const{
			return json_t::make_array(
				{
					json_t(basetype_str),
					struct_definition_to_json(interner, e.def)
				}
			);
		}
		json_t operator()(const vector_t& e) const{
			const auto d = type.get_vector_element_type(interner);
			return json_t::make_array( { json_t(basetype_str), itype_to_json(interner, d) });
		}
		json_t operator()(const dict_t& e) const{
			const auto d = type.get_dict_value_type(interner);
			return json_t::make_array( { json_t(basetype_str), itype_to_json(interner, d) });
		}
		json_t operator()(const function_t& e) const{
			const auto ret = type.get_function_return(interner);
			const auto args = type.get_function_args(interner);
			const auto pure = type.get_function_pure(interner);
			const auto dyn = type.get_function_dyn_return_type(interner);

			//	Only include dyn-type it it's different than return_dyn_type::none.
			const auto dyn_type = dyn != return_dyn_type::none ? json_t(static_cast<int>(dyn)) : json_t();
			return make_array_skip_nulls({
				basetype_str,
				itype_to_json(interner, ret),
				typeids_to_json_array(interner, args),
				pure == epure::pure ? true : false,
				dyn_type
			});

			std::vector<std::string> args_str;
			for(const auto& a: args){
				args_str.push_back(itype_to_compact_string(interner, a, resolve_named_types::dont_resolve));
			}

			return std::string() + "func " + itype_to_compact_string(interner, ret, resolve_named_types::dont_resolve) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
		}
		json_t operator()(const symbol_ref_t& e) const {
			const auto identifier = e.s;
			return json_t(std::string("%") + identifier);
		}
		json_t operator()(const named_type_t& e) const {
			return itype_to_json(interner, e.destination_type);
/*
			//	Find the itype the current itype refers to using its name.
			const auto it = std::find_if(
				interner.interned2.begin(),
				interner.interned2.end(),
				[&tag](const auto& m){ return m.optional_tag == tag; }
			);
			if(it == interner.interned2.end()){
				throw std::exception();
			}
*/
		}
	};

	const auto result = std::visit(visitor_t{ interner, type, basetype_str }, get_itype_variant(interner, type));
	return result;
}

itype_t itype_from_json(type_interner_t& interner, const json_t& t){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	if(t.is_string()){
		const auto s = t.get_string();

		//	Identifier.
		if(s.front() == '%'){
			return typeid_t::make_symbol_ref(interner, s.substr(1));
		}

		//	Tagged type.
		else if(is_type_tag(s)){
			return typeid_t::make_named_type(interner, unpack_type_tag(s));
		}

		//	Other types.
		else {
			if(s == ""){
				return typeid_t::make_undefined();
			}

			const auto b = opcode_to_base_type(s);


			if(b == base_type::k_undefined){
				return typeid_t::make_undefined();
			}
			else if(b == base_type::k_any){
				return typeid_t::make_any();
			}
			else if(b == base_type::k_void){
				return typeid_t::make_void();
			}
			else if(b == base_type::k_bool){
				return typeid_t::make_bool();
			}
			else if(b == base_type::k_int){
				return typeid_t::make_int();
			}
			else if(b == base_type::k_double){
				return typeid_t::make_double();
			}
			else if(b == base_type::k_string){
				return typeid_t::make_string();
			}
			else if(b == base_type::k_typeid){
				return typeid_t::make_typeid();
			}
			else if(b == base_type::k_json){
				return typeid_t::make_json();
			}
			else{
				quark::throw_exception();
			}
		}
	}
	else if(t.is_array()){
		const auto a = t.get_array();
		const auto s = a[0].get_string();
		QUARK_ASSERT(s.front() != '#');
		QUARK_ASSERT(s.front() != '^');
		QUARK_ASSERT(s.front() != '%');
		QUARK_ASSERT(s.front() != '/');

		if(s == "struct"){
			const auto struct_def_array = a[1].get_array();
			const auto member_array = struct_def_array[0].get_array();

			const std::vector<member_t> struct_members = members_from_json(interner, member_array);
			return typeid_t::make_struct2(interner, struct_members);
		}
		else if(s == "vector"){
			const auto element_type = itype_from_json(interner, a[1]);
			return typeid_t::make_vector(interner, element_type);
		}
		else if(s == "dict"){
			const auto value_type = itype_from_json(interner, a[1]);
			return typeid_t::make_dict(interner, value_type);
		}
		else if(s == "func"){
			const auto ret_type = itype_from_json(interner, a[1]);
			const auto arg_types_array = a[2].get_array();
			const std::vector<typeid_t> arg_types = typeids_from_json_array(interner, arg_types_array);

			if(a[3].is_true() == false && a[3].is_false() == false){
				quark::throw_exception();
			}
			const bool pure = a[3].is_true();

			if(a.size() > 4){
				const auto dyn = static_cast<return_dyn_type>(a[4].get_number());
				if(dyn == return_dyn_type::none){
					return typeid_t::make_function(interner, ret_type, arg_types, pure ? epure::pure : epure::impure);
				}
				else{
					return typeid_t::make_function_dyn_return(interner, arg_types, pure ? epure::pure : epure::impure, dyn);
				}
			}
			else{
				return typeid_t::make_function(interner, ret_type, arg_types, pure ? epure::pure : epure::impure);
			}
		}
		else if(s == "unknown-identifier"){
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		else {
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
	}
	else{
		quark::throw_runtime_error("Invalid typeid-json.");
	}
	return typeid_t::make_undefined();
}



//??? General note: port tests from typeid.h


itype_t itype_from_json_shallow(const json_t& j){
	QUARK_ASSERT(j.check_invariant());

	if(j.is_string() && j.get_string().substr(0, 6) == "itype:"){
		const auto n_str = j.get_string().substr(6);
		const auto i = std::stoi(n_str);
		const auto itype = itype_t(i);
		return itype;
	}
	else{
		throw std::exception();
	}
}

std::string itype_to_debug_string(const itype_t& itype){
	std::stringstream s;
	s << "{ " << itype.get_lookup_index() << ": " << base_type_to_opcode(itype.get_base_type()) << " }";
	return s.str();
}

std::string itype_to_compact_string(const type_interner_t& interner, const itype_t& type, resolve_named_types resolve){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const type_interner_t& interner;
		const itype_t& type;
		const resolve_named_types resolve;


		std::string operator()(const undefined_t& e) const{
			return "undef";
		}
		std::string operator()(const any_t& e) const{
			return "any";
		}

		std::string operator()(const void_t& e) const{
			return "void";
		}
		std::string operator()(const bool_t& e) const{
			return "bool";
		}
		std::string operator()(const int_t& e) const{
			return "int";
		}
		std::string operator()(const double_t& e) const{
			return "double";
		}
		std::string operator()(const string_t& e) const{
			return "string";
		}

		std::string operator()(const json_type_t& e) const{
			return "json";
		}
		std::string operator()(const typeid_type_t& e) const{
			return "typeid";
		}

		std::string operator()(const struct_t& e) const{
			std::string members_acc;
			for(const auto& m: e.def._members){
				members_acc = itype_to_compact_string(interner, m._type, resolve) + " " + members_acc + m._name + ";";
			}
			return "struct {" + members_acc + "}";
		}
		std::string operator()(const vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			return "[" + itype_to_compact_string(interner, e._parts[0], resolve) + "]";
		}
		std::string operator()(const dict_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			return "[string:" + itype_to_compact_string(interner, e._parts[0], resolve) + "]";
		}
		std::string operator()(const function_t& e) const{
			const auto ret = type.get_function_return(interner);
			const auto args = type.get_function_args(interner);
			const auto pure = type.get_function_pure(interner);

			std::vector<std::string> args_str;
			for(const auto& a: args){
				args_str.push_back(itype_to_compact_string(interner, a, resolve));
			}

			return std::string() + "func " + itype_to_compact_string(interner, ret, resolve) + "(" + concat_strings_with_divider(args_str, ",") + ") " + (pure == epure::pure ? "pure" : "impure");
		}
		std::string operator()(const symbol_ref_t& e) const {
//			QUARK_ASSERT(e.s != "");
			return std::string("%") + e.s;
		}
		std::string operator()(const named_type_t& e) const {
			if(resolve == resolve_named_types::resolve){
				const auto p = peek(interner, type);
				return itype_to_compact_string(interner, p, resolve);
			}

			//	Return the name of the type.
			else if(resolve == resolve_named_types::dont_resolve){
				const auto& info = lookup_typeinfo_from_itype(interner, type);
				return info.optional_tag.lexical_path.back();
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
		}
	};

	const auto result = std::visit(visitor_t{ interner, type, resolve }, get_itype_variant(interner, type));
	return result;
}








itype_t make_struct(type_interner_t& interner, const struct_def_itype_t& struct_def){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_struct,
		std::vector<itype_t>{},
		struct_def,
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}

itype_t make_struct(const type_interner_t& interner, const struct_def_itype_t& struct_def){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_struct,
		std::vector<itype_t>{},
		struct_def,
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}


itype_t make_vector(type_interner_t& interner, const itype_t& element_type){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_vector,
		{ element_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}
itype_t make_vector(const type_interner_t& interner, const itype_t& element_type){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_vector,
		{ element_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}

itype_t make_dict(type_interner_t& interner, const itype_t& value_type){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_dict,
		{ value_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}

itype_t make_dict(const type_interner_t& interner, const itype_t& value_type){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_dict,
		{ value_type },
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return lookup_node(interner, node);
}

itype_t make_function3(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_function,
		concat(
			std::vector<itype_t>{ ret },
			args
		),
		{},
		pure,
		dyn_return,
		""
	};
	return intern_node(interner, node);
}


itype_t make_function3(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_function,
		concat(
			std::vector<itype_t>{ ret },
			args
		),
		{},
		pure,
		dyn_return,
		""
	};
	return lookup_node(interner, node);
}

itype_t make_function_dyn_return(type_interner_t& interner, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return){
	return make_function3(interner, itype_t::make_any(), args, pure, dyn_return);
}

itype_t make_function(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure){
	QUARK_ASSERT(ret.is_any() == false);

	return make_function3(interner, ret, args, pure, return_dyn_type::none);
}

itype_t make_function(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure){
	QUARK_ASSERT(ret.is_any() == false);

	return make_function3(interner, ret, args, pure, return_dyn_type::none);
}



itype_t make_symbol_ref(type_interner_t& interner, const std::string& s){
	const auto node = type_node_t{
		make_empty_type_tag(),
		base_type::k_symbol_ref,
		std::vector<itype_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		s
	};
	return intern_node(interner, node);
}

itype_t make_named_type(type_interner_t& interner, const type_tag_t& type){
	const auto node = type_node_t{
		type,
		base_type::k_symbol_ref,
		std::vector<itype_t>{},
		{},
		epure::pure,
		return_dyn_type::none,
		""
	};
	return intern_node(interner, node);
}





//??? Doesnt work. Needs to use typeid_t-style code that genreates contents of type
json_t type_interner_to_json(const type_interner_t& interner){
	std::vector<json_t> types;
	for(auto i = 0 ; i < interner.interned2.size() ; i++){
		const auto& itype = lookup_itype_from_index(interner, i);

		const auto& e = interner.interned2[i];
		const auto tag = pack_type_tag(e.optional_tag);
		const auto desc = itype_to_json(interner, itype);
		const auto x = json_t::make_object({
			{ "tag", tag },
			{ "desc", desc }
		});
		types.push_back(x);
	}

	//??? support tags!
	return types;
}


//??? Doesnt work. Needs to use typeid_t-style code that genreates contents of type
//??? support tags!
type_interner_t type_interner_from_json(const json_t& j){
	type_interner_t interner;
	for(const auto& t: j.get_array()){
		const auto tag = t.get_object_element("tag").get_string();
		const auto desc = t.get_object_element("desc");

		const auto tag2 = unpack_type_tag(tag);
		const auto type2 = itype_from_json(interner, desc);
		(void)type2;
//		types.push_back(type_node_t { tag2, type2 });
	}
	return interner;
}





const type_node_t& lookup_typeinfo_from_itype(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	const auto& result = interner.interned2[lookup_index];
	return result;
}
type_node_t& lookup_typeinfo_from_itype(type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	auto& result = interner.interned2[lookup_index];
	return result;
}


bool is_atomic_type(itype_t type){
	const auto bt = type.get_base_type();
	if(
		bt == base_type::k_undefined
		|| bt == base_type::k_any
		|| bt == base_type::k_void

		|| bt == base_type::k_bool
		|| bt == base_type::k_int
		|| bt == base_type::k_double
		|| bt == base_type::k_string
		|| bt == base_type::k_json

		|| bt == base_type::k_typeid
		|| bt == base_type::k_symbol_ref
//		|| bt == base_type::k_named_type
	){
		return true;
	}
	else{
		return false;
	}
}


} //	floyd
