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
	Immutable, value-semantics.

	The value_t is completely standalone and is not coupled to the runtime etc.

	A value of type *struct* can hold a huge, deeply nested struct containing dictionaries and so on.
	It will be deep-copied alternatively some internal immutable state may be shared with other instances.


	value_t is mostly used in the API:s of the compiler.
	It is not extremely efficient.

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
*/

#include "utils.h"
#include "quark.h"

#include "types.h"
#include "json_support.h"

#include <vector>
#include <string>
#include <map>



namespace floyd {

struct body_t;
struct value_t;
struct value_ext_t;


#if DEBUG
std::string make_value_debug_str(const value_t& v);
#endif




////////////////////////////////////////		link_name_t


struct link_name_t {
	std::string s;
};

inline bool operator==(const link_name_t& lhs, const link_name_t& rhs){
	return lhs.s == rhs.s;
}
inline bool operator!=(const link_name_t& lhs, const link_name_t& rhs){
	return lhs.s != rhs.s;
}
inline bool operator<(const link_name_t& lhs, const link_name_t& rhs){
	return lhs.s < rhs.s;
}

const link_name_t k_no_link_name = link_name_t { "" };




////////////////////////////////////////		function_id_t


struct function_id_t {
	std::string name;
};

inline bool operator==(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name == rhs.name;
}
inline bool operator!=(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name != rhs.name;
}
inline bool operator<(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name < rhs.name;
}

const function_id_t k_no_function_id = function_id_t { "" };





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
	public: struct_type_desc_t _def;
	public: std::vector<value_t> _member_values;
};


//////////////////////////////////////////////////		value_ext_t

/*
	Holds data that can't be fit directly inlined into the value_t itself.
*/

struct value_ext_t {
	public: bool check_invariant() const{
		QUARK_ASSERT(_rc > 0);
		QUARK_ASSERT(_type.check_invariant());
		QUARK_ASSERT(_typeid_value.check_invariant());

		const auto base_type = _type.get_base_type();
		if(base_type == base_type::k_string){
//				QUARK_ASSERT(_string);
			QUARK_ASSERT(_json == nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct == nullptr);
			QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);
		}
		else if(base_type == base_type::k_json){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json != nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct == nullptr);
			QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);

			QUARK_ASSERT(_json->check_invariant());
		}

		else if(base_type == base_type::k_typeid){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json == nullptr);
	//		QUARK_ASSERT(_typeid_value != type_t::make_undefined());
			QUARK_ASSERT(_struct == nullptr);
			QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);

			QUARK_ASSERT(_typeid_value.check_invariant());
		}
		else if(base_type == base_type::k_struct){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json == nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct != nullptr);
			QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);

			QUARK_ASSERT(_struct && _struct->check_invariant());
		}
		else if(base_type == base_type::k_vector){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json == nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct == nullptr);
	//		QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);
		}
		else if(base_type == base_type::k_dict){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json == nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct == nullptr);
			QUARK_ASSERT(_vector_elements.empty());
	//		QUARK_ASSERT(_dict_entries.empty());
			QUARK_ASSERT(_function_id == k_no_function_id);
		}
		else if(base_type == base_type::k_function){
			QUARK_ASSERT(_string.empty());
			QUARK_ASSERT(_json == nullptr);
			QUARK_ASSERT(_typeid_value.is_undefined());
			QUARK_ASSERT(_struct == nullptr);
			QUARK_ASSERT(_vector_elements.empty());
			QUARK_ASSERT(_dict_entries.empty());
//				QUARK_ASSERT(_function_id != k_no_function_id);
		}
		else {
			QUARK_ASSERT(false);
		}
		return true;
	}

	public: bool operator==(const value_ext_t& other) const;


	public: value_ext_t(const std::string& s) :
		_rc(1),
		_type(type_t::make_string()),
		_string(s)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: value_ext_t(const std::shared_ptr<json_t>& s) :
		_rc(1),
		_type(type_t::make_json()),
		_json(s)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: value_ext_t(const type_t& s);

	public: value_ext_t(const type_t& type, std::shared_ptr<struct_value_t>& s);
	public: value_ext_t(const type_t& type, const std::vector<value_t>& s);
	public: value_ext_t(const type_t& type, const std::map<std::string, value_t>& s);
	public: value_ext_t(const type_t& type, function_id_t function_id);


	//	??? NOTICE: Use std::variant or subclasses.
	public: int _rc;
	public: type_t _type;
	public: std::string _string;
	public: std::shared_ptr<json_t> _json;
	public: type_t _typeid_value = type_t::make_undefined();
	public: std::shared_ptr<struct_value_t> _struct;
	public: std::vector<value_t> _vector_elements;
	public: std::map<std::string, value_t> _dict_entries;
	public: function_id_t _function_id = k_no_function_id;
};


