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

#include <variant>
#include <vector>
#include <map>

struct json_t;

namespace floyd {




struct member_itype_t;
struct struct_def_itype_t;
struct type_interner_t;



typedef int32_t type_lookup_index_t;

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

#if DEBUG
	debug_bt0 = get_base_type();
	debug_lookup_index = get_lookup_index();
#endif


		QUARK_ASSERT(check_invariant());
	}

	static itype_t make_undefined(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_undefined, base_type::k_undefined, base_type::k_undefined));
	}
	static itype_t make_any(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_any, base_type::k_any, base_type::k_undefined));
	}
	static itype_t make_void(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_void, base_type::k_void, base_type::k_undefined));
	}
	static itype_t make_bool(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_bool, base_type::k_bool, base_type::k_undefined));
	}
	static itype_t make_int(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_int, base_type::k_int, base_type::k_undefined));
	}
	static itype_t make_double(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_double, base_type::k_double, base_type::k_undefined));
	}
	static itype_t make_string(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_string, base_type::k_string, base_type::k_undefined));
	}
	static itype_t make_json(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_json, base_type::k_json, base_type::k_undefined));
	}

	static itype_t make_typeid(){
		return itype_t(assemble((type_lookup_index_t)base_type::k_typeid, base_type::k_typeid, base_type::k_undefined));
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

	static itype_t make_identifier(type_lookup_index_t lookup_index){
		return itype_t(assemble(lookup_index, base_type::k_identifier, base_type::k_undefined));
	}



	bool check_invariant() const {
//		QUARK_ASSERT(get_base_type() != base_type::k_identifier);
		return true;
	}


	bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_undefined;
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

	struct_def_itype_t get_struct(const type_interner_t& interner) const;



	bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_vector;
	}


	itype_t get_vector_element_type(const type_interner_t& interner) const;




	bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_dict;
	}

	itype_t get_dict_value_type(const type_interner_t& interner) const;




	bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_function;
	}

	public: itype_t get_function_return(const type_interner_t& interner) const;
	public: std::vector<itype_t> get_function_args(const type_interner_t& interner) const;

	public: typeid_t::return_dyn_type get_function_dyn_return_type(const type_interner_t& interner) const;

	public: epure get_function_pure(const type_interner_t& interner) const;




	bool is_identifier() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_identifier;
	}




	base_type get_base_type() const {
//		QUARK_ASSERT(check_invariant());

		const auto value = (data >> 28) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}

	base_type get_vector_element_basetype() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}
	base_type get_dict_value_basetype() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(is_vector());

		const auto value = (data >> 24) & 15;
		const auto bt = static_cast<base_type>(value);
		return bt;
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


json_t itype_to_json(const itype_t& itype);
itype_t itype_from_json(const json_t& j);


std::string itype_to_debug_string(const itype_t& itype);

std::string typeid_to_compact_string(const itype_t& itype);


struct member_itype_t {
	itype_t _type;
	std::string _name;
};

inline bool operator==(const member_itype_t& lhs, const member_itype_t& rhs){
	return lhs._name == rhs._name
	&& lhs._type == rhs._type;
}

struct struct_def_itype_t {
	std::vector<member_itype_t> _members;
};

inline bool operator==(const struct_def_itype_t& lhs, const struct_def_itype_t& rhs){
	return lhs._members == rhs._members;
}

int find_struct_member_index(const struct_def_itype_t& def, const std::string& name);





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
	typeid_t::return_dyn_type func_return_dyn_type;

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


itype_t intern_anonymous_type(type_interner_t& interner, const typeid_t& type);

itype_t make_struct(type_interner_t& interner, const struct_def_itype_t& struct_def);
itype_t make_vector(type_interner_t& interner, const itype_t& element_type);
itype_t make_vector(const type_interner_t& interner, const itype_t& element_type);
itype_t make_dict(type_interner_t& interner, const itype_t& value_type);
itype_t make_function3(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, typeid_t::return_dyn_type dyn_return);

itype_t make_function3(const type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure, typeid_t::return_dyn_type dyn_return);

itype_t make_function_dyn_return(type_interner_t& interner, const std::vector<itype_t>& args, epure pure, typeid_t::return_dyn_type dyn_return);
itype_t make_function(type_interner_t& interner, const itype_t& ret, const std::vector<itype_t>& args, epure pure);

