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

seq_t skip_whitespace(const seq_t& s){
	return read_while(s, whitespace_chars).second;
}

/*
	"{}"
	"{ "one": 10.0, "two": "2" }"
*/

std::pair<json_value_t, std::string> parse_json_object(const seq_t&& s2){
/*
	const auto s = skip_whitespace(s2);

	seq body = get_balanced_pair(s, '{', '}');

	const auto a = skip_whitespace(s);
	if(a.empty()){
		return { json_value_t(std::map<string, json_value_t>()), body.second };
	}
	else {
		std::pair<json_value_t, std::string> key0 = parse_json(a);



		return {};
	}
*/
		return {};
}



std::pair<json_value_t, seq_t> parse_json(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto ch = a.first();
	if(ch == "{"){
		std::map<string, json_value_t> obj;
		auto p2 = skip_whitespace(a.rest());
		while(p2.first() != "}"){

			//	"my_key": EXPRESSION,

			const auto key_p = parse_json(p2);
			if(!key_p.first.is_string()){
				throw std::runtime_error("");
			}
			const string key = key_p.first.get_string();

			auto p3 = skip_whitespace(key_p.second);
			if(p3.first() != ":"){
				throw std::runtime_error("");
			}

			p3 = skip_whitespace(p3.rest());

			const auto expression_p = parse_json(p3);
			obj.insert(std::make_pair(key, expression_p.first));

			auto post_p = skip_whitespace(expression_p.second);
			if(post_p.first() != "," && post_p.first() != "}"){
				throw std::runtime_error("");
			}

			if(post_p.first() == ","){
				post_p = skip_whitespace(post_p.rest());
			}
			p2 = post_p;
		}
		return { json_value_t::make_object(obj), p2.rest() };
	}
	else if(ch == "["){
		vector<json_value_t> array;
		auto p2 = skip_whitespace(a.rest());
		while(p2.first() != "]"){
			const auto expression_p = parse_json(p2);
			array.push_back(expression_p.first);

			auto post_p = skip_whitespace(expression_p.second);
			if(post_p.first() != "," && post_p.first() != "]"){
				throw std::runtime_error("");
			}

			if(post_p.first() == ","){
				post_p = skip_whitespace(post_p.rest());
			}
			p2 = post_p;
		}
		return { json_value_t::make_array2(array), p2.rest() };
	}
	else if(ch == "\""){
		const auto b = read_while_not(a.rest(), "\"");
		return { json_value_t(b.first), b.second.rest() };
	}
	else if(peek(a, "true").first){
		return { json_value_t(true), peek(a, "true").second };
	}
	else if(peek(a, "false").first){
		return { json_value_t(false), peek(a, "false").second };
	}
	else if(peek(a, "null").first){
		return { json_value_t(), peek(a, "null").second };
	}
	else{
		const auto number_pos = read_while(a, "-0123456789.+");
		if(number_pos.first.empty()){
			return { json_value_t(), a };
		}
		else{
			double number = parse_float(number_pos.first);
			return { json_value_t(number), number_pos.second };
		}
	}
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("\"xyz\"xxx")), { json_value_t("xyz"), seq_t("xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("\"\"xxx")), { json_value_t(""), seq_t("xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("13.0 xxx")), { json_value_t(13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("-13.0 xxx")), { json_value_t(-13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("4 xxx")), { json_value_t(4.0), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("true xxx")), { json_value_t(true), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("false xxx")), { json_value_t(false), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("null xxx")), { json_value_t(), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "array - empty"){
	quark::ut_compare(parse_json(seq_t("[] xxx")), { json_value_t::make_array(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - two numbers"){
	quark::ut_compare(parse_json(seq_t("[10, 11] xxx")), { json_value_t::make_array2({json_value_t(10.0), json_value_t(11.0)}), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - nested"){
	quark::ut_compare(
		parse_json(seq_t("[10, 11, [ 12, 13]] xxx")),
		{
			json_value_t::make_array2({json_value_t(10.0), json_value_t(11.0), json_value_t::make_array2({json_value_t(12.0), json_value_t(13.0)}) }),
			seq_t(" xxx")
		}
	);
}


QUARK_UNIT_TESTQ("parse_json()", "object - empty"){
	quark::ut_compare(parse_json(seq_t("{} xxx")), { json_value_t::make_object(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "object - two entries"){
	const auto result = parse_json(seq_t("{\"one\": 1, \"two\": 2} xxx"));
	QUARK_TRACE(json_to_compact_string(result.first));

	quark::ut_compare(result, { json_value_t::make_object({{"one", json_value_t(1.0)}, {"two", json_value_t(2.0)}}), seq_t(" xxx") });
}



//???	Parse objects and arrays

