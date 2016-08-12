//
//  json.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef json_support_hpp
#define json_support_hpp

#include <string>
#include <vector>
#include <map>
#include "quark.h"




////////////////////////////////////////		json_value_t



struct json_value_t {
	public: json_value_t(const std::map<std::string, json_value_t>& object) :
		_type(k_object),
		_object(object)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t(const std::vector<json_value_t>& array) :
		_type(k_array),
		_array(array)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(const std::string& s) :
		_type(k_string),
		_string(s)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(const char s[]) :
		_type(k_string),
		_string(std::string(s))
	{
		QUARK_ASSERT(s != nullptr);
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t(double number) :
		_type(k_number),
		_number(number)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(bool value) :
		_type(value ? k_true : k_false)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t() :
		_type(k_null)
	{
		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const;
	public: json_value_t(const json_value_t& other);
	public: json_value_t& operator=(const json_value_t& other);
	public: void swap(json_value_t& other);

	//	Deep equality compare.
	public: bool operator==(const json_value_t& other) const;

	enum etype {
		k_object,
		k_array,
		k_string,
		k_number,
		k_true,
		k_false,
		k_null
	};

	etype get_type() const {
		QUARK_ASSERT(check_invariant());
		return _type;
	}

	bool is_object() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_object;
	}

	const std::map<std::string, json_value_t>& get_object() const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _object;
	}


	bool is_array() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_array;
	}

	const std::vector<json_value_t>& get_array() const {
		QUARK_ASSERT(check_invariant());

		if(!is_array()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _array;
	}


	bool is_string() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_string;
	}

	const std::string get_string() const {
		QUARK_ASSERT(check_invariant());

		if(!is_string()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _string;
	}


	bool is_number() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_number;
	}

	const double get_number() const {
		QUARK_ASSERT(check_invariant());

		if(!is_number()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _number;
	}


	bool is_true() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_true;
	}
	bool is_false() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_false;
	}
	bool is_null() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_null;
	}

	/////////////////////////////////////		STATE

	private: etype _type = k_null;
	private: std::map<std::string, json_value_t> _object;
	private: std::vector<json_value_t> _array;
	private: std::string _string;
	private: double _number = 0.0;
};



#endif /* json_support_hpp */
