//
//  parser_primitives.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_primitives.h"

#include "text_parser.h"
#include "json_support.h"
#include "floyd_syntax.h"
#include "compiler_basics.h"
#include "ast_value.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd {

namespace parser {

//////////////////////////////////////////////////		Text parsing primitives


std::string skip_whitespace(const std::string& s){
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
	std::string a = "";
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
			throw_compiler_error_nopos("Unbalanaced comments /* ... */");
		}
	}
	throw_compiler_error_nopos("Unbalanaced comments /* ... */");
}

std::pair<std::string, seq_t> skip_whitespace2(const seq_t& s){
	std::pair<std::string, seq_t> p = { "", s };

	while(!p.second.empty()){
		const auto ch2 = p.second.first(2);

		//	Whitespace?
		if(k_whitespace_chars.find(p.second.first1_char()) != std::string::npos){
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

QUARK_TEST("", "skip_whitespace2()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("")).second == seq_t(""));
}
QUARK_TEST("", "skip_whitespace2()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t(" ")).second == seq_t(""));
}
QUARK_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("\t")).second == seq_t(""));
}
QUARK_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("int blob()")).second == seq_t("int blob()"));
}
QUARK_TEST("", "skip_whitespace2(seq_t()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("\t\t\t int blob()")).second == seq_t("int blob()"));
}


QUARK_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("//xyz")).second == seq_t(""));
}
QUARK_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("//xyz\nabc")).second == seq_t("abc"));
}
QUARK_TEST("", "skip_whitespace2()", "//", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("   \t//xyz \t\n\t abc")).second == seq_t("abc"));
}

QUARK_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/**/xyz")).second == seq_t("xyz"));
}
QUARK_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*abc*/xyz")).second == seq_t("xyz"));
}
QUARK_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*abc*/xyz")).second == seq_t("xyz"));
}
QUARK_TEST("", "skip_whitespace2()", "/* */", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("   \t/*a \tbc*/   \txyz")).second == seq_t("xyz"));
}

QUARK_TEST("", "skip_whitespace2()", "/* */", ""){
	try {
		QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*xyz")).second == seq_t(""));
		fail_test(QUARK_POS);
	}
	catch(...) {
	}
}

QUARK_TEST("", "skip_whitespace2()", "/* */ -- nested", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*/**/*/xyz")).second == seq_t("xyz"));
}
QUARK_TEST("", "skip_whitespace2()", "/* */ -- nested", ""){
	QUARK_TEST_VERIFY(skip_whitespace2(seq_t("/*xyz/*abc*/123*/789")).second == seq_t("789"));
}


bool is_whitespace(char ch){
	return k_whitespace_chars.find(std::string(1, ch)) != std::string::npos;
}

QUARK_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
}
QUARK_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
}
QUARK_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
}
QUARK_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}


std::string skip_whitespace_ends(const std::string& s){
	const auto a = skip_whitespace(s);

	std::string::size_type i = a.size();
	while(i > 0 && k_whitespace_chars.find(a[i - 1]) != std::string::npos){
		i--;
	}
	return a.substr(0, i);
}

QUARK_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("") == "");
}
QUARK_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  a") == "a");
}
QUARK_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("b  ") == "b");
}
QUARK_TEST("", "skip_whitespace_ends()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace_ends("  c  ") == "c");
}


//////////////////////////////////////////////////		BALANCING PARANTHESES, BRACKETS


std::pair<std::string, seq_t> get_balanced(const seq_t& s){
	QUARK_ASSERT(s.size() > 0);

	const auto r = read_balanced2(s, k_bracket_pairs);
	if(r.first == ""){
		throw_compiler_error_nopos("unbalanced ([{< >}])");
	}

	return r;
}

