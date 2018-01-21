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

#include "floyd_basics.h"

namespace floyd {
	struct statement_t;
	struct expression_t;
	struct value_t;
	struct interpreter_t;
	struct function_definition_t;


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

	std::string to_compact_string(const struct_instance_t& instance);
	json_t to_json(const struct_instance_t& instance);


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

	std::string to_compact_string(const vector_instance_t& instance);






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
		public: json_t to_json() const;


		const std::vector<member_t> _args;
		const std::vector<std::shared_ptr<statement_t>> _statements;
		const int _host_function;
		const typeid_t _return_type;
	};

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);

	std::string to_string(const function_definition_t& v);

	typeid_t get_function_type(const function_definition_t& f);



	//////////////////////////////////////////////////		function_instance_t


	struct function_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const function_instance_t& other) const;


		public: function_definition_t _def;
	};





	//////////////////////////////////////		vector_def_t


	/*
		Notice that vector has no scope of its own.
	*/
	struct vector_def_t {
		public: static vector_def_t make2(const floyd::typeid_t& element_type);

		public: vector_def_t(floyd::typeid_t element_type) :
			_element_type(element_type)
		{
		}
		public: bool check_invariant() const;
		public: bool operator==(const vector_def_t& other) const;


		/////////////////////////////		STATE
		public: floyd::typeid_t _element_type;
	};

	void trace(const vector_def_t& e);
	json_t vector_def_to_json(const vector_def_t& s);




	//////////////////////////////////////////////////		value_t

	/*
		Hold a value with an explicit type.
		Immutable

		- Values of basic types like ints and strings
		- Struct instances
		- Function "pointers"
		- Vectors instances
		
		NOTICE: Encoding is very inefficient at the moment.
	*/

	struct value_t {
		public: bool check_invariant() const{
			QUARK_ASSERT(_typeid.check_invariant());

			const auto base_type = _typeid.get_base_type();
			if(base_type == base_type::k_null){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}
			else if(base_type == base_type::k_bool){
//				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}
			else if(base_type == base_type::k_int){
				QUARK_ASSERT(_bool == false);
//				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}
			else if(base_type == base_type::k_float){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
//				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}
			else if(base_type == base_type::k_string){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
//				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}

			else if(base_type == base_type::k_typeid){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);
			}
			else if(base_type == base_type::k_struct){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct != nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function == nullptr);

				QUARK_ASSERT(_struct && _struct->check_invariant());
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector != nullptr);
				QUARK_ASSERT(_function == nullptr);

				QUARK_ASSERT(_vector && _vector->check_invariant());
				QUARK_ASSERT(_typeid.get_vector_element_type() == _vector->_element_type);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(_bool == false);
				QUARK_ASSERT(_int == 0);
				QUARK_ASSERT(_float == 0.0f);
				QUARK_ASSERT(_string == "");
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector == nullptr);
				QUARK_ASSERT(_function != nullptr);

				QUARK_ASSERT(_function && _function->check_invariant());
			}
			else if(base_type == base_type::k_unknown_identifier){
				//	 Cannot have a value of unknown type.
				QUARK_ASSERT(false);
			}
			else {
				QUARK_ASSERT(false);
			}
			return true;
		}

		public: value_t() :
			_typeid(typeid_t::make_null())
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(bool value) :
			_typeid(typeid_t::make_bool()),
			_bool(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(int value) :
			_typeid(typeid_t::make_int()),
			_int(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_t(float value) :
			_typeid(typeid_t::make_float()),
			_float(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const char s[]) :
			_typeid(typeid_t::make_string()),
			_string(s)
		{
			QUARK_ASSERT(s != nullptr);

			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const std::string& s) :
			_typeid(typeid_t::make_string()),
			_string(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const typeid_t& type) :
			_typeid(typeid_t::make_typeid(type))
		{
			QUARK_ASSERT(type.check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const typeid_t& struct_type, const std::shared_ptr<struct_instance_t>& instance) :
			_typeid(struct_type),
			_struct(instance)
		{
			QUARK_ASSERT(struct_type.get_base_type() == base_type::k_struct);
			QUARK_ASSERT(instance && instance->check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const std::shared_ptr<vector_instance_t>& instance) :
			_typeid(typeid_t::make_vector(instance->_element_type)),
			_vector(instance)
		{
			QUARK_ASSERT(instance && instance->check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		public: explicit value_t(const std::shared_ptr<function_instance_t>& function_instance) :
			_typeid(get_function_type(function_instance->_def)),
			_function(function_instance)
		{
			QUARK_ASSERT(function_instance && function_instance->check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		value_t(const value_t& other):
			_typeid(other._typeid),

			_bool(other._bool),
			_int(other._int),
			_float(other._float),
			_string(other._string),
			_struct(other._struct),
			_vector(other._vector),
			_function(other._function)
		{
			QUARK_ASSERT(other.check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		value_t& operator=(const value_t& other){
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

			else if(base_type == base_type::k_typeid){
				return true;
			}
			else if(base_type == base_type::k_struct){
				return *_struct == *other._struct;
			}
			else if(base_type == base_type::k_vector){
				return *_vector == *other._vector;
			}
			else if(base_type == base_type::k_function){
				return *_function == *other._function;
			}
			else {
				QUARK_ASSERT(false);
			}
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

		public: bool operator!=(const value_t& other) const{
			return !(*this == other);
		}

		/*
			"true"
			"0"
			"1003"
			"Hello, world"
			Notice, strings don't get wrapped in "".
		*/
		std::string to_compact_string() const {
			QUARK_ASSERT(check_invariant());

			const auto base_type = _typeid.get_base_type();
			if(base_type == base_type::k_null){
				return "<null>";
			}
			else if(base_type == base_type::k_bool){
				return _bool ? keyword_t::k_true : keyword_t::k_false;
			}
			else if(base_type == base_type::k_int){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%d", _int);
				return std::string(temp);
			}
			else if(base_type == base_type::k_float){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%f", _float);
				return std::string(temp);
			}
			else if(base_type == base_type::k_string){
				return _string;
			}

			else if(base_type == base_type::k_typeid){
				return floyd::typeid_to_compact_string(_typeid);
			}
			else if(base_type == base_type::k_struct){
				return floyd::to_compact_string(*_struct);
			}
			else if(base_type == base_type::k_vector){
				return floyd::to_compact_string(*_vector);
			}
			else if(base_type == base_type::k_function){
				return floyd::typeid_to_compact_string(_typeid);
			}
			else if(base_type == base_type::k_unknown_identifier){
				QUARK_ASSERT(false);
				return "";
			}

			else{
				return "??";
			}
		}

		//	Special handling of strings, we want to wrap in "".
		std::string to_compact_string_quote_strings() const {
			const auto s = to_compact_string();
			if(is_string()){
				return "\"" + s + "\"";
			}
			else{
				return s;
			}
		}

		/*
			bool: "true"
			int: "0"
			string: "1003"
			string: "Hello, world"
		*/
		std::string value_and_type_to_string() const {
			QUARK_ASSERT(check_invariant());

			if(is_null()){
				return "<null>";
			}
			std::string type_string = floyd::typeid_to_compact_string(_typeid);
			return type_string + ": " + to_compact_string_quote_strings();
		}

		public: typeid_t get_type() const{
			QUARK_ASSERT(check_invariant());

			return _typeid;
		}

		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_null;
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_bool;
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_int;
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_float;
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_string;
		}

		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_typeid;
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_struct;
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_vector;
		}

		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type() == base_type::k_function;
		}

		public: bool get_bool_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_bool()){
				throw std::runtime_error("Type mismatch!");
			}

			return _bool;
		}

		public: int get_int_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_int()){
				throw std::runtime_error("Type mismatch!");
			}

			return _int;
		}

		public: float get_float_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_float()){
				throw std::runtime_error("Type mismatch!");
			}

			return _float;
		}

		public: std::string get_string_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_string()){
				throw std::runtime_error("Type mismatch!");
			}

			return _string;
		}

		public: typeid_t get_typeid_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_typeid()){
				throw std::runtime_error("Type mismatch!");
			}

			return _typeid.get_typeid_typeid();
		}

		public: std::shared_ptr<struct_instance_t> get_struct_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_struct()){
				throw std::runtime_error("Type mismatch!");
			}

			return _struct;
		}

		public: std::shared_ptr<vector_instance_t> get_vector_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_vector()){
				throw std::runtime_error("Type mismatch!");
			}

			return _vector;
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

			_typeid.swap(other._typeid);

			std::swap(_bool, other._bool);
			std::swap(_int, other._int);
			std::swap(_float, other._float);
			std::swap(_string, other._string);
			std::swap(_struct, other._struct);
			std::swap(_vector, other._vector);
			std::swap(_function, other._function);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}


		private: static int compare_value_true_deep(const struct_instance_t& left, const struct_instance_t& right);


		////////////////		STATE
		private: typeid_t _typeid;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::shared_ptr<struct_instance_t> _struct;
		private: std::shared_ptr<vector_instance_t> _vector;
		private: std::shared_ptr<const function_instance_t> _function;
	};


	inline value_t make_typeid_value(const typeid_t& type_id){
		return value_t(type_id);
	}
	inline value_t make_struct_value(const typeid_t& struct_type, const struct_definition_t& def, const std::vector<value_t>& values){
		QUARK_ASSERT(struct_type.get_base_type() != base_type::k_unknown_identifier);
		QUARK_ASSERT(def.check_invariant());

		auto f = std::shared_ptr<struct_instance_t>(new struct_instance_t{def, values});
		return value_t(struct_type, f);
	}
	inline value_t make_vector_value(const typeid_t& element_type, const std::vector<value_t>& elements){
		auto f = std::shared_ptr<vector_instance_t>(new vector_instance_t{element_type, elements});
		return value_t(f);
	}

	inline value_t make_function_value(const function_definition_t& def){
		auto f = std::shared_ptr<function_instance_t>(new function_instance_t{def});
		return value_t(f);
	}

	void trace(const value_t& e);
	json_t value_to_json(const value_t& v);
	value_t make_test_func();


}	//	floyd

#endif /* parser_value_hpp */
