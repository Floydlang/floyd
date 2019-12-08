//
//  types.h
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef types_h
#define types_h

/*
These features are very central in the Floyd compiler.
base_type, type_t, type_desc_t and types_t works together to describe, compare and pick apart data types.

They can describe types as you get them from source code with undefined types. vector<undefined> is OK for example.
They can also refer to types using their symbol name in the source code / lexical scope.

Later passes can improve the types gradually to resolve symbols, infer left out (undefined) types and so on.


# base_type
Enum with all basic types of types in Floyd. These are only the roots: you need to use type_t to fully describe a composite data structure.


# type_t -- LOGICAL TYPE
Opaque, immutable value that specifies ANY type. It may be a reference to a named type (like "/main/pixel_t") or a concrete type, like "int".
You can compare and copy these but you cannot access their contents.

- It can hold unresolved symbols (ties to a lexical scope in source code).
- It can hold the name of a named type, like "pixel_t"
- The type_t is normalized and can be compared with other type_t:s.


# type_desc_t
Always holds a concrete type, never a named type. Notice that child-types can be named types.
You can query its type, get function arguments, get struct members etc.

??? CAN CHILD TYPES BE NAMED?


# types_t
Used to hold all data types used in the program.
Composite data types are split to their individual parts.
Each data type gets its own unique lookup index that is great to use at runtime.
Types are automatically de-duplicated.
Previous nodes / lookup indexes are never modified and safe to store.


# Symbols
These needs to be resolved in semantic analysis since there depend on the lexical scope where it is used. "x" will mean
different types depending on *where* you access "x".


# Named types
Named types can be re-route after they have been stored into types_t. The name must be unique in
the entire program. You can encode the name of the lexical scopes where the type is defined in its
name, which makes it unique.

Named types allows to support recursive types, like

	struct object_t {
		string name
		[object_t] inside
	}


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
This is a nice user-visible representation of the type_t. It may be lossy. It's for REPLs etc. UI.
*/

#define DEBUG_DEEP_TYPEID_T 1


#include "utils.h"
#include "quark.h"

#include <string>
#include <vector>
#include <variant>

struct json_t;

