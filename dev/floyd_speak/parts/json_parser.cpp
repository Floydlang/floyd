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
				throw std::runtime_error("Missing key in JSON object");
			}
			const string key = key_p.first.get_string();

			auto p3 = skip_whitespace(key_p.second);
			if(p3.first1() != ":"){
				throw std::runtime_error("Missing : betweeen key and value in JSON object");
			}

			p3 = skip_whitespace(p3.rest1());

			const auto expression_p = parse_json(p3);
			obj.insert(std::make_pair(key, expression_p.first));

			auto post_p = skip_whitespace(expression_p.second);
			if(post_p.first1() != "," && post_p.first1() != "}"){
				throw std::runtime_error("Expected either , or } after JSON object field");
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
				throw std::runtime_error("Expected , or ] after JSON array element");
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


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("\"xyz\"xxx")), { json_t("xyz"), seq_t("xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("\"\"xxx")), { json_t(""), seq_t("xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("13.0 xxx")), { json_t(13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("-13.0 xxx")), { json_t(-13.0), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("4 xxx")), { json_t(4.0), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("true xxx")), { json_t(true), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("false xxx")), { json_t(false), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "primitive"){
	quark::ut_compare(parse_json(seq_t("null xxx")), { json_t(), seq_t(" xxx") });
}


QUARK_UNIT_TESTQ("parse_json()", "array - empty"){
	quark::ut_compare(parse_json(seq_t("[] xxx")), { json_t::make_array(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - two numbers"){
	quark::ut_compare(parse_json(seq_t("[10, 11] xxx")), { json_t::make_array({json_t(10.0), json_t(11.0)}), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "array - nested"){
	quark::ut_compare(
		parse_json(seq_t("[10, 11, [ 12, 13]] xxx")),
		{
			json_t::make_array({json_t(10.0), json_t(11.0), json_t::make_array({json_t(12.0), json_t(13.0)}) }),
			seq_t(" xxx")
		}
	);
}


QUARK_UNIT_TESTQ("parse_json()", "object - empty"){
	quark::ut_compare(parse_json(seq_t("{} xxx")), { json_t::make_object(), seq_t(" xxx") });
}

QUARK_UNIT_TESTQ("parse_json()", "object - two entries"){
	const auto result = parse_json(seq_t("{\"one\": 1, \"two\": 2} xxx"));
	QUARK_TRACE(json_to_compact_string(result.first));

	quark::ut_compare(result, { json_t::make_object({{"one", json_t(1.0)}, {"two", json_t(2.0)}}), seq_t(" xxx") });
}


//???	Parse objects and arrays

