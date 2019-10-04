//
//  types.h
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef types_h
#define types_h

#include "utils.h"
#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include <variant>


struct json_t;

namespace floyd {


/*
	typeid_t

	This is a very central type in the Floyd compiler.

	It specifies an exact Floyd type. Both for base types like "int" and "string" and composite types
	like "struct { [float] p; string s }.
	It can hold *any Floyd type*. It can also hold unresolved type identifiers and a few types internal to compiler.

	typeid_t can be convert to/from JSON and is written in source code according to Floyd source syntax, see table below.

	typeid_t is an immutable value object.
	The typeid_t is normalized and can be compared with other typeid_t:s.

	Composite types can form trees of types,
		like:
			"[string: struct {int x; int y}]"
		This is a dictionary with structs, each holding two integers.

	Source code						base_type								AST JSON
	================================================================================================================
	null							k_undefined					"null"
	bool							k_bool									"bool"
	int								k_int									"int"
	double							k_double								"double"
	string							k_string								"string"
	json						k_json							"json"
	"typeid"						k_typeid								"typeid"
	struct red { int x;float y}		k_struct								["struct", [{"type": "in", "name": "x"}, {"type": "float", "name": "y"}]]
	[int]							k_vector								["vector", "int"]
	[string: int]					k_dict									["dict", "int"]
	int ()							k_function								["function", "int", []]
	int (double, [string])			k_function								["function", "int", ["double", ["vector", "string"]]]
	randomize_player			k_identifier		["identifier", "randomize_player"]


	AST JSON
	This is the JSON format we use to pass AST around. Use typeid_to_ast_json() and typeid_from_ast_json().

	COMPACT_STRING
	This is a nice user-visible representation of the typeid_t. It may be lossy. It's for REPLs etc. UI.

	SOURCE CODE TYPE
	Use read_type(), read_required_type()
*/




struct member_t;
struct struct_def_itype_t;
struct type_interner_t;
struct type_t;


#define typeid_t type_t
#define itype_t type_t




//std::string typeid_to_compact_string(const typeid_t& t);



#define DEBUG_DEEP_TYPEID_T 1

//////////////////////////////////////		base_type

/*
	The atomic building block of all types.
	Some of the types are ready as-is, like bool or double.
	Some types needs further information to be 100% defined, like struct (needs its members), vector needs its element-type.
*/

enum class base_type {
	//	k_undefined is never exposed in code, only used internally in compiler.
	k_undefined = 0,

	//	Used as function return/args to tell this is a dynamic value, not static type.
	k_any = 1,

	//	Means no value. Used as return type for print() etc.
	k_void = 2,

	k_bool = 3,
	k_int = 4,
	k_double = 5,
	k_string = 6,
	k_json = 7,

	//	This is a type that specifies any other type at runtime.
	k_typeid = 8,

	k_struct = 9,
	k_vector = 10,
	k_dict = 11,
	k_function = 12,

	k_symbol_ref = 13,
	k_named_type = 14
};

std::string base_type_to_opcode(const base_type t);
base_type opcode_to_base_type(const std::string& s);

void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected);


//	Is this type final and has no variations? True for int. False for struct or vector because those needs more definition.
inline bool is_atomic_type(base_type type){
	return false
		|| type == base_type::k_undefined

		|| type == base_type::k_any

		|| type == base_type::k_void

		|| type == base_type::k_bool
		|| type == base_type::k_int
		|| type == base_type::k_double
		|| type == base_type::k_string
		|| type == base_type::k_json

		|| type == base_type::k_typeid

//		|| type == base_type::k_struct
//		|| type == base_type::k_vector
//		|| type == base_type::k_dict
//		|| type == base_type::k_function
	;
}


//////////////////////////////////////////////////		get_json_type()


int get_json_type(const json_t& value);


//??? This struct should be *separate* from the actual struct_definition_t. Rename struct_type_description_t



enum class epure {
	pure,
	impure
};



//////////////////////////////////////		type_tag_t


//	Internal name that uniquely names a type in a program. Used for name-equivalence.

struct type_tag_t {
	bool check_invariant() const {
//		QUARK_ASSERT(lexical_path.size() > 0);
		return true;
	}

	std::vector<std::string> lexical_path;
};


inline bool operator==(const type_tag_t& lhs, const type_tag_t& rhs){
	return lhs.lexical_path == rhs.lexical_path;
}
inline bool operator<(const type_tag_t& lhs, const type_tag_t& rhs){
	return lhs.lexical_path < rhs.lexical_path;
}

//	A type tag is a unique string that names a type that should only type-equivalent to itself, no other types.
std::string pack_type_tag(const type_tag_t& path);
type_tag_t unpack_type_tag(const std::string& tag);
bool is_type_tag(const std::string& s);

type_tag_t make_empty_type_tag();
inline bool is_empty_type_tag(const type_tag_t& tag){
	return tag.lexical_path.empty();
}



typedef int32_t type_lookup_index_t;



//??? Should not need to store dyn-type inside function types anymore, confirm and remove from here.
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

itype_t make_symbol_ref(type_interner_t& interner, const std::string& s);
itype_t make_named_type(type_interner_t& interner, const type_tag_t& type);


std::vector<itype_t> get_member_types(const std::vector<member_t>& m);



//////////////////////////////////////////////////		itype_t


