//
//  parser_value.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_value_hpp
#define parser_value_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <map>
#include "ast_basics.h"
#include "ast_typeid.h"


namespace floyd {
	struct statement_t;
	struct value_t;



	/*
		"true"
		"0"
		"1003"
		"Hello, world"
		Notice, strings don't get wrapped in "".
	*/
	std::string to_compact_string2(const value_t& value);

	//	Special handling of strings, we want to wrap in "".
	std::string to_compact_string_quote_strings(const value_t& value);

	/*
		bool: "true"
		int: "0"
		string: "1003"
		string: "Hello, world"
	*/
	std::string value_and_type_to_string(const value_t& value);


	ast_json_t value_to_ast_json(const value_t& v);

	ast_json_t value_and_type_to_ast_json(const value_t& v);

#if DEBUG
	std::string make_value_debug_str(const value_t& v);
#endif


	//////////////////////////////////////////////////		struct_instance_t

	/*
		An instance of a struct-type = a value of this struct.
	*/
	struct struct_instance_t {
		public: struct_instance_t(const struct_definition_t& def, const std::vector<value_t>& member_values) :
			_def(def),
			_member_values(member_values)
		{
			QUARK_ASSERT(_def.check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other) const;


		public: struct_definition_t _def;
		public: std::vector<value_t> _member_values;
	};


	//////////////////////////////////////		vector_def_t


	struct vector_def_t {
		public: vector_def_t(floyd::typeid_t element_type) :
			_element_type(element_type)
		{
		}
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		/////////////////////////////		STATE
		public: floyd::typeid_t _element_type;
	};


	//////////////////////////////////////////////////		vector_instance_t


	struct vector_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const vector_instance_t& other) const;

		public: vector_instance_t(
			typeid_t element_type,
			const std::vector<value_t>& elements
		):
			_element_type(element_type),
			_elements(elements)
		{
		}

