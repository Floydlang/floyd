//
//  parser_primitives.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_primitives.h"


#include "text_parser.h"

#include "statement.h"
#include "ast_value.h"
#include "json_support.h"
#include "json_parser.h"
#include "json_writer.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd {

using std::make_shared;
using std::shared_ptr;
using std::vector;
using std::string;
using std::pair;


//////////////////////////////////////////////////		Text parsing primitives


std::string skip_whitespace(const string& s){
	return skip_whitespace2(seq_t(s)).second.str();
}
seq_t skip_whitespace(const seq_t& s){
	return skip_whitespace2(s).second;
}

//	Test where C++ lets you insert comments:

	int my_global1 = 3;/*xyz*/
	int my_global2 = 3 /*xyz*/;
	int my_global3 = /*xyz*/ 3;
	int my_global4 /*xyz*/ = 3;
	int /*xyz*/ my_global5 = 3;
	int /*xyz*/ my_global6 = 3;
	/*xyz*/int my_global7 = 3;


std::pair<std::string, seq_t> while_multicomment(const seq_t& s){
	QUARK_ASSERT(s.first(2) == "/*");

	auto p = s.rest(2);
	string a = "";
	while(!p.empty()){
		//	Skip uninteresting chars.
		while(!p.empty() && p.first(2) != "/*" && p.first(2) != "*/"){
			a = a + p.first(1);
			p = p.rest(1);
		}

		if(p.first(2) == "/*"){
			const auto t = while_multicomment(p);
			a = a + t.first;
			p = t.second;
		}
		else if(p.first(2) == "*/"){
			return { a, p.rest(2) };
		}
		else{
			throw std::runtime_error("Unbalanaced comments /* ... */");
		}
	}
	throw std::runtime_error("Unbalanaced comments /* ... */");
}

pair<string, seq_t> skip_whitespace2(const seq_t& s){
	pair<string, seq_t> p = { "", s };

	while(!p.second.empty()){
		const auto ch2 = p.second.first(2);

		//	Whitespace?
		if(whitespace_chars.find(p.second.first1_char()) != string::npos){
			p = { p.first + p.second.first(1), p.second.rest(1) };
		}
		else if(ch2 == "//"){
			auto p2 = read_until(p.second.rest(2), "\n");
			p = { p.first + p2.first, p2.second };
		}
		else if(ch2 == "/*"){
			const auto px = while_multicomment(p.second);
			p = { p.first + px.first, px.second };
		}
		else{
			return p;
		}
	}

	return p;
}

QUARK_UNIT_TEST("", "skip_whitespace2()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("")).second == seq_t(""));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t(" ")).second == seq_t(""));
}
QUARK_UNIT_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("\t")).second == seq_t(""));
}
QUARK_UNIT_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("int blob()")).second == seq_t("int blob()"));
}
QUARK_UNIT_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("\t\t\t int blob()")).second == seq_t("int blob()"));
}


QUARK_UNIT_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("//xyz")).second == seq_t(""));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("//xyz\nabc")).second == seq_t("abc"));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("   \t//xyz \t\n\t abc")).second == seq_t("abc"));
}

QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/**/xyz")).second == seq_t("xyz"));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*abc*/xyz")).second == seq_t("xyz"));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*abc*/xyz")).second == seq_t("xyz"));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("   \t/*a \tbc*/   \txyz")).second == seq_t("xyz"));
}

QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */", ""){
	try {
		QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*xyz")).second == seq_t(""));
		QUARK_UT_VERIFY(false);
	}
	catch(...) {
	}
}

QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */ -- nested", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*/**/*/xyz")).second == seq_t("xyz"));
}
QUARK_UNIT_TEST("", "skip_whitespace2()", "/* */ -- nested", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*xyz/*abc*/123*/789")).second == seq_t("789"));
}


bool is_whitespace(char ch){
	return whitespace_chars.find(string(1, ch)) != string::npos;
}

QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
}
QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}


std::string skip_whitespace_ends(const std::string& s){
	const auto a = skip_whitespace(s);

	string::size_type i = a.size();
	while(i > 0 && whitespace_chars.find(a[i - 1]) != string::npos){
		i--;
	}
	return a.substr(0, i);
}

QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("") == "");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  a") == "a");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("b  ") == "b");
}
QUARK_UNIT_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  c  ") == "c");
}


//////////////////////////////////////////////////		BALANCING PARANTHESES, BRACKETS


std::pair<std::string, seq_t> get_balanced(const seq_t& s){
	QUARK_ASSERT(s.size() > 0);

	const auto r = read_balanced2(s, bracket_pairs);
	if(r.first == ""){
		throw std::runtime_error("unbalanced ([{< >}])");
	}

	return r;
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced(seq_t("()")) == (std::pair<std::string, seq_t>("()", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)")) == (std::pair<std::string, seq_t>("(abc)", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)xyz")) == (std::pair<std::string, seq_t>("(abc)", seq_t("xyz"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc))xyz")) == (std::pair<std::string, seq_t>("((abc))", seq_t("xyz"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc)[])xyz")) == (std::pair<std::string, seq_t>("((abc)[])", seq_t("xyz"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(return 4 < 5;)xxx")) == (std::pair<std::string, seq_t>("(return 4 < 5;)", seq_t("xxx"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{}")) == (std::pair<std::string, seq_t>("{}", seq_t(""))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{aaa}bbb")) == (std::pair<std::string, seq_t>("{aaa}", seq_t("bbb"))));
}
QUARK_UNIT_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{return 4 < 5;}xxx")) == (std::pair<std::string, seq_t>("{return 4 < 5;}", seq_t("xxx"))));
}
//	QUARK_TEST_VERIFY(get_balanced("{\n\t\t\t\treturn 4 < 5;\n\t\t\t}\n\t\t") == seq("((abc)[])", xyz));


std::pair<std::string, seq_t> read_enclosed_in_parantheses(const seq_t& pos){
	const auto pos2 = skip_whitespace(pos);
	read_required(pos2, "(");
	const auto range = get_balanced(pos2);
	const auto range2 = seq_t(trim_ends(range.first));
	return { range2.str(), range.second };
}

QUARK_UNIT_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t("()xyz")) == std::pair<std::string, seq_t>{"", seq_t("xyz") } 	));
}
QUARK_UNIT_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t(" ( abc )xyz")) == std::pair<std::string, seq_t>{" abc ", seq_t("xyz") } 	));
}


const auto open_close2 = deinterleave_string(bracket_pairs);


std::pair<string, seq_t> read_until_toplevel_match(const seq_t& s, const std::string& match_chars){
	auto pos = s;
	while(pos.empty() == false && match_chars.find(pos.first1()) == string::npos){
		const auto ch = pos.first();
		if(open_close2.first.find(ch) != string::npos){
			const auto end = get_balanced(pos).second;
			pos = end;
		}
		else{
			pos = pos.rest1();
		}
	}
	if (pos.empty()){
		return { s.str(), seq_t("") };
//		return { "", s };
	}
	else{
		const auto r = get_range(s, pos);
		return { r, pos };
	}
}

