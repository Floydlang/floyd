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


////////////////////////////////////////		json_t




bool json_t::check_invariant() const {
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

json_t::json_t(const json_t& other) :
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

json_t& json_t::operator=(const json_t& other){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	auto temp(other);
	temp.swap(*this);

	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());
	return *this;
}

void json_t::swap(json_t& other){
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

bool json_t::operator==(const json_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _type == other._type && _object == other._object && _array == other._array && _string == other._string && _number == other._number;
}



QUARK_UNIT_TESTQ("json_t()", ""){
	std::map<string, json_t> value {
		{ "one", json_t(1.0) },
		{ "two", json_t("zwei") }
	};
	const auto a = json_t(value);
	QUARK_UT_VERIFY(a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_object() == value);
	QUARK_UT_VERIFY((a.get_object() != std::map<string, json_t>()));
}

QUARK_UNIT_TESTQ("json_t()", ""){
	std::map<string, json_t> value {
		{ "one", json_t(1.0) },
		{ "two", json_t("zwei") },
		{
			"three",
			json_t(
				std::map<string, json_t>{
					{ "A", json_t("aaa") },
					{ "B", json_t("bbb") },
					{ "C", json_t("ccc") }
				}
			)
		}
	};
	const auto a = json_t(value);
	QUARK_UT_VERIFY(a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_object() == value);
	QUARK_UT_VERIFY(a.get_object_element("one") == json_t(1.0));
	QUARK_UT_VERIFY(a.get_object_element("three").get_object_element("C") == json_t("ccc"));
}


QUARK_UNIT_TESTQ("json_t()", ""){
	std::vector<json_t> value { json_t("A"), json_t("B") };
	const auto a = json_t(value);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_array() == value);
//	QUARK_UT_VERIFY((a.get_object() != std::map<string, json_t>()));
}

QUARK_UNIT_TESTQ("json_t()", ""){
	const auto a = json_t("hello");
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_string() == "hello");
}

QUARK_UNIT_TESTQ("json_t()", ""){
	const auto a = json_t(123.0);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
	QUARK_UT_VERIFY(a.get_number() == 123.0);
}

QUARK_UNIT_TESTQ("json_t()", ""){
	const auto a = json_t(true);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
}

QUARK_UNIT_TESTQ("json_t()", ""){
	const auto a = json_t(false);
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(a.is_false());
	QUARK_UT_VERIFY(!a.is_null());
}

QUARK_UNIT_TESTQ("json_t()", ""){
	const auto a = json_t();
	QUARK_UT_VERIFY(!a.is_object());
	QUARK_UT_VERIFY(!a.is_array());
	QUARK_UT_VERIFY(!a.is_string());
	QUARK_UT_VERIFY(!a.is_number());
	QUARK_UT_VERIFY(!a.is_true());
	QUARK_UT_VERIFY(!a.is_false());
	QUARK_UT_VERIFY(a.is_null());
}



////////////////////////////////////////		HELPERS



void ut_compare_jsons(const json_t& result, const json_t& expected){
	const auto result2 = json_to_pretty_string(result);
	const auto expected2 = json_to_pretty_string(expected);

	if(result2 != expected2){
		{
			QUARK_SCOPED_TRACE("RESULT");
			QUARK_TRACE(result2);
		}
		{
			QUARK_SCOPED_TRACE("EXPECTED");
			QUARK_TRACE(expected2);
		}
		QUARK_TEST_VERIFY(false)
	}
}



long long double_to_int(const double value){
	return std::llround(value);
}

QUARK_UNIT_TESTQ("make_vec()", ""){
	const auto a = make_vec({ "one", "two" });
	QUARK_UT_VERIFY(a.size() == 2);
}



json_t make_object(const std::vector<std::pair<std::string, json_t>>& entries){
	std::map<string, json_t> result;

	for(const auto i: entries){
		if(i.second.is_null()){
		}
		else{
			result.insert(i);
		}
	}
	return json_t(result);
}