//////////////////////////////////////////////////		value_t


struct value_t {

	//////////////////////////////////////////////////		PUBLIC - SPECIFIC TO TYPE

	public: value_t() :
		_basetype(base_type::k_undefined)
	{
		_value_internals._int = 0xdeadbeef;
#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}
	private: explicit value_t(base_type type) :
		_basetype(type)
	{
		_value_internals._int = 0xdeadbeef;
#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	//	Used internally in check_invariant() -- don't call check_invariant().
	public: type_t get_type() const;
	public: base_type get_basetype() const{
		return _basetype;
	}


	//------------------------------------------------		undefined


	public: static value_t make_undefined(){
		return value_t(base_type::k_undefined);
	}
	public: bool is_undefined() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_undefined;
	}


	//------------------------------------------------		internal-dynamic


	public: static value_t make_any(){
		return value_t(base_type::k_any);
	}
	public: bool is_any() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_any;
	}


	//------------------------------------------------		void


	public: static value_t make_void(){
		return value_t(base_type::k_void);
	}
	public: bool is_void() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_void;
	}


	//------------------------------------------------		bool


	public: static value_t make_bool(bool value);
	public: bool is_bool() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_bool;
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
		return value_t(value);
	}
	public: bool is_int() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_int;
	}
	public: int64_t get_int_value() const{
		QUARK_ASSERT(check_invariant());
		if(!is_int()){
			quark::throw_runtime_error("Type mismatch!");
		}

		return _value_internals._int;
	}

	public: int64_t get_int_value_quick() const{
		return _value_internals._int;
	}

	//------------------------------------------------		double


	public: static value_t make_double(double value);
	public: bool is_double() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_double;
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
		return value_t(value);
	}
	public: static inline value_t make_string(const char value[]){
		return make_string(std::string(value));
	}
	public: bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_string;
	}
	public: std::string get_string_value() const;


	//------------------------------------------------		json


	public: static value_t make_json(const json_t& v);
	public: bool is_json() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_json;
	}
	public: json_t get_json() const;


	//------------------------------------------------		typeid


	public: static value_t make_typeid_value(const type_t& type_id);
	public: bool is_typeid() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_typeid;
	}
	public: type_t get_typeid_value() const;


	//------------------------------------------------		struct


	public: static value_t make_struct_value(const type_interner_t& interner, const type_t& struct_type, const std::vector<value_t>& values);
	public: static value_t make_struct_value(type_interner_t& interner, const type_t& struct_type, const std::vector<value_t>& values);
	public: bool is_struct() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_struct;
	}
	public: std::shared_ptr<struct_value_t> get_struct_value() const;


	//------------------------------------------------		vector


	public: static value_t make_vector_value(const type_interner_t& interner, const type_t& element_type, const std::vector<value_t>& elements);
	public: static value_t make_vector_value(type_interner_t& interner, const type_t& element_type, const std::vector<value_t>& elements);
	public: bool is_vector() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_vector;
	}
	public: const std::vector<value_t>& get_vector_value() const;


	//------------------------------------------------		dict


	public: static value_t make_dict_value(const type_interner_t& interner, const type_t& value_type, const std::map<std::string, value_t>& entries);
	public: static value_t make_dict_value(type_interner_t& interner, const type_t& value_type, const std::map<std::string, value_t>& entries);
	public: bool is_dict() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_dict;
	}
	public: const std::map<std::string, value_t>& get_dict_value() const;


	//------------------------------------------------		function


	public: static value_t make_function_value(const type_t& function_type, const function_id_t& function_id);
	public: bool is_function() const {
		QUARK_ASSERT(check_invariant());

		return _basetype == base_type::k_function;
	}
	public: function_id_t get_function_value() const;


	//////////////////////////////////////////////////		PUBLIC - TYPE INDEPENDANT


	private: static bool is_ext_slow(base_type basetype){
		return false
			|| basetype == base_type::k_string
			|| basetype == base_type::k_json
			|| basetype == base_type::k_typeid
			|| basetype == base_type::k_struct
			|| basetype == base_type::k_vector
			|| basetype == base_type::k_dict
			|| basetype == base_type::k_function;
	}

	private: bool is_ext(base_type basetype) const{
		bool ext = basetype > base_type::k_double;

		//	Make sure above assumtion about order of base types is valid.
		QUARK_ASSERT(ext == is_ext_slow(basetype));
		return ext;
	}


	public: bool check_invariant() const;

	public: value_t(const value_t& other):
		_basetype(other._basetype),
		_value_internals(other._value_internals)
	{
		QUARK_ASSERT(other.check_invariant());

		if(is_ext(_basetype)){
			_value_internals._ext->_rc++;
		}

#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	public: ~value_t(){
		QUARK_ASSERT(check_invariant());

		if(is_ext(_basetype)){
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

		if(!(_basetype == other._basetype)){
			return false;
		}

		if(_basetype == base_type::k_undefined){
			return true;
		}
		else if(_basetype == base_type::k_any){
			return true;
		}
		else if(_basetype == base_type::k_void){
			return true;
		}
		else if(_basetype == base_type::k_bool){
			return _value_internals.bool_value == other._value_internals.bool_value;
		}
		else if(_basetype == base_type::k_int){
			return _value_internals._int == other._value_internals._int;
		}
		else if(_basetype == base_type::k_double){
			return _value_internals._double == other._value_internals._double;
		}
		else{
			QUARK_ASSERT(is_ext(_basetype));
			return compare_shared_values(_value_internals._ext, other._value_internals._ext);
		}
	}

	public: bool operator!=(const value_t& other) const{
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
	public: static int compare_value_true_deep(const value_t& left, const value_t& right);

	public: void swap(value_t& other){
		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());

#if DEBUG_DEEP
		std::swap(DEBUG_STR, other.DEBUG_STR);
#endif

		std::swap(_basetype, other._basetype);

		std::swap(_value_internals, other._value_internals);

		QUARK_ASSERT(other.check_invariant());
		QUARK_ASSERT(check_invariant());
	}


	//////////////////////////////////////////////////		INTERNALS


	private: explicit value_t(bool value) :
		_basetype(base_type::k_bool)
	{
		_value_internals.bool_value = value;
#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	private: explicit value_t(int64_t value) :
		_basetype(base_type::k_int)
	{
		_value_internals._int = value;
#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	private: value_t(double value) :
		_basetype(base_type::k_double)
	{
		_value_internals._double = value;
#if DEBUG_DEEP
		DEBUG_STR = make_value_debug_str(*this);
#endif

		QUARK_ASSERT(check_invariant());
	}

	private: explicit value_t(const char s[]);
	private: explicit value_t(const std::string& s);

	private: explicit value_t(const std::shared_ptr<json_t>& s);
	private: explicit value_t(const type_t& type);

	private: explicit value_t(const type_t& struct_type, std::shared_ptr<struct_value_t>& instance);
	private: explicit value_t(type_t& struct_type, std::shared_ptr<struct_value_t>& instance);

	private: explicit value_t(const type_interner_t& interner, const type_t& element_type, const std::vector<value_t>& elements);
	private: explicit value_t(type_interner_t& interner, const type_t& element_type, const std::vector<value_t>& elements);

	private: explicit value_t(const type_interner_t& interner, const type_t& value_type, const std::map<std::string, value_t>& entries);
	private: explicit value_t(type_interner_t& interner, const type_t& value_type, const std::map<std::string, value_t>& entries);

	private: explicit value_t(const type_t& type, function_id_t function_id);


	//////////////////////////////////////////////////		STATE

	private: union value_internals_t {
		bool bool_value;
		int64_t _int;
		double _double;
		value_ext_t* _ext;
	};

#if DEBUG_DEEP
	private: std::string DEBUG_STR;
#endif
	private: base_type _basetype;
	private: value_internals_t _value_internals;
};


//////////////////////////////////////////////////		Helpers



/*
	"true"
	"0"
	"1003"
	"Hello, world"
	Notice, strings don't get wrapped in "".
*/
std::string to_compact_string2(const type_interner_t& interner, const value_t& value);

//	Special handling of strings, we want to wrap in "".
std::string to_compact_string_quote_strings(const type_interner_t& interner, const value_t& value);

/*
	bool: "true"
	int: "0"
	string: "1003"
	string: "Hello, world"
*/
std::string value_and_type_to_string(const type_interner_t& interner, const value_t& value);

json_t value_to_ast_json(const type_interner_t& interner, const value_t& v);
value_t ast_json_to_value(type_interner_t& interner, const type_t& type, const json_t& v);


//	json array: [ TYPE, VALUE ]
json_t value_and_type_to_ast_json(const type_interner_t& interner, const value_t& v);

//	json array: [ TYPE, VALUE ]
value_t ast_json_to_value_and_type(type_interner_t& interner, const json_t& v);


json_t values_to_json_array(const type_interner_t& interner, const std::vector<value_t>& values);


value_t make_def(const type_t& type);

void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected);


}	//	floyd

#endif /* parser_value_hpp */
