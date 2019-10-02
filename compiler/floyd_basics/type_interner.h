//
//  type_interner.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef type_interner_hpp
#define type_interner_hpp

#include "types.h"
#include "quark.h"

#include <variant>
#include <vector>
#include <map>

struct json_t;

namespace floyd {



struct member_itype_t;
struct struct_def_itype_t;
struct type_interner_t;
struct itype_t;



#define typeid_t itype_t
#define member_t member_itype_t


typedef int32_t type_lookup_index_t;


enum class return_dyn_type {
	none = 0,
	arg0 = 1,
	arg1 = 2,
	arg1_typeid_constant_type = 3,
	vector_of_arg1func_return = 4,
	vector_of_arg2func_return = 5
};

//??? store itype_variant_t inside type_info_t!

//??? Name into make_x() vs get_x()
itype_t make_struct(type_interner_t& interner, const struct_def_itype_t& def);
itype_t make_struct(const type_interner_t& interner, const struct_def_itype_t& def);
itype_t make_vector(type_interner_t& interner, const itype_t& element_type);
itype_t make_vector(const type_interner_t& interner, const itype_t& element_type);
itype_t make_dict(type_interner_t& interner, const itype_t& value_type);
itype_t make_dict(const type_interner_t& interner, const itype_t& value_type);

itype_t make_function3(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return);

itype_t make_function3(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return);
itype_t make_function_dyn_return(type_interner_t& interner, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return);
itype_t make_function(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure);
itype_t make_function(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure);

itype_t make_identifier(type_interner_t& interner, const std::string& identifier);
itype_t make_identifier_symbol(type_interner_t& interner, const std::string& identifier);
itype_t make_identifier_typetag(type_interner_t& interner, const std::string& identifier);


std::vector<itype_t> get_member_types(const std::vector<member_itype_t>& m);

//////////////////////////////////////////////////		itype_t

//	IMPORTANT: Collect all used types in a vector so we can use itype_t as an index into it for O(1)
/*
	bits 31 - 28 base_type (4 bits)
	bits 27 - 24 base_type 2 (4 bits) -- used to tell basetype of vector element or dictionary value.
	bits 23 - 0 intern_index (24 bits)
*/

struct itype_t {






	bool check_invariant() const {
//		QUARK_ASSERT(get_base_type() != base_type::k_identifier);
		return true;
	}


	//////////////////////////////////////////////////		UNDEFINED