json_t store_object_member(const json_t& obj, const std::string& key, const json_t& value){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(key.size() > 0);
	QUARK_ASSERT(value.check_invariant());

	auto m = obj.get_object();
	m[key] = value;
	return json_t::make_object(m);
}


json_t push_back(const json_t& obj, const json_t& element){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj.is_array());
	QUARK_ASSERT(element.check_invariant());

	auto a = obj.get_array();
	a.push_back(element);
	return json_t(a);
}


json_t make_array_skip_nulls(const std::vector<json_t>& elements){
	for(const auto& i: elements){ QUARK_ASSERT(i.check_invariant()); }

	std::vector<json_t> elements2;
	std::copy_if(elements.begin(), elements.end(), std::back_inserter(elements2), [&] (const json_t& v) { return !v.is_null(); });
	return json_t(elements2);
}


json_t make_test_tree(){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) }
	});
	const auto obj2 = json_t::make_object({
		{ "name", json_t("Ernst Stavro Blofeld") },
		{ "height", json_t(160.0) }
	});
	const auto obj3 = json_t::make_object({
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

json_t make_mixed_test_tree(){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) },
		{ "movies", json_t::make_array({ "No one lives forever", "Moonraker", "Live and let die" }) }
	});
	const auto obj2 = json_t::make_object({
		{ "name", json_t("Ernst Stavro Blofeld") },
		{ "height", json_t(160.0) }
	});
	const auto obj3 = json_t::make_array({ obj1, obj2 });
	return obj3;
}

bool exists_in(const json_t& parent, const std::vector<json_t>& path){
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
				std::vector<json_t>(path.begin() + 1, path.end())
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
				parent.get_array_n(index),
				std::vector<json_t>(path.begin() + 1, path.end())
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
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(0.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(1.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(2.0) })) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(0.0), "movies", json_t(0.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(0.0), "movies", json_t(2.0) })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(0.0), "movies", json_t(3.0) })) == false);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(0.0), "name" })) == true);
	QUARK_UT_VERIFY(exists_in(obj1, make_vec({ json_t(1.0), "name" })) == true);
}



json_t get_in(const json_t& parent, const std::vector<json_t>& path){
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
				std::vector<json_t>(path.begin() + 1, path.end())
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
			return parent.get_array_n(index);
		}
		else if(path.size() > 1){
			return get_in(
				parent.get_array_n(index),
				std::vector<json_t>(path.begin() + 1, path.end())
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
	const auto obj1 = json_t::make_object({
		{ "name", "James Bond" },
		{ "height", json_t(178.0) }
	});
	QUARK_UT_VERIFY(get_in(obj1, { "name" }) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, { "height" }) == json_t(178.0));
}

QUARK_UNIT_TESTQ("get_in()", "two-level get"){
	const auto obj1 = make_test_tree();
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"hero", "name"})) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"hero", "height"})) == json_t(178.0));
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({"villain", "height"})) == json_t(160.0));
}

QUARK_UNIT_TESTQ("get_in()", "mixed arrays and trees"){
	const auto obj1 = make_mixed_test_tree();
	QUARK_TRACE(json_to_compact_string(obj1));
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(0.0), "name" })) == "James Bond");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(0.0), "movies", json_t(0.0) })) == "No one lives forever");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(0.0), "movies", json_t(1.0) })) == "Moonraker");
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(0.0), "movies", json_t(2.0) })) == "Live and let die");

	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(1.0) })) != json_t());
	QUARK_UT_VERIFY(get_in(obj1, make_vec({ json_t(1.0), "name" })) == "Ernst Stavro Blofeld");
}



