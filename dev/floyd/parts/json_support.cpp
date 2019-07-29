//
//  json.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_support.h"


#include <map>
#include <cmath>
#include <algorithm>

#include "utils.h"
#include "text_parser.h"

using std::string;
using std::vector;





void ut_verify(const quark::call_context_t& context, const json_t& result, const json_t& expected){
	if(result == expected){
	}
	else{
		const auto result2 = json_to_pretty_string(result);
		const auto expected2 = json_to_pretty_string(expected);
		quark::ut_verify(context, result2, expected2);
	}
}

void ut_verify(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result, const std::pair<json_t, seq_t>& expected){
	if(result == expected){
	}
	else{
		if(result.first != expected.first){
			const auto result2 = json_to_pretty_string(result.first);
			const auto expected2 = json_to_pretty_string(expected.first);
			quark::ut_verify(context, result2, expected2);
		}
		else {
			quark::ut_verify(context, result.second.str(), expected.second.str());
		}
	}
}


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

/*
QUARK_UNIT_TEST_VIP("json_t", "json_t()", "", ""){
	const uint64_t k = 0b1000000000000000000000000000000000000000000000000000000000000001;
	const auto a = json_t((int64_t)k);
	const auto d = a.get_number();
	const uint64_t i = d;
	QUARK_ASSERT(i == k);
}
*/



////////////////////////////////////////		HELPERS




long long double_to_int(const double value){
	return std::llround(value);
}

QUARK_UNIT_TESTQ("make_vec()", ""){
	const auto a = make_vec({ "one", "two" });
	QUARK_UT_VERIFY(a.size() == 2);
}


json_t make_object(const std::vector<std::pair<std::string, json_t>>& entries){
	std::map<string, json_t> result;

	for(const auto& i: entries){
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
	for(const auto& n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

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
			quark::throw_exception();
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
			quark::throw_exception();
		}
	}
	else{
		quark::throw_runtime_error("");
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
	for(const auto& n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

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
			quark::throw_exception();
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
			quark::throw_exception();
		}
	}
	else{
		quark::throw_runtime_error("");
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
		quark::throw_runtime_error("");
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
		quark::throw_runtime_error("");
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
	for(const auto& n: path){ QUARK_ASSERT(n.check_invariant()); QUARK_ASSERT(n.is_string() || n.is_number()); }

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
			quark::throw_exception();
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
			quark::throw_exception();
		}
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
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
	for(const auto& e: vec){
		str.push_back(e);
	}
	return json_t::make_array(str);
}













using std::string;
using std::vector;
using std::pair;

static const std::string k_json_whitespace_chars = " \n\t";

seq_t skip_whitespace(const seq_t& s){
	return read_while(s, k_json_whitespace_chars).second;
}



std::pair<json_t, seq_t> parse_json(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto ch = a.first1();
	if(ch == "{"){
		std::map<string, json_t> obj;
		auto p2 = skip_whitespace(a.rest1());
		while(p2.first1() != "}"){

			//	"my_key": EXPRESSION,

			const auto key_p = parse_json(p2);
			if(!key_p.first.is_string()){
				quark::throw_runtime_error("Missing key in JSON object");
			}
			const string key = key_p.first.get_string();

			auto p3 = skip_whitespace(key_p.second);
			if(p3.first1() != ":"){
				quark::throw_runtime_error("Missing : betweeen key and value in JSON object");
			}

			p3 = skip_whitespace(p3.rest1());

			const auto expression_p = parse_json(p3);
			obj.insert(std::make_pair(key, expression_p.first));

			auto post_p = skip_whitespace(expression_p.second);
			if(post_p.first1() != "," && post_p.first1() != "}"){
				quark::throw_runtime_error("Expected either , or } after JSON object field");
			}

			if(post_p.first1() == ","){
				post_p = skip_whitespace(post_p.rest1());
			}
			p2 = post_p;
		}
		return { json_t::make_object(obj), p2.rest1() };
	}
	else if(ch == "["){
		vector<json_t> array;
		auto p2 = skip_whitespace(a.rest1());
		while(p2.first1() != "]"){
			const auto expression_p = parse_json(p2);
			array.push_back(expression_p.first);

			auto post_p = skip_whitespace(expression_p.second);
			if(post_p.first1() != "," && post_p.first1() != "]"){
				quark::throw_runtime_error("Expected , or ] after JSON array element");
			}

			if(post_p.first1() == ","){
				post_p = skip_whitespace(post_p.rest1());
			}
			p2 = post_p;
		}
		return { json_t::make_array(array), p2.rest1() };
	}
	else if(ch == "\""){
		const auto b = read_until(a.rest1(), "\"");
		return { json_t(b.first), b.second.rest1() };
	}
	else if(if_first(a, "true").first){
		return { json_t(true), if_first(a, "true").second };
	}
	else if(if_first(a, "false").first){
		return { json_t(false), if_first(a, "false").second };
	}
	else if(if_first(a, "null").first){
		return { json_t(), if_first(a, "null").second };
	}
	else{
		const auto number_pos = read_while(a, "-0123456789.+");
		if(number_pos.first.empty()){
			return { json_t(), a };
		}
		else{
			double number = parse_float(number_pos.first);
			return { json_t(number), number_pos.second };
		}
	}
}


