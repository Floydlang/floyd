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

#include <limits.h>

namespace floyd {



//////////////////////////////////////////////////		type_interner_t

//??? move out of ast. Needed at runtime


static std::pair<std::string, typeid_t> make_entry(const typeid_t& type){
	return {
		"",
		type
	};
}


type_interner_t::type_interner_t(){
	//	Order is designed to match up the interned2[] with base_type indexes.
	interned2.push_back(make_entry(typeid_t::make_undefined()));
	interned2.push_back(make_entry(typeid_t::make_any()));
	interned2.push_back(make_entry(typeid_t::make_void()));


	interned2.push_back(make_entry(typeid_t::make_bool()));
	interned2.push_back(make_entry(typeid_t::make_int()));
	interned2.push_back(make_entry(typeid_t::make_double()));
	interned2.push_back(make_entry(typeid_t::make_string()));
	interned2.push_back(make_entry(typeid_t::make_json()));

	interned2.push_back(make_entry(typeid_t::make_typeid()));

	//	These are complex types and are undefined. We need them to take up space in the interned2-vector.
	interned2.push_back(make_entry(typeid_t::make_undefined()));
	interned2.push_back(make_entry(typeid_t::make_undefined()));
	interned2.push_back(make_entry(typeid_t::make_undefined()));
	interned2.push_back(make_entry(typeid_t::make_undefined()));

	interned2.push_back(make_entry(typeid_t::make_identifier("")));

	QUARK_ASSERT(check_invariant());
}

bool type_interner_t::check_invariant() const {
	QUARK_ASSERT(interned2.size() < INT_MAX);

	QUARK_ASSERT(interned2[(int)base_type::k_undefined] == make_entry(typeid_t::make_undefined()));
	QUARK_ASSERT(interned2[(int)base_type::k_any] == make_entry(typeid_t::make_any()));
	QUARK_ASSERT(interned2[(int)base_type::k_void] == make_entry(typeid_t::make_void()));

	QUARK_ASSERT(interned2[(int)base_type::k_bool] == make_entry(typeid_t::make_bool()));
	QUARK_ASSERT(interned2[(int)base_type::k_int] == make_entry(typeid_t::make_int()));
	QUARK_ASSERT(interned2[(int)base_type::k_double] == make_entry(typeid_t::make_double()));
	QUARK_ASSERT(interned2[(int)base_type::k_string] == make_entry(typeid_t::make_string()));
	QUARK_ASSERT(interned2[(int)base_type::k_json] == make_entry(typeid_t::make_json()));

	QUARK_ASSERT(interned2[(int)base_type::k_typeid] == make_entry(typeid_t::make_typeid()));

	QUARK_ASSERT(interned2[(int)base_type::k_struct] == make_entry(typeid_t::make_undefined()));
	QUARK_ASSERT(interned2[(int)base_type::k_vector] == make_entry(typeid_t::make_undefined()));
	QUARK_ASSERT(interned2[(int)base_type::k_dict] == make_entry(typeid_t::make_undefined()));
	QUARK_ASSERT(interned2[(int)base_type::k_function] == make_entry(typeid_t::make_undefined()));
	QUARK_ASSERT(interned2[(int)base_type::k_identifier] == make_entry(typeid_t::make_identifier("")));


	//??? Add other common combinations, like vectors with each atomic type, dict with each atomic type. This allows us to hardcoded their itype indexes.
	return true;
}


static itype_t make_itype_from_parts(int lookup_index, const typeid_t& type){
	if(type.is_struct()){
		return itype_t::make_struct(lookup_index);
	}
	else if(type.is_vector()){
		return itype_t::make_vector(lookup_index, type.get_vector_element_type().get_base_type());
	}
	else if(type.is_dict()){
		return itype_t::make_dict(lookup_index, type.get_dict_value_type().get_base_type());
	}
	else if(type.is_function()){
		return itype_t::make_function(lookup_index);
	}
	else{
		const auto bt = type.get_base_type();
		return itype_t::assemble2((int)bt, bt, base_type::k_undefined);
	}
}

static itype_t record_and_resolve_recursive(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		type_interner_t& interner;
		const typeid_t& type;

		itype_t operator()(const typeid_t::undefined_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::any_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}

		itype_t operator()(const typeid_t::void_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::bool_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::int_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::double_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::string_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}

		itype_t operator()(const typeid_t::json_type_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		itype_t operator()(const typeid_t::typeid_type_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}

		itype_t operator()(const typeid_t::struct_t& e) const{
			for(const auto& m: e._struct_def->_members){
				intern_type(interner, m._type);
			}
			interner.interned2.push_back({ "", type });
			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_struct(lookup_index);
		}
		itype_t operator()(const typeid_t::vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_type(interner, e._parts[0]);

			interner.interned2.push_back({ "", type });
			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_vector(lookup_index, element_type.first.get_base_type());
		}
		itype_t operator()(const typeid_t::dict_t& e) const{
			//	Warning: Need to remember dict_next_id since members can add intern other structs and bump dict_next_id.
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_type(interner, e._parts[0]);

			interner.interned2.push_back({ "", type });
			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_dict(lookup_index, element_type.first.get_base_type());
		}
		itype_t operator()(const typeid_t::function_t& e) const{
			for(const auto& m: e._parts){
				intern_type(interner, m);
			}
			interner.interned2.push_back({ "", type });
			const auto lookup_index = static_cast<int32_t>(interner.interned2.size() - 1);
			return itype_t::make_function(lookup_index);
		}
		itype_t operator()(const typeid_t::identifier_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
	};
	const auto result = std::visit(visitor_t{ interner, type }, type._contents);

	QUARK_ASSERT(interner.check_invariant());

	return result;
}


std::pair<itype_t, typeid_t> intern_type_with_name(type_interner_t& interner, const std::string& name, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned2.begin(), interner.interned2.end(), [&](const auto& e){ return e.first == name; });

	//	This name already exists, should we update it?
	if(it != interner.interned2.end()){
		if(it->second == type){
			const auto index = it - interner.interned2.begin();
			const auto lookup_index = static_cast<int32_t>(index);
			return { make_itype_from_parts(lookup_index, type), type };
		}
		else if(it->second.is_undefined()){
			it->second = type;

			const auto index = it - interner.interned2.begin();
			const auto lookup_index = static_cast<int32_t>(index);
			return { make_itype_from_parts(lookup_index, type), type };
		}
		else{
			//	Name already exists, but with a conflicting type!
			throw std::exception();
		}
	}

	//	New type, store it.
	else{
		interner.interned2.push_back( { name, type } );
		const auto index = interner.interned2.size() - 1;
		const auto lookup_index = static_cast<int32_t>(index);

		QUARK_ASSERT(interner.check_invariant());

		return { make_itype_from_parts(lookup_index, type), type };
	}
}

std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned2.begin(), interner.interned2.end(), [&](const auto& e){ return e.first == "" && e.second == type; });

	//	This name already exists, should we update it?
	if(it != interner.interned2.end()){
		const auto index = it - interner.interned2.begin();
		const auto lookup_index = static_cast<int32_t>(index);
		return { make_itype_from_parts(lookup_index, type), type };
	}

	//	New type, store it.
	else{
		interner.interned2.push_back( { "", type } );
		const auto index = interner.interned2.size() - 1;
		const auto lookup_index = static_cast<int32_t>(index);

		QUARK_ASSERT(interner.check_invariant());

		return { make_itype_from_parts(lookup_index, type), type };
	}
}