		typeid_t _element_type;
		std::vector<value_t> _elements;
	};


	//////////////////////////////////////		dict_def_t


	struct dict_def_t {
		public: dict_def_t(floyd::typeid_t value_type) :
			_value_type(value_type)
		{
		}
		public: bool check_invariant() const;
		public: bool operator==(const dict_def_t& other) const;


		/////////////////////////////		STATE
		public: floyd::typeid_t _value_type;
	};


	//////////////////////////////////////////////////		dict_instance_t


	struct dict_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const dict_instance_t& other) const;

		public: dict_instance_t(
			typeid_t value_type,
			const std::map<std::string, value_t>& elements
		):
			_value_type(value_type),
			_elements(elements)
		{
		}

		typeid_t _value_type;
		std::map<std::string, value_t> _elements;
	};



	//////////////////////////////////////////////////		function_definition_t



	struct function_definition_t {
		public: function_definition_t(
			const std::vector<member_t>& args,
			const std::vector<std::shared_ptr<statement_t>> statements,
			const typeid_t& return_type
		);
		public: function_definition_t(
			const std::vector<member_t>& args,
			int host_function,
			const typeid_t& return_type
		);

		const std::vector<member_t> _args;
		const std::vector<std::shared_ptr<statement_t>> _statements;
		const int _host_function;
		const typeid_t _return_type;
	};

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);

	//??? lose this
	ast_json_t function_def_to_ast_json(const function_definition_t& v);

	typeid_t get_function_type(const function_definition_t& f);



	//////////////////////////////////////////////////		function_instance_t


	struct function_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const function_instance_t& other) const;


		public: function_definition_t _def;
	};



	//////////////////////////////////////////////////		value_t

	/*
		Hold a value with an explicit type.
		Immutable

		- Values of basic types like ints and strings
		- Struct instances
		- Vector instances
		- Dictionary instances
		- Function "pointers"

		NOTICE: Encoding is very inefficient at the moment.
	*/

	struct value_t {
		public: bool check_invariant() const;

		public: value_t() :
			_typeid(typeid_t::make_null())
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(bool value) :
			_typeid(typeid_t::make_bool()),
			_bool(value)
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(int value) :
			_typeid(typeid_t::make_int()),
			_int(value)
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: value_t(float value) :
			_typeid(typeid_t::make_float()),
			_float(value)
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const char s[]) :
			_typeid(typeid_t::make_string()),
			_string(s)
		{
			QUARK_ASSERT(s != nullptr);

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const std::string& s) :
			_typeid(typeid_t::make_string()),
			_string(s)
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const std::shared_ptr<json_t>& s) :
			_typeid(typeid_t::make_json_value()),
			_json_value(s)
		{
			QUARK_ASSERT(s != nullptr && s->check_invariant());
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}
		private: explicit value_t(const typeid_t& type) :
			_typeid(typeid_t::make_typeid()),
			_typeid_value(type)
		{
			QUARK_ASSERT(type.check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const typeid_t& struct_type, const std::shared_ptr<struct_instance_t>& instance) :
			_typeid(struct_type),
			_struct(instance)
		{
			QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
			QUARK_ASSERT(instance && instance->check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const std::shared_ptr<vector_instance_t>& instance) :
			_typeid(typeid_t::make_vector(instance->_element_type)),
			_vector(instance)
		{
			QUARK_ASSERT(instance && instance->check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const std::shared_ptr<dict_instance_t>& instance) :
			_typeid(typeid_t::make_dict(instance->_value_type)),
			_dict(instance)
		{
			QUARK_ASSERT(instance && instance->check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const std::shared_ptr<function_instance_t>& function_instance) :
			_typeid(get_function_type(function_instance->_def)),
			_function(function_instance)
		{
			QUARK_ASSERT(function_instance && function_instance->check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}



		public: value_t(const value_t& other):
			_typeid(other._typeid),

			_bool(other._bool),
			_int(other._int),
			_float(other._float),
			_string(other._string),
			_json_value(other._json_value),
			_typeid_value(other._typeid_value),
			_struct(other._struct),
			_vector(other._vector),
			_dict(other._dict),
			_function(other._function)
		{
			QUARK_ASSERT(other.check_invariant());

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
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

			if(!(_typeid == other._typeid)){
				return false;
			}

			const auto base_type = _typeid.get_base_type();
			if(base_type == base_type::k_null){
				return true;
			}
			else if(base_type == base_type::k_bool){
				return _bool == other._bool;
			}
			else if(base_type == base_type::k_int){
				return _int == other._int;
			}
			else if(base_type == base_type::k_float){
				return _float == other._float;
			}
			else if(base_type == base_type::k_string){
				return _string == other._string;
			}
			else if(base_type == base_type::k_json_value){
				return *_json_value == *other._json_value;
			}
			else if(base_type == base_type::k_typeid){
				return _typeid_value == other._typeid_value;
			}
			else if(base_type == base_type::k_struct){
				return *_struct == *other._struct;
			}
			else if(base_type == base_type::k_vector){
				return *_vector == *other._vector;
			}
			else if(base_type == base_type::k_dict){
				return *_dict == *other._dict;
			}
			else if(base_type == base_type::k_function){
				return *_function == *other._function;
			}
			else {
				QUARK_ASSERT(false);
				throw std::exception();
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



		public: typeid_t get_type() const{
			QUARK_ASSERT(check_invariant());

			return _typeid;
		}



		public: static value_t make_null();
		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_null;
		}



		public: static value_t make_bool(bool value);
		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_bool;
		}
		public: bool get_bool_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_bool()){
				throw std::runtime_error("Type mismatch!");
			}

			return _bool;
		}

		public: static value_t make_int(int value);
		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_int;
		}
		public: int get_int_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_int()){
				throw std::runtime_error("Type mismatch!");
			}

			return _int;
		}

		public: static value_t make_float(float value);
		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_float;
		}
		public: float get_float_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_float()){
				throw std::runtime_error("Type mismatch!");
			}

			return _float;
		}

		public: static value_t make_string(const std::string& value);
		public: static inline value_t make_string(const char value[]){
			return make_string(std::string(value));
		}
		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_string;
		}
		public: std::string get_string_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_string()){
				throw std::runtime_error("Type mismatch!");
			}

			return _string;
		}

		public: static value_t make_json_value(const json_t& v);
		public: bool is_json_value() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_json_value;
		}
		public: json_t get_json_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_json_value()){
				throw std::runtime_error("Type mismatch!");
			}

			return *_json_value;
		}

		public: static value_t make_typeid_value(const typeid_t& type_id);
		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_typeid;
		}
		public: typeid_t get_typeid_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_typeid()){
				throw std::runtime_error("Type mismatch!");
			}

			return _typeid_value;
		}

		public: static value_t make_struct_value(const typeid_t& struct_type, const std::vector<value_t>& values);
		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_struct;
		}
		public: std::shared_ptr<struct_instance_t> get_struct_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_struct()){
				throw std::runtime_error("Type mismatch!");
			}

			return _struct;
		}

		public: static value_t make_vector_value(const typeid_t& element_type, const std::vector<value_t>& elements);
		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_vector;
		}
		public: std::shared_ptr<vector_instance_t> get_vector_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_vector()){
				throw std::runtime_error("Type mismatch!");
			}

			return _vector;
		}

		public: static value_t make_dict_value(const typeid_t& value_type, const std::map<std::string, value_t>& elements);
		public: bool is_dict() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_dict;
		}
		public: std::shared_ptr<dict_instance_t> get_dict_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_dict()){
				throw std::runtime_error("Type mismatch!");
			}

			return _dict;
		}

		public: static value_t make_function_value(const function_definition_t& def);
		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_function;
		}
		public: std::shared_ptr<const function_instance_t> get_function_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_function()){
				throw std::runtime_error("Type mismatch!");
			}

			return _function;
		}



		public: void swap(value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

#if DEBUG
			std::swap(DEBUG_STR, other.DEBUG_STR);
#endif

			_typeid.swap(other._typeid);

			std::swap(_bool, other._bool);
			std::swap(_int, other._int);
			std::swap(_float, other._float);
			std::swap(_string, other._string);
			std::swap(_json_value, other._json_value);
			std::swap(_typeid_value, other._typeid_value);
			std::swap(_struct, other._struct);
			std::swap(_vector, other._vector);
			std::swap(_dict, other._dict);
			std::swap(_function, other._function);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}

		private: static int compare_struct_true_deep(const struct_instance_t& left, const struct_instance_t& right);


		////////////////////		STATE

#if DEBUG
		private: std::string DEBUG_STR;
#endif
		private: typeid_t _typeid;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::shared_ptr<json_t> _json_value;
		private: typeid_t _typeid_value = typeid_t::make_null();
		private: std::shared_ptr<struct_instance_t> _struct;
		private: std::shared_ptr<vector_instance_t> _vector;
		private: std::shared_ptr<dict_instance_t> _dict;
		private: std::shared_ptr<const function_instance_t> _function;
	};


}	//	floyd

#endif /* parser_value_hpp */
