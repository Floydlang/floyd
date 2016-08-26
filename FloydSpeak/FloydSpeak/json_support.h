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


#include "json_writer.h"

/*
PRETTY FORMAT FOR READING:

[
  "+",
  [ "load", [ "->", [ "@", "p" ],"s" ] ],
  [ "load", [ "@", "a" ] ]
]
*/

////////////////////////////////////////		json_value_t

/*???
template <typename T>
std::vector<T> filter(const std::vector<T>& source, bool (const T& entry) predicate){
	std::vector<json_value_t> temp;
	std::copy_if(soure.begin(), source.end(), temp.begin(), predicate);
	return temp;
}
*/

struct json_value_t {
	public: static json_value_t make_object(const std::map<std::string, json_value_t>& m){
		return json_value_t(m);
	}

	public: static json_value_t make_array2(const std::vector<json_value_t>& elements){
		return json_value_t(elements);
	}

	public: static json_value_t make_array_skip_nulls(const std::vector<json_value_t>& elements){
#if 1
		for(const auto& i: elements){ QUARK_ASSERT(i.check_invariant()); }

		std::vector<json_value_t> elements2;
		std::copy_if(elements.begin(), elements.end(), std::back_inserter(elements2), [&] (const json_value_t& v) { return !v.is_null(); });
		return json_value_t(elements2);
#else
		return make_array2(elements);
#endif
	}

	public: json_value_t(const std::map<std::string, json_value_t>& object) :
		_type(k_object),
		_object(object)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t(const std::vector<json_value_t>& array) :
		_type(k_array),
		_array(array)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(const std::string& s) :
		_type(k_string),
		_string(s)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(const char s[]) :
		_type(k_string),
		_string(std::string(s))
	{
		QUARK_ASSERT(s != nullptr);
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t(double number) :
		_type(k_number),
		_number(number)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: explicit json_value_t(bool value) :
		_type(value ? k_true : k_false)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t() :
		_type(k_null)
	{
		__debug = json_to_compact_string(*this);
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

	private: std::string __debug;
	private: etype _type = k_null;
	private: std::map<std::string, json_value_t> _object;
	private: std::vector<json_value_t> _array;
	private: std::string _string;
	private: double _number = 0.0;
};


	json_value_t make_object(const std::vector<std::pair<std::string, json_value_t>>& entries);



#endif /* json_support_hpp */
