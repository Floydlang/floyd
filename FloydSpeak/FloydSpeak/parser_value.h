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

#include "parser_types.h"
#include "parser_primitives.h"


namespace floyd_parser {
	struct statement_t;
	struct value_t;






	//////////////////////////////////////////////////		c_function_spec_t

	/*
		A C-function callback, used to register C-functions that the Floyd program can call into host.
	*/

	struct c_function_spec_t {
		std::vector<arg_t> _arguments;
		type_identifier_t _return_type;
	};

	typedef value_t (*c_function_t)(const std::vector<arg_t>& args);



	//////////////////////////////////////////////////		value_t

	/*
		Hold a value with an explicit type.
		Used only for parsing. Encoding is very inefficient.
	*/

	struct value_t {
		public: bool check_invariant() const{
			QUARK_ASSERT((_type.to_string() == "__data_type_value") == (_data_type_value != nullptr));
			return true;
		}

		public: value_t() :
			_type("null")
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

		public: value_t(int value) :
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

		public: value_t(const type_identifier_t& s) :
			_type("__data_type_value"),
			_data_type_value(std::make_shared<type_identifier_t>(s))
		{
			QUARK_ASSERT(s.check_invariant());

			QUARK_ASSERT(check_invariant());
		}

		public: value_t(c_function_t f, const c_function_spec_t& spec) :
			_type("__c_function"),
			_c_function(f),
			_c_spec(std::make_shared<c_function_spec_t>(spec))
		{
			QUARK_ASSERT(f != nullptr);

			QUARK_ASSERT(check_invariant());
		}

		value_t(const value_t& other):
			_type(other._type),

			_bool(other._bool),
			_int(other._int),
			_float(other._float),
			_string(other._string),
			_function_id(other._function_id),
			_data_type_value(other._data_type_value)
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

			return _type == other._type && _bool == other._bool && _int == other._int && _float == other._float && _string == other._string && _function_id == other._function_id && _data_type_value == other._data_type_value;
		}
		std::string plain_value_to_string() const {
			QUARK_ASSERT(check_invariant());

			const auto d = _type.to_string();
			if(d == "null"){
				return "<null>";
			}
	/*
			else if(_type == "bool"){
				return _bool ? "true" : "false";
			}
	*/

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
			else if(d == "function_id"){
				return _function_id;
			}
			else if(d == "__data_type_value"){//???
				return _data_type_value->to_string();
			}
			else{
				return "???";
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

		public: int get_int() const{
			QUARK_ASSERT(check_invariant());

			return _int;
		}

		public: float get_float() const{
			QUARK_ASSERT(check_invariant());

			return _float;
		}

		public: std::string get_string() const{
			QUARK_ASSERT(check_invariant());

			return _string;
		}

		public: void swap(value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

			_type.swap(other._type);

			std::swap(_bool, other._bool);
			std::swap(_int, other._int);
			std::swap(_float, other._float);
			std::swap(_string, other._string);
			std::swap(_function_id, other._function_id);
			std::swap(_data_type_value, other._data_type_value);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}


		////////////////		STATE

		private: type_identifier_t _type;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::string _function_id = "";
		private: std::shared_ptr<type_identifier_t> _data_type_value;


		private: c_function_t _c_function;
		private: std::shared_ptr<c_function_spec_t> _c_spec;
	};


	void trace(const value_t& e);


}	//	floyd_parser

#endif /* parser_value_hpp */