json_t assoc(const json_t& obj, const json_t& key, const json_t& new_element){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj.is_null() || obj.is_object() || obj.is_array());
	QUARK_ASSERT(new_element.check_invariant());
	QUARK_ASSERT(key.check_invariant());
	QUARK_ASSERT(key.is_string() || key.is_number());;

	if(obj.is_null()){
		QUARK_ASSERT(key.is_string());
		return json_t::make_object({
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
		return json_t(array);
	}
	else{
		throw std::runtime_error("");
	}
}

QUARK_UNIT_TESTQ("assoc()", "replace obj member value"){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) }
	});
	QUARK_UT_VERIFY(obj1.get_object_element("name") == json_t("James Bond"));
	QUARK_UT_VERIFY(obj1.get_object_element("height") == json_t(178.0));
	const auto obj2 = assoc(obj1, json_t("name"), json_t("Stewie"));
	QUARK_UT_VERIFY(obj2.get_object_element("name") == json_t("Stewie"));
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_t(178.0));
}

QUARK_UNIT_TESTQ("assoc()", "add obj member value"){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) }
	});
	QUARK_UT_VERIFY(obj1.get_object_element("name") == json_t("James Bond"));
	QUARK_UT_VERIFY(obj1.get_object_element("height") == json_t(178.0));
	const auto obj2 = assoc(obj1, json_t("score"), json_t("*****"));
	QUARK_UT_VERIFY(obj2.get_object_element("name") == json_t("James Bond"));
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_t(178.0));
	QUARK_UT_VERIFY(obj2.get_object_element("score") == json_t("*****"));
}

QUARK_UNIT_TESTQ("assoc()", "add obj member value -- no object causes new to be made"){
	const auto obj2 = assoc(json_t(), json_t("hello"), json_t("world!"));
	QUARK_UT_VERIFY(obj2.get_object_element("hello") == json_t("world!"));
}

QUARK_UNIT_TESTQ("assoc()", "replace array element"){
	const auto obj1 = json_t::make_array({ json_t("zero"), json_t("one"), json_t("two") });
	QUARK_UT_VERIFY((obj1.get_array() == vector<json_t>{ json_t("zero"), json_t("one"), json_t("two") }));
	const auto obj2 = assoc(obj1, json_t(1.0), json_t("UNO"));
	QUARK_UT_VERIFY((obj2.get_array() == vector<json_t>{ json_t("zero"), json_t("UNO"), json_t("two") }));
}





/*
	key must exist.
	Can remove entire subtree.
	Works on arrays and objects.
*/
json_t dissoc(const json_t& obj, const json_t& key){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj.is_null() || obj.is_object() || obj.is_array());
	QUARK_ASSERT(key.check_invariant());
	QUARK_ASSERT(key.is_string() || key.is_number());;

	if(obj.is_null()){
		throw std::out_of_range("");
	}
	else if(obj.is_object()){
		QUARK_ASSERT(key.is_string());
		auto temp = obj.get_object();
		temp.erase(key.get_string());
		return json_t(temp);
	}
	else if(obj.is_array()){
		QUARK_ASSERT(key.is_number());
		const auto index = double_to_int(key.get_number());
		QUARK_ASSERT(index >= 0);

		auto array = obj.get_array();
		if(index >= array.size()){
			throw std::out_of_range("");
		}
		array.erase(array.begin() + index);
		return json_t(array);
	}
	else{
		throw std::runtime_error("");
	}
}

QUARK_UNIT_TESTQ("dissoc()", "erase obj member value"){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) }
	});
	const auto obj2 = dissoc(obj1, json_t("name"));
	QUARK_UT_VERIFY(!exists_in(obj2, {"name"}));
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_t(178.0));
}

QUARK_UNIT_TESTQ("dissoc()", "erase non-existing object member"){
	const auto obj1 = json_t::make_object({
		{ "name", json_t("James Bond") },
		{ "height", json_t(178.0) }
	});
	const auto obj2 = dissoc(obj1, json_t("XYZ"));
}