std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const ast_type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(std::holds_alternative<typeid_t>(type._contents)){
		return intern_type(interner, std::get<typeid_t>(type._contents));
	}
	else if(std::holds_alternative<itype_t>(type._contents)){
		const auto& itype = std::get<itype_t>(type._contents);
		const auto t = lookup_type(interner, itype);
		return { itype, t };
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}



#if 0
QUARK_TEST_VIP("type_interner_t()", "type_interner_t()", "", ""){
	type_interner_t a;
	const auto r = record_and_resolve_recursive(a, typeid_t::make_undefined().name("node_t"));
}
#endif


itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned2.begin(), interner.interned2.end(), [&](const auto& e){ return e.second == type; });
	if(it != interner.interned2.end()){
		const auto index = it - interner.interned2.begin();
		const auto lookup_index = static_cast<int32_t>(index);
		return make_itype_from_parts(lookup_index, type);
	}
	throw std::exception();
}

static long find_named_type(const type_interner_t& interner, const std::string& name){
	QUARK_ASSERT(interner.check_invariant());

	if(name == ""){
		return -1;
	}
	else{
		const auto it = std::find_if(interner.interned2.begin(), interner.interned2.end(), [&](const auto& m){ return m.first == name; });
		if(it == interner.interned2.end()){
			return -1;
		}

		return it - interner.interned2.begin();
	}
}

const typeid_t& lookup_type(const type_interner_t& interner, const std::string& name){
	QUARK_ASSERT(interner.check_invariant());

	const auto index = find_named_type(interner, name);
	if(index == -1){
		throw std::exception();
	}
	else {
		return interner.interned2[index].second;
	}
}