	static itype_t make_undefined(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_undefined, base_type::k_undefined, base_type::k_undefined));
	}

	bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_undefined;
	}


	//////////////////////////////////////////////////		ANY


	static itype_t make_any(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_any, base_type::k_any, base_type::k_undefined));
	}

	bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_any;
	}


	//////////////////////////////////////////////////		VOID


	static itype_t make_void(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_void, base_type::k_void, base_type::k_undefined));
	}

	bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_void;
	}


	//////////////////////////////////////////////////		BOOL

	static itype_t make_bool(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_bool, base_type::k_bool, base_type::k_undefined));
	}

	bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_bool;
	}


	//////////////////////////////////////////////////		INT


	static itype_t make_int(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_int, base_type::k_int, base_type::k_undefined));
	}

	bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_int;
	}


	//////////////////////////////////////////////////		DOUBLE


	static itype_t make_double(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_double, base_type::k_double, base_type::k_undefined));
	}

	bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_double;
	}


	//////////////////////////////////////////////////		STRING


	static itype_t make_string(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_string, base_type::k_string, base_type::k_undefined));
	}

	bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_string;
	}


	//////////////////////////////////////////////////		JSON


	static itype_t make_json(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_json, base_type::k_json, base_type::k_undefined));
	}

	bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_json;
	}

	//////////////////////////////////////////////////		TYPEID


	static itype_t make_typeid(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_typeid, base_type::k_typeid, base_type::k_undefined));
	}

	bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_typeid;
	}



	//////////////////////////////////////////////////		STRUCT


	static itype_t make_struct(
		type_interner_t& interner,
		const struct_def_itype_t& def
	)
	{
		return floyd::make_struct(interner, def);
	}

	static itype_t make_struct2(type_interner_t& interner, const std::vector<member_itype_t>& members);

	bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_struct;
	}

	struct_def_itype_t get_struct(const type_interner_t& interner) const;




	//////////////////////////////////////////////////		VECTOR


	static itype_t make_vector(type_interner_t& interner, const itype_t& element_type){
		return floyd::make_vector(interner, element_type);
	}

	static itype_t make_vector(const type_interner_t& interner, const itype_t& element_type){
		return floyd::make_vector(interner, element_type);
	}

	bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_vector;
	}


	itype_t get_vector_element_type(const type_interner_t& interner) const;

	base_type get_vector_element_basetype() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}











	//////////////////////////////////////////////////		DICT


	static itype_t make_dict(type_interner_t& interner, const itype_t& value_type){
		return floyd::make_dict(interner, value_type);
	}

	bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_dict;
	}

	itype_t get_dict_value_type(const type_interner_t& interner) const;

	base_type get_dict_value_basetype() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}



	//////////////////////////////////////////////////		FUNCTION


	static itype_t make_function3(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return){
		return floyd::make_function3(interner, ret, args, pure, dyn_return);
	}
	
	static itype_t make_function_dyn_return(type_interner_t& interner, const std::vector<itype_t>& args, epure pure, return_dyn_type dyn_return){
		return floyd::make_function_dyn_return(interner, args, pure, dyn_return);
	}

	static itype_t make_function(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure){
		return floyd::make_function(interner, ret, args, pure);
	}
	static itype_t make_function(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure){
		return floyd::make_function(interner, ret, args, pure);
	}


	bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_function;
	}

	public: itype_t get_function_return(const type_interner_t& interner) const;
	public: std::vector<itype_t> get_function_args(const type_interner_t& interner) const;

	public: return_dyn_type get_function_dyn_return_type(const type_interner_t& interner) const;

	public: epure get_function_pure(const type_interner_t& interner) const;





	//////////////////////////////////////////////////		IDENTIFIER

	static itype_t make_identifier(type_interner_t& interner, const std::string& s){
		return floyd::make_identifier(interner, s);
	}

	bool is_identifier() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_identifier;
	}

	std::string get_identifier(const type_interner_t& interner) const;



	//////////////////////////////////////////////////		BASETYPE

	base_type get_base_type() const {
//		QUARK_ASSERT(check_invariant());

		const auto value = (data >> 28) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}




	


	//////////////////////////////////////////////////		INTERNALS



	explicit itype_t(int32_t data) :
		data(data)
	{
	#if DEBUG
		debug_bt0 = get_base_type();
		debug_lookup_index = get_lookup_index();
#endif

		QUARK_ASSERT(check_invariant());
	}

	static itype_t make_struct(type_lookup_index_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_struct, base_type::k_undefined));
	}

	static itype_t make_vector(type_lookup_index_t lookup_index, base_type element_bt){
		return itype_t(assemble(lookup_index, base_type::k_vector, element_bt));
	}
	static itype_t make_dict(type_lookup_index_t lookup_index, base_type value_bt){
		return itype_t(assemble(lookup_index, base_type::k_dict, value_bt));
	}
	static itype_t make_function(type_lookup_index_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_function, base_type::k_undefined));
	}

	static itype_t make_identifier_int(type_lookup_index_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_identifier, base_type::k_undefined));
	}


	inline type_lookup_index_t get_lookup_index() const {
		QUARK_ASSERT(check_invariant());

		return data & 0b00000000'11111111'11111111'11111111;
	}
	int32_t get_data() const {
		QUARK_ASSERT(check_invariant());

		return data;
	}

	static uint32_t assemble(type_lookup_index_t lookup_index, base_type bt1, base_type bt2){
		const auto a = static_cast<uint32_t>(bt1);
		const auto b = static_cast<uint32_t>(bt2);

		return (a << 28) | (b << 24) | lookup_index;
	}

	static itype_t assemble2(type_lookup_index_t lookup_index, base_type bt1, base_type bt2){
		return itype_t(assemble(lookup_index, bt1, bt2));
	}


	////////////////////////////////	STATE

	private: uint32_t data;
#if DEBUG
	private: base_type debug_bt0;
	private: type_lookup_index_t debug_lookup_index;
#endif
};


inline bool operator<(itype_t lhs, itype_t rhs){
	return lhs.get_data() < rhs.get_data();
}
inline bool operator==(itype_t lhs, itype_t rhs){
	return lhs.get_data() == rhs.get_data();
}
inline bool operator!=(itype_t lhs, itype_t rhs){ return (lhs == rhs) == false; };

inline itype_t get_undefined_itype(){
	return itype_t::make_undefined();
}
inline itype_t get_json_itype(){
	return itype_t::make_json();
}

inline bool is_empty(const itype_t& type){
	return type == itype_t::make_undefined();
}

bool is_atomic_type(itype_t type);


json_t itype_to_json_shallow(const itype_t& itype);
itype_t itype_from_json_shallow(const json_t& j);

json_t itype_to_json(const type_interner_t& interner, const itype_t& type);
itype_t itype_from_json(type_interner_t& interner, const json_t& j);


std::string itype_to_debug_string(const itype_t& itype);

std::string itype_to_compact_string(const type_interner_t& interner, const itype_t& itype);


struct member_itype_t {
	member_itype_t(itype_t type, const std::string& name) :
		_type(type),
		_name(name)
	{
	}

	itype_t _type;
	std::string _name;
};

inline bool operator==(const member_itype_t& lhs, const member_itype_t& rhs){
	return lhs._name == rhs._name
	&& lhs._type == rhs._type;
}