QUARK_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced(seq_t("()")) == (std::pair<std::string, seq_t>("()", seq_t(""))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)")) == (std::pair<std::string, seq_t>("(abc)", seq_t(""))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(abc)xyz")) == (std::pair<std::string, seq_t>("(abc)", seq_t("xyz"))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc))xyz")) == (std::pair<std::string, seq_t>("((abc))", seq_t("xyz"))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("((abc)[])xyz")) == (std::pair<std::string, seq_t>("((abc)[])", seq_t("xyz"))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("(return 4 < 5;)xxx")) == (std::pair<std::string, seq_t>("(return 4 < 5;)", seq_t("xxx"))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{}")) == (std::pair<std::string, seq_t>("{}", seq_t(""))));
}
QUARK_TEST("", "get_balanced()", "", ""){
	QUARK_TEST_VERIFY(get_balanced(seq_t("{aaa}bbb")) == (std::pair<std::string, seq_t>("{aaa}", seq_t("bbb"))));
}
QUARK_TEST("", "get_balanced()", "", ""){
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

QUARK_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t("()xyz")) == std::pair<std::string, seq_t>{"", seq_t("xyz") } 	));
}
QUARK_TEST("", "read_enclosed_in_parantheses()", "", ""){
	QUARK_UT_VERIFY((	read_enclosed_in_parantheses(seq_t(" ( abc )xyz")) == std::pair<std::string, seq_t>{" abc ", seq_t("xyz") } 	));
}


const auto open_close2 = deinterleave_string(k_bracket_pairs);