itype_t make_identifier(type_interner_t& interner, const std::string& identifier);
itype_t make_identifier_symbol(type_interner_t& interner, const std::string& identifier);
itype_t make_identifier_typetag(type_interner_t& interner, const std::string& identifier);



itype_t lookup_itype_from_typeid(const type_interner_t& interner, const typeid_t& type);
typeid_t lookup_typeid_from_itype(const type_interner_t& interner, const itype_t& type);
const type_node_t& lookup_typeinfo_from_itype(const type_interner_t& interner, const itype_t& type);
type_node_t& lookup_typeinfo_from_itype(type_interner_t& interner, const itype_t& type);
itype_t lookup_itype_from_tagged_type(const type_interner_t& interner, const type_tag_t& tag);

itype_t lookup_itype_from_index(const type_interner_t& interner, type_lookup_index_t type_index);

void trace_type_interner(const type_interner_t& interner);


//	Makes the type concrete by expanding any indirections via identifiers.
typeid_t flatten_type_description_deep(const type_interner_t& interner, const itype_t& type);
typeid_t flatten_type_description1(const type_interner_t& interner, const itype_t& type);


itype_t peek(const type_interner_t& interner, const itype_t& type);

itype_t refresh_itype(const type_interner_t& interner, const itype_t& type);



json_t type_interner_to_json(const type_interner_t& interner);
type_interner_t type_interner_from_json(const json_t& j);




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

struct struct_t {};
struct vector_t {};
struct dict_t {};
struct function_t {};
struct identifier_t {};

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


itype_variant_t get_itype_variant(const itype_t& type);







//////////////////////////////////////////////////		ast_type_t

/*
	This is how the C++ AST structs define a Floyd type.

	It can be
	1. A type description using optional_typeid, including undefined and unresolved_identifier.
	2. An itype into the type_interner.

	Notice that both 1 & 2 may contain subtypes that are undefined or uses unresolved_identifiers.

	ast_type_t is built ontop of the type_interner and itype.
*/

struct ast_type_t;
ast_type_t to_asttype(const typeid_t& t);
ast_type_t to_asttype(const itype_t& t);

struct ast_type_t {
	bool check_invariant() const {
		return true;
	}

	inline bool is_undefined() const ;

	static ast_type_t make_undefined() {
		return to_asttype(typeid_t::make_undefined());
	}

	static ast_type_t make_bool() {
		return to_asttype(typeid_t::make_bool());
	}
	static ast_type_t make_int() {
		return to_asttype(typeid_t::make_int());
	}
	static ast_type_t make_double() {
		return to_asttype(typeid_t::make_double());
	}
	static ast_type_t make_string() {
		return to_asttype(typeid_t::make_string());
	}
	static ast_type_t make_json() {
		return to_asttype(typeid_t::make_json());
	}


	////////////////////////////////////////		STATE

	typedef std::variant<
		std::monostate,
		typeid_t,
		itype_t
	> type_variant_t;

	type_variant_t _contents;
};

inline typeid_t get_typeid(const ast_type_t& type){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(std::holds_alternative<typeid_t>(type._contents));

	return std::get<typeid_t>(type._contents);
}
inline itype_t get_itype(const ast_type_t& type){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(std::holds_alternative<itype_t>(type._contents));

	return std::get<itype_t>(type._contents);
}

inline ast_type_t make_no_asttype(){
	return { std::monostate() };
}
inline bool is_empty(const ast_type_t& name){
	return std::holds_alternative<std::monostate>(name._contents);
}

inline bool is_itype(const ast_type_t& name){
	return std::holds_alternative<itype_t>(name._contents);
}

inline bool operator==(const ast_type_t& lhs, const ast_type_t& rhs){
	return lhs._contents == rhs._contents;
}

inline bool ast_type_t::is_undefined() const {
	return (*this) == make_undefined();
}

json_t ast_type_to_json(const ast_type_t& name);
ast_type_t ast_type_from_json(const json_t& j);

std::string ast_type_to_string(const ast_type_t& type);


itype_t intern_anonymous_type(type_interner_t& interner, const ast_type_t& type);

//	Returns typeid_t::make_undefined() if ast_type_t is in monostate mode
itype_t lookup_itype_from_asttype(const type_interner_t& interner, const ast_type_t& type);



}	//	floyd

#endif /* type_interner_hpp */