QUARK_UNIT_TESTQ("dissoc()", "erase array entry"){
	const auto obj1 = json_t::make_array({ "one", "two", "three" });
	const auto obj2 = dissoc(obj1, json_t(1.0));
	QUARK_UT_VERIFY(obj2 == json_t::make_array({ "one", "three" }));
}





json_t assoc_in(const json_t& parent, const std::vector<json_t>& path, const json_t& new_element){
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(new_element.check_invariant());
	QUARK_ASSERT(!path.empty());
	for(const auto n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

	if(parent.is_null()){
		const auto empty = json_t::make_object();
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
				std::vector<json_t>(path.begin() + 1, path.end()),
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

			const auto member_value = parent.get_array_n(member_index);
			const auto member_value2 = assoc_in(
				member_value,
				std::vector<json_t>(path.begin() + 1, path.end()),
				new_element
			);
			auto array2 = parent.get_array();
			array2[member_index] = member_value2;
			return json_t(array2);
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
	const auto obj1 = json_t::make_object({
		{ "name", "James Bond" },
		{ "height", json_t(178.0) }
	});
	const auto obj2 = assoc_in(obj1, make_vec({ "name" }), "Stewie");
	QUARK_UT_VERIFY(obj2.get_object_element("name") == "Stewie");
	QUARK_UT_VERIFY(obj2.get_object_element("height") == json_t(178.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "replace obj member value - level0"){
	const auto obj1 = make_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ "hero" }), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({ "hero", })) == "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "height"})) == json_t(160.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "replace obj member value - level1"){
	const auto obj1 = make_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ "hero", "name" }), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"hero", "name"})) == "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"hero", "height"})) == json_t(178.0));
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "name"})) == "Ernst Stavro Blofeld");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"villain", "height"})) == json_t(160.0));
}

QUARK_UNIT_TESTQ("assoc_in()", "force create deep tree"){
	const auto obj2 = assoc_in(json_t(), make_vec({"Action Movies", "James Bond Series", "hero", "name"}), "Stewie");
	QUARK_UT_VERIFY(get_in(obj2, make_vec({"Action Movies", "James Bond Series", "hero", "name"})) == "Stewie");
}

QUARK_UNIT_TESTQ("assoc_in()", "array"){
	const auto obj1 = json_t::make_array({ "one", "two", "three" });
	const auto obj2 = assoc_in(obj1, make_vec({ json_t(0.0) }), "ONE");
	QUARK_ASSERT(obj2 == json_t::make_array({ "ONE", "two", "three" }));
}
QUARK_UNIT_TESTQ("assoc_in()", "array"){
	const auto obj1 = json_t::make_array({ "one", "two", "three" });
	const auto obj2 = assoc_in(obj1, make_vec({ json_t(3.0) }), "four");
	QUARK_ASSERT(obj2 == json_t::make_array({ "one", "two", "three","four" }));
}

QUARK_UNIT_TESTQ("assoc_in()", "mixed arrays and trees"){
	const auto obj1 = make_mixed_test_tree();
	const auto obj2 = assoc_in(obj1, make_vec({ json_t(0.0), "movies", json_t(1.0) }), "Moonraker SUX");


	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_t(0.0), "movies", json_t(1.0) })) == "Moonraker SUX");

	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_t(1.0) })) != json_t());
	QUARK_UT_VERIFY(get_in(obj2, make_vec({ json_t(1.0), "name" })) == "Ernst Stavro Blofeld");
}




std::vector<std::string> to_string_vec(const json_t& json){
	QUARK_ASSERT(json.is_array());

	const auto size = json.get_array_size();
	vector<string> result;
	for(int i = 0 ; i < size ; i++){
		const auto e = json.get_array_n(i);
		result.push_back(e.get_string());
	}
	return result;
}

json_t from_string_vec(const std::vector<std::string>& vec){
	vector<json_t> str;
	for(const auto e: vec){
		str.push_back(e);
	}
	return json_t::make_array(str);
}


