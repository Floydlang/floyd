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

////////////////////////////////////////		json_t


struct json_t {
	public: static json_t make_object(){
		return make_object({});
	}
	public: static json_t make_object(const std::map<std::string, json_t>& m){
		return json_t(m);
	}

	public: static json_t make_array(){
		return make_array({});
	}
	public: static json_t make_array(const std::vector<json_t>& elements){
		return json_t(elements);
	}

	public: json_t(const std::map<std::string, json_t>& object) :
		_type(k_object),
		_object(object)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(const std::vector<json_t>& array) :
		_type(k_array),
		_array(array)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(const std::string& s) :
		_type(k_string),
		_string(s)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(const char s[]) :
		_type(k_string),
		_string(std::string(s))
	{
		QUARK_ASSERT(s != nullptr);
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(double number) :
		_type(k_number),
		_number(number)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(int number) :
		_type(k_number),
		_number((double)number)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}
	public: json_t(int64_t number) :
		_type(k_number),
		_number((double)number)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t(bool value) :
		_type(value ? k_true : k_false)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_t() :
		_type(k_null)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const;
	public: json_t(const json_t& other);
	public: json_t& operator=(const json_t& other);
	public: void swap(json_t& other);

	//	Deep equality compare.
	public: bool operator==(const json_t& other) const;
	public: bool operator!=(const json_t& other) const{
		return !(*this == other);
	}

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

	explicit operator bool() const {
		QUARK_ASSERT(check_invariant());
		return is_null() ? false : true;
	}

	bool is_object() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_object;
	}

	const std::map<std::string, json_t>& get_object() const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _object;
	}

	/*
		Throws exception if this isn't an object.
		Throws exception if key doesn't exist.
	*/
	const json_t& get_object_element(const std::string& key) const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _object.at(key);
	}

	/*
		Throws exception if this isn't an object.
		if key doesn't exist, return def_value.
	*/
	json_t get_optional_object_element(const std::string& key, const json_t& def_value = json_t()) const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		if(does_object_element_exist(key)){
			return get_object_element(key);
		}
		else{
			return def_value;
		}
	}

	const bool does_object_element_exist(const std::string& key) const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _object.find(key) != _object.end();
	}

	size_t get_object_size() const {
		QUARK_ASSERT(check_invariant());

		if(!is_object()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _object.size();
	}


	bool is_array() const {
		QUARK_ASSERT(check_invariant());

		return _type == k_array;
	}

	/*
		Throws exception if this isn't an array.
	*/
	const std::vector<json_t>& get_array() const {
		QUARK_ASSERT(check_invariant());

		if(!is_array()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _array;
	}

	const json_t& get_array_n(size_t index) const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(index >= 0);

		if(!is_array()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		QUARK_ASSERT(index < _array.size());
		return _array[index];
	}

	size_t get_array_size() const {
		QUARK_ASSERT(check_invariant());

		if(!is_array()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _array.size();
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
	//	??? Make this fast to copy = move map/ vector into shared_ptr.
	//	??? Should use std::variant.
	private: std::string __debug;
	private: etype _type = k_null;
	private: std::map<std::string, json_t> _object;
	private: std::vector<json_t> _array;
	private: std::string _string;
	private: double _number = 0.0;
};


////////////////////////////////////////		HELPERS

/*
	Used for unit tests, if pretty-string version of inputs are different, trace them and fail.
*/
struct seq_t;

void ut_compare(const quark::call_context_t& context, const json_t& result, const json_t& expected);
void ut_compare(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result, const std::pair<json_t, seq_t>& expected);



inline std::vector<json_t> make_vec(std::initializer_list<json_t> args){
	return std::vector<json_t>(args.begin(), args.end());
}

json_t make_object(const std::vector<std::pair<std::string, json_t>>& entries);
json_t store_object_member(const json_t& obj, const std::string& key, const json_t& value);

/*
	Adds element last to this vector, or inserts it if object is a map.
*/
json_t push_back(const json_t& obj, const json_t& element);

json_t make_array_skip_nulls(const std::vector<json_t>& elements);


bool exists_in(const json_t& parent, const std::vector<json_t>& path);

json_t get_in(const json_t& parent, const std::vector<json_t>& path);


/*
	Works on both arrays and objects.
	Arrays entries can be overwritten or appended precisely at end of array.
*/
json_t assoc(const json_t& parent, const json_t& member, const json_t& new_element);

/*
	Will walk a path of maps and arrays and add/replace the new element to the last object.
	Path nodes are string for object-member, number for array-index.
	Arrays entries can be overwritten or appended precisely at end of array.

	Entries in path are strings for specifying object-members or integer number for array indexes.
	If node is missing, an object is created for it.
*/
json_t assoc_in(const json_t& parent, const std::vector<json_t>& path, const json_t& new_element);

/*
	Erase member
*/
json_t dissoc(const json_t& value, const json_t& key);


//	Converts the value into a vector of strings.
std::vector<std::string> to_string_vec(const json_t& json);

json_t from_string_vec(const std::vector<std::string>& vec);


#endif /* json_support_hpp */
