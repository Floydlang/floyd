//
//  json_parser.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 12/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "json_parser.h"

#include "json_support.h"

#include "text_parser.h"


using std::string;
using std::vector;
using std::pair;

	const std::string whitespace_chars = " \n\t";

std::string skip_whitespace(const std::string& s){
	return read_while(s, whitespace_chars).second;
}

std::pair<json_value_t, std::string> parse_json(const std::string& s){
	const auto a = skip_whitespace(s);
	if(peek_string(a, "{")){
		return { json_value_t(), a };
	}
	else if(peek_string(a, "[")){
		return { json_value_t(), a };
	}
	else if(peek_string(a, "\"")){
		const auto b = read_until(a.substr(1), "\"");
		const auto result = b.first;
		return { json_value_t(result), b.second.substr(1) };
	}
	else if(peek_string(a, "true")){
		return { json_value_t(true), a.substr(4) };
	}
	else if(peek_string(a, "false")){
		return { json_value_t(false), a.substr(5) };
	}
	else if(peek_string(a, "null")){
		return { json_value_t(), a.substr(4) };
	}
	else{
		const auto number_pos = read_while(a, "-0123456789.");
		//float parse_float(const std::string& pos);
		if(number_pos.first.empty()){
			return { json_value_t(), a };
		}
		else{
			double number = parse_float(number_pos.first);
			return { json_value_t(number), number_pos.second };
		}
	}
}

//double      stod( const std::string& str, std::size_t* pos = 0 );

QUARK_UNIT_TESTQ("parse_json()", ""){
	quark::ut_compare(parse_json("\"xyz\"xxx"), pair<json_value_t, string>{ json_value_t("xyz"), "xxx" });
}

QUARK_UNIT_TESTQ("parse_json()", ""){
	quark::ut_compare(parse_json("\"\"xxx"), pair<json_value_t, string>{ json_value_t(""), "xxx" });
}


QUARK_UNIT_TESTQ("parse_json()", ""){
	quark::ut_compare(parse_json("13.0 xxx"), pair<json_value_t, string>{ json_value_t(13.0), " xxx" });
}

QUARK_UNIT_TESTQ("parse_json()", ""){
	quark::ut_compare(parse_json("-13.0 xxx"), pair<json_value_t, string>{ json_value_t(-13.0), " xxx" });
}

QUARK_UNIT_TESTQ("parse_json()", ""){
	quark::ut_compare(parse_json("4 xxx"), pair<json_value_t, string>{ json_value_t(4.0), " xxx" });
}

