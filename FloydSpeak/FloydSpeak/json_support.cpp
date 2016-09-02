//
//  json.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_support.h"

#include "utils.h"

#include <map>
#include <cmath>

using std::string;
using std::vector;


////////////////////////////////////////		json_value_t




bool json_value_t::check_invariant() const {
	if(_type == k_object){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
		QUARK_ASSERT(_array.empty());
	}
	else if(_type == k_array){
		QUARK_ASSERT(_object.empty());
//		QUARK_ASSERT(_array.empty());
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
	}
	else if(_type == k_string){
		QUARK_ASSERT(_object.empty());
		QUARK_ASSERT(_array.empty());
//		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
	}
	else if(_type == k_number){
		QUARK_ASSERT(_object.empty());
		QUARK_ASSERT(_array.empty());
		QUARK_ASSERT(_string.empty());
//		QUARK_ASSERT(_number == 0.0);
	}
	else if(_type == k_true){
		QUARK_ASSERT(_object.empty());
		QUARK_ASSERT(_array.empty());
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
	}
	else if(_type == k_false){
		QUARK_ASSERT(_object.empty());
		QUARK_ASSERT(_array.empty());
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
	}
	else if(_type == k_null){
		QUARK_ASSERT(_object.empty());
		QUARK_ASSERT(_array.empty());
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_number == 0.0);
	}
	else{
		QUARK_ASSERT(false);
	}
	return true;
}

json_value_t::json_value_t(const json_value_t& other) :
	__debug(other.__debug),
	_type(other._type),
	_object(other._object),
	_array(other._array),
	_string(other._string),
	_number(other._number)
{
	QUARK_ASSERT(other.check_invariant());

	QUARK_ASSERT(check_invariant());
}

json_value_t& json_value_t::operator=(const json_value_t& other){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	auto temp(other);
	temp.swap(*this);

	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());
	return *this;
}

void json_value_t::swap(json_value_t& other){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	std::swap(__debug, other.__debug);
	std::swap(_type, other._type);
	_object.swap(other._object);
	_array.swap(other._array);
	_string.swap(other._string);
	std::swap(_number, other._number);

	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());
}

bool json_value_t::operator==(const json_value_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _type == other._type && _object == other._object && _array == other._array && _string == other._string && _number == other._number;
}



QUARK_UNIT_TESTQ("json_value_t()", ""){
	std::map<string, json_value_t> value {
		{ "one", json_value_t(1.0) },
		{ "two", json_value_t("zwei") }
	};
	const auto a = json_value_t(value);
	QUARK_UT_VERIFY(a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_object() == value);
	QUARK_UT_VERIFY((a.get_object() != std::map<string, json_value_t>()));
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	std::map<string, json_value_t> value {
		{ "one", json_value_t(1.0) },
		{ "two", json_value_t("zwei") },
		{
			"three",
			json_value_t(
				std::map<string, json_value_t>{
					{ "A", json_value_t("aaa") },
					{ "B", json_value_t("bbb") },
					{ "C", json_value_t("ccc") }
				}
			)
		}
	};
	const auto a = json_value_t(value);
	QUARK_UT_VERIFY(a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_object() == value);
	QUARK_UT_VERIFY(a.get_object_element("one") == json_value_t(1.0));
	QUARK_UT_VERIFY(a.get_object_element("three").get_object_element("C") == json_value_t("ccc"));
}


QUARK_UNIT_TESTQ("json_value_t()", ""){
	std::vector<json_value_t> value { json_value_t("A"), json_value_t("B") };
	const auto a = json_value_t(value);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_array() == value);
//	QUARK_UT_VERIFY((a.get_object() != std::map<string, json_value_t>()));
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	const auto a = json_value_t("hello");
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_string() == "hello");
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	const auto a = json_value_t(123.0);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_number() == 123.0);
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	const auto a = json_value_t(true);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	const auto a = json_value_t(false);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
}

QUARK_UNIT_TESTQ("json_value_t()", ""){
	const auto a = json_value_t();
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(a.is_null());
}



////////////////////////////////////////		HELPERS


long long double_to_int(const double value){
	return std::llround(value);
}

QUARK_UNIT_TESTQ("make_vec()", ""){
	const auto a = make_vec({ "one", "two" });
	QUARK_UT_VERIFY(a.size() == 2);
}