QUARK_UNIT_TEST("", "read_until_toplevel_match()", "", ""){
	try {
		read_until_toplevel_match(seq_t("print(json_to_string(json_to_value(value_to_json(\"cola\"));\n\t"), ";{");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

///### TESTS


//////////////////////////////////////		BASIC STRING


std::string reverse(const std::string& s){
	return std::string(s.rbegin(), s.rend());
}


//////////////////////////////////////		IDENTIFIER


//	Returns "" if no symbol is found.
std::pair<std::string, seq_t> read_identifier(const seq_t& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);
	return b;
}
std::pair<std::string, seq_t> read_required_identifier(const seq_t& s){
	const auto b = read_identifier(s);
	if(b.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	return b;
}

QUARK_UNIT_TESTQ("read_required_identifier()", ""){
	QUARK_TEST_VERIFY(read_required_identifier(seq_t("\thello\txxx")) == (std::pair<std::string, seq_t>("hello", seq_t("\txxx"))));
}


//////////////////////////////////////		TYPES


std::pair<shared_ptr<typeid_t>, seq_t> read_basic_type(const seq_t& s){
	const auto pos0 = skip_whitespace(s);

	const auto pos1 = read_while(pos0, identifier_chars + "*");
	if(pos1.first.empty()){
		return { nullptr, pos1.second };
	}
	else if(pos1.first == keyword_t::k_internal_undefined){
		return { make_shared<typeid_t>(typeid_t::make_undefined()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_internal_dynamic){
		return { make_shared<typeid_t>(typeid_t::make_internal_dynamic()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_void){
		return { make_shared<typeid_t>(typeid_t::make_void()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_bool){
		return { make_shared<typeid_t>(typeid_t::make_bool()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_int){
		return { make_shared<typeid_t>(typeid_t::make_int()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_double){
		return { make_shared<typeid_t>(typeid_t::make_double()), pos1.second };
	}
	//??? typeid_t
	else if(pos1.first == keyword_t::k_string){
		return { make_shared<typeid_t>(typeid_t::make_string()), pos1.second };
	}
	else if(pos1.first == keyword_t::k_json_value){
		return { make_shared<typeid_t>(typeid_t::make_json_value()), pos1.second };
	}
	else{
		return { make_shared<typeid_t>(typeid_t::make_unresolved_type_identifier(pos1.first)), pos1.second };
	}
}


vector<member_t> parse_functiondef_arguments2(const string& s){
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '(');
	QUARK_ASSERT(s.back() == ')');

	const auto s2 = seq_t(trim_ends(s));
	vector<member_t> args;
	auto pos = skip_whitespace(s2);
	while(!pos.empty()){
		const auto arg_type = read_required_type(pos);
		const auto arg_name = read_identifier(arg_type.second);
		if(arg_name.first.empty()){
			throw std::runtime_error("Invalid function definition.");
		}
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ',');
		args.push_back({ arg_type.first, arg_name.first });
		pos = skip_whitespace(optional_comma.second);
	}
	return args;
}

QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("()") == vector<member_t>{}		));
}
QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int a)") == vector<member_t>{ { typeid_t::make_int(), "a" }}		));
}
QUARK_UNIT_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int x, string y, double z)") == vector<member_t>{
		{ typeid_t::make_int(), "x" },
		{ typeid_t::make_string(), "y" },
		{ typeid_t::make_double(), "z" }

	}		));
}

std::pair<vector<member_t>, seq_t> read_function_arg_parantheses(const seq_t& s){
	QUARK_ASSERT(s.first1() == "(");

	std::pair<std::string, seq_t> args_pos = read_balanced2(s, bracket_pairs);
	if(args_pos.first.empty()){
		throw std::runtime_error("unbalanced ()");
	}
	const auto r = parse_functiondef_arguments2(args_pos.first);
	return { r, args_pos.second };
}

std::pair<shared_ptr<typeid_t>, seq_t> read_basic_or_vector(const seq_t& s){
	const auto pos0 = skip_whitespace(s);
	if(pos0.first1() == "["){
		const auto pos2 = pos0.rest1();
		const auto element_type_pos = read_required_type(pos2);
		const auto pos3 = skip_whitespace(element_type_pos.second);
		if(pos3.first1() == ":"){
			const auto pos4 = pos3.rest1();

			if(element_type_pos.first.is_string() == false){
				throw std::runtime_error("Dict only support string as key!");
			}
			else{
				const auto element_type2_pos = read_required_type(skip_whitespace(pos4));

				if(element_type2_pos.second.first1() == "]"){
					return {
						make_shared<typeid_t>(
							typeid_t::make_dict(element_type2_pos.first)
						),
						element_type2_pos.second.rest1()
					};
				}
				else{
					throw std::runtime_error("unbalanced []");
				}
			}
		}
		else if(pos3.first1() == "]"){
			return { make_shared<typeid_t>(typeid_t::make_vector(element_type_pos.first)), pos3.rest1() };
		}
		else{
			throw std::runtime_error("unbalanced []");
		}
	}
	else {
		//	Read basic type.
		const auto basic = read_basic_type(pos0);
		return basic;
	}
}

