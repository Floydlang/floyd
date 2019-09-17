//
//  type_interner.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef type_interner_hpp
#define type_interner_hpp

#include "typeid.h"
#include "quark.h"

#include <vector>
#include <map>

struct json_t;

namespace floyd {


//////////////////////////////////////////////////		itype_t

//	IMPORTANT: Collect all used types in a vector so we can use itype_t as an index into it for O(1)
/*
	bits 31 - 28 base_type (4 bits)
	bits 27 - 24 base_type 2 (4 bits) -- used to tell basetype of vector element or dictionary value.
	bits 23 - 0 intern_index (24 bits)
*/

struct itype_t {
	explicit itype_t(int32_t data) :
		data(data)
	{
		QUARK_ASSERT(check_invariant());
	}

	static itype_t make_undefined(){
		return itype_t(assemble((int)base_type::k_undefined, base_type::k_undefined, base_type::k_undefined));
	}
	static itype_t make_any(){
		return itype_t(assemble((int)base_type::k_any, base_type::k_any, base_type::k_undefined));
	}
	static itype_t make_void(){
		return itype_t(assemble((int)base_type::k_void, base_type::k_void, base_type::k_undefined));
	}
	static itype_t make_bool(){
		return itype_t(assemble((int)base_type::k_bool, base_type::k_bool, base_type::k_undefined));
	}
	static itype_t make_int(){
		return itype_t(assemble((int)base_type::k_int, base_type::k_int, base_type::k_undefined));
	}
	static itype_t make_double(){
		return itype_t(assemble((int)base_type::k_double, base_type::k_double, base_type::k_undefined));
	}
	static itype_t make_string(){
		return itype_t(assemble((int)base_type::k_string, base_type::k_string, base_type::k_undefined));
	}
	static itype_t make_json(){
		return itype_t(assemble((int)base_type::k_json, base_type::k_json, base_type::k_undefined));
	}

	static itype_t make_typeid(){
		return itype_t(assemble((int)base_type::k_typeid, base_type::k_typeid, base_type::k_undefined));
	}

	static itype_t make_struct(uint32_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_struct, base_type::k_undefined));
	}

	static itype_t make_vector(uint32_t lookup_index, base_type element_bt){
		return itype_t(assemble(lookup_index, base_type::k_vector, element_bt));
	}
	static itype_t make_dict(uint32_t lookup_index, base_type value_bt){
		return itype_t(assemble(lookup_index, base_type::k_dict, value_bt));
	}
	static itype_t make_function(uint32_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_function, base_type::k_undefined));
	}

	bool check_invariant() const {
		return true;
	}




	bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_string;
	}


	bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_any;
	}
	bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_void;
	}
	bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_bool;
	}
	bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_int;
	}
	bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_double;
	}


	bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_string;
	}
	bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_json;
	}

	bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_typeid;
	}



	bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_struct;
	}
	bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_vector;
	}
	bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_dict;
	}

	bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_function;
	}




	base_type get_base_type() const {
		QUARK_ASSERT(check_invariant());

		const auto value = (data >> 28) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}

	base_type get_vector_element_type() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}
	base_type get_dict_value_type() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}

	inline uint32_t get_lookup_index() const {
		QUARK_ASSERT(check_invariant());

		return data & 0b00000000'11111111'11111111'11111111;
	}
	int32_t get_data() const {
		QUARK_ASSERT(check_invariant());

		return data;
	}

	static uint32_t assemble(uint32_t lookup_index, base_type bt1, base_type bt2){
		const auto a = static_cast<uint32_t>(bt1);
		const auto b = static_cast<uint32_t>(bt2);

		return (a << 28) | (b << 24) | lookup_index;
	}

	static itype_t assemble2(uint32_t lookup_index, base_type bt1, base_type bt2){
		return itype_t(assemble(lookup_index, bt1, bt2));
	}

	////////////////////////////////	STATE

	private: uint32_t data;
};

inline bool operator<(itype_t lhs, itype_t rhs){
	return lhs.get_data() < rhs.get_data();
}
inline bool operator==(itype_t lhs, itype_t rhs){
	return lhs.get_data() == rhs.get_data();
}

inline itype_t get_undefined_itype(){
	return itype_t::make_undefined();
}
inline itype_t get_json_itype(){
	return itype_t::make_json();
}




//////////////////////////////////////////////////		type_interner_t




//	Assigns 32 bit ID to types. You can lookup the type using the ID.
//	This allows us to describe a type using a single 32 bit integer (compact, fast to copy around).
//	Each type has exactly ONE ID.

// Automatically insert all basetype-types so they ALWAYS have EXPLICIT integer IDs as itypes.

struct type_interner_t {
	type_interner_t();

	bool check_invariant() const;


	////////////////////////////////	STATE

	//	All types are recorded here, an uniqued. Including named types.
	//	itype uses the INDEX into this array for fast lookups.
	std::vector<std::pair<type_name_t, typeid_t>> interned2;
};





//	Records AND resolves the type. The returned type may be improved over input type.
std::pair<itype_t, typeid_t> intern_type2(type_interner_t& interner, const typeid_t& type);
std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const type_name_t& name, const typeid_t& type);



itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type);
inline const typeid_t& lookup_type(const type_interner_t& interner, const itype_t& type);
const typeid_t& lookup_type(const type_interner_t& interner, const type_name_t& name);

//	Returns true if the type is completely described with no subnodes that are undefined.
bool is_resolved(const type_interner_t& interner, const type_name_t& t);


void trace_type_interner(const type_interner_t& interner);

inline bool is_atomic_type(itype_t type);



//////////////////////////////////////////////////		INLINES



//	Used at runtime.
inline const typeid_t& lookup_type(const type_interner_t& interner, const itype_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned2.size());

	const auto& result = interner.interned2[lookup_index].second;
	return result;
}

inline bool is_atomic_type(itype_t type){
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
		|| bt == base_type::k_unresolved_identifier
	){
		return true;
	}
	else{
		return false;
	}
}



/*
Do this without using typeid_t at all.
inline itype_t get_vector_element_type_quick(const type_interner_t& interner, itype_t vec){
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(vec.is_vector());

	const auto lookup_index = type.get_lookup_index();
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(lookup_index < interner.interned.size());

	const auto& result = interner.interned[lookup_index];
	return result;


	const auto value = (data >> 24) & 15;
	const auto bt = static_cast<base_type>(value);
	return bt;
}

inline itype_t get_dict_value_type(const type_interner_t& interner, itype_t vec) const {
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(vec.is_vector());

	const auto value = (data >> 24) & 15;
	const auto bt = static_cast<base_type>(value);
	return bt;
}
*/


}	//	floyd

#endif /* type_interner_hpp */