QUARK_UNIT_TEST("", "parse_json()", "primitive", ""){
	ut_verify(QUARK_POS, parse_json(seq_t("\"xyz\"xxx")), { json_t("xyz"), seq_t("xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("\"\"xxx")), { json_t(""), seq_t("xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("13.0 xxx")), { json_t(13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("-13.0 xxx")), { json_t(-13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("4 xxx")), { json_t(4.0), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("true xxx")), { json_t(true), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("false xxx")), { json_t(false), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	ut_verify(QUARK_POS, parse_json(seq_t("null xxx")), { json_t(), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "array - empty"){
	ut_verify(QUARK_POS, parse_json(seq_t("[] xxx")), { json_t::make_array(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - two numbers"){
	ut_verify(QUARK_POS, parse_json(seq_t("[10, 11] xxx")), { json_t::make_array({json_t(10.0), json_t(11.0)}), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - nested"){
	ut_verify(QUARK_POS,
		parse_json(seq_t("[10, 11, [ 12, 13]] xxx")),
		{
			json_t::make_array({json_t(10.0), json_t(11.0), json_t::make_array({json_t(12.0), json_t(13.0)}) }),
			seq_t(" xxx")
		}
	);
}


QUARK_UNIT_TESTQ("parse_json()", "object - empty"){
	ut_verify(QUARK_POS, parse_json(seq_t("{} xxx")), { json_t::make_object(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "object - two entries"){
	const auto result = parse_json(seq_t("{\"one\": 1, \"two\": 2} xxx"));
	QUARK_TRACE(json_to_compact_string(result.first));

	ut_verify(QUARK_POS, result, { json_t::make_object({{"one", json_t(1.0)}, {"two", json_t(2.0)}}), seq_t(" xxx") });
}





















static const size_t k_default_pretty_columns = 160;


std::string json_to_compact_string2(const json_t& v, bool quote_fields);


////////////////////////////////////////////////		C++11 raw string literals tests


string erase_linefeed(const std::string& s){
/*
	string r = s;
//	str.erase(std::remove(str.begin(), str.end(), 'a'), str.end());
	r.erase(std::remove(r.begin(), r.end(), 'a'), r.end());
//	return s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
	return r;
*/
	string r;
	for(const auto c: s){
		if(c != '\r'){
			r.push_back(c);
		}
	}
	return r;
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){

const char* s1 = R"foo(
Hello
World
)foo";

	ut_verify(QUARK_POS, s1, "\nHello\nWorld\n");

}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	const auto test_cpp11_raw_string_literals = R"aaa({ "firstName": "John", "lastName": "Doe" })aaa";
	ut_verify(QUARK_POS, test_cpp11_raw_string_literals, "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	ut_verify(QUARK_POS, R"aaa(["version": 2])aaa", "[\"version\": 2]");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	ut_verify(QUARK_POS, R"___(["version": 2])___", "[\"version\": 2]");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	ut_verify(QUARK_POS, R"<>(["version": 2])<>", "[\"version\": 2]");
}


QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	const auto test_cpp11_raw_string_literals = R"aaa({ "firstName": "John", "lastName": "Doe" })aaa";
	ut_verify(QUARK_POS, test_cpp11_raw_string_literals, "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
}


//	Online escape tool: http://www.freeformatter.com/java-dotnet-escape.html#ad-output

const string compact_escaped = "{\"menu\": {\"id\": \"file\",\"popup\": {\"menuitem\": [{\"value\": \"New\",\"onclick\": \"CreateNewDoc()\"},{\"value\": \"Close\",\"onclick\": \"CloseDoc()\"}]}}}";
const string compact_raw_string = R"___({"menu": {"id": "file","popup": {"menuitem": [{"value": "New","onclick": "CreateNewDoc()"},{"value": "Close","onclick": "CloseDoc()"}]}}})___";

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	ut_verify(QUARK_POS, compact_escaped, compact_raw_string);
}


const string beautiful_escaped = "{\r\n  \"menu\": {\r\n    \"id\": \"file\",\r\n    \"popup\": {\r\n      \"menuitem\": [\r\n        {\r\n          \"value\": \"New\",\r\n          \"onclick\": \"CreateNewDoc()\"\r\n        },\r\n        {\r\n          \"value\": \"Close\",\r\n          \"onclick\": \"CloseDoc()\"\r\n        }\r\n      ]\r\n    }\r\n  }\r\n}";

const string beautiful_raw_string = R"___({
  "menu": {
    "id": "file",
    "popup": {
      "menuitem": [
        {
          "value": "New",
          "onclick": "CreateNewDoc()"
        },
        {
          "value": "Close",
          "onclick": "CloseDoc()"
        }
      ]
    }
  }
})___";


QUARK_UNIT_TESTQ("erase_linefeed()", ""){
	ut_verify(QUARK_POS, erase_linefeed("\raaa\rbbb\r"), "aaabbb");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	ut_verify(QUARK_POS, erase_linefeed(beautiful_escaped), beautiful_raw_string);
}


std::string object_to_compact_string(const std::map<std::string, json_t>& object, bool quote_fields){
	if(object.empty()){
		return "{}";
	}
	else{
		string members;
		for(const auto& m: object){
			const auto s = (quote_fields ? quote(m.first) : m.first) + ": " + json_to_compact_string2(m.second, quote_fields) + ", ";
			members = members + s;
		}

		//	Remove last ", ".
		members = members.substr(0, members.length() - 2);

		const auto result = std::string("{ ") + members + " }";
		return result;
	}
}

QUARK_UNIT_TESTQ("object_to_compact_string()", ""){
	ut_verify(QUARK_POS, object_to_compact_string(std::map<string, json_t>{}, true), "{}");
}

QUARK_UNIT_TESTQ("object_to_compact_string()", ""){
	ut_verify(QUARK_POS,
		object_to_compact_string(std::map<string, json_t>{
			{ "one", json_t("1") },
			{ "two", json_t("2") }
		}
		, true
		),
		"{ \"one\": \"1\", \"two\": \"2\" }"
	);
}


std::string array_to_compact_string(const std::vector<json_t>& array, bool quote_fields){
	if(array.empty()){
		return "[]";
	}
	else{
		string items;
		size_t count = array.size();
		size_t index = 0;
		while(count > 1){
			items = items + json_to_compact_string2(array[index], quote_fields) + ", ";
			count--;
			index++;
		}
		if(count > 0){
			items = items + json_to_compact_string2(array[index], quote_fields);
		}
		const auto result = std::string("[") + items + "]";
		return result;
	}
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	ut_verify(QUARK_POS, array_to_compact_string(std::vector<json_t>{}, true), "[]");
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	ut_verify(QUARK_POS, array_to_compact_string(std::vector<json_t>{ json_t(13.4) }, true), "[13.4]");
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	ut_verify(QUARK_POS,
		array_to_compact_string(vector<json_t>{
			json_t("a"),
			json_t("b")
		}, true),
		"[\"a\", \"b\"]"
	);
}


std::string json_to_compact_string2(const json_t& v, bool quote_fields){
	if(v.is_object()){
		return object_to_compact_string(v.get_object(), quote_fields);
	}
	else if(v.is_array()){
		return array_to_compact_string(v.get_array(), quote_fields);
	}
	else if(v.is_string()){
		return quote(v.get_string());
	}
	else if(v.is_number()){
		return double_to_string_simplify(v.get_number());
	}
	else if(v.is_true()){
		return "true";
	}
	else if(v.is_false()){
		return "false";
	}
	else if(v.is_null()){
		return "null";
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

std::string json_to_compact_string_minimal_quotes(const json_t& v){
	return json_to_compact_string2(v, false);
}

std::string json_to_compact_string(const json_t& v){
	return json_to_compact_string2(v, true);
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	const auto a = std::map<string, json_t>{
		{ "firstName", json_t("John") },
		{ "lastName", json_t("Doe") }
	};
	ut_verify(QUARK_POS, json_to_compact_string(json_t(a)), "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
	ut_verify(QUARK_POS, json_to_compact_string(json_t(a)), R"aaa({ "firstName": "John", "lastName": "Doe" })aaa");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	ut_verify(QUARK_POS,
		json_to_compact_string(json_t(
			vector<json_t>{
				json_t("a"),
				json_t("b")
			}
		)),
		"[\"a\", \"b\"]"
	);
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t("")) == "\"\"");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t("xyz")) == "\"xyz\"");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t(12.3)) == "12.3");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t(true)) == "true");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t(false)) == "false");
}

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_t()) == "null");
}