const typeid_t& lookup_type(const type_interner_t& interner, const ast_type_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(std::holds_alternative<std::monostate>(type._contents)){
//		throw std::exception();
		return interner.interned2[(int)base_type::k_undefined].second;
	}
	else if(std::holds_alternative<typeid_t>(type._contents)){
		return std::get<typeid_t>(type._contents);
	}
	else if(std::holds_alternative<itype_t>(type._contents)){
		const auto itype = std::get<itype_t>(type._contents);
		return lookup_type(interner, itype);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





void trace_type_interner(const type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

#if 0
	{
		QUARK_SCOPED_TRACE("ITYPES");
		for(auto i = 0 ; i < interner.interned2.size() ; i++){
			const auto& e = interner.interned2[i];
			QUARK_TRACE_SS(
				"itype_t: " << i
				<< "\t=\ttypeid_t: " << typeid_to_compact_string(e.second)
				<< (e.first == "" ? "" : std::string() + "\t\tNAME: " + e.first)
			);
		}
	}
#endif

	{
		QUARK_SCOPED_TRACE("ITYPES");

		std::vector<std::vector<std::string>> matrix;
		for(auto i = 0 ; i < interner.interned2.size() ; i++){
			const auto& e = interner.interned2[i];
			const auto line = std::vector<std::string>{
				std::to_string(i),
				e.first,
				typeid_to_compact_string(e.second),
			};
			matrix.push_back(line);
		}

		const auto result = generate_table_type1(
			{ "itype_t", "NAME", "typeid_t" },
			matrix
		);
		QUARK_TRACE(result);
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



QUARK_TEST("type_interner_t()", "type_interner_t()", "Check that built in types work with lookup_itype()", ""){
	const type_interner_t a;
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_undefined()) == itype_t::make_undefined());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_any()) == itype_t::make_any());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_void()) == itype_t::make_void());

	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_bool()) == itype_t::make_bool());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_int()) == itype_t::make_int());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_double()) == itype_t::make_double());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_string()) == itype_t::make_string());
	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_json()) == itype_t::make_json());

	QUARK_UT_VERIFY(lookup_itype(a, typeid_t::make_typeid()) == itype_t::make_typeid());
}

QUARK_TEST("type_interner_t()", "type_interner_t()", "Check that built in types work with lookup_type()", ""){
	const type_interner_t a;
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_undefined()) == typeid_t::make_undefined());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_any()) == typeid_t::make_any());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_void()) == typeid_t::make_void());

	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_bool()) == typeid_t::make_bool());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_int()) == typeid_t::make_int());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_double()) == typeid_t::make_double());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_string()) == typeid_t::make_string());
	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_json()) == typeid_t::make_json());

	QUARK_UT_VERIFY(lookup_type(a, itype_t::make_typeid()) == typeid_t::make_typeid());
}





json_t itype_to_json(const itype_t& itype){
	const auto s = std::string("itype:") + std::to_string(itype.get_data());
	return json_t(s);
}

itype_t itype_from_json(const json_t& j){
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

ast_type_t make_type_name_from_typeid(const typeid_t& t){
	QUARK_ASSERT(t.check_invariant());

	return { t };
}

ast_type_t make_type_name_from_itype(const itype_t& t){
	QUARK_ASSERT(t.check_invariant());

	return { t };
}


//??? bad to have special-case for json null!?
json_t ast_type_to_json(const ast_type_t& name){
	QUARK_ASSERT(name.check_invariant());

	if(std::holds_alternative<std::monostate>(name._contents)){
		return json_t();
	}
	else if(std::holds_alternative<typeid_t>(name._contents)){
		return typeid_to_ast_json(std::get<typeid_t>(name._contents), json_tags::k_tag_resolve_state);
	}
	else if(std::holds_alternative<itype_t>(name._contents)){
		const auto itype = std::get<itype_t>(name._contents);
		return itype_to_json(itype);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
/*
	if(tags == json_tags::k_tag_resolve_state){
		return json_t(std::string(1, tag_unresolved_type_char) + t.get_identifier());
	}
	else if(tags == json_tags::k_plain){
	}
	else{
		QUARK_ASSERT(false);
	}

	}
	else if(tags == json_tags::k_plain){
		return json_t(t.get_identifier());
	}
*/

}

ast_type_t ast_type_from_json(const json_t& j){
	QUARK_ASSERT(j.check_invariant());

	if(j.is_null()){
		return make_no_type_name();
	}
	else if(j.is_string() && j.get_string().substr(0, 6) == "itype:"){
		return make_type_name_from_itype(itype_from_json(j));
	}
	else {
		const auto t = typeid_from_ast_json(j);
		return make_type_name_from_typeid(t);
	}
}

std::string ast_type_to_string(const ast_type_t& type){
	QUARK_ASSERT(type.check_invariant());

	return json_to_compact_string(ast_type_to_json(type));
}



} //	floyd
