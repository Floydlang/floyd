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
	struct struct_def_t;
	struct type_identifier_t;

	//////////////////////////////////////////////////		struct_instance_t


	struct struct_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other);

		//	??? Remove this points at later time, when we statically track the type of structs OK.
		const struct_def_t* __def;

		//	Use index of member to find the value.
//???		std::vector<value_t> _member_values;
		std::map<std::string, value_t> _member_values;
	};

	std::string to_preview(const struct_instance_t& instance);



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
				if(_struct_instance){
					QUARK_ASSERT(_struct_instance->check_invariant());
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

		public: value_t(const std::shared_ptr<struct_instance_t>& struct_instance) :
			_type(struct_instance->__def->_name),
			_struct_instance(struct_instance)
		{
			QUARK_ASSERT(check_invariant());
		}

		value_t(const value_t& other):
			_type(other._type),

			_bool(other._bool),
			_int(other._int),
			_float(other._float),
			_string(other._string),
			_struct_instance(other._struct_instance)
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
				if(_struct_instance){
					return *_struct_instance == *other._struct_instance;
				}
				else{
					QUARK_ASSERT(false);
				}
			}
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
				return std::string("'") + _string + "'";
			}
			else{
				if(_struct_instance){
					return to_preview(*_struct_instance);
				}
				else{
					return "???";
				}
			}
		}

		std::string value_and_type_to_string() const {
			QUARK_ASSERT(check_invariant());

			return "<" + _type.to_string() + ">" + plain_value_to_string();
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

		public: bool is_struct_instance() const {
			QUARK_ASSERT(check_invariant());

			return _struct_instance ? true : false
			;
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

		public: std::shared_ptr<struct_instance_t> get_struct_instance() const{
			QUARK_ASSERT(check_invariant());
			if(!is_struct_instance()){
				throw std::runtime_error("Type mismatch!");
			}

			return _struct_instance;
		}

		public: void swap(value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

			_type.swap(other._type);

			std::swap(_bool, other._bool);
			std::swap(_int, other._int);
			std::swap(_float, other._float);
			std::swap(_string, other._string);
			std::swap(_struct_instance, other._struct_instance);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}


		////////////////		STATE

		private: type_identifier_t _type;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::shared_ptr<struct_instance_t> _struct_instance;

	};


	void trace(const value_t& e);


}	//	floyd_parser

#endif /* parser_value_hpp */
