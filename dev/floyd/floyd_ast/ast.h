//
//  parser_ast.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_ast_hpp
#define parser_ast_hpp

#include "statement.h"
#include "software_system.h"

#include "quark.h"

#include <vector>

struct json_t;

namespace floyd {


//////////////////////////////////////////////////		itype_t


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
	static itype_t make_unresolved(){
		return itype_t(assemble((int)base_type::k_unresolved, base_type::k_unresolved, base_type::k_undefined));
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

	bool is_unresolved() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_unresolved;
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

	uint32_t get_lookup_index() const {
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
	std::vector<typeid_t> interned;
};


std::pair<itype_t, typeid_t> intern_type(type_interner_t& interner, const typeid_t& type);
itype_t lookup_itype(const type_interner_t& interner, const typeid_t& type);
typeid_t lookup_type(const type_interner_t& interner, const itype_t& type);

void trace_type_interner(const type_interner_t& interner);



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
		|| bt == base_type::k_unresolved
	){
		return true;
	}
	else{
		return false;
	}
}




//////////////////////////////////////////////////		general_purpose_ast_t


struct general_purpose_ast_t {
	public: bool check_invariant() const{
		QUARK_ASSERT(_globals.check_invariant());
		return true;
	}

	/////////////////////////////		STATE
	public: body_t _globals;
	public: std::vector<floyd::function_definition_t> _function_defs;
	public: type_interner_t _interned_types;
	public: software_system_t _software_system;
	public: container_t _container_def;
};




json_t gp_ast_to_json(const general_purpose_ast_t& ast);
general_purpose_ast_t json_to_gp_ast(const json_t& json);




//////////////////////////////////////////////////		unchecked_ast_t

/*
	An AST that may contain unresolved symbols.
	It can optionally be annotated with all expression types OR NOT.
	Immutable
*/

struct unchecked_ast_t {
	public: bool check_invariant() const{
		QUARK_ASSERT(_tree.check_invariant());
		return true;
	}

	/////////////////////////////		STATE
	public: general_purpose_ast_t _tree;
};

}	//	floyd

#endif /* parser_ast_hpp */