json_value_t make_object(const std::vector<std::pair<std::string, json_value_t>>& entries){
	std::map<string, json_value_t> result;

	for(const auto i: entries){
		if(i.second.is_null()){
		}
		else{
			result.insert(i);
		}
	}
	return json_value_t(result);
}


json_value_t store_object_member(const json_value_t& obj, const std::string& key, const json_value_t& value){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(key.size() > 0);
	QUARK_ASSERT(value.check_invariant());

	auto m = obj.get_object();
	m[key] = value;
	return json_value_t::make_object(m);
}


json_value_t push_back(const json_value_t& obj, const json_value_t& element){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj.is_array());
	QUARK_ASSERT(element.check_invariant());

	auto a = obj.get_array();
	a.push_back(element);
	return json_value_t(a);
}




json_value_t make_test_tree(){
	const auto obj1 = json_value_t::make_object({
		{ "name", json_value_t("James Bond") },
		{ "height", json_value_t(178.0) }
	});
	const auto obj2 = json_value_t::make_object({
		{ "name", json_value_t("Ernst Stavro Blofeld") },
		{ "height", json_value_t(160.0) }
	});
	const auto obj3 = json_value_t::make_object({
		{ "hero", obj1 },
		{ "villain", obj2 }
	});
	return obj3;
}

/*
	[
		{
			"height": 178,
			"movies": [
				"No one lives forever",
				"Moonraker",
				"Live and let die"
			],
			"name": "James Bond"
		},
		{
			"height": 160,
			"name": "Ernst Stavro Blofeld"
		}
	]
*/

json_value_t make_mixed_test_tree(){
	const auto obj1 = json_value_t::make_object({
		{ "name", json_value_t("James Bond") },
		{ "height", json_value_t(178.0) },
		{ "movies", json_value_t::make_array2({ "No one lives forever", "Moonraker", "Live and let die" }) }
	});
	const auto obj2 = json_value_t::make_object({
		{ "name", json_value_t("Ernst Stavro Blofeld") },
		{ "height", json_value_t(160.0) }
	});
	const auto obj3 = json_value_t::make_array2({ obj1, obj2 });
	return obj3;
}

bool exists_in(const json_value_t& parent, const std::vector<json_value_t>& path){
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(!path.empty());
	for(const auto n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

	if(parent.is_object()){
		QUARK_ASSERT(path[0].is_string());

		const auto member_name = path[0].get_string();
		if(path.size() == 1){
			return parent.does_object_element_exist(member_name);
		}
		else if(path.size() > 1){
			return exists_in(
				parent.get_object_element(member_name),
				std::vector<json_value_t>(path.begin() + 1, path.end())
			);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(parent.is_array()){
		QUARK_ASSERT(path[0].is_number());

		const size_t index = double_to_int(path[0].get_number());
		if(path.size() == 1){
			return index < parent.get_array_size();
		}
		else if(path.size() > 1){
			return exists_in(
				parent.get_array_element(index),
				std::vector<json_value_t>(path.begin() + 1, path.end())
			);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
		throw std::runtime_error("");
	}
}

QUARK_UNIT_TESTQ("exists_in()", "object level 0 - found"){
	const auto obj1 = make_test_tree();
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"yeti"})) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"hero", "name"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"hero", "height"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"villain", "name"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"villain", "height"})) == true);
}

QUARK_UNIT_TESTQ("exists_in()", "object level 1 - found"){
	const auto obj1 = make_test_tree();
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"hero", "name"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"hero", "height"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"villain", "name"})) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"villain", "height"})) == true);
}

QUARK_UNIT_TESTQ("exists_in()", "object missing"){
	const auto obj1 = make_test_tree();
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"hero", "job"})) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({"yeti"})) == false);
}

QUARK_UNIT_TESTQ("exists_in()", "mixed arrays and trees - lost & found"){
	const auto obj1 = make_mixed_test_tree();
	QUARK_TRACE(json_to_compact_string(obj1));
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(0.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(1.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(2.0) })) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(0.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(2.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(3.0) })) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(0.0), "name" })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_value_t(1.0), "name" })) == true);
}