std::string json_to_pretty_string(const json_t& v){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	return json_to_pretty_string(v, 0, pretty);
}


bool is_collection(const json_t& type){
	return type.is_object() || type.is_array();
}
bool is_subtree(const json_t& type){
	return (type.is_object() && type.get_object_size() > 0) || (type.is_array() && type.get_array_size() > 0);
}

string make_key_str(const std::string& key){
	return key.empty() ? "" : (key + ": ");
}


//	Returns the width in character-columns of the widest line in s.
size_t count_char_positions(const std::string& s, size_t tab_chars){
	size_t widest_line_chars = 0;
	size_t count = 0;
	for(const auto ch: s){
		if(ch == '\n'){
			count = 0;
		}
		else if(ch == '\t'){
			count += tab_chars;
		}
		else{
			 count++;
		}
		widest_line_chars = std::max(widest_line_chars, count);
	}
	return widest_line_chars;
}

QUARK_UNIT_TESTQ("count_char_positions()", ""){
	QUARK_UT_VERIFY(count_char_positions("", 4) == 0);
}
QUARK_UNIT_TESTQ("count_char_positions()", ""){
	QUARK_UT_VERIFY(count_char_positions("a", 4) == 1);
}
QUARK_UNIT_TESTQ("count_char_positions()", ""){
	QUARK_UT_VERIFY(count_char_positions("aaaa\nbb", 4) == 4);
}
QUARK_UNIT_TESTQ("count_char_positions()", ""){
	QUARK_UT_VERIFY(count_char_positions("\t", 4) == 4);
}
QUARK_UNIT_TESTQ("count_char_positions()", ""){
	QUARK_UT_VERIFY(count_char_positions("a\nbb\nccc\n", 4) == 3);
}


