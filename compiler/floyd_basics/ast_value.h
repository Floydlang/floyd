//
//  parser_value.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_value_hpp
#define parser_value_hpp

/*
	value_t

	Hold a Floyd value with an explicit type.
	It is NOT VERY EFFICIENT - limit use in critical paths in final user program exectuables.
	Immutable, value-semantics.

	In final user program executable: prefer runtime_value_t & runtime_type_t and type_t -- these are fast.

	- type: the semantic type, which could be a "game_object_t" or int.
	- physical type: a concrete type that tells which field, element etc to use -- all named types are gone here.

	The value_t is completely standalone and is not coupled to the runtime etc.

	A value of type *struct* can hold a huge, deeply nested struct containing dictionaries and so on.
	It will be deep-copied alternatively some internal immutable state may be shared with other instances.

	value_t is mostly used in the API:s of the compiler.

	Can hold *any* value that is legal in Floyd.
	Also supports a handful of types used internally in tools.

	Example values:
		Int
		String
		Float
		GameObject struct instance
		function pointer
		Vector instance
		Dictionary instance

	??? Remove all value_t::is_bool() etc.
*/

#include "utils.h"
#include "quark.h"

#include "types.h"
#include "json_support.h"
#include "compiler_basics.h"

#include <vector>
#include <string>
#include <map>



namespace floyd {

struct value_t;
struct value_ext_t;


#if DEBUG
std::string make_value_debug_str(const value_t& v);
#endif




//////////////////////////////////////////////////		struct_value_t

/*
	An instance of a struct-type = a value of this struct.
*/
struct struct_value_t {
	public: struct_value_t(const struct_type_desc_t& def, const std::vector<value_t>& member_values) :
		_def(def),
		_member_values(member_values)
	{
//		QUARK_ASSERT(_def.check_invariant());

		QUARK_ASSERT(check_invariant());
	}
	public: bool check_invariant() const;
	public: bool operator==(const struct_value_t& other) const;


	////////////////////////////////////////		STATE
	//??? _def might be unnecessary
	public: struct_type_desc_t _def;
	public: std::vector<value_t> _member_values;
};


//////////////////////////////////////////////////		value_ext_t

/*
	Holds data that can't be fit directly inlined into the value_t itself.
*/

struct value_ext_t {
	public: bool check_invariant() const;
	public: bool operator==(const value_ext_t& other) const;

	public: value_ext_t(const std::string& s);
	public: value_ext_t(const std::shared_ptr<json_t>& s);

	public: value_ext_t(const type_t& s);
	public: value_ext_t(std::shared_ptr<struct_value_t>& s);
	public: value_ext_t(const std::vector<value_t>& s);
	public: value_ext_t(const std::map<std::string, value_t>& s);
	public: value_ext_t(module_symbol_t function_id);


	//	??? NOTICE: Use std::variant or subclasses.
	public: int _rc;

	public: base_type _physical_base_type;

	public: std::string _string;
	public: std::shared_ptr<json_t> _json;
	public: type_t _typeid_value = type_t::make_undefined();
	public: std::shared_ptr<struct_value_t> _struct;
	public: std::vector<value_t> _vector_elements;
	public: std::map<std::string, value_t> _dict_entries;
	public: module_symbol_t _function_id = k_no_module_symbol;
};


//////////////////////////////////////////////////		value_t


struct value_t {

	private: union value_internals_t {
		bool bool_value;
		int64_t _int;
		double _double;
		value_ext_t* _ext;
	};


	//////////////////////////////////////////////////		PUBLIC - SPECIFIC TO TYPE

	public: value_t() : value_t(type_t::make_undefined()) {}


	public: type_t get_type() const;
	public: type_t get_physical_type() const{
		return _physical_type;
	}

	public: static value_t replace_logical_type(const value_t& value, const type_t& logical_type){
		auto copy = value;
		copy._logical_type = logical_type;
		return copy;
	}

	//------------------------------------------------		undefined


	public: static value_t make_undefined(){
		return value_t(type_t::make_undefined());
	}
	public: bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_undefined();
	}


	//------------------------------------------------		any