std::pair<shared_ptr<typeid_t>, seq_t> read_optional_trailing_function_args(const typeid_t& type, const seq_t& s){
	//	See if there is a () afterward type_pos -- that would be that type_pos is the return value of a function-type.
	const auto more_pos = skip_whitespace(s);
	if(more_pos.first1() == "("){
		const auto function_args_pos = read_function_arg_parantheses(more_pos);
		vector<typeid_t> nameless_args = get_member_types(function_args_pos.first);

		const auto impure_pos = if_first(skip_whitespace(function_args_pos.second), keyword_t::k_impure);

		const auto pos = function_args_pos.second;
		const auto function_type = typeid_t::make_function(type, nameless_args, impure_pos.first ? epure::impure : epure::pure);
		const auto result = read_optional_trailing_function_args(function_type, pos);
		return result;
	}
	else{
		return { make_shared<typeid_t>(type), s };
	}
}

std::pair<bool, seq_t> read_type_verify(const seq_t& s){
	QUARK_ASSERT(s.check_invariant());

	const auto a = read_type(s);
	return { a.first != nullptr, a.second };
}

std::pair<std::shared_ptr<typeid_t>, seq_t> read_type(const seq_t& s){
	const auto type_pos = read_basic_or_vector(s);
	if(type_pos.first == nullptr){
		return type_pos;
	}
	else {
		return read_optional_trailing_function_args(*type_pos.first, type_pos.second);
	}
}

QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(read_type(seq_t("-3")).first == nullptr);
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("**undef**")).first == typeid_t::make_undefined());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("**dyn**")).first == typeid_t::make_internal_dynamic());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("void")).first == typeid_t::make_void());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("bool")).first == typeid_t::make_bool());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("int")).first == typeid_t::make_int());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("double")).first == typeid_t::make_double());
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	QUARK_TEST_VERIFY(*read_type(seq_t("string")).first == typeid_t::make_string());
}
QUARK_UNIT_TEST("", "read_type()", "identifier", ""){
	const auto r = read_type(seq_t("temp"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_unresolved_type_identifier("temp"));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type()", "vector", ""){
	const auto r = read_type(seq_t("[int]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(typeid_t::make_int())		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type()", "vector", ""){
	const auto r = read_type(seq_t("[[int]]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(typeid_t::make_vector(typeid_t::make_int()))		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}

QUARK_UNIT_TEST("", "read_type()", "dict", ""){
	const auto r = read_type(seq_t("[string: int]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_dict(typeid_t::make_int())		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}


QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("int ()"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}

QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("string (double a, double b)"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_double(), typeid_t::make_double() }, epure::pure)	);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("int (double a) ()"));

	QUARK_TEST_VERIFY( *r.first == typeid_t::make_function(
			typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_double() }, epure::pure),
			{},
			epure::pure
		)
	);
	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
QUARK_UNIT_TEST("", "read_type()", "", ""){
	const auto r = read_type(seq_t("bool (int (double a) b)"));

	QUARK_TEST_VERIFY(
		*r.first
		==
		typeid_t::make_function(
			typeid_t::make_bool(),
			{
				typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_double() }, epure::pure)
			},
			epure::pure
		)
	);

	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
/*
QUARK_UNIT_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int (double a) g(int(double b))")).first == "int (double a) g(int(double b))");
}

-	TYPE-IDENTIFIER (TYPE-IDENTIFIER a, TYPE-IDENTIFIER b, ...)

[int]([int] audio)
*/


pair<typeid_t, seq_t> read_required_type(const seq_t& s){
	const auto type_pos = read_type(s);
	if(type_pos.first == nullptr){
		throw std::runtime_error("illegal character in type identifier");
	}
	return { *type_pos.first, type_pos.second };
}




ast_json_t make_statement(const location_t& loc, const std::string& opcode, const std::vector<json_t>& params){
	std::vector<json_t> e = { json_t(opcode) };
	e.insert(e.end(), params.begin(), params.end());
	const auto r = json_t(e);
	return ast_json_t{r};
}


QUARK_UNIT_TEST("", "make_statement()", "", ""){
	const auto r = make_statement({ "hello.floyd", 123, 80 }, "def-struct", {});

	QUARK_TEST_VERIFY( r._value == json_t::make_array({ "def-struct" }));
}


}	//	floyd