//??? escape string. Also unescape them when parsing.
/*
	key == name of this value inside a parent object if any. If this is an entry in an array, key == "". Else key == "".
*/
std::string json_to_pretty_string_internal(const string& key, const json_t& value, int pos, const pretty_t& pretty){
	const auto key_str = make_key_str(key);
	if(value.is_object()){
		const auto& object = value.get_object();
		if(object.empty()){
			return string(pos, '\t') + key_str + "{}";
		}
		else{
			/*
				Attempt to put all object entires on one text line, recursively.
			*/
			string one_line = string(pos, '\t') + key_str + json_to_compact_string(value);
			size_t width = count_char_positions(one_line, pretty._tab_char_setting);
			if(width <= pretty._max_column_chars){
				return one_line;
			}

			else{
				string lines = string(pos, '\t') + key_str + "{\n";
				size_t index = 0;
				for(auto member: object){
					const auto last = index == object.size() - 1;
					const auto member_key = quote(member.first);
					const auto member_value = member.second;

					//	Can be multi-line if this member is a collection.
					const auto member_lines = json_to_pretty_string_internal(member_key, member_value, pos + 1, pretty);
					lines = lines + member_lines + (last ? "\n" : ",\n");
					index++;
				}
				lines = lines + string(pos, '\t') + "}";
				return lines;
			}
		}
	}
	else if(value.is_array()){
		const auto& array = value.get_array();
		if(array.empty()){
			return string(pos, '\t') + key_str + "[]";
		}
		else{
			/*
				Attempt to put all array entires on one text line, recursively.

				"array1": [ 100, 20, 30, 40 ]
				"array2": [ 100, 20, 30, [ "yes", "no" ], { "x": 10, "y": 100 }]
			*/
			string one_line = string(pos, '\t') + key_str + json_to_compact_string(value);
			size_t width = count_char_positions(one_line, pretty._tab_char_setting);
			if(width <= pretty._max_column_chars){
				return one_line;
			}

			/*
				Make one-member-per-line layout.
				"array1": [
					100,
					20,
					30, 40
				]
			*/
			else{
				string lines = string(pos, '\t') + key_str + "[\n";
				const size_t count = array.size();
				for(auto index = 0 ; index < count ; index++){
					const auto last = (index == count - 1);
					const auto member_value = array[index];
					const auto member_lines = json_to_pretty_string_internal("", member_value, pos + 1, pretty);
					lines = lines + member_lines + (last ? "\n" : ",\n");
				}
				lines = lines + string(pos, '\t') + "]";
				return lines;
			}
		}
	}
	else if(value.is_string()){
		return string(pos, '\t') + key_str + quote(value.get_string());
	}
	else if(value.is_number()){
		return string(pos, '\t') + key_str + double_to_string_simplify(value.get_number());
	}
	else if(value.is_true()){
		return string(pos, '\t') + key_str + "true";
	}
	else if(value.is_false()){
		return string(pos, '\t') + key_str + "false";
	}
	else if(value.is_null()){
		return string(pos, '\t') + key_str + "null";
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

std::string json_to_pretty_string(const json_t& value, int pos, const pretty_t& pretty){
	return json_to_pretty_string_internal("", value, pos, pretty);
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", ""){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
   	QUARK_TRACE_SS(
		json_to_pretty_string(json_t::make_object(), quark::get_log_indent(), pretty)
	);
}


QUARK_UNIT_TESTQ("cout()", ""){
#if 0
	std::cout <<("\t\t\t{\n\t\t\t\t\"a\": 100\n\t\t\t}\n");
	std::cout << R"(
			{
				"a": 100,
				"b": 234
			}
	)";
#endif

}

QUARK_UNIT_TESTQ("json_to_pretty_string()", "object"){
	const auto value = json_t::make_object({
		{ "one", json_t(111.0) },
		{ "two", json_t(222.0) }
	});

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	QUARK_TRACE_SS(result);
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", "nested object"){
	const auto italian = json_t::make_object({
		{ "uno", json_t(1.0) },
		{ "duo", json_t(2.0) },
		{ "tres", json_t(3.0) },
		{ "quattro", json_t(3.0) }
	});

	const auto value = json_t::make_object({
		{ "one", json_t(111.0) },
		{ "two", json_t(222.0) },
		{ "italian", italian }
	});

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	QUARK_TRACE_SS(result);
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", "array"){
	const auto value = json_t::make_array({ "one", "two", "three" });

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	QUARK_TRACE_SS(result);
}

QUARK_UNIT_TESTQ("json_to_pretty_string()", "nested arrays"){
	const auto italian = json_t::make_array({ "uno", "duo", "tres", "quattro" });
	const auto value = json_t::make_array({ "one", "two", italian, "three" });

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	QUARK_TRACE_SS(result);
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", ""){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const auto result =	json_to_pretty_string(
		parse_json(seq_t(beautiful_raw_string)).first,
		quark::get_log_indent(),
		pretty
	);
	QUARK_TRACE_SS(result);
}


string get_test2(){
return R"(
{
	"main": [
		{
			"base_type": "function",
			"scope_def": {
				"args": [
			 
				],
				"locals": [
			 
				],
				"members": [
			 
				],
				"name": "main",
				"return_type": "int",
				"statements": [
					[
						"return",
						[
							"k",
							"int",
							3
						]
					]
				],
				"type": "function",
				"types": [
					[
						"return",
						[
							"k",
							"int",
							3
						]
					]
				]
			}
		}
	]
}
)";
}

string get_test2_pretty(){
return R"(
{
	"main":
	[
		{
			"base_type": "function",
			"scope_def":
			{
				"args": [],
				"locals": [],
				"members": [],
				"name": "main",
				"return_type": "int",
				"statements": [ [ "return", [ "k", "int", 3 ] ] ],
				"type": "function",
				"types": [ [ "return", [ "k", "int", 3 ] ] ]
			}
		}
	]
}
)";
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", ""){
	const auto a = get_test2();

	const auto json = parse_json(seq_t(a)).first;
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const auto b = json_to_pretty_string(json, 0, pretty );

	QUARK_TRACE_SS(b);
}