	public: static value_t make_any(){
		return value_t(type_t::make_any());
	}
	public: bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_any();
	}


	//------------------------------------------------		void


	public: static value_t make_void(){
		return value_t(type_t::make_void());
	}
	public: bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_void();
	}


	//------------------------------------------------		bool


	public: static value_t make_bool(bool value){
		return value_t(value_internals_t { .bool_value = value }, type_t::make_bool());
	}
	public: bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_bool();
	}
	public: bool get_bool_value() const{
		QUARK_ASSERT(check_invariant());
		if(!is_bool()){
			quark::throw_runtime_error("Type mismatch!");
		}

		return _value_internals.bool_value;
	}
	public: bool get_bool_value_quick() const{
		return _value_internals.bool_value;
	}


	//------------------------------------------------		int


	static value_t make_int(int64_t value){
		return value_t(value_internals_t { ._int = value }, type_t::make_int());
	}
	public: bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_int();
	}
	public: int64_t get_int_value() const{
		QUARK_ASSERT(check_invariant());
		if(!is_int()){
			quark::throw_runtime_error("Type mismatch!");
		}

		return _value_internals._int;
	}


	//------------------------------------------------		double


	static value_t make_double(double value){
		return value_t(value_internals_t { ._double = value }, type_t::make_double());
	}
	public: bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_double();
	}
	public: double get_double_value() const{
		QUARK_ASSERT(check_invariant());
		if(!is_double()){
			quark::throw_runtime_error("Type mismatch!");
		}

		return _value_internals._double;
	}


	//------------------------------------------------		string


	public: static value_t make_string(const std::string& value){
		auto ext = new value_ext_t{ value };
		QUARK_ASSERT(ext->_rc == 1);
		return value_t(value_internals_t { ._ext = ext }, type_t::make_string());
	}
	public: static inline value_t make_string(const char value[]){
		return make_string(std::string(value));
	}
	public: bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_string();
	}
	public: std::string get_string_value() const;

	private: explicit value_t(const char s[]);
	private: explicit value_t(const std::string& s);


	//------------------------------------------------		json


	public: static value_t make_json(const json_t& v){
		auto f = std::make_shared<json_t>(v);
		auto ext = new value_ext_t{ f };
		QUARK_ASSERT(ext->_rc == 1);
		return value_t(value_internals_t { ._ext = ext }, type_t::make_json());
	}

	public: bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_json();
	}
	public: json_t get_json() const;


	//------------------------------------------------		typeid


	public: static value_t make_typeid_value(const type_t& type_id){
		auto ext = new value_ext_t{ type_id };
		QUARK_ASSERT(ext->_rc == 1);
		return value_t(value_internals_t { ._ext = ext }, type_t::make_typeid());
	}
	public: bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_typeid();
	}
	public: type_t get_typeid_value() const;


	//------------------------------------------------		struct


	public: static value_t make_struct_value(const types_t& types, const type_t& struct_type, const std::vector<value_t>& values);
	public: bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_struct();
	}
	public: std::shared_ptr<struct_value_t> get_struct_value() const;


	//------------------------------------------------		vector


	public: static value_t make_vector_value(const types_t& types, const type_t& element_type, const std::vector<value_t>& elements);
	public: static value_t make_vector_value(types_t& types, const type_t& element_type, const std::vector<value_t>& elements);
	public: bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_vector();
	}
	public: const std::vector<value_t>& get_vector_value() const;


	//------------------------------------------------		dict


	public: static value_t make_dict_value(const types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries);
	public: static value_t make_dict_value(types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries);
	public: bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_dict();
	}
	public: const std::map<std::string, value_t>& get_dict_value() const;


	//------------------------------------------------		function


	public: static value_t make_function_value(const types_t& types, const type_t& type, const module_symbol_t& function_id);
	public: static value_t make_function_value(types_t& types, const type_t& type, const module_symbol_t& function_id);
	public: bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return _logical_type.is_function();
	}
	public: module_symbol_t get_function_value() const;


	//------------------------------------------------		named type

	//??? Helper that makes a value where logical type and physical type are different



	//////////////////////////////////////////////////		PUBLIC - TYPE INDEPENDANT


	private: bool uses_ext_data() const;


	public: bool check_invariant() const;

	public: value_t(const value_t& other):
		_logical_type(other._logical_type),
		_physical_type(other._physical_type),
		_value_internals(other._value_internals)
	{
		QUARK_ASSERT(other.check_invariant());

		if(uses_ext_data()){
			_value_internals._ext->_rc++;
		}

#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	public: ~value_t(){
		QUARK_ASSERT(check_invariant());

		if(uses_ext_data()){
			_value_internals._ext->_rc--;
			if(_value_internals._ext->_rc == 0){
				delete _value_internals._ext;
			}
			_value_internals._ext = nullptr;
		}
	}

	public: value_t& operator=(const value_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

		value_t temp(other);
		temp.swap(*this);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
		return *this;
	}

	public: bool operator==(const value_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_logical_type == other._logical_type)){
			return false;
		}

		const auto basetype = _logical_type.get_base_type();
		if(basetype == base_type::k_undefined){
			return true;
		}
		else if(basetype == base_type::k_any){
			return true;
		}
		else if(basetype == base_type::k_void){
			return true;
		}
		else if(basetype == base_type::k_bool){
			return _value_internals.bool_value == other._value_internals.bool_value;
		}
		else if(basetype == base_type::k_int){
			return _value_internals._int == other._value_internals._int;
		}
		else if(basetype == base_type::k_double){
			return _value_internals._double == other._value_internals._double;
		}
		else{
			QUARK_ASSERT(uses_ext_data());
			return compare_shared_values(_value_internals._ext, other._value_internals._ext);
		}
	}

	public: bool operator !=(const value_t& other) const{
		return !(*this == other);
	}

	/*
		diff == 0: equal
		diff == 1: left side is bigger
		diff == -1: right side is bigger.

		Think (left_struct - right_struct).

		This technique lets us do most comparison operations *ontop* of compare_value_true_deep() with
		only a single compare function.
	*/
	public: static int compare_value_true_deep(const types_t& types, const value_t& left, const value_t& right);

	public: void swap(value_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

#if DEBUG_DEEP
		std::swap(DEBUG_STR, other.DEBUG_STR);
#endif

		std::swap(_logical_type, other._logical_type);
		std::swap(_physical_type, other._physical_type);

		std::swap(_value_internals, other._value_internals);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}


	//////////////////////////////////////////////////		INTERNALS


	private: value_t(const type_t& logical_type) :
		_value_internals(),
		_logical_type(logical_type),
		_physical_type(logical_type)
