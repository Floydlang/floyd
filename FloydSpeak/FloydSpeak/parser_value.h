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

//??? add float & vector types to value_t


namespace floyd_parser {
	struct statement_t;
	struct value_t;
	struct struct_def_t;
	struct type_identifier_t;

	//////////////////////////////////////////////////		struct_instance_t


	struct struct_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other);

		//	??? Remove this points at later time, when we statically track the type of structs OK.
		std::shared_ptr<const struct_def_t> __def;

		//	### Use ::vector<value_t> _member_values and index of member to find the value.
		std::map<std::string, value_t> _member_values;
	};

	std::string to_preview(const struct_instance_t& instance);



	//////////////////////////////////////////////////		vector_instance_t


	struct vector_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const vector_instance_t& other);

		//	??? Remove this points at later time, when we statically track the type of structs OK.
		std::shared_ptr<const vector_def_t> __def;

		std::vector<value_t> _elements;
	};

	std::string to_preview(const vector_instance_t& instance);



	//////////////////////////////////////////////////		value_t

	/*
		Hold a value with an explicit type.
		Used only for parsing. Encoding is very inefficient.

		null
		bool
		int
		float
		string
		struct
		vector
	*/

	struct value_t {
		public: bool check_invariant() const{
			if(_type.to_string() == "null"){
			}
			else if(_type.to_string() == "bool"){
			}
			else if(_type.to_string() == "int"){
			}
			else if(_type.to_string() == "float"){
			}
			else if(_type.to_string() == "string"){
			}
			else {
				if(_struct){
					QUARK_ASSERT(_struct->check_invariant());
				}
				else if(_vector){
					QUARK_ASSERT(_vector->check_invariant());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			return true;
		}

		public: value_t() :
			_type("null")
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(bool value) :
			_type("bool"),
			_bool(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(int value) :
			_type("int"),
			_int(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_t(float value) :
			_type("float"),
			_float(value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const char s[]) :
			_type("string"),
			_string(s)
		{
			QUARK_ASSERT(s != nullptr);

			QUARK_ASSERT(check_invariant());
		}

		public: explicit value_t(const std::string& s) :
			_type("string"),
			_string(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_t(const std::shared_ptr<struct_instance_t>& instance) :
			_type(instance->__def->_name),
			_struct(instance)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_t(const std::shared_ptr<vector_instance_t>& instance) :
			_type(instance->__def->_name),
			_vector(instance)
		{
			QUARK_ASSERT(check_invariant());
		}

		value_t(const value_t& other):
			_type(other._type),

			_bool(other._bool),
			_int(other._int),
			_float(other._float),
			_string(other._string),
			_struct(other._struct),
			_vector(other._vector)
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

			if(_type != other._type){
				return false;
			}
			//??? Use the base_type enum instead of strings for types.
			if(_type.to_string() == "null"){
				return true;
			}
			else if(_type.to_string() == "bool"){
				return _bool == other._bool;
			}
			else if(_type.to_string() == "int"){
				return _int == other._int;
			}
			else if(_type.to_string() == "float"){
				return _float == other._float;
			}
			else if(_type.to_string() == "string"){
				return _string == other._string;
			}
			else {
				if(_struct){
					return *_struct == *other._struct;
				}
				else if(_vector){
					return *_vector == *other._vector;
				}
				else{
					QUARK_ASSERT(false);
				}
			}
		}

		public: bool operator!=(const value_t& other) const{
			return !(*this == other);
		}

		std::string plain_value_to_string() const {
			QUARK_ASSERT(check_invariant());

			const auto d = _type.to_string();
			if(d == "null"){
				return "<null>";
			}
			else if(d == "bool"){
				return _bool ? "true" : "false";
			}
			else if(d == "int"){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%d", _int);
				return std::string(temp);
			}
			else if(d == "float"){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%f", _float);
				return std::string(temp);
			}
			else if(d == "string"){
				return std::string("\"") + _string + "\"";
			}
			else{
				if(_struct){
					return to_preview(*_struct);
				}
				else if(_vector){
					return to_preview(*_vector);
				}
				else{
					return "???";
				}
			}
		}

		std::string to_json() const {
			QUARK_ASSERT(check_invariant());

			const auto d = _type.to_string();
			if(d == "null"){
				return "null";
			}
			else if(d == "bool"){
				return _bool ? "true" : "false";
			}
			else if(d == "int"){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%d", _int);
				return std::string(temp);
			}
			else if(d == "float"){
				char temp[200 + 1];//### Use C++ function instead.
				sprintf(temp, "%f", _float);
				return std::string(temp);
			}
			else if(d == "string"){
				return std::string("\"") + _string + "\"";
			}
			else{
				if(_struct){
					return to_preview(*_struct);
				}
				else if(_vector){
					return to_preview(*_vector);
				}
				else{
					return "???";
				}
			}
		}


		std::string value_and_type_to_string() const {
			QUARK_ASSERT(check_invariant());

			if(is_null()){
				return "<null>";
			}
			else{
				return "<" + _type.to_string() + ">" + plain_value_to_string();
			}
		}

		public: type_identifier_t get_type() const{
			QUARK_ASSERT(check_invariant());

			return _type;
		}

		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _type == type_identifier_t("null");
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _type == type_identifier_t("bool");
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _type == type_identifier_t::make_int();
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _type == type_identifier_t("float");
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _type == type_identifier_t("string");
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _struct ? true : false;
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _vector ? true : false;
		}

		//	???	Use enum from type system instead of strings
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

		public: void swap(value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

			_type.swap(other._type);

			std::swap(_bool, other._bool);
			std::swap(_int, other._int);
			std::swap(_float, other._float);
			std::swap(_string, other._string);
			std::swap(_struct, other._struct);
			std::swap(_vector, other._vector);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}


		////////////////		STATE

		private: type_identifier_t _type;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::shared_ptr<struct_instance_t> _struct;
		private: std::shared_ptr<vector_instance_t> _vector;

	};


	void trace(const value_t& e);





	/*
		Resolves the type, starting at _scope_instances.back() then moving towards to global space. This is a compile-time operation.
	*/
	//??? const version
	std::shared_ptr<floyd_parser::type_def_t> resolve_type(const scope_ref_t scope_def, const std::string& s);


	floyd_parser::value_t make_default_value(const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);

	floyd_parser::value_t make_default_value(const floyd_parser::type_def_t& t);
	floyd_parser::value_t make_default_value(const std::shared_ptr<struct_def_t>& t);

	floyd_parser::value_t make_struct_instance(const std::shared_ptr<const floyd_parser::struct_def_t>& def);
	floyd_parser::value_t make_vector_instance(const std::shared_ptr<const floyd_parser::vector_def_t>& def);


}	//	floyd_parser

#endif /* parser_value_hpp */