json_value_t get_in(const json_value_t& parent, const std::vector<json_value_t>& path){
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(!path.empty());
	for(const auto n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

	if(parent.is_object()){
		const auto member_name = path[0].get_string();
		if(path.size() == 1){
			return parent.get_object_element(member_name);
		}
		else if(path.size() > 1){
			return get_in(
				parent.get_object_element(member_name),
				std::vector<json_value_t>(path.begin() + 1, path.end())
			);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(parent.is_array()){
		QUARK_ASSERT(path[0].is_number());

		const size_t index = double_to_int(path[0].get_number());
		if(path.size() == 1){
			return parent.get_array_element(index);
		}
		else if(path.size() > 1){
			return get_in(
				parent.get_array_element(index),
				std::vector<json_value_t>(path.begin() + 1, path.end())
			);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
		throw std::runtime_error("");
	}
}

//?? Also test getting an object itself.
QUARK_UNIT_TESTQ("get_in()", "one level get"){
	const auto obj1 = json_value_t::make_object({
		{ "name", "James Bond" },
		{ "height", json_value_t(178.0) }
	});
	QUARK_UT_VERIFY(get_in(obj1, { "name" }) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, { "height" }) == json_value_t(178.0));
}

QUARK_UNIT_TESTQ("get_in()", "two-level get"){
	const auto obj1 = make_test_tree();
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"hero", "name"})) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"hero", "height"})) == json_value_t(178.0));
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"villain", "height"})) == json_value_t(160.0));
}

QUARK_UNIT_TESTQ("get_in()", "mixed arrays and trees"){
	const auto obj1 = make_mixed_test_tree();
	QUARK_TRACE(json_to_compact_string(obj1));
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(0.0), "name" })) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(0.0) })) == "No one lives forever");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(1.0) })) == "Moonraker");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(2.0) })) == "Live and let die");

	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(1.0) })) != json_value_t());
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_value_t(1.0), "name" })) == "Ernst Stavro Blofeld");
}



json_value_t assoc(const json_value_t& obj, const json_value_t& key, const json_value_t& new_element){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj.is_null() || obj.is_object() || obj.is_array());
	QUARK_ASSERT(new_element.check_invariant());
	QUARK_ASSERT(key.check_invariant());
	QUARK_ASSERT(key.is_string() || key.is_number());;

	if(obj.is_null()){
		QUARK_ASSERT(key.is_string());
		return json_value_t::make_object({
			{ key.get_string(), new_element }
		});
	}
	else if(obj.is_object()){
		QUARK_ASSERT(key.is_string());
		return store_object_member(obj, key.get_string(), new_element);
	}
	else if(obj.is_array()){
		QUARK_ASSERT(key.is_number());
		const auto index = double_to_int(key.get_number());
		QUARK_ASSERT(index >= 0);

		auto array = obj.get_array();
		if(index > array.size()){
			throw std::out_of_range("");
		}
		else if(index == array.size()){
			array.push_back(new_element);
		}
		else{
			array[index] = new_element;
		}
		return json_value_t(array);
	}
	else{
		throw std::runtime_error("");
	}
}

QUARK_UNIT_TESTQ("assoc()", "replace obj member value"){
	const auto obj1 = json_value_t::make_object({
		{ "name", json_value_t("James Bond") },
		{ "height", json_value_t(178.0) }
	});
	QUARK_UT_VERIFY(obj1.get_object_element("name") == json_value_t("James Bond"));
	QUARK_UT_VERIFY(obj1.get_object_element("height") == json_value_t(178.0));
	const auto obj2 = assoc(obj1, json_value_t("name"), json_value_t("Stewie"));
	QUARK_UT_VERIFY(obj2.get_object_element("name") == json_value_t("Stewie"));
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_value_t(178.0));
}

QUARK_UNIT_TESTQ("assoc()", "add obj member value"){
	const auto obj1 = json_value_t::make_object({
		{ "name", json_value_t("James Bond") },
		{ "height", json_value_t(178.0) }
	});
	QUARK_UT_VERIFY(obj1.get_object_element("name") == json_value_t("James Bond"));
	QUARK_UT_VERIFY(obj1.get_object_element("height") == json_value_t(178.0));
	const auto obj2 = assoc(obj1, json_value_t("score"), json_value_t("*****"));
	QUARK_UT_VERIFY(obj2.get_object_element("name") == json_value_t("James Bond"));
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_value_t(178.0));
	QUARK_UT_VERIFY(obj2.get_object_element("score") == json_value_t("*****"));
}

QUARK_UNIT_TESTQ("assoc()", "add obj member value -- no object causes new to be made"){
	const auto obj2 = assoc(json_value_t(), json_value_t("hello"), json_value_t("world!"));
	QUARK_UT_VERIFY(obj2.get_object_element("hello") == json_value_t("world!"));
}