std::pair<std::string, seq_t> read_until_toplevel_match(const seq_t& s, const std::string& match_chars){
	auto pos = s;
	while(pos.empty() == false && match_chars.find(pos.first1()) == std::string::npos){
		const auto ch = pos.first();
		if(open_close2.first.find(ch) != std::string::npos){
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

QUARK_TEST("", "read_until_toplevel_match()", "", ""){
	try {
		read_until_toplevel_match(seq_t("print(json_to_string(json_to_value(value_to_json(\"cola\"));\n\t"), ";{");
		fail_test(QUARK_POS);
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
	const auto b = read_while(a, k_identifier_chars);
	return b;
}
std::pair<std::string, seq_t> read_required_identifier(const seq_t& s){
	const auto b = read_identifier(s);
	if(b.first.empty()){
		throw_compiler_error_nopos("missing identifier");
	}
	return b;
}

QUARK_TEST("read_required_identifier()", "", "", ""){
	QUARK_TEST_VERIFY(read_required_identifier(seq_t("\thello\txxx")) == (std::pair<std::string, seq_t>("hello", seq_t("\txxx"))));
}


//////////////////////////////////////		TYPES


static std::pair<itype_t, seq_t> read_basic_type(type_interner_t& type_interner, const seq_t& s){
	const auto pos0 = skip_whitespace(s);

	const auto pos1 = read_while(pos0, k_identifier_chars + "*");
	if(pos1.first.empty()){
		return { itype_t::make_undefined(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_undefined){
		return { itype_t::make_undefined(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_any){
		return { itype_t::make_any(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_void){
		return { itype_t::make_void(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_bool){
		return { itype_t::make_bool(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_int){
		return { itype_t::make_int(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_double){
		return { itype_t::make_double(), pos1.second };
	}
	//??? typeid_t
	else if(pos1.first == keyword_t::k_string){
		return { itype_t::make_string(), pos1.second };
	}
	else if(pos1.first == keyword_t::k_json){
		return { itype_t::make_json(), pos1.second };
	}
	else{
		return { make_identifier(type_interner, pos1.first), pos1.second };
	}
}


std::vector<member_itype_t> parse_functiondef_arguments2(const std::string& s, bool require_arg_names){
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '(');
	QUARK_ASSERT(s.back() == ')');

	const auto s2 = seq_t(trim_ends(s));
	std::vector<member_itype_t> args;
	auto pos = skip_whitespace(s2);
	while(!pos.empty()){
		type_interner_t temp_type_interner;
		const auto arg_type = read_required_type(temp_type_interner, pos);
		const auto arg_name = read_identifier(arg_type.second);
		if(require_arg_names && arg_name.first.empty()){
			quark::throw_runtime_error("Invalid function definition.");
		}
		const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ',');
		args.push_back({ arg_type.first, arg_name.first });
		pos = skip_whitespace(optional_comma.second);
	}
	return args;
}

QUARK_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("()", true) == std::vector<member_itype_t>{}		));
}
QUARK_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int a)", true) == std::vector<member_itype_t>{ { typeid_t::make_int(), "a" }}		));
}
QUARK_TEST("", "parse_functiondef_arguments2()", "", ""){
	QUARK_TEST_VERIFY((		parse_functiondef_arguments2("(int x, string y, double z)", true) == std::vector<member_itype_t>{
		{ typeid_t::make_int(), "x" },
		{ typeid_t::make_string(), "y" },
		{ typeid_t::make_double(), "z" }

	}		));
}

std::pair<std::vector<member_itype_t>, seq_t> read_functiondef_arg_parantheses(const seq_t& s){
	QUARK_ASSERT(s.first1() == "(");

	std::pair<std::string, seq_t> args_pos = read_balanced2(s, k_bracket_pairs);
	if(args_pos.first.empty()){
		throw_compiler_error_nopos("unbalanced ()");
	}
	const auto r = parse_functiondef_arguments2(args_pos.first, true);
	return { r, args_pos.second };
}


std::pair<std::vector<member_itype_t>, seq_t> read_function_type_args(const seq_t& s){
	QUARK_ASSERT(s.first1() == "(");

	std::pair<std::string, seq_t> args_pos = read_balanced2(s, k_bracket_pairs);
	if(args_pos.first.empty()){
		throw_compiler_error_nopos("unbalanced ()");
	}
	const auto r = parse_functiondef_arguments2(args_pos.first, false);
	return { r, args_pos.second };
}

QUARK_TEST("", "read_function_type_args()", "", ""){
	const auto result = read_function_type_args(seq_t("()"));
	QUARK_TEST_VERIFY(result.first.empty());
	QUARK_TEST_VERIFY(result.second.empty());
}
QUARK_TEST("", "read_function_type_args()", "", ""){
	const auto result = read_function_type_args(seq_t("(int, double)"));
	QUARK_TEST_VERIFY(result.first.size() == 2);
	QUARK_TEST_VERIFY(result.first[0]._type.is_int());
	QUARK_TEST_VERIFY(result.first[0]._name == "");
	QUARK_TEST_VERIFY(result.first[1]._type.is_double());
	QUARK_TEST_VERIFY(result.first[1]._name == "");
	QUARK_TEST_VERIFY(result.second.empty());
}
QUARK_TEST("", "read_function_type_args()", "", ""){
	const auto result = read_function_type_args(seq_t("(int x, double y)"));
	QUARK_TEST_VERIFY(result.first.size() == 2);
	QUARK_TEST_VERIFY(result.first[0]._type.is_int());
	QUARK_TEST_VERIFY(result.first[0]._name == "x");
	QUARK_TEST_VERIFY(result.first[1]._type.is_double());
	QUARK_TEST_VERIFY(result.first[1]._name == "y");
	QUARK_TEST_VERIFY(result.second.empty());
}

static std::pair<itype_t, seq_t> read_basic_or_vector(type_interner_t& type_interner, const seq_t& s){
	const auto pos0 = skip_whitespace(s);
	if(pos0.first1() == "["){
		const auto pos2 = pos0.rest1();
		type_interner_t temp_type_interner;
		const auto element_type_pos = read_required_type(temp_type_interner, pos2);
		const auto pos3 = skip_whitespace(element_type_pos.second);
		if(pos3.first1() == ":"){
			const auto pos4 = pos3.rest1();

			if(element_type_pos.first.is_string() == false){
				throw_compiler_error_nopos("Dict only support string as key!");
			}
			else{
				const auto element_type2_pos = read_required_type(type_interner, skip_whitespace(pos4));

				if(element_type2_pos.second.first1() == "]"){
					return {
						itype_t::make_dict(type_interner, element_type2_pos.first),
						element_type2_pos.second.rest1()
					};
				}
				else{
					throw_compiler_error_nopos("unbalanced [].");
				}
			}
		}
		else if(pos3.first1() == "]"){
			return { typeid_t::make_vector(type_interner, element_type_pos.first), pos3.rest1() };
		}
		else{
			throw_compiler_error_nopos("unbalanced [].");
		}
	}
	else {
		//	Read basic type.
		const auto basic = read_basic_type(type_interner, pos0);
		return basic;
	}
}


static std::pair<itype_t, seq_t> read_optional_trailing_function_type_args(type_interner_t& type_interner, const typeid_t& type, const seq_t& s){
	//	See if there is a () afterward type_pos -- that would be that type_pos is the return value of a function-type.
	const auto more_pos = skip_whitespace(s);
	if(more_pos.first1() == "("){
		const auto function_args_pos = read_function_type_args(more_pos);
		std::vector<itype_t> nameless_args = get_member_types(function_args_pos.first);

		const auto impure_pos = if_first(skip_whitespace(function_args_pos.second), keyword_t::k_impure);

		const auto pos = function_args_pos.second;
		const auto function_type = itype_t::make_function(type_interner, type, nameless_args, impure_pos.first ? epure::impure : epure::pure);
		const auto result = read_optional_trailing_function_type_args(type_interner, function_type, pos);
		return result;
	}
	else{
		return { type, s };
	}
}

std::pair<std::shared_ptr<typeid_t>, seq_t> read_type(type_interner_t& type_interner, const seq_t& s){
	const auto type_pos = read_basic_or_vector(type_interner, s);
	if(type_pos.first.is_undefined()){
		return std::pair<std::shared_ptr<typeid_t>, seq_t>(nullptr, type_pos.second);
	}
	else {
		const auto r = read_optional_trailing_function_type_args(type_interner, type_pos.first, type_pos.second);
		return { r.first.is_undefined() ? nullptr : std::make_shared<typeid_t>(r.first), r.second };
	}
}

QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(read_type(i, seq_t("-3")).first == nullptr);
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("undef")).first == typeid_t::make_undefined());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("any")).first == typeid_t::make_any());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("void")).first == typeid_t::make_void());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("bool")).first == typeid_t::make_bool());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("int")).first == typeid_t::make_int());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("double")).first == typeid_t::make_double());
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	QUARK_TEST_VERIFY(*read_type(i, seq_t("string")).first == typeid_t::make_string());
}
QUARK_TEST("", "read_type()", "identifier", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("temp"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_identifier(i, "temp"));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_TEST("", "read_type()", "vector", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("[int]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(i, typeid_t::make_int())		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_TEST("", "read_type()", "vector", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("[[int]]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_vector(i, typeid_t::make_vector(i, typeid_t::make_int()))		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}

QUARK_TEST("", "read_type()", "dict", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("[string: int]"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_dict(i, typeid_t::make_int())		);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}


QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("int ()"));
	QUARK_TEST_VERIFY(*r.first ==  typeid_t::make_function(i, typeid_t::make_int(), {}, epure::pure));
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}

QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("string (double a, double b)"));
	QUARK_TEST_VERIFY(	*r.first ==  typeid_t::make_function(i, typeid_t::make_string(), { typeid_t::make_double(), typeid_t::make_double() }, epure::pure)	);
	QUARK_TEST_VERIFY(r.second == seq_t(""));
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("int (double a) ()"));

	QUARK_TEST_VERIFY( *r.first == typeid_t::make_function(
		i,
		typeid_t::make_function(i, typeid_t::make_int(), { typeid_t::make_double() }, epure::pure),
		{},
		epure::pure
	));
	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
QUARK_TEST("", "read_type()", "", ""){
	type_interner_t i;
	const auto r = read_type(i, seq_t("bool (int (double a) b)"));

	QUARK_TEST_VERIFY(
		*r.first
		==
		typeid_t::make_function(
			i,
			typeid_t::make_bool(),
			{
				typeid_t::make_function(i, typeid_t::make_int(), { typeid_t::make_double() }, epure::pure)
			},
			epure::pure
		)
	);

	QUARK_TEST_VERIFY(	r.second == seq_t("") );
}
/*
QUARK_TEST("", "read_type_identifier()", "", ""){
	QUARK_TEST_VERIFY(read_type_identifier(seq_t("int (double a) g(int(double b))")).first == "int (double a) g(int(double b))");
}

-	TYPE-IDENTIFIER (TYPE-IDENTIFIER a, TYPE-IDENTIFIER b, ...)

[int]([int] audio)
*/


std::pair<typeid_t, seq_t> read_required_type(type_interner_t& type_interner, const seq_t& s){
	const auto type_pos = read_type(type_interner, s);
	if(type_pos.first == nullptr){
		throw_compiler_error_nopos("illegal character in type identifier");
	}
	return { *type_pos.first, type_pos.second };
}







json_t make_parser_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

json_t parser__make_literal(const value_t& value){
	QUARK_ASSERT(is_floyd_literal(value.get_type()));

	return make_parser_node(
		floyd::k_no_location,
		parse_tree_expression_opcode_t::k_literal,
		{
			value_to_ast_json(value),
			itype_to_json(value.get_type())
		}
	);
}

}	// parser

}	//	floyd
