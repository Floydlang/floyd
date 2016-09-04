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
#include "text_parser.h"
#include "json_parser.h"

#include <map>
#include <iostream>

using std::string;
using std::vector;

using namespace std;


static const size_t k_default_pretty_columns = 120;



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

	quark::ut_compare(s1, "\nHello\nWorld\n");

}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	const auto test_cpp11_raw_string_literals = R"aaa({ "firstName": "John", "lastName": "Doe" })aaa";
	quark::ut_compare(test_cpp11_raw_string_literals, "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	quark::ut_compare(R"aaa(["version": 2])aaa", "[\"version\": 2]");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	quark::ut_compare(R"___(["version": 2])___", "[\"version\": 2]");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	quark::ut_compare(R"<>(["version": 2])<>", "[\"version\": 2]");
}


QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	const auto test_cpp11_raw_string_literals = R"aaa({ "firstName": "John", "lastName": "Doe" })aaa";
	quark::ut_compare(test_cpp11_raw_string_literals, "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
}


//	Online escape tool: http://www.freeformatter.com/java-dotnet-escape.html#ad-output

const string compact_escaped = "{\"menu\": {\"id\": \"file\",\"popup\": {\"menuitem\": [{\"value\": \"New\",\"onclick\": \"CreateNewDoc()\"},{\"value\": \"Close\",\"onclick\": \"CloseDoc()\"}]}}}";
const string compact_raw_string = R"___({"menu": {"id": "file","popup": {"menuitem": [{"value": "New","onclick": "CreateNewDoc()"},{"value": "Close","onclick": "CloseDoc()"}]}}})___";

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	quark::ut_compare(compact_escaped, compact_raw_string);
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
	quark::ut_compare(erase_linefeed("\raaa\rbbb\r"), "aaabbb");
}

QUARK_UNIT_TESTQ("C++11 raw string literals", ""){
	quark::ut_compare(erase_linefeed(beautiful_escaped), beautiful_raw_string);
}








std::string object_to_compact_string(const std::map<std::string, json_value_t>& object){
	if(object.empty()){
		return "{}";
	}
	else{
		string members;
		for(const auto m: object){
			const auto s = quote(m.first) + ": " + json_to_compact_string(m.second) + ", ";
			members = members + s;
		}

		//	Remove last ", ".
		members = members.substr(0, members.length() - 2);

		const auto result = std::string("{ ") + members + " }";
		return result;
	}
}

QUARK_UNIT_TESTQ("object_to_compact_string()", ""){
	quark::ut_compare(object_to_compact_string(std::map<string, json_value_t>{}), "{}");
}

QUARK_UNIT_TESTQ("object_to_compact_string()", ""){
	quark::ut_compare(
		object_to_compact_string(std::map<string, json_value_t>{
			{ "one", json_value_t("1") },
			{ "two", json_value_t("2") }
		}
		),
		"{ \"one\": \"1\", \"two\": \"2\" }"
	);
}


std::string array_to_compact_string(const std::vector<json_value_t>& array){
	if(array.empty()){
		return "[]";
	}
	else{
		string items;
		size_t count = array.size();
		size_t index = 0;
		while(count > 1){
			items = items + json_to_compact_string(array[index]) + ", ";
			count--;
			index++;
		}
		if(count > 0){
			items = items + json_to_compact_string(array[index]);
		}
		const auto result = std::string("[") + items + "]";
		return result;
	}
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	quark::ut_compare(array_to_compact_string(std::vector<json_value_t>{}), "[]");
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	quark::ut_compare(array_to_compact_string(std::vector<json_value_t>{ json_value_t(13.4) }), "[13.4]");
}

QUARK_UNIT_TESTQ("array_to_compact_string()", ""){
	quark::ut_compare(
		array_to_compact_string(vector<json_value_t>{
			json_value_t("a"),
			json_value_t("b")
		}),
		"[\"a\", \"b\"]"
	);
}


