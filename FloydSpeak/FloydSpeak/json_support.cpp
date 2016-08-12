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

using std::string;
using std::vector;

std::string to_json2(const std::vector<std::string>& values){
	if(values.empty()){
		return "[]";
	}
	else{
		string r;
		for(auto i = 0 ; i < values.size() ; i++){
			r = r + values[i] + ", ";
		}

		//	Remove last ", ".
		if(values.size() > 0){
			r = r.substr(0, r.length() - 2);
		}

		const auto result = string("[ " + r + " ]");
		return result;
	}
}

QUARK_UNIT_TESTQ("to_json2()", ""){
	QUARK_UT_VERIFY(to_json2({}) == "[]");
}

QUARK_UNIT_TESTQ("to_json2()", ""){
	QUARK_UT_VERIFY(to_json2({ "\"a\"" }) == "[ \"a\" ]");
}

QUARK_UNIT_TESTQ("to_json2()", ""){
	QUARK_UT_VERIFY(to_json2({ "\"a\"", "\"b\"" }) == "[ \"a\", \"b\" ]");
}

QUARK_UNIT_TESTQ("to_json2()", ""){
	QUARK_UT_VERIFY(to_json2({ std::to_string(123), std::to_string(456.7f) }) == "[ 123, 456.700012 ]");
}



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
	QUARK_UT_VERIFY(a.get_object().at("one") == json_value_t(1.0));
	QUARK_UT_VERIFY(a.get_object().at("three").get_object().at("C") == json_value_t("ccc"));
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