//	IMPORTANT: Collect all used types in a vector so we can use itype_t as an index into it for O(1)
/*
	A: 0b11110000'00000000'00000000'00000000;
	B: 0b00001111'00000000'00000000'00000000;
	C: 0b00000000'11111111'11111111'11111111;

	A: bits 31 - 28 base_type (4 bits)
	B: bits 27 - 24 base_type 2 (4 bits) -- used to tell basetype of vector element or dictionary value.
	C: bits 23 - 0 intern_index (24 bits)

	4294967296
	0-41 = basetype * 1.000.000.000

	Decimal AA BB CCCCC
	(bt0 * 100 + bt1) * 10.000.000
*/

struct type_t {
	itype_t() :
		itype_t(assemble((type_lookup_index_t)base_type::k_undefined, base_type::k_undefined, base_type::k_undefined))
	{
		QUARK_ASSERT(check_invariant());
	}


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

	static itype_t make_struct2(type_interner_t& interner, const std::vector<member_t>& members);

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

		return get_bt1(data);
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

		return get_bt1(data);
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


	//////////////////////////////////////////////////		SYMBOL


	static itype_t make_symbol_ref(type_interner_t& interner, const std::string& s){
		return floyd::make_symbol_ref(interner, s);
	}

	bool is_symbol_ref() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_symbol_ref;
	}

	std::string get_symbol_ref(const type_interner_t& interner) const;


	//////////////////////////////////////////////////		NAMED TYPE


	static itype_t make_named_type(type_interner_t& interner, const type_tag_t& type){
		return floyd::make_named_type(interner, type);
	}

	bool is_named_type() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_named_type;
	}

	type_tag_t get_named_type(const type_interner_t& interner) const;


	//////////////////////////////////////////////////		BASETYPE


	base_type get_base_type() const {
//		QUARK_ASSERT(check_invariant());

		return get_bt0(data);
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

	inline type_lookup_index_t get_lookup_index() const {
		QUARK_ASSERT(check_invariant());

		return get_index(data);
	}

	int32_t get_data() const {
		QUARK_ASSERT(check_invariant());

		return data;
	}

	public: static itype_t assemble2(type_lookup_index_t lookup_index, base_type bt1, base_type bt2){
		return itype_t(assemble(lookup_index, bt1, bt2));
	}


	//////////////////////////////////////////////////		BIT MANGLING


#if 0
	public: static uint32_t assemble(type_lookup_index_t lookup_index, base_type bt1, base_type bt2){
		const auto a = static_cast<uint32_t>(bt1);
		const auto b = static_cast<uint32_t>(bt2);

		return (a << 28) | (b << 24) | lookup_index;
	}

	private: inline static type_lookup_index_t get_index(uint32_t data) {
		return data & 0b00000000'11111111'11111111'11111111;
	}
	private: inline static base_type get_bt0(uint32_t data){
		const auto value = (data >> 28) & 0x0f;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}
	private: inline static base_type get_bt1(uint32_t data){
		const auto value = (data >> 24) & 0x0f;
		const auto bt = static_cast<base_type>(value);
		return bt;
	}
#else
	public: static uint32_t assemble(type_lookup_index_t lookup_index, base_type bt1, base_type bt2){
		const auto a = static_cast<uint32_t>(bt1);
		const auto b = static_cast<uint32_t>(bt2);
		return a * 100 * 10000 + b * 10000 + lookup_index;
	}

	private: inline static type_lookup_index_t get_index(uint32_t data) {
		return data % 10000;
	}
	private: inline static base_type get_bt0(uint32_t data){
		const uint32_t result = data / (100 * 10000);
		return static_cast<base_type>(result);
	}
	private: inline static base_type get_bt1(uint32_t data){
		const uint32_t result = (data / 10000) % 100;
		return static_cast<base_type>(result);
	}
#endif


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

enum class resolve_named_types { resolve, dont_resolve };

std::string itype_to_compact_string(const type_interner_t& interner, const itype_t& itype, resolve_named_types resolve = resolve_named_types::dont_resolve);



//////////////////////////////////////////////////		member_t



struct member_t {
	member_t(itype_t type, const std::string& name) :
		_type(type),
		_name(name)
	{
	}

	itype_t _type;
	std::string _name;
};

inline bool operator==(const member_t& lhs, const member_t& rhs){
	return lhs._name == rhs._name
	&& lhs._type == rhs._type;
}



//////////////////////////////////////////////////		struct_def_itype_t



struct struct_def_itype_t {
	struct_def_itype_t(const std::vector<member_t>& members) :
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

	std::vector<member_t> _members;
};

inline bool operator==(const struct_def_itype_t& lhs, const struct_def_itype_t& rhs){
	return lhs._members == rhs._members;
}

int find_struct_member_index(const struct_def_itype_t& def, const std::string& name);

json_t members_to_json(const type_interner_t& interner, const std::vector<member_t>& members);
std::vector<member_t> members_from_json(type_interner_t& interner, const json_t& members);



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


const type_node_t& lookup_typeinfo_from_itype(const type_interner_t& interner, const itype_t& type);
type_node_t& lookup_typeinfo_from_itype(type_interner_t& interner, const itype_t& type);
itype_t lookup_itype_from_tagged_type(const type_interner_t& interner, const type_tag_t& tag);

itype_t lookup_itype_from_index(const type_interner_t& interner, type_lookup_index_t type_index);

void trace_type_interner(const type_interner_t& interner);



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



//////////////////////////////////////////////////		get_itype_variant()



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
struct symbol_ref_t {
	std::string s;
};
struct named_type_t {
	itype_t destination_type;
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
	symbol_ref_t,
	named_type_t
> itype_variant_t;


itype_variant_t get_itype_variant(const type_interner_t& interner, const itype_t& type);



}	// floyd

#endif /* types_h */