struct struct_def_itype_t {
	struct_def_itype_t(const std::vector<member_itype_t>& members) :
		_members(members)
	{
		QUARK_ASSERT(check_invariant());
	}
	struct_def_itype_t(){
		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const {
		return true;
	}

	std::vector<member_itype_t> _members;
};

inline bool operator==(const struct_def_itype_t& lhs, const struct_def_itype_t& rhs){
	return lhs._members == rhs._members;
}

int find_struct_member_index(const struct_def_itype_t& def, const std::string& name);

json_t members_to_json(const type_interner_t& interner, const std::vector<member_itype_t>& members);
std::vector<member_itype_t> members_from_json(type_interner_t& interner, const json_t& members);




//////////////////////////////////////////////////		type_node_t



//	Assigns 32 bit ID to types. You can lookup the type using the ID.
//	This allows us to describe a type using a single 32 bit integer (compact, fast to copy around).
//	Each type has exactly ONE ID.
//	Automatically insert all basetype-types so they ALWAYS have EXPLICIT integer IDs as itypes.

struct type_node_t {
	//	If optional_tag is used, this node is a tag node and child_type_indexes[0] will be undefined or hold the real type.
	type_tag_t optional_tag;

	base_type bt;
	std::vector<itype_t> child_types;


	struct_def_itype_t struct_def;

	//	Only used when bt == k_function.
	epure func_pure;

	//	Only used when bt == k_function.
	//??? I think we can lose this field now that we have intrinsics handling in semast.
	return_dyn_type func_return_dyn_type;

	std::string identifier_str;
};

inline bool operator==(const type_node_t& lhs, const type_node_t& rhs){
	return
		lhs.optional_tag == rhs.optional_tag
		&& lhs.bt == rhs.bt
		&& lhs.child_types == rhs.child_types

		&& lhs.struct_def == rhs.struct_def
		&& lhs.func_pure == rhs.func_pure
		&& lhs.func_return_dyn_type == rhs.func_return_dyn_type
		&& lhs.identifier_str == rhs.identifier_str
		;
}



//////////////////////////////////////////////////		type_interner_t



struct type_interner_t {
	type_interner_t();

	bool check_invariant() const;


	////////////////////////////////	STATE

	//	All types are recorded here, an uniqued. Including tagged types.
	//	itype uses the INDEX into this array for fast lookups.
	std::vector<type_node_t> interned2;
};


//??? dummy
inline itype_t intern_anonymous_type(type_interner_t& interner, const itype_t& type){
	return type;
}


//itype_t lookup_itype_from_typeid(const type_interner_t& interner, const typeid_t& type);
//typeid_t lookup_typeid_from_itype(const type_interner_t& interner, const itype_t& type);
const type_node_t& lookup_typeinfo_from_itype(const type_interner_t& interner, const itype_t& type);
type_node_t& lookup_typeinfo_from_itype(type_interner_t& interner, const itype_t& type);
itype_t lookup_itype_from_tagged_type(const type_interner_t& interner, const type_tag_t& tag);

itype_t lookup_itype_from_index(const type_interner_t& interner, type_lookup_index_t type_index);

void trace_type_interner(const type_interner_t& interner);



itype_t peek(const type_interner_t& interner, const itype_t& type);

itype_t refresh_itype(const type_interner_t& interner, const itype_t& type);



json_t type_interner_to_json(const type_interner_t& interner);
type_interner_t type_interner_from_json(const json_t& j);

inline itype_t to_asttype(const itype_t& a){
	return a;
}


//////////////////////////////////////////////////		TAGGED TYPES


//	Allocates a new itype for this tag. The tag must not already exist.
//	Interns the type for this tag. You can use typeid_t::make_undefined() and
//	later update the type using update_tagged_type()
itype_t new_tagged_type(type_interner_t& interner, const type_tag_t& tag);
itype_t new_tagged_type(type_interner_t& interner, const type_tag_t& tag, const itype_t& type);

//	Update the tagged type's type. The tagged type must already exist. Any usage of this
//	tag will also get the new type.
itype_t update_tagged_type(type_interner_t& interner, const itype_t& named, const itype_t& type);

itype_t get_tagged_type2(const type_interner_t& interner, const type_tag_t& tag);





struct undefined_t {};
struct any_t {};
struct void_t {};
struct bool_t {};
struct int_t {};
struct double_t {};
struct string_t {};
struct json_type_t {};
struct typeid_type_t {};

struct struct_t {
	struct_def_itype_t def;
};
struct vector_t {
	std::vector<itype_t> _parts;
};
struct dict_t {
	std::vector<itype_t> _parts;
};
struct function_t {
	std::vector<itype_t> _parts;
};
struct identifier_t {
	std::string s;
};

typedef std::variant<
	undefined_t,
	any_t,
	void_t,
	bool_t,
	int_t,
	double_t,
	string_t,
	json_type_t,
	typeid_type_t,

	struct_t,
	vector_t,
	dict_t,
	function_t,
	identifier_t
> itype_variant_t;


itype_variant_t get_itype_variant(const type_interner_t& interner, const itype_t& type);



}	//	floyd

#endif /* type_interner_hpp */
