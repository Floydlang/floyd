//
//  type_interner.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "type_interner.h"


#include "utils.h"
#include <limits.h>

namespace floyd {



//////////////////////////////////////////////////		type_interner_t

//??? move out of ast. Needed at runtime


type_interner_t::type_interner_t(){
	//	Order is designed to match up the interned[] with base_type indexes.
	interned.push_back(typeid_t::make_undefined());
	interned.push_back(typeid_t::make_any());
	interned.push_back(typeid_t::make_void());

	interned.push_back(typeid_t::make_bool());
	interned.push_back(typeid_t::make_int());
	interned.push_back(typeid_t::make_double());
	interned.push_back(typeid_t::make_string());
	interned.push_back(typeid_t::make_json());

	interned.push_back(typeid_t::make_typeid());

	//	These are complex types and are undefined. We need them to take up space in the interned-vector.
	interned.push_back(typeid_t::make_undefined());
	interned.push_back(typeid_t::make_undefined());
	interned.push_back(typeid_t::make_undefined());
	interned.push_back(typeid_t::make_undefined());

	QUARK_ASSERT(check_invariant());
}

bool type_interner_t::check_invariant() const {
	QUARK_ASSERT(interned.size() < INT_MAX);

	QUARK_ASSERT(interned[(int)base_type::k_undefined] == typeid_t::make_undefined());
	QUARK_ASSERT(interned[(int)base_type::k_any] == typeid_t::make_any());
	QUARK_ASSERT(interned[(int)base_type::k_void] == typeid_t::make_void());

	QUARK_ASSERT(interned[(int)base_type::k_bool] == typeid_t::make_bool());
	QUARK_ASSERT(interned[(int)base_type::k_int] == typeid_t::make_int());
	QUARK_ASSERT(interned[(int)base_type::k_double] == typeid_t::make_double());
	QUARK_ASSERT(interned[(int)base_type::k_string] == typeid_t::make_string());
	QUARK_ASSERT(interned[(int)base_type::k_json] == typeid_t::make_json());

	QUARK_ASSERT(interned[(int)base_type::k_typeid] == typeid_t::make_typeid());

	QUARK_ASSERT(interned[(int)base_type::k_struct].is_undefined());
	QUARK_ASSERT(interned[(int)base_type::k_vector].is_undefined());
	QUARK_ASSERT(interned[(int)base_type::k_dict].is_undefined());
	QUARK_ASSERT(interned[(int)base_type::k_function].is_undefined());


//??? Add other common combinations, like vectors with each atomic type, dict with each atomic type.
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

static itype_t make_new_itype_recursive(type_interner_t& interner, const typeid_t& type){
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
			interner.interned.push_back(type);
			const auto lookup_index = static_cast<int32_t>(interner.interned.size() - 1);
			return itype_t::make_struct(lookup_index);
		}
		itype_t operator()(const typeid_t::vector_t& e) const{
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_type(interner, e._parts[0]);

			interner.interned.push_back(type);
			const auto lookup_index = static_cast<int32_t>(interner.interned.size() - 1);
			return itype_t::make_vector(lookup_index, element_type.first.get_base_type());
		}
		itype_t operator()(const typeid_t::dict_t& e) const{
			//	Warning: Need to remember dict_next_id since members can add intern other structs and bump dict_next_id.
			QUARK_ASSERT(e._parts.size() == 1);

			const auto element_type = intern_type(interner, e._parts[0]);

			interner.interned.push_back(type);
			const auto lookup_index = static_cast<int32_t>(interner.interned.size() - 1);
			return itype_t::make_dict(lookup_index, element_type.first.get_base_type());
		}
		itype_t operator()(const typeid_t::function_t& e) const{
			for(const auto& m: e._parts){
				intern_type(interner, m);
			}
			interner.interned.push_back(type);
			const auto lookup_index = static_cast<int32_t>(interner.interned.size() - 1);
			return itype_t::make_function(lookup_index);
		}
/*
		itype_t operator()(const typeid_t::resolved_t& e) const{
			const auto it = std::find_if(interner.lookup_type_name.begin(), interner.lookup_type_name.end(), [&](const auto& m){ return m.first == e._resolved_type_identifier; });
			if(it == interner.lookup_type_name.end()){
				QUARK_ASSERT(false);
				throw std::exception();
			}
			else{
				interner.interned.push_back(type);

				const auto name_index = it - interner.lookup_type_name.begin();
				return itype_t::make_resolved((int)name_index);
			}
		}
*/

	};
	const auto result = std::visit(visitor_t{ interner, type }, type._contents);

	QUARK_ASSERT(interner.check_invariant());

	return result;
}



std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const auto& e){ return e == type; });
	if(it != interner.interned.end()){
		const auto index = it - interner.interned.begin();
		const auto lookup_index = static_cast<int32_t>(index);
		return { make_itype_from_parts(lookup_index, type), type };
	}
	else{
		const auto itype = make_new_itype_recursive(interner, type);
		return { itype, type };
	}
}


itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const auto& e){ return e == type; });
	if(it != interner.interned.end()){
		const auto index = it - interner.interned.begin();
		const auto lookup_index = static_cast<int32_t>(index);
		return make_itype_from_parts(lookup_index, type);
	}
	throw std::exception();
}

#if 0
typeid_t resolve_named_type(const type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_resolved_type_identifier()){
		const auto identifier = type.get_resolved_type_identifer();
		const auto it = std::find_if(interner.lookup_type_name.begin(), interner.lookup_type_name.end(), [&](const auto& m){ return m.first == identifier; });
		if(it == interner.lookup_type_name.end()){
			throw std::exception();
		}

		return lookup_type(interner, it->second);
	}
	else{
		return type;
	}
}
#endif




void trace_type_interner(const type_interner_t& interner){
	{
		QUARK_SCOPED_TRACE("ITYPES");
		for(auto i = 0 ; i < interner.interned.size() ; i++){
			const auto& e = interner.interned[i];
			QUARK_TRACE_SS("itype_t: " << i << "\t=\ttypeid_t: " << typeid_to_compact_string(e));
		}
	}

	{
		QUARK_SCOPED_TRACE("NAMED TYPES");
		for(auto i = 0 ; i < interner.lookup_type_name.size() ; i++){
			const auto& e = interner.lookup_type_name[i];
			const auto t = lookup_type(interner, e.second);
			QUARK_TRACE_SS(
				"name_index: " << i
				<< "\t name: \"" << e.first << "\""
				<< " itype_data: " << e.second.get_data()
				<< " itype_lookup_index: " << e.second.get_lookup_index()
				<< " typeid: " << typeid_to_compact_string(t)
			);
		}
	}
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



} //	floyd