std::string json_to_compact_string(const json_value_t& v){
	if(v.is_object()){
		return object_to_compact_string(v.get_object());
	}
	else if(v.is_array()){
		return array_to_compact_string(v.get_array());
	}
	else if(v.is_string()){
		return quote(v.get_string());
	}
	else if(v.is_number()){
		return double_to_string(v.get_number());
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

QUARK_UNIT_TESTQ("json_to_compact_string()", ""){
	const auto a = std::map<string, json_value_t>{
		{ "firstName", json_value_t("John") },
		{ "lastName", json_value_t("Doe") }
	};
	quark::ut_compare(json_to_compact_string(json_value_t(a)), "{ \"firstName\": \"John\", \"lastName\": \"Doe\" }");
	quark::ut_compare(json_to_compact_string(json_value_t(a)), R"aaa({ "firstName": "John", "lastName": "Doe" })aaa");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	quark::ut_compare(
		json_to_compact_string(json_value_t(
			vector<json_value_t>{
				json_value_t("a"),
				json_value_t("b")
			}
		)),
		"[\"a\", \"b\"]"
	);
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t("")) == "\"\"");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t("xyz")) == "\"xyz\"");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t(12.3)) == "12.3");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t(true)) == "true");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t(false)) == "false");
}

QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_UT_VERIFY(json_to_compact_string(json_value_t()) == "null");
}



std::string json_to_pretty_string(const json_value_t& v){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	return json_to_pretty_string(v, quark::get_log_indent(), pretty);
}


bool is_collection(const json_value_t& type){
	return type.is_object() || type.is_array();
}
bool is_subtree(const json_value_t& type){
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
		widest_line_chars = max(widest_line_chars, count);
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
std::string json_to_pretty_string_internal(const string& key, const json_value_t& value, int pos, const pretty_t& pretty){
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
		return string(pos, '\t') + key_str + double_to_string(value.get_number());
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
	}
}

std::string json_to_pretty_string(const json_value_t& value, int pos, const pretty_t& pretty){
	return json_to_pretty_string_internal("", value, pos, pretty);
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", ""){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	std::cout <<(
		json_to_pretty_string(
			json_value_t::make_object({}),
			quark::get_log_indent(),
			pretty
		)
	);
}


QUARK_UNIT_TESTQ("cout()", ""){
	std::cout <<("\t\t\t{\n\t\t\t\t\"a\": 100\n\t\t\t}\n");
	std::cout << R"(
			{
				"a": 100,
				"b": 234
			}
	)";
}

QUARK_UNIT_TESTQ("json_to_pretty_string()", "object"){
	const auto value = json_value_t::make_object({
		{ "one", json_value_t(111.0) },
		{ "two", json_value_t(222.0) }
	});

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	std::cout << result << "\n";
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", "nested object"){
	const auto italian = json_value_t::make_object({
		{ "uno", json_value_t(1.0) },
		{ "duo", json_value_t(2.0) },
		{ "tres", json_value_t(3.0) },
		{ "quattro", json_value_t(3.0) }
	});

	const auto value = json_value_t::make_object({
		{ "one", json_value_t(111.0) },
		{ "two", json_value_t(222.0) },
		{ "italian", italian }
	});

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	std::cout << result<<"\n";
}


QUARK_UNIT_TESTQ("json_to_pretty_string()", "array"){
	const auto value = json_value_t::make_array2({ "one", "two", "three" });

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	std::cout << result << "\n";
}

QUARK_UNIT_TESTQ("json_to_pretty_string()", "nested arrays"){
	const auto italian = json_value_t::make_array2({ "uno", "duo", "tres", "quattro" });
	const auto value = json_value_t::make_array2({ "one", "two", italian, "three" });

	const pretty_t pretty{ k_default_pretty_columns, 4 };
	const string result = json_to_pretty_string(
		value,
		quark::get_log_indent(),
		pretty
	);
	std::cout << result << "\n";
}





QUARK_UNIT_TESTQ("json_to_pretty_string()", ""){
	const pretty_t pretty{ k_default_pretty_columns, 4 };
	std::cout <<(
		json_to_pretty_string(
			parse_json(seq_t(beautiful_raw_string)).first,
			quark::get_log_indent(),
			pretty
		)
	) << "\n";
}



string get_test2(){
return R"(
{
	"main": [
		{
			"base_type": "function",
			"scope_def": {
				"_args": [
					
				],
				"_locals": [
					
				],
				"_members": [
					
				],
				"_name": "main",
				"_return_type": "int",
				"_statements": [
					[
						"return",
						[
							"k",
							"<int>",
							3
						]
					]
				],
				"_type": "function",
				"_types": [
					[
						"return",
						[
							"k",
							"<int>",
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
				"_args": [],
				"_locals": [],
				"_members": [],
				"_name": "main",
				"_return_type": "int",
				"_statements": [ [ "return", [ "k", "<int>", 3 ] ] ],
				"_type": "function",
				"_types": [ [ "return", [ "k", "<int>", 3 ] ] ]
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

	std::cout << b << "\n";
}



