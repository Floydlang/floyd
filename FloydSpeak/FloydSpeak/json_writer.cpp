//
//  json.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_writer.h"

#include "json_support.h"
#include "utils.h"

#include <map>

using std::string;
using std::vector;




std::string to_string(const std::map<std::string, json_value_t>& object){
	if(object.empty()){
		return "{}";
	}
	else{
		string members;
		for(const auto m: object){
			const auto s = quote(m.first) + ": " + to_string(m.second) + ", ";
			members = members + s;
		}

		//	Remove last ", ".
		members = members.substr(0, members.length() - 2);

		const auto result = std::string("{ ") + members + " }";
		return result;
	}
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(to_string(json_value_t(std::map<string, json_value_t>{})), "{}");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(
		to_string(json_value_t(std::map<string, json_value_t>{
			{ "one", json_value_t("1") },
			{ "two", json_value_t("2") }
		}
		)),
		"{ \"one\": \"1\", \"two\": \"2\" }"
	);
}


std::string to_string(const std::vector<json_value_t>& array){
	if(array.empty()){
		return "[]";
	}
	else{
		string items;
		size_t count = array.size();
		size_t index = 0;
		while(count > 1){
			items = items + to_string(array[index]) + ", ";
			count--;
			index++;
		}
		if(count > 0){
			items = items + to_string(array[index]);
		}
		const auto result = std::string("[ ") + items + " ]";
		return result;
	}
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(to_string(std::vector<json_value_t>{}), "[]");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(to_string(std::vector<json_value_t>{ json_value_t(13.4) }), "[ 13.400000 ]");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(
		to_string(vector<json_value_t>{
			json_value_t("a"),
			json_value_t("b")
		}),
		"[ \"a\", \"b\" ]"
	);
}


std::string to_string(const json_value_t& v){
	if(v.is_object()){
		return to_string(v.get_object());
	}
	else if(v.is_array()){
		return to_string(v.get_array());
	}
	else if(v.is_string()){
		return quote(v.get_string());
	}
	else if(v.is_number()){
		return std::to_string(v.get_number());
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
	}
}


QUARK_UNIT_TESTQ("to_string()", ""){
	const auto a = std::map<string, json_value_t>{
		{ "firstName", json_value_t("John") },
		{ "lastName", json_value_t("Doe") }
	};
	quark::ut_compare(to_string(json_value_t(a)), "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(
		to_string(json_value_t(
			vector<json_value_t>{
				json_value_t("a"),
				json_value_t("b")
			}
		)),
		"[ \"a\", \"b\" ]"
	);
}


QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t("")) == "\"\"");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t("xyz")) == "\"xyz\"");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t(12.3)) == "12.300000");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t(true)) == "true");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t(false)) == "false");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(to_string(json_value_t()) == "null");
}


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
		r = r.substr(0, r.length() - 2);

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