namespace floyd {

struct member_t;
struct struct_type_desc_t;
struct types_t;
struct type_t;
struct type_desc_t;


////////////////////////////////	base_type

/*
	The atomic building block of all types.
	Some of the types are ready as-is, like bool or double.
	Some types needs further information to be 100% defined, like struct (needs its members),
	vector needs its element-type.
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

void ut_verify_basetype(const quark::call_context_t& context, const base_type& result, const base_type& expected);


//	Is this type final and has no variations? True for int. False for struct or vector because
//	those needs more definition.
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
		|| type == base_type::k_symbol_ref

//		|| type == base_type::k_struct
//		|| type == base_type::k_vector
//		|| type == base_type::k_dict
//		|| type == base_type::k_function
	;
}

////////////////////////////////	get_json_type()


int get_json_type(const json_t& value);


////////////////////////////////	function type


//	Functions are statically defined as pure or unpure.
enum class epure {
	pure,
	impure
};

enum class return_dyn_type {
	none = 0,
	arg0 = 1,
	arg1 = 2,
	arg1_typeid_constant_type = 3,
	vector_of_arg1func_return = 4,
	vector_of_arg2func_return = 5
};


////////////////////////////////	type_name_t


//	A type name is a unique string that names a type that should only type-equivalent to itself,
//	no other types.

struct type_name_t {
	bool check_invariant() const {
//		QUARK_ASSERT(lexical_path.size() > 0);
		return true;
	}

	std::vector<std::string> lexical_path;
};

inline bool operator==(const type_name_t& lhs, const type_name_t& rhs){
	return lhs.lexical_path == rhs.lexical_path;
}
inline bool operator<(const type_name_t& lhs, const type_name_t& rhs){
	return lhs.lexical_path < rhs.lexical_path;
}

std::string pack_type_name(const type_name_t& path);
type_name_t unpack_type_name(const std::string& s);
bool is_type_name(const std::string& s);

type_name_t make_empty_type_name();
inline bool is_empty_type_name(const type_name_t& n){
	return n.lexical_path.empty();
}



//////////////////////////////////////////////////		type_t


typedef int32_t type_lookup_index_t;

//	IMPORTANT: Collect all used types in a vector so we can use type_t as an index into it for O(1)
struct type_t {
	//??? Make this explicit
	type_t(const type_desc_t& desc);
	type_t() :
		type_t(assemble((type_lookup_index_t)base_type::k_undefined, base_type::k_undefined))
	{
		QUARK_ASSERT(check_invariant());
	}


	bool check_invariant() const {
//		QUARK_ASSERT(get_base_type() != base_type::k_identifier);
		return true;
	}

	static type_t make_undefined(){
		return type_t(assemble((type_lookup_index_t)base_type::k_undefined, base_type::k_undefined));
	}


	bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_undefined;
	}


	static type_t make_any(){
		return type_t(assemble((type_lookup_index_t)base_type::k_any, base_type::k_any));
	}
	bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_any;
	}


	static type_t make_void(){
		return type_t(assemble((type_lookup_index_t)base_type::k_void, base_type::k_void));
	}
	bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_void;
	}


	static type_t make_bool(){
		return type_t(assemble((type_lookup_index_t)base_type::k_bool, base_type::k_bool));
	}
	bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_bool;
	}

	static type_t make_int(){
		return type_t(assemble((type_lookup_index_t)base_type::k_int, base_type::k_int));
	}
	bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_int;
	}


	static type_t make_double(){
		return type_t(assemble((type_lookup_index_t)base_type::k_double, base_type::k_double));
	}
	bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_double;
	}


	static type_t make_string(){
		return type_t(assemble((type_lookup_index_t)base_type::k_string, base_type::k_string));
	}
	bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_string;
	}


	static type_t make_json(){
		return type_t(assemble((type_lookup_index_t)base_type::k_json, base_type::k_json));
	}
	bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_json;
	}


	static type_t make_typeid(){
		return type_t::assemble2((type_lookup_index_t)base_type::k_typeid, base_type::k_typeid);
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


	public: bool is_symbol_ref() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_symbol_ref;
	}

	std::string get_symbol_ref(const types_t& types) const;


	//////////////////////////////////////////////////		NAMED TYPE


	bool is_named_type() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_named_type;
	}

	type_name_t get_named_type(const types_t& types) const;


	//////////////////////////////////////////////////		BASETYPE


	base_type get_base_type() const {
		return get_bt0(data);
	}


	//////////////////////////////////////////////////		INTERNALS


	explicit type_t(int32_t data) :
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

	public: static type_t assemble2(type_lookup_index_t lookup_index, base_type bt1){
		return type_t(assemble(lookup_index, bt1));
	}


	//////////////////////////////////////////////////		BIT MANGLING


	public: static uint32_t assemble(type_lookup_index_t lookup_index, base_type bt1){
		const auto a = static_cast<uint32_t>(bt1);
		return a * 100 * 10000 + lookup_index;
	}

	///??? bump to large number
	private: inline static type_lookup_index_t get_index(uint32_t data) {
		return data % (100 * 10000);
	}
	private: inline static base_type get_bt0(uint32_t data){
		const uint32_t result = data / (100 * 10000);
		return static_cast<base_type>(result);
	}


	////////////////////////////////	STATE
	friend class type_desc_t;

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

	private: uint32_t data;
#if DEBUG
	private: base_type debug_bt0;
	private: type_lookup_index_t debug_lookup_index;
#endif
};


inline bool operator<(type_t lhs, type_t rhs){
	return lhs.get_data() < rhs.get_data();
}
inline bool operator==(type_t lhs, type_t rhs){
	return lhs.get_data() == rhs.get_data();
}
inline bool operator!=(type_t lhs, type_t rhs){ return (lhs == rhs) == false; };



/////////////////////////////////////////////////		type_desc_t


struct type_desc_t {
	bool check_invariant() const {
//		QUARK_ASSERT(get_base_type() != base_type::k_identifier);
		return true;
	}

	type_desc_t(){
		QUARK_ASSERT(check_invariant());
	}

	inline type_lookup_index_t get_lookup_index() const {
		QUARK_ASSERT(check_invariant());

		return non_name_type.get_lookup_index();
	}


	//////////////////////////////////////////////////		UNDEFINED


	bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_undefined;
	}


	//////////////////////////////////////////////////		ANY



	static type_desc_t make_any(){
		return type_desc_t(type_t::make_any());
	}

	bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_any;
	}


	//////////////////////////////////////////////////		VOID


	static type_desc_t make_void(){
		return type_desc_t(type_t::make_void());
	}

	bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_void;
	}


	//////////////////////////////////////////////////		BOOL


	static type_desc_t make_bool(){
		return type_desc_t(type_t::make_bool());
	}

	bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_bool;
	}


	//////////////////////////////////////////////////		INT


	static type_desc_t make_int(){
		return type_desc_t(type_t::make_int());
	}

	bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_int;
	}


	//////////////////////////////////////////////////		DOUBLE


	static type_desc_t make_double(){
		return type_desc_t(type_t::make_double());
	}

	bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_double;
	}


	//////////////////////////////////////////////////		STRING


	static type_desc_t make_string(){
		return type_desc_t(type_t::make_string());
	}

	bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_string;
	}


	//////////////////////////////////////////////////		JSON


	static type_desc_t make_json(){
		return type_desc_t(type_t::make_json());
	}

	bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_json;
	}


	//////////////////////////////////////////////////		TYPEID


	static type_desc_t make_typeid(){
		return type_desc_t(type_t::make_typeid());
	}

	bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_typeid;
	}



	//////////////////////////////////////////////////		STRUCT


	bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return non_name_type.get_base_type() == base_type::k_struct;
	}

	struct_type_desc_t get_struct(const types_t& types) const;


	
	//////////////////////////////////////////////////		VECTOR


	bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_vector;
	}

	type_t get_vector_element_type(const types_t& types) const;



	//////////////////////////////////////////////////		DICT


	bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_dict;
	}

	type_t get_dict_value_type(const types_t& types) const;



	//////////////////////////////////////////////////		FUNCTION


	bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return non_name_type.get_base_type() == base_type::k_function;
	}

	public: type_t get_function_return(const types_t& types) const;
	public: type_desc_t get_function_return2(const types_t& types) const {
		return get_function_return(types);
	}
	public: std::vector<type_t> get_function_args(const types_t& types) const;
	public: std::vector<type_desc_t> get_function_args2(const types_t& types) const{
		const auto args = get_function_args(types);
		return mapf<type_desc_t>(args, [&](const auto& e){ return type_desc_t(e); } );
	}

	public: return_dyn_type get_function_dyn_return_type(const types_t& types) const;
	public: epure get_function_pure(const types_t& types) const;


	//////////////////////////////////////////////////		SYMBOL


	public: bool is_symbol_ref() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_symbol_ref;
	}

	std::string get_symbol_ref(const types_t& types) const;


	//////////////////////////////////////////////////		NAMED TYPE


	bool is_named_type() const {
		QUARK_ASSERT(check_invariant());

		return get_base_type() == base_type::k_named_type;
	}

	type_name_t get_named_type(const types_t& types) const;


	base_type get_base_type() const {
		return non_name_type.get_base_type();
	}


	//////////////////////////////////////////////////		INTERNALS


	static type_desc_t wrap_non_named(const type_t& type){
		QUARK_ASSERT(type.is_named_type() == false);
		return type_desc_t(type);
	}
	private: type_desc_t(const type_t& non_name_type) :
		non_name_type(non_name_type)
	{
	}

	////////////////////////////////	STATE

	public: type_t non_name_type;
};

inline bool operator<(type_desc_t lhs, type_desc_t rhs){
	return lhs.non_name_type < rhs.non_name_type;
}
inline bool operator==(type_desc_t lhs, type_desc_t rhs){
	return lhs.non_name_type == rhs.non_name_type;
}
inline bool operator!=(type_desc_t lhs, type_desc_t rhs){ return (lhs == rhs) == false; };



/////////////////////////////////////////////////		FREE FUNCTIONS


inline type_t make_undefined(){
	return type_t::assemble2(
		(type_lookup_index_t)base_type::k_undefined,
		base_type::k_undefined
	);
}

inline type_t make_json_type(){
	return type_t::make_json();
}

type_t make_struct(types_t& types, const struct_type_desc_t& desc);
type_t make_struct(const types_t& types, const struct_type_desc_t& desc);
type_t make_vector(types_t& types, const type_t& element_type);
type_t make_vector(const types_t& types, const type_t& element_type);
type_t make_dict(types_t& types, const type_t& value_type);
type_t make_dict(const types_t& types, const type_t& value_type);

type_t make_function3(
	types_t& types,
	const type_t& ret,
	const std::vector<type_t>& args,
	epure pure,
	return_dyn_type dyn_return
);

type_t make_function3(
	const types_t& types,
	const type_t& ret,
	const std::vector<type_t>& args,
	epure pure,
	return_dyn_type dyn_return
);
type_t make_function_dyn_return(
	types_t& types,
	const std::vector<type_t>& args,
	epure pure,
	return_dyn_type dyn_return
);
type_t make_function(
	types_t& types,
	const type_t& ret,
	const std::vector<type_t>& args,
	epure pure
);
type_t make_function(
	const types_t& types,
	const type_t& ret,
	const std::vector<type_t>& args,
	epure pure
);

type_t make_symbol_ref(types_t& types, const std::string& s);



inline bool is_empty(const type_t& type){
	return type.is_undefined();
}

bool is_atomic_type(type_t type);


std::string type_to_debug_string(const type_t& type);

enum class enamed_type_mode { full_names, short_names };

std::string type_to_compact_string(
	const types_t& types,
	const type_t& type,
	enamed_type_mode named_type_mode = enamed_type_mode::short_names
);


//////////////////////////////////////////////////		member_t


struct member_t {
	member_t(type_t type, const std::string& name) :
		_type(type),
		_name(name)
	{
	}

	type_t _type;
	std::string _name;
};

inline bool operator==(const member_t& lhs, const member_t& rhs){
	return lhs._name == rhs._name
	&& lhs._type == rhs._type;
}

std::vector<type_t> get_member_types(const std::vector<member_t>& m);
std::vector<std::string> get_member_names(const std::vector<member_t>& m);


//////////////////////////////////////////////////		struct_type_desc_t


struct struct_type_desc_t {
	struct_type_desc_t(const std::vector<member_t>& members) :
		_members(members)
	{
		QUARK_ASSERT(check_invariant());
	}
	struct_type_desc_t(){
		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const {
		return true;
	}

	std::vector<member_t> _members;
};

inline bool operator==(const struct_type_desc_t& lhs, const struct_type_desc_t& rhs){
	return lhs._members == rhs._members;
}

int find_struct_member_index(const struct_type_desc_t& desc, const std::string& name);


//////////////////////////////////////////////////		type_node_t


//	Assigns 32 bit ID to types. You can lookup the type using the ID.
//	This allows us to describe a type using a single 32 bit integer (compact, fast to copy around).
//	Each type has exactly ONE ID.
//	Automatically insert all basetype-types so they ALWAYS have EXPLICIT integer IDs as types.

struct type_def_t {
	bool check_invariant() const {
		for(const auto& e: child_types){
			QUARK_ASSERT(e.check_invariant());
		}
		return true;
	}

	base_type bt;
	std::vector<type_t> child_types;

	std::vector<std::string> names;

	//	Only used when bt == k_function.
	epure func_pure;

	//	Only used when bt == k_function.
	//??? I think we can lose this field now that we have intrinsics handling in semast.
	return_dyn_type func_return_dyn_type;

	std::string symbol_identifier;
};
inline bool operator==(const type_def_t& lhs, const type_def_t& rhs){
	return
		lhs.bt == rhs.bt
		&& lhs.child_types == rhs.child_types
		&& lhs.names == rhs.names
		&& lhs.func_pure == rhs.func_pure
		&& lhs.func_return_dyn_type == rhs.func_return_dyn_type
		&& lhs.symbol_identifier == rhs.symbol_identifier
		;
}


//////////////////////////////////////////////////		physical_type_t


//	Represents an instantiatable type where all types and subtypes are built-in floyd types, never named types.
//??? use this to replace peek2() and type_desc_t.
struct physical_type_t {
	type_t physical;
};



struct type_node_t {
	bool check_invariant() const {
		QUARK_ASSERT(def.check_invariant());
		return true;
	}

	//	If optional_name is used, this node is a named node and child_type_indexes[0] will be
	//	undefined or hold the real type.
	type_name_t optional_name;

	type_def_t def;

	physical_type_t physical_type;
};

inline bool operator==(const type_node_t& lhs, const type_node_t& rhs){
	return
		lhs.optional_name == rhs.optional_name
		&& lhs.def == rhs.def
		;
}


//////////////////////////////////////////////////		types_t


struct types_t {
	types_t();

	bool check_invariant() const;


	////////////////////////////////	STATE

	//	All types are recorded here, an uniqued. Including named types.
	//	type uses the INDEX into this array for fast lookups.
	std::vector<type_node_t> nodes;
};

type_t lookup_type_from_index(const types_t& types, type_lookup_index_t type_index);
const type_node_t& lookup_typeinfo_from_type(const types_t& types, const type_t& type);

void trace_types(const types_t& types);

//	Undefined-type = not a physical type.
physical_type_t get_physical_type(const types_t& types, const type_t& type);



type_t refresh_type(const types_t& types, const type_t& type);

//	Is this type instantiatable: it uses no symbols and uses no undefined. Deep and follows named types.
bool is_fully_defined(const types_t& types, const type_t& t);


//////////////////////////////////////////////////		NAMED TYPES


type_t lookup_type_from_name(const types_t& types, const type_name_t& n);

//	Allocates a new type for this name. The name must not already exist.
//	You can use destination_type = type_t::make_undefined() and
//	later update the type using update_named_type()
type_t make_named_type(types_t& types, const type_name_t& n, const type_t& destination_type);

//	Update the named type's destination type. The tagged type must already exist. Any usage of this
//	tag will also get the new type.
type_t update_named_type(types_t& types, const type_t& named, const type_t& destination_type);

type_t get_named_type_destination(const types_t& types, const type_t& named_type);

type_desc_t peek2(const types_t& types, const type_t& type);



//////////////////////////////////////////////////		get_type_variant()


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
	struct_type_desc_t desc;
};
struct vector_t {
	std::vector<type_t> _parts;
};
struct dict_t {
	std::vector<type_t> _parts;
};
struct function_t {
	std::vector<type_t> _parts;
};
struct symbol_ref_t {
	std::string s;
};
struct named_type_t {
	type_t destination_type;
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
> type_variant_t;


type_variant_t get_type_variant(const types_t& types, const type_t& type);


//////////////////////////////////////////////////		JSON


json_t type_to_json_shallow(const type_t& type);
type_t type_from_json_shallow(const json_t& j);

json_t type_to_json(const types_t& types, const type_t& type);
type_t type_from_json(types_t& types, const json_t& j);

json_t members_to_json(const types_t& types, const std::vector<member_t>& members);
std::vector<member_t> members_from_json(types_t& types, const json_t& members);

json_t types_to_json(const types_t& types);
types_t types_from_json(const json_t& j);

}	// floyd

#endif /* types_h */
