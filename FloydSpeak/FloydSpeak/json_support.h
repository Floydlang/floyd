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
	public: static json_value_t make_object(){
		return make_object({});
	}
	public: static json_value_t make_object(const std::map<std::string, json_value_t>& m){
		return json_value_t(m);
	}

	public: static json_value_t make_array(){
		return make_array2({});
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

	public: json_value_t(const std::string& s) :
		_type(k_string),
		_string(s)
	{
		__debug = json_to_compact_string(*this);
		QUARK_ASSERT(check_invariant());
	}

	public: json_value_t(const char s[]) :
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
	public: bool operator!=(const json_value_t& other) const{
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

	const std::map<std::string, json_value_t>& get_object() const {
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
	const json_value_t& get_object_element(const std::string& key) const {
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
	json_value_t get_optional_object_element(const std::string& key, const json_value_t& def_value = json_value_t()) const {
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
	const std::vector<json_value_t>& get_array() const {
		QUARK_ASSERT(check_invariant());

		if(!is_array()){
			throw std::runtime_error("Wrong type of JSON value");
		}
		return _array;
	}

	const json_value_t& get_array_n(size_t index) const {
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

/*
	class const_iterator
	{
		public:
			typedef const_iterator self_type;
			typedef json_value_t value_type;
			typedef json_value_t& reference;
			typedef json_value_t* pointer;
			typedef int difference_type;
			typedef std::forward_iterator_tag iterator_category;
			const_iterator(pointer ptr) : ptr_(ptr) { }
			self_type operator++() { self_type i = *this; ptr_++; return i; }
			self_type operator++(int junk) { ptr_++; return *this; }
			const reference operator*() { return *ptr_; }
			const pointer operator->() { return ptr_; }
			bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
			bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
		private:
			pointer ptr_;
	};
*/



	/////////////////////////////////////		STATE
	//???? Make this fast to copy = move map/ vector into shared_ptr.
	private: std::string __debug;
	private: etype _type = k_null;
	private: std::map<std::string, json_value_t> _object;
	private: std::vector<json_value_t> _array;
	private: std::string _string;
	private: double _number = 0.0;
};








////////////////////////////////////////		HELPERS


/*
	Used for unit tests, if pretty-string version of inputs are different, trace them and fail.
*/
void ut_compare_jsons(const json_value_t& result, const json_value_t& expected);



inline std::vector<json_value_t> make_vec(std::initializer_list<json_value_t> args){
	return std::vector<json_value_t>(args.begin(), args.end());
}

json_value_t make_object(const std::vector<std::pair<std::string, json_value_t>>& entries);
json_value_t store_object_member(const json_value_t& obj, const std::string& key, const json_value_t& value);

/*
	Adds element last to this vector, or inserts it if object is a map.
*/
json_value_t push_back(const json_value_t& obj, const json_value_t& element);


bool exists_in(const json_value_t& parent, const std::vector<json_value_t>& path);

json_value_t get_in(const json_value_t& parent, const std::vector<json_value_t>& path);


/*
	Works on both arrays and objects.
	Arrays entries can be overwritten or appended precisely at end of array.
*/
json_value_t assoc(const json_value_t& parent, const json_value_t& member, const json_value_t& new_element);

/*
	Will walk a path of maps and arrays and add/replace the new element to the last object.
	Path nodes are string for object-member, number for array-index.
	Arrays entries can be overwritten or appended precisely at end of array.

	Entries in path are strings for specifying object-members or integer number for array indexes.
	If node is missing, an object is created for it.
*/
json_value_t assoc_in(const json_value_t& parent, const std::vector<json_value_t>& path, const json_value_t& new_element);

/*
	Erase member
*/
json_value_t dissoc(const json_value_t& value, const json_value_t& key);



#endif /* json_support_hpp */