#if DEBUG_DEEP
		,
		DEBUG_STR = make_value_debug_str(*this);
#endif
	{
		QUARK_ASSERT(logical_type.is_undefined() || logical_type.is_any() || logical_type.is_void())
	}


	private: value_t(const types_t& types, const value_internals_t& value_internals, const type_t& logical_type) :
		_value_internals(value_internals),
		_logical_type(logical_type),
		_physical_type(floyd::get_physical_type(types, logical_type))
#if DEBUG_DEEP
		,
		DEBUG_STR = make_value_debug_str(*this);
#endif
	{
		QUARK_ASSERT(check_invariant());
	}

	//	WARNING: Can only be used for types guaranteed to be physical, like built-in simple types. Bool etc.
	private: value_t(const value_internals_t& value_internals, const type_t& logical_type) :
		_value_internals(value_internals),
		_logical_type(logical_type),
		_physical_type(logical_type)
#if DEBUG_DEEP
		,
		DEBUG_STR = make_value_debug_str(*this);
#endif
	{
		QUARK_ASSERT(logical_type.is_named_type() == false);

		QUARK_ASSERT(check_invariant());
	}


	//////////////////////////////////////////////////		STATE

#if DEBUG_DEEP
	private: std::string DEBUG_STR;
#endif
	private: value_internals_t _value_internals;
	private: type_t _logical_type;
	private: type_t _physical_type;
};


//////////////////////////////////////////////////		HELPERS


value_t make_default_value(const type_t& type);

void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected);


//////////////////////////////////////////////////		STRING

/*
	"true"
	"0"
	"1003"
	"Hello, world"
	Notice, strings don't get wrapped in "".
*/
std::string to_compact_string2(const types_t& types, const value_t& value);

/*
	bool: "true"
	int: "0"
	string: "1003"
	string: "Hello, world"
*/
std::string value_and_type_to_string(const types_t& types, const value_t& value);


//////////////////////////////////////////////////		JSON


json_t value_to_json(const types_t& types, const value_t& v);
value_t json_to_value(types_t& types, const type_t& type, const json_t& v);


//	json array: [ TYPE, VALUE ]
json_t value_and_type_to_json(const types_t& types, const value_t& v);

//	json array: [ TYPE, VALUE ]
value_t json_to_value_and_type(types_t& types, const json_t& v);




//////////////////////////////////////////////////		INLINES


inline static bool is_basetype_ext__slow(base_type basetype){
	return false
		|| basetype == base_type::k_string
		|| basetype == base_type::k_json
		|| basetype == base_type::k_typeid
		|| basetype == base_type::k_struct
		|| basetype == base_type::k_vector
		|| basetype == base_type::k_dict
		|| basetype == base_type::k_function;
}

inline static bool is_basetype_ext(base_type basetype){
	bool ext = basetype > base_type::k_double;

	//	Make sure above assumtion about order of base types is valid.
	QUARK_ASSERT(ext == is_basetype_ext__slow(basetype));
	return ext;
}

inline bool value_t::uses_ext_data() const{
#if 0
	return is_basetype_ext(_logical_type.get_base_type());
#else
	const auto bt = _physical_type.get_base_type();
	return is_basetype_ext(bt);
#endif
}

}	//	floyd

#endif /* parser_value_hpp */
