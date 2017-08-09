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

#include "parser_ast.h"

namespace floyd_parser {
	struct statement_t;
	struct value_t;
	struct typeid_t;

	//////////////////////////////////////////////////		struct_instance_t

	/*
		An instance of a struct-type = a value of this struct.
		Each memeber is initialized.

		### Merge into environment_t to make a generic construct.
	*/
	struct struct_instance_t {
		public: struct_instance_t(const typeid_t& struct_type, const std::map<std::string, value_t>& member_values) :
			_struct_type(struct_type),
			_member_values(member_values)
		{
			QUARK_ASSERT(struct_type.check_invariant());
			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other);

		//	??? Remove this pointer at later time, when we statically track the type of structs OK. We alreay know this via __def!
		public: typeid_t _struct_type;

		//	??? Use ::vector<value_t> _member_values and index of member to find the value.
		public: std::map<std::string, value_t> _member_values;
	};

	std::string to_preview(const struct_instance_t& instance);


	//////////////////////////////////////////////////		vector_instance_t


	struct vector_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const vector_instance_t& other);

		//	??? Remove this at later time, when we statically track the type of structs OK.
		typeid_t _vector_type;

		std::vector<value_t> _elements;
	};

	std::string to_preview(const vector_instance_t& instance);


	//////////////////////////////////////////////////		function_instance_t


	struct function_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const function_instance_t& other);


		public: typeid_t _function_type;
		public: int _function_id;
	};




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

		public: value_t(const std::shared_ptr<struct_instance_t>& instance) :
			_typeid(instance->_struct_type),
			_struct(instance)
		{
			QUARK_ASSERT(instance && instance->check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		public: value_t(const std::shared_ptr<vector_instance_t>& instance) :
			_typeid(instance->_vector_type),
			_vector(instance)
		{
			QUARK_ASSERT(instance && instance->check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		public: value_t(const std::shared_ptr<function_instance_t>& function_instance) :
			_typeid(function_instance->_function_type),
			_function(function_instance)
		{
			QUARK_ASSERT(function_instance && function_instance->check_invariant());
			QUARK_ASSERT(function_instance->_function_type.get_base_type() == base_type::k_function);

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

			//??? Use better way to compare types!!!
			// Use the base_type enum instead of strings for basic types? Use string for basics, hash for composite, functions, typedefs?

			if(!(_typeid == other._typeid)){
				return false;
			}

			const auto base_type = get_base_type();
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
			"false"
			"0"
			"1003"
			"Hello, world",
		*/
		std::string plain_value_to_string() const {
			QUARK_ASSERT(check_invariant());

			const auto base_type = get_base_type();
			if(base_type == base_type::k_null){
				return "<null>";
			}
			else if(base_type == base_type::k_bool){
				return _bool ? "true" : "false";
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
				return std::string("\"") + _string + "\"";
			}

			else if(base_type == base_type::k_struct){
				return to_preview(*_struct);
			}
			else if(base_type == base_type::k_vector){
				return to_preview(*_vector);
			}
			else if(base_type == base_type::k_function){
				return json_to_compact_string(typeid_to_json(_typeid));
			}

			else{
				return "??";
			}
		}

		std::string value_and_type_to_string() const {
			QUARK_ASSERT(check_invariant());

			if(is_null()){
				return "<null>";
			}
			else{
				std::string type_string = json_to_compact_string(typeid_to_json(_typeid));
				return type_string + ": " + plain_value_to_string();
			}
		}

		public: typeid_t get_type() const{
			QUARK_ASSERT(check_invariant());

			return _typeid;
		}

		public: base_type get_base_type() const {
			QUARK_ASSERT(check_invariant());

			return _typeid.get_base_type();
		}

		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_null;
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_bool;
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_int;
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_float;
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_string;
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_struct;
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_vector;
		}

		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return get_base_type() == base_type::k_function;
		}

		public: bool get_bool() const{
			QUARK_ASSERT(check_invariant());
			if(!is_bool()){
				throw std::runtime_error("Type mismatch!");
			}

			return _bool;
		}

		public: int get_int() const{
			QUARK_ASSERT(check_invariant());
			if(!is_int()){
				throw std::runtime_error("Type mismatch!");
			}

			return _int;
		}

		public: float get_float() const{
			QUARK_ASSERT(check_invariant());
			if(!is_float()){
				throw std::runtime_error("Type mismatch!");
			}

			return _float;
		}

		public: std::string get_string() const{
			QUARK_ASSERT(check_invariant());
			if(!is_string()){
				throw std::runtime_error("Type mismatch!");
			}

			return _string;
		}

		public: std::shared_ptr<struct_instance_t> get_struct() const{
			QUARK_ASSERT(check_invariant());
			if(!is_struct()){
				throw std::runtime_error("Type mismatch!");
			}

			return _struct;
		}

		public: std::shared_ptr<vector_instance_t> get_vector() const{
			QUARK_ASSERT(check_invariant());
			if(!is_vector()){
				throw std::runtime_error("Type mismatch!");
			}

			return _vector;
		}

		public: std::shared_ptr<const function_instance_t> get_function() const{
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
		private: std::shared_ptr<function_instance_t> _function;
	};



	inline value_t make_function_value(const typeid_t& function_type, int function_id){
		auto f = std::shared_ptr<function_instance_t>(new function_instance_t{function_type, function_id});
		return value_t(f);
	}



	void trace(const value_t& e);

	json_t value_to_json(const value_t& v);

#if false
struct struct_fixture_t {
	public: ast_t _ast;
	public: scope_ref_t _struct6_def;
	public: value_t _struct6_instance0;
	public: value_t _struct6_instance1;

	public: struct_fixture_t();
};
#endif

	value_t make_test_func();


}	//	floyd_parser

#endif /* parser_value_hpp */