QUARK_UNIT_TESTQ("assoc()", "replace array element"){
	const auto obj1 = json_value_t::make_array2({ json_value_t("zero"), json_value_t("one"), json_value_t("two") });
	QUARK_UT_VERIFY((obj1.get_array() == vector<json_value_t>{ json_value_t("zero"), json_value_t("one"), json_value_t("two") }));
	const auto obj2 = assoc(obj1, json_value_t(1.0), json_value_t("UNO"));
	QUARK_UT_VERIFY((obj2.get_array() == vector<json_value_t>{ json_value_t("zero"), json_value_t("UNO"), json_value_t("two") }));
}





/*
	Entries in path are strings for specifying object-members or integer number for array indexes.
	If node is missing, an object is created for it.
*/
json_value_t assoc_in(const json_value_t& parent, const std::vector<json_value_t>& path, const json_value_t& new_element){
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(new_element.check_invariant());
	QUARK_ASSERT(!path.empty());
	for(const auto n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

	if(parent.is_null()){
		const auto empty = json_value_t::make_object({});
		return assoc_in(empty, path, new_element);
	}
	if(parent.is_object()){
		if(path.size() == 1){
			return assoc(parent, path[0], new_element);
		}
		else if(path.size() > 1){
			const auto member_name = path[0].get_string();
			const auto member_value = parent.get_optional_object_element(member_name);
			const auto member_value2 = assoc_in(
				member_value,
				std::vector<json_value_t>(path.begin() + 1, path.end()),
				new_element
			);
			return store_object_member(parent, member_name, member_value2);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(parent.is_array()){
		QUARK_ASSERT(path[0].is_number());

		if(path.size() == 1){
			return assoc(parent, path[0], new_element);
		}
		else if(path.size() > 1){
			const size_t member_index = double_to_int(path[0].get_number());
			QUARK_ASSERT(member_index >= 0);
			QUARK_ASSERT(member_index <= parent.get_array_size());

			const auto member_value = parent.get_array_element(member_index);
			const auto member_value2 = assoc_in(
				member_value,
				std::vector<json_value_t>(path.begin() + 1, path.end()),
				new_element
			);
			auto array2 = parent.get_array();
			array2[member_index] = member_value2;
			return json_value_t(array2);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}




QUARK_UNIT_TESTQ("assoc_in()", "replace obj member value"){
	const auto obj1 = json_value_t::make_object({
		{ "name", "James Bond" },
		{ "height", json_value_t(178.0) }
	});
	const auto obj2 = assoc_in(obj1, make_vec({ "name" }), "Stewie");
	QUARK_UT_VERIFY(obj2.get_object_element("name") == "Stewie");
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_value_t(178.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "replace obj member value - level0"){
	const auto obj1 = make_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ "hero" }), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({ "hero", })) == "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "height"})) == json_value_t(160.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "replace obj member value - level1"){
	const auto obj1 = make_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ "hero", "name" }), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"hero", "name"})) == "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"hero", "height"})) == json_value_t(178.0));
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "height"})) == json_value_t(160.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "force create deep tree"){
	const auto obj2 = assoc_in(json_value_t(), make_vec({"Action Movies", "James Bond Series", "hero", "name"}), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"Action Movies", "James Bond Series", "hero", "name"})) == "Stewie");
}

QUARK_UNIT_TESTQ("assoc_in()", "array"){
	const auto obj1 = json_value_t::make_array2({ "one", "two", "three" });
	const auto obj2 = assoc_in(obj1, make_vec({ json_value_t(0.0) }), "ONE");
	QUARK_ASSERT(obj2 == json_value_t::make_array2({ "ONE", "two", "three" }));
}
QUARK_UNIT_TESTQ("assoc_in()", "array"){
	const auto obj1 = json_value_t::make_array2({ "one", "two", "three" });
	const auto obj2 = assoc_in(obj1, make_vec({ json_value_t(3.0) }), "four");
	QUARK_ASSERT(obj2 == json_value_t::make_array2({ "one", "two", "three","four" }));
}

QUARK_UNIT_TESTQ("assoc_in()", "mixed arrays and trees"){
	const auto obj1 = make_mixed_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ json_value_t(0.0), "movies", json_value_t(1.0) }), "Moonraker SUX");


	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_value_t(0.0), "movies", json_value_t(1.0) })) == "Moonraker SUX");

	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_value_t(1.0) })) != json_value_t());
	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_value_t(1.0), "name" })) == "Ernst Stavro Blofeld");
}





