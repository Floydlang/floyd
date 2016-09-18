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
	struct type_identifier_t;
	struct resolved_path_t;

	//////////////////////////////////////////////////		struct_instance_t


	//??? Merge into stack_frame_t to make a generic construct.
	struct struct_instance_t {
		public: struct_instance_t(const scope_ref_t& def, const std::map<std::string, value_t>& member_values) :
			__def(def),
			_member_values(member_values)
		{
			QUARK_ASSERT(def && def->check_invariant());
			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other);

		//	??? Remove this points at later time, when we statically track the type of structs OK. We alreay know this via __def!
		scope_ref_t __def;

		//	??? Use ::vector<value_t> _member_values and index of member to find the value.
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

			//??? Use better way to compare types!!!
			// Use the base_type enum instead of strings for basic types? Use string for basics, hash for composite, functions, typedefs?

			if(_type.to_string() != other._type.to_string()){
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
					return "??";
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

			return _type.to_string() == "null";
		}

		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _type.to_string() == "bool";
		}

		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _type.to_string() == "int";
		}

		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _type.to_string() == "float";
		}

		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _type.to_string() == "string";
		}

		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _struct ? true : false;
		}

		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _vector ? true : false;
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

		public: value_t resolve_type(const type_identifier_t& resolved) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(resolved.check_invariant());
			QUARK_ASSERT(get_type().to_string() == resolved.to_string());

			value_t result = *this;
			result._type = resolved;
			QUARK_ASSERT(result.check_invariant());
			return result;
		}

		public: bool is_type_resolved() const{
			QUARK_ASSERT(check_invariant());
			return _type.is_resolved();
		}


		private: static int compare_value_true_deep(const struct_instance_t& left, const struct_instance_t& right);


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


//	value_t make_struct_instance(const resolved_path_t& path, const scope_ref_t& struct_def);
//	value_t make_vector_instance(const std::shared_ptr<const vector_def_t>& def, const std::vector<value_t>& elements);



#if false
struct struct_fixture_t {
	public: ast_t _ast;
	public: scope_ref_t _struct6_def;
	public: value_t _struct6_instance0;
	public: value_t _struct6_instance1;

	public: struct_fixture_t();
};
#endif


}	//	floyd_parser

#endif /* parser_value_hpp */
