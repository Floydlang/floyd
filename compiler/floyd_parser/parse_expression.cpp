//
//  parse_expression.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_expression.h"

#include "json_support.h"
#include "compiler_basics.h"
#include "parser_primitives.h"
#include "parse_statement.h"
#include "floyd_syntax.h"

#include "types.h"
#include "ast_value.h"


namespace floyd {
namespace parser {

static const char k_literal_divider_char = '\'';
const std::string k_hex_chars = "0123456789abcdef";


QUARK_TEST("parser", "C++ operators", "", ""){
	const int a = 2-+-2;
	ut_verify_auto(QUARK_POS, a, 4);
}


QUARK_TEST("parser", "C++ enum class()", "", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}


std::pair<json_t, seq_t> parse_expression_deep(const seq_t& p, const eoperator_precedence precedence);






std::string expr_to_string(const json_t& e){
	return json_to_compact_string(e);
}

/*
	()
	(100)
	(100, 200)
	(get_first() * 3, 4)

	[:]
	["one": 1000, "two": 2000]
*/
struct collection_element_t {
	std::shared_ptr<json_t> _key;
	json_t _value;
};

bool operator==(const collection_element_t& lhs, const collection_element_t& rhs){
	return compare_shared_values(lhs._key, rhs._key)
	&& lhs._value == rhs._value;
}


struct collection_def_t {
	bool _has_keys;
	std::vector<collection_element_t> _elements;
};

bool operator==(const collection_def_t& lhs, const collection_def_t& rhs){
	return
		lhs._has_keys == rhs._has_keys
		&& lhs._elements == rhs._elements;
}
std::vector<json_t> get_values(const collection_def_t& c){
	std::vector<json_t> result;
	for(const auto& e: c._elements){
		result.push_back(e._value);
	}
	return result;
}

void ut_verify_collection(const quark::call_context_t& context, const std::pair<collection_def_t, seq_t> result, const std::pair<collection_def_t, seq_t> expected){
	if(result == expected){
	}
	else{
		quark::fail_test(context);
	}
}

//	???	Return json_t directly, no need for collection_def_t-type.
std::pair<collection_def_t, seq_t> parse_bounded_list(const seq_t& s, const std::string& start_char, const std::string& end_char){
	QUARK_ASSERT(s.check_invariant());
	QUARK_ASSERT(s.first() == start_char);
	QUARK_ASSERT(start_char.size() == 1);
	QUARK_ASSERT(end_char.size() == 1);

	auto pos = skip_whitespace(s.rest1());
	if(pos.first1() == end_char){
		return {
			collection_def_t{ false, {} },
			pos.rest1()
		};
	}
	else if(pos.first1() == ":" && skip_whitespace(pos.rest1()).first1() == end_char){
		return {
			collection_def_t{ true, {} },
			skip_whitespace(pos.rest1()).rest1()
		};
	}
	else{
		collection_def_t result{false, {}};
		while(pos.first1() != end_char){
			const auto expression_pos = parse_expression_deep(pos, eoperator_precedence::k_super_weak);
			const auto pos2 = skip_whitespace(expression_pos.second);
			const auto ch = pos2.first1();
			if(ch == ","){
				result._elements.push_back(collection_element_t{ nullptr, expression_pos.first });
				pos = pos2.rest1();
			}
			else if(ch == end_char){
				result._elements.push_back(collection_element_t{ nullptr, expression_pos.first });
				pos = pos2;
			}
			else if(ch == ":"){
				result._has_keys = true;

				const auto pos3 = skip_whitespace(pos2.rest1());
				const auto expression2_pos = parse_expression_deep(pos3, eoperator_precedence::k_super_weak);
				const auto pos4 = skip_whitespace(expression2_pos.second);
				const auto ch2 = pos4.first1();
				if(ch2 == ","){
					result._elements.push_back(collection_element_t{ std::make_shared<json_t>(expression_pos.first), expression2_pos.first});
					pos = pos4.rest1();
				}
				else if(ch2 == end_char){
					result._elements.push_back(collection_element_t{ std::make_shared<json_t>(expression_pos.first), expression2_pos.first});
					pos = pos4;
				}
				else{
					throw_compiler_error_nopos("Unexpected char \"" + ch2 + "\" in bounded list " + start_char + " " + end_char + "!");
				}
			}
			else{
				throw_compiler_error_nopos("Unexpected char \"" + ch + "\" in bounded list " + start_char + " " + end_char + "!");
			}
		}
		return { result, pos.rest1() };
	}
}

QUARK_TEST("parser", "parse_bounded_list()", "", ""){
	ut_verify_collection(
		QUARK_POS,
		parse_bounded_list(seq_t("(3)xyz"), "(", ")"),
		std::pair<collection_def_t, seq_t>({false, {	{ nullptr, parser__make_literal(value_t::make_int(3)) }}}, seq_t("xyz"))
	);
}

QUARK_TEST("parser", "parse_bounded_list()", "", ""){
	ut_verify_collection(QUARK_POS, parse_bounded_list(seq_t("[]xyz"), "[", "]"), std::pair<collection_def_t, seq_t>({false, {}}, seq_t("xyz")));
}

QUARK_TEST("parser", "parse_bounded_list()", "", ""){
	ut_verify_collection(
		QUARK_POS,
		parse_bounded_list(seq_t("[1,2]xyz"), "[", "]"),
		std::pair<collection_def_t, seq_t>(
			{
				false,
				{
					{ nullptr, parser__make_literal(value_t::make_int(1)) },
					{ nullptr, parser__make_literal(value_t::make_int(2)) }
				}
			},
			seq_t("xyz")
		)
	);
}

QUARK_TEST("parser", "parse_bounded_list()", "blank dict", ""){
	ut_verify_collection(
		QUARK_POS,
		parse_bounded_list(seq_t(R"([:]xyz)"), "[", "]"),
		std::pair<collection_def_t, seq_t>(
			{
				true,
				{}
			},
			seq_t("xyz")
		)
	);
}

QUARK_TEST("parser", "parse_bounded_list()", "two elements", ""){
	ut_verify_collection(
		QUARK_POS,
		parse_bounded_list(seq_t(R"(["one": 1, "two": 2]xyz)"), "[", "]"),
		std::pair<collection_def_t, seq_t>(
			{
				true,
				{
					{ std::make_shared<json_t>(parser__make_literal(value_t::make_string("one"))), parser__make_literal(value_t::make_int(1)) },
					{ std::make_shared<json_t>(parser__make_literal(value_t::make_string("two"))), parser__make_literal(value_t::make_int(2)) }
				}
			},
			seq_t("xyz")
		)
	);
}

/*
Escape sequence	Hex value in ASCII	Character represented
\a	07	Alert (Beep, Bell) (added in C89)[1]
\b	08	Backspace
\f	0C	Formfeed
\n	0A	Newline (Line Feed); see notes below
\r	0D	Carriage Return
\t	09	Horizontal Tab
\v	0B	Vertical Tab
\\	5C	Backslash
\'	27	Single quotation mark
\"	22	Double quotation mark
\?	3F	Question mark (used to avoid trigraphs)
\nnnnote 1	any	The byte whose numerical value is given by nnn interpreted as an octal number
\xhh…	any	The byte whose numerical value is given by hh… interpreted as a hexadecimal number
\enote 2	1B	escape character (some character sets)
\Uhhhhhhhhnote 3	none	Unicode code point where h is a hexadecimal digit
\uhhhhnote 4	none	Unicode code point below 10000 hexadecimal
*/
//??? add tests for this.
int64_t expand_one_char_escape(const char ch2){
	switch(ch2){
		case '0':
			return 0x00;
		case 'a':
			return 0x07;
		case 'b':
			return 0x08;
		case 'f':
			return 0x0c;
		case 'n':
			return 0x0a;
		case 'r':
			return 0x0d;
		case 't':
			return 0x09;
		case 'v':
			return 0x0b;
		case '\\':
			return 0x5c;
		case '\'':
			return 0x27;
		case '"':
			return 0x22;
		case '/':
			return 0x2f;
		default:
			return -1;
	}
}

std::pair<std::string, seq_t> parse_string_literal_internal(const seq_t& s, const char delimiter){
	QUARK_ASSERT(!s.empty());
	QUARK_ASSERT(s.first1_char() == delimiter);

	auto pos = s.rest();
	std::string result = "";
	while(pos.empty() == false && pos.first() != std::string(1, delimiter)){
		//	Look for escape char
		if(pos.first1_char() == 0x5c){
			if(pos.size() < 2){
				throw_compiler_error_nopos("Incomplete escape sequence in string literal: \"" + result + "\"!");
			}
			else{
				const auto ch2 = pos.first(2)[1];
				const auto expanded_char = expand_one_char_escape(ch2);
				if(expanded_char == -1){
					throw_compiler_error_nopos("Unknown escape character \"" + std::string(1, ch2) + "\" in string literal: \"" + result + "\"!");
				}
				else{
					QUARK_ASSERT(expanded_char >= 0 && expanded_char < 256);
					result += std::string(1, static_cast<uint8_t>(expanded_char));
					pos = pos.rest(2);
				}
			}
		}
		else {
			result += pos.first();
			pos = pos.rest();
		}
	}
	if(pos.first() != std::string(1, delimiter)){
		throw_compiler_error_nopos("Incomplete string literal -- missing ending " + std::string(1, delimiter) + "-character in string literal: \"" + result + "\"!");
	}
	return { result, pos.rest() };
}



std::pair<std::string, seq_t> parse_string_literal(const seq_t& s){
	return parse_string_literal_internal(s, '\"');
}

QUARK_TEST("parser", "parse_string_literal()", "", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"("" xxx)")), std::pair<std::string, seq_t>("", seq_t(" xxx")));
}

QUARK_TEST("parser", "parse_string_literal()", "", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"("hello" xxx)")), std::pair<std::string, seq_t>("hello", seq_t(" xxx")));
}

QUARK_TEST("parser", "parse_string_literal()", "", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"(".5" xxx)")), std::pair<std::string, seq_t>(".5", seq_t(" xxx")));
}

QUARK_TEST("parser", "parse_string_literal()", "", ""){
	ut_verify(
		QUARK_POS,
		//	NOTICE that \" are Floyd-escapes in the Floyd source code.
		parse_string_literal(seq_t(R"___("hello \"Bob\"!" xxx)___")),
		std::pair<std::string, seq_t>(R"(hello "Bob"!)", seq_t(" xxx"))
	);
}

QUARK_TEST("parser", "parse_string_literal()", "Escape \0", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\0" xxx)___")), std::pair<std::string, seq_t>(std::string(1, '\0'), seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \t", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\t" xxx)___")), std::pair<std::string, seq_t>("\t", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \\", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\\" xxx)___")), std::pair<std::string, seq_t>("\\", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \n", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\n" xxx)___")), std::pair<std::string, seq_t>("\n", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \r", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\r" xxx)___")), std::pair<std::string, seq_t>("\r", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \"", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\"" xxx)___")), std::pair<std::string, seq_t>("\"", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \'", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\'" xxx)___")), std::pair<std::string, seq_t>("\'", seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \'", ""){
	ut_verify(QUARK_POS, parse_string_literal(seq_t(R"___("\/" xxx)___")), std::pair<std::string, seq_t>("/", seq_t(" xxx")));
}




std::pair<int64_t, seq_t> parse_character_literal(const seq_t& s){
	QUARK_ASSERT(!s.empty());
	QUARK_ASSERT(s.first1_char() == '\'');

	const auto a = parse_string_literal_internal(s, '\'');
	if(a.first.size() != 1){
		throw_compiler_error_nopos("Character literal must be a single character: \"" + a.first + "\"!");
	}

	const int64_t number = a.first[0];
	QUARK_ASSERT(number >= 0 && number < 256);

	return { number, a.second };
}

QUARK_TEST("parser", "parse_character_literal()", "", ""){
	ut_verify(QUARK_POS, parse_character_literal(seq_t(R"('A' xxx)")), std::pair<int64_t, seq_t>(65, seq_t(" xxx")));
}
QUARK_TEST("parser", "parse_string_literal()", "Escape \0", ""){
	ut_verify(QUARK_POS, parse_character_literal(seq_t(R"___('\0' xxx)___")), std::pair<int64_t, seq_t>(0x00, seq_t(" xxx")));
}

QUARK_TEST("parser", "parse_character_literal()", "", ""){
	try {
		parse_character_literal(seq_t(R"('AB' xxx)"));
		QUARK_ASSERT(false);
	}
	catch(const compiler_error& e){
		const auto w = e.what();
		ut_verify(QUARK_POS, w, R"___(Character literal must be a single character: "AB"!)___");
	}
}




// [0-9] and "."  => numeric constant.
//	Only works with positive numbers. Any sign is parsed first.
std::pair<value_t, seq_t> parse_decimal_literal(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(k_c99_number_chars.find(p.first()) != std::string::npos);

	const auto number_pos = read_while(p, k_c99_number_chars);
	if(number_pos.first.empty()){
		throw_compiler_error_nopos("Illegal numerical literal.");
	}

	//	If it contains a "." its a double, else an int.
	if(number_pos.first.find('.') != std::string::npos){
		const auto number = parse_double(number_pos.first);
		return { value_t::make_double(number), number_pos.second };
	}
	else{
//		const int64_t number = std::atoll(number_pos.first.c_str());
		char* end_ptr = nullptr;

		//	Important. Since strtoul() can legitimately return 0 or ULONG_MAX (ULLONG_MAX for strtoull()) on both success and failure, the calling program should set errno to 0 before the call, and then determine if an error occurred by checking whether errno has a nonzero value after the call.
		errno = 0;

		const unsigned long long int number = std::strtoull(number_pos.first.c_str(), &end_ptr, 10);
		const auto error = errno;
		if (error == ERANGE){
			const std::string number_str(number_pos.first.c_str(), (const char*)end_ptr);
			throw_compiler_error_nopos(std::string() + "Integer literal \"" + number_str + "\" larger than maxium allowed, which is " + std::to_string(k_floyd_uint64_max) + " aka 0x7fffffff'ffffffff - maxium for an unsigned 64-bit integer.");
		}

		return { value_t::make_int(number), number_pos.second };
	}
}

QUARK_TEST("parser", "std::strtoull()", "", ""){
	const char* start = "6669223372036854775807";
	char* end_ptr = nullptr;
	unsigned long long int number = std::strtoull(start, &end_ptr, 10);
	(void)number;
	const auto error = errno;
	QUARK_UT_VERIFY(error == ERANGE);
}


QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("0 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("1234 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 1234);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("0.5 xxx"));
	QUARK_UT_VERIFY(a.first.get_double_value() == 0.5f);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("17179869184 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 17179869184);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("9223372036854775807 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == k_floyd_int64_max);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("9223372036854775808 xxx"));
	QUARK_UT_VERIFY((uint64_t)a.first.get_int_value() == 9223372036854775808ull);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	const auto a = parse_decimal_literal(seq_t("18446744073709551615 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == k_floyd_uint64_max);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_decimal_literal()", "", ""){
	try {
		parse_decimal_literal(seq_t("618446744073709551615 xxx"));
	}
	catch(const std::runtime_error& e){
		QUARK_UT_VERIFY(std::string(e.what()) == "Integer literal \"618446744073709551615\" larger than maxium allowed, which is 18446744073709551615 aka 0x7fffffff'ffffffff - maxium for an unsigned 64-bit integer.");
	}
}



/*
	Makes sure any spans dividers sit at OK positions then removes them.

	If a divider exists at all, we expect the format to be like this:

	"a'bbbbbbbb"
	"aaaaaaaa'bbbbbbbb"
	"a'bbbbbbbb'cccccccc"

	A multiple of 8 characters is a span.
	There must be a divider between each span.
	None at the end.
	First span can be shorted but not zero characters
*/
std::string strip_optional_dividers(const std::string& s){
	std::vector<std::string> spans;
	spans.push_back("");
	for(auto pos = 0 ; pos < s.size() ; pos++){
		const auto ch = s[pos];
		if(ch == k_literal_divider_char){
			spans.push_back("");
		}
		else{
			spans.back().push_back(ch);
		}
	}

	if(spans.size() == 1){
		return s;
	}
	else{
		for(int i = 1 ; i < spans.size() ; i++){
			if(spans[i].size() != 8){
				throw_compiler_error_nopos(std::string() + "Unexpected \''\' divider in literal .");
			}
		}
		if(spans.front().size() > 0 && spans.front().size() <= 8){
		}
		else{
			throw_compiler_error_nopos(std::string() + "Unexpected \''\' divider in literal .");
		}
	}
	std::string result;
	for(const auto& e: spans){
		result.append(e);
	}
	return result;
}

QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	QUARK_UT_VERIFY(strip_optional_dividers("") == "");
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	QUARK_UT_VERIFY(strip_optional_dividers("aaa") == "aaa");
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	QUARK_UT_VERIFY(strip_optional_dividers("aa'bbbbbbbb") == "aabbbbbbbb");
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	QUARK_UT_VERIFY(strip_optional_dividers("aa'bbbbbbbb'cccccccc") == "aabbbbbbbbcccccccc");
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	QUARK_UT_VERIFY(strip_optional_dividers("aaaaaaaa'bbbbbbbb") == "aaaaaaaabbbbbbbb");
}


QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("aaaaaaaa'");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("a'");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("a'bbbb");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("a'bbbb'cccccccc");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("a'bbbbbbbbbbbbb");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("'aaaaaaaa");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "strip_optional_dividers()", "", ""){
	try {
		strip_optional_dividers("'aaaaa");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}


/*
	0b0
	0b1
	0b00000000'11111111
	0b0'11111111
*/

std::pair<value_t, seq_t> parse_binary_literal(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	const auto pos = read_required(p, "0b");
	const auto number_pos = read_while(pos, k_c99_number_chars + k_c99_identifier_chars + std::string(1, k_literal_divider_char));
	if(number_pos.first.empty()){
		throw_compiler_error_nopos("Illegal binary literal.");
	}

	const auto s = strip_optional_dividers(number_pos.first);
	if(s.size() > 64){
		throw_compiler_error_nopos(std::string() + "Binary literal bigger than supported 64 bits.");
	}

	uint64_t acc = 0;
	for(int i = 0 ; i < s.size() ; i++){
		acc = acc << 1;

		const auto ch = s[i];
		if(ch == '1'){
			acc = acc | 1;
		}
		else if(ch == '0'){
		}
		else if(ch == k_literal_divider_char){
		}
		else{
			throw_compiler_error_nopos(std::string() + "Illegal character \'" +  ch + "\' in binary literal.");
		}
	}
	return { value_t::make_int(acc), number_pos.second };
}

QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	try {
		const auto a = parse_binary_literal(seq_t("0b xxx"));
		QUARK_ASSERT(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b0 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b1 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 1);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b00000000 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b00000001 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 1);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b10000000 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 128);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b11111111 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 255);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b1000000000000000000000000000000000000000000000000000000000000001 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0b1000000000000000000000000000000000000000000000000000000000000001);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	try {
		const auto a = parse_binary_literal(seq_t("0b000011112 xxx"));
		QUARK_ASSERT(false);
	}
	catch(...){
	}
}
QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	try {
		const auto a = parse_binary_literal(seq_t("0b00001111a xxx"));
		QUARK_ASSERT(false);
	}
	catch(...){
	}
}


QUARK_TEST("parser", "parse_binary_literal()", "range", ""){
	//											 --------XXXXXXXX--------XXXXXXXX--------XXXXXXXX--------XXXXXXXX
	const auto a = parse_binary_literal(seq_t("0b0111111111111111111111111111111111111111111111111111111111111111 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0b0111111111111111111111111111111111111111111111111111111111111111);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "range", ""){
	//											 --------XXXXXXXX--------XXXXXXXX--------XXXXXXXX--------XXXXXXXX
	const auto a = parse_binary_literal(seq_t("0b1000000000000000000000000000000000000000000000000000000000000000 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0b1000000000000000000000000000000000000000000000000000000000000000);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_binary_literal()", "range", ""){
	//											 --------XXXXXXXX--------XXXXXXXX--------XXXXXXXX--------XXXXXXXX
	const auto a = parse_binary_literal(seq_t("0b1111111111111111111111111111111111111111111111111111111111111111 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0b1111111111111111111111111111111111111111111111111111111111111111);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}



QUARK_TEST("parser", "parse_binary_literal()", "", ""){
											//   ----XXXX----XXXX----XXXX----XXXX----XXXX----XXXX----XXXX----XXXX
	const auto a = parse_binary_literal(seq_t("0b0001001000110100010101100111100010011010101111001101111011110001 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x123456789abcdef1);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b00010010'00110100'01010110'01111000'10011010'10111100'11011110'11110001 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x123456789abcdef1);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	const auto a = parse_binary_literal(seq_t("0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x8000000000000001);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	try {
		const auto a = parse_binary_literal(seq_t("0b1'10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001 xxx"));
		QUARK_ASSERT(false);
	}
	catch(...){
	}
}





std::pair<value_t, seq_t> parse_hexadecimal_literal(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	const auto pos = read_required(p, "0x");
	const auto number_pos = read_while(pos, k_c99_number_chars + k_c99_identifier_chars + k_hex_chars + std::string(1, k_literal_divider_char));
	if(number_pos.first.empty()){
		throw_compiler_error_nopos("Illegal hexadecimal literal.");
	}

	const auto s = strip_optional_dividers(number_pos.first);
	if(s.size() > 16){
		throw_compiler_error_nopos(std::string() + "Hexadecimal literal bigger than supported 64 bits.");
	}

	uint64_t acc = 0;
	for(int i = 0 ; i < s.size() ; i++){
		acc = acc << 4;

		const auto ch0 = s[i];
		const char ch = static_cast<char>(std::tolower(ch0));
		const auto value = k_hex_chars.find(ch);
		if(value != std::string::npos){
			acc = acc | value;
		}
		else{
			throw_compiler_error_nopos(std::string() + "Illegal character \'" +  ch + "\' in binary literal.");
		}
	}
	return { value_t::make_int(acc), number_pos.second };
}

QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0x00 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x00);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0x1234 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x1234);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0xabcdef0123456789 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0xabcdef0123456789);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}


QUARK_TEST("parser", "parse_hexadecimal_literal()", "range", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0x7fffffffffffffff xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x7fffffffffffffff);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_hexadecimal_literal()", "range", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0x8000000000000000 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x8000000000000000);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_hexadecimal_literal()", "range", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0xffffffffffffffff xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0xffffffffffffffff);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}


QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0xabcdef01'23456789 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0xabcdef0123456789);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0xabcdef01'23456789 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0xabcdef0123456789);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}
QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0x01'23456789 xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0x0123456789);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_hexadecimal_literal()", "", ""){
	const auto a = parse_hexadecimal_literal(seq_t("0xABCD xxx"));
	QUARK_UT_VERIFY(a.first.get_int_value() == 0xabcd);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_TEST("parser", "parse_binary_literal()", "", ""){
	try {
		const auto a = parse_hexadecimal_literal(seq_t("0xf'abcdef01'23456789 xxx"));
		QUARK_ASSERT(false);
	}
	catch(...){
	}
}




/*
	Constant literal or identifier.
		3
		3.0
		0b00000011
		0xffdeadbeefaa

		'T'

		"three"
		true
		false
		hello2
		x
*/
std::pair<json_t, seq_t> parse_terminal(const seq_t& p0) {
	QUARK_ASSERT(p0.check_invariant());

	const auto p = skip_whitespace(p0);

	//	String literal?
	if(p.first1() == "\""){
		const auto value_pos = parse_string_literal(p);
		const auto result = parser__make_literal(value_t::make_string(value_pos.first));
		return { result, value_pos.second };
	}
	else if(p.first1() == "\'"){
		const auto value_pos = parse_character_literal(p);
		const auto result = parser__make_literal(value_t::make_int(value_pos.first));
		return { result, value_pos.second };
	}

	else if(is_first(p, "0b")){
		const auto value_p = parse_binary_literal(p);
		const auto result = parser__make_literal(value_p.first);
		return { result, value_p.second };
	}
	else if(is_first(p, "0x")){
		const auto value_p = parse_hexadecimal_literal(p);
		const auto result = parser__make_literal(value_p.first);
		return { result, value_p.second };
	}

	//	Number constant?
	// [0-9] and "."  => numeric constant.
	else if(k_c99_number_chars.find(p.first1()) != std::string::npos){
		const auto value_p = parse_decimal_literal(p);
		const auto result = parser__make_literal(value_p.first);
		return { result, value_p.second };
	}

	else if(if_first(p, keyword_t::k_true).first){
		const auto result = parser__make_literal(value_t::make_bool(true));
		return { result, if_first(p, keyword_t::k_true).second };
	}

	else if(if_first(p, keyword_t::k_false).first){
		const auto result = parser__make_literal(value_t::make_bool(false));
		return { result, if_first(p, keyword_t::k_false).second };
	}

	//	Identifier?
	{
		const auto identifier_s = read_while(p, k_c99_identifier_chars);
		if(!identifier_s.first.empty()){
			const auto result = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_load, { identifier_s.first } );
			return { result, identifier_s.second };
		}
	}

	throw_compiler_error_nopos("Expected constant or identifier.");
}

void ut_verify_terminal(const std::string& expression, const std::string& expected_value, const std::string& expected_seq){
	const auto result = parse_terminal(seq_t(expression));
	const std::string json_s = expr_to_string(result.first);
	if(json_s == expected_value && result.second.get_s() == expected_seq){
	}
	else{
		QUARK_TRACE_SS("input:" << expression);
		QUARK_TRACE_SS("expect:" << expected_value);
		QUARK_TRACE_SS("result:" << json_s);
		fail_test(QUARK_POS);
	}
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"123 xxx",
		R"(["k", 123, "int"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"123.5 xxx",
		R"(["k", 123.5, "double"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"0.0 xxx",
		R"(["k", 0, "double"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"hello xxx",
		R"(["@", "hello"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		R"("world!" xxx)",
		R"(["k", "world!", "string"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		R"("" xxx)",
		R"(["k", "", "string"])",
		" xxx"
	);
}


QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"true xxx",
		R"(["k", true, "bool"])",
		" xxx"
	);
}
QUARK_TEST("parser", "parse_terminal()", "identifier", ""){
	ut_verify_terminal(
		"false xxx",
		R"(["k", false, "bool"])",
		" xxx"
	);
}


/*

	lhs operation EXPR +++
	lhs OPERATION EXPRESSION ...
*/
std::pair<json_t, seq_t> parse_optional_operation_rightward(const seq_t& p0, const json_t& lhs, const eoperator_precedence precedence){
	QUARK_ASSERT(p0.check_invariant());

	const auto p = skip_whitespace(p0);
	if(p.empty()){
		return { lhs, p0 };
	}
	else {
		const auto op1 = p.first();
		const auto op2 = p.first(2);

		//	Detect end of chain. Notice that we leave the ")" or "]".
		if(op1 == ")" && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p0 };
		}
		else if(op1 == "]" && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p0 };
		}

		//	lhs OPERATOR rhs
		else {
			//	Function call
			//	EXPRESSION (EXPRESSION +, EXPRESSION)
			if(op1 == "(" && precedence > eoperator_precedence::k_function_call){
				const auto a_pos = parse_bounded_list(p, "(", ")");

				if(a_pos.first._has_keys){
					throw_compiler_error_nopos("Cannot name arguments in function call!");
				}

				const auto values = get_values(a_pos.first);
				const auto call = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_call, { lhs, json_t::make_array(values) } );

				return parse_optional_operation_rightward(a_pos.second, call, precedence);
			}

			//	Member access
			//	EXPRESSION "." EXPRESSION +
			else if(op1 == "."  && precedence > eoperator_precedence::k_member_access){
				const auto identifier_s = read_while(skip_whitespace(p.rest()), k_c99_identifier_chars);
				if(identifier_s.first.empty()){
					throw_compiler_error_nopos("Expected ')'");
				}
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_resolve_member, { lhs, identifier_s.first } );

				return parse_optional_operation_rightward(identifier_s.second, value2, precedence);
			}

			//	Lookup / subscription
			//	EXPRESSION "[" EXPRESSION "]" +
			else if(op1 == "["  && precedence > eoperator_precedence::k_lookup){
				const auto p2 = skip_whitespace(p.rest());
				const auto key = parse_expression_deep(p2, eoperator_precedence::k_super_weak);
				const auto result = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_lookup_element, { lhs, key.first } );
				const auto p3 = skip_whitespace(key.second);

				// Closing "]".
				if(p3.first() != "]"){
					throw_compiler_error_nopos("Expected closing \"]\"");
				}
				return parse_optional_operation_rightward(p3.rest(), result, precedence);
			}

			//	EXPRESSION "+" EXPRESSION
			else if(op1 == "+"  && precedence > eoperator_precedence::k_add_sub){
				const auto rhs = parse_expression_deep(p.rest(), eoperator_precedence::k_add_sub);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_add, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "-" EXPRESSION
			else if(op1 == "-" && precedence > eoperator_precedence::k_add_sub){
				const auto rhs = parse_expression_deep(p.rest(), eoperator_precedence::k_add_sub);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_subtract, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "*" EXPRESSION
			else if(op1 == "*" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_deep(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_multiply, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}
			//	EXPRESSION "/" EXPRESSION
			else if(op1 == "/" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_deep(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_divide, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "%" EXPRESSION
			else if(op1 == "%" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
				const auto rhs = parse_expression_deep(p.rest(), eoperator_precedence::k_multiply_divider_remainder);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_remainder, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	EXPRESSION "?" EXPRESSION ":" EXPRESSION
			else if(op1 == "?" && precedence > eoperator_precedence::k_comparison_operator) {
				const auto true_expr_p = parse_expression_deep(p.rest(), eoperator_precedence::k_comparison_operator);

				const auto pos2 = skip_whitespace(true_expr_p.second);
				const auto colon = pos2.first();
				if(colon != ":"){
					throw_compiler_error_nopos("Expected \":\"");
				}

				const auto false_expr_p = parse_expression_deep(pos2.rest(), precedence);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_conditional_operator, { lhs, true_expr_p.first, false_expr_p.first} );
				return parse_optional_operation_rightward(false_expr_p.second, value2, precedence);
			}


			//	EXPRESSION "==" EXPRESSION
			else if(op2 == "==" && precedence > eoperator_precedence::k_equal__not_equal){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_equal__not_equal);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_logical_equal, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}
			//	EXPRESSION "!=" EXPRESSION
			else if(op2 == "!=" && precedence > eoperator_precedence::k_equal__not_equal){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_equal__not_equal);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_logical_nonequal, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	!!! Check for "<=" before we check for "<".
			//	EXPRESSION "<=" EXPRESSION
			else if(op2 == "<=" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_comparison_smaller_or_equal, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "<" EXPRESSION
			else if(op1 == "<" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_comparison_smaller, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	!!! Check for ">=" before we check for ">".
			//	EXPRESSION ">=" EXPRESSION
			else if(op2 == ">=" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_comparison_larger_or_equal, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION ">" EXPRESSION
			else if(op1 == ">" && precedence > eoperator_precedence::k_larger_smaller){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_larger_smaller);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_comparison_larger, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}


			//	EXPRESSION "&&" EXPRESSION
			else if(op2 == "&&" && precedence > eoperator_precedence::k_logical_and){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_logical_and);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_logical_and, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION "||" EXPRESSION
			else if(op2 == "||" && precedence > eoperator_precedence::k_logical_or){
				const auto rhs = parse_expression_deep(p.rest(2), eoperator_precedence::k_logical_or);
				const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_logical_or, { lhs, rhs.first } );
				return parse_optional_operation_rightward(rhs.second, value2, precedence);
			}

			//	EXPRESSION
			//	Nope, no operation -- just use the lhs expression on its own.
			else{
				return { lhs, p0 };
			}
		}
	}
}


static bool is_identifier(const seq_t& p, const std::string identifier){
	const auto pos = read_identifier(p);
	return pos.first == identifier;
}


/*
	Atom = standalone expression, like a constant, a function call.
	It can be composed of subexpressions

	It may then be chained rightwards using operations like "+" etc.

	Examples:
		123
		-123
		--+-123
		(123 + 123 * x + f(y*3))
		[ 1, 2, calc_exp(3) ]
*/
std::pair<json_t, seq_t> parse_lhs_atom(const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

	types_t temp;

    const auto p2 = skip_whitespace(p);
	if(p2.empty()){
		throw_compiler_error_nopos("Unexpected end of program.");
	}

	const char ch1 = p2.first1_char();

	//	Negate? "-xxx"
	if(ch1 == '-'){
		const auto a = parse_expression_deep(p2.rest1(), eoperator_precedence::k_super_strong);
		const auto value2 = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_arithmetic_unary_minus, { a.first } );
		return { value2, a.second };
	}
	else if(ch1 == '+'){
		const auto a = parse_expression_deep(p2.rest1(), eoperator_precedence::k_super_strong);
		return { a.first, a.second };
	}
	//	Expression within parantheses?
	//	(EXPRESSION)xxx"
	else if(ch1 == '('){
		const auto a = parse_expression_deep(p2.rest1(), eoperator_precedence::k_super_weak);
		const auto p3 = skip_whitespace(a.second);
		if (p3.first() != ")"){
			throw_compiler_error(location_t(p2.pos()), "Expected ')' character.");
		}
		return { a.first, p3.rest() };
	}

	else if(is_first(p2, keyword_t::k_struct)){
		throw_compiler_error(location_t(p2.pos()), "No support for struct definition expressions!");
	}

	/*
		Vector definition: "[" EXPRESSION "," EXPRESSION... "]"
		 	[ 1, 2, 3 ]
		 	[ calc_pi(), 2.8, calc_pi * 2.0]
	
		OR

		Dict definition: "{" EXPRESSION ":" EXPRESSION, EXPRESSION ":" EXPRESSION, ... "}"
			{ "one": 1, "two": 2, "three": 3 }
	*/
	else if(ch1 == '['){
		const auto a = parse_bounded_list(p2, "[", "]");
		if(a.first._has_keys){
			throw_compiler_error(location_t(p2.pos()), "Illegal vector, use {} to make a dictionary!");
		}
		else{
			const auto element_type2 = type_to_json(temp, make_vector(temp, make_undefined()));
			const auto result = make_parser_node(
				floyd::k_no_location,
				parse_tree_expression_opcode_t::k_value_constructor,
				{ element_type2, get_values(a.first) }
			);
			return {result, a.second };
		}
	}
	else if(ch1 == '{'){
		const auto a = parse_bounded_list(p2, "{", "}");
		if(a.first._elements.size() > 0 && a.first._has_keys == false){
			throw_compiler_error(location_t(p2.pos()), "Dictionary needs keys!");
		}
		std::vector<json_t> flat_dict;
		for(const auto& b: a.first._elements){
			if(b._key == nullptr){
				throw_compiler_error(location_t(p2.pos()), "Dictionary definition misses element key(s)!");
			}
			flat_dict.push_back(*b._key);
			flat_dict.push_back(b._value);
		}

		const auto element_type2 = type_to_json(temp, make_dict(temp, make_undefined()));
		const auto result = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_value_constructor, { element_type2, json_t::make_array(flat_dict) } );
		return {result, a.second };
	}

	else if(is_identifier(p2, keyword_t::k_benchmark)){
		const auto pos = read_identifier(p2);
		const auto block_pos = parse_statement_body(pos.second);
		const auto result = make_parser_node(floyd::k_no_location, parse_tree_expression_opcode_t::k_benchmark, { block_pos.parse_tree } );
		return { result, block_pos.pos };
	}

	//	Single constant number, string literal, function call, variable access, lookup or member access. Can be a chain.
	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = parse_terminal(p2);
		return a;
	}
}

QUARK_TEST("parser", "parse_lhs_atom()", "", ""){
	const auto a = parse_lhs_atom(seq_t("3"));
	QUARK_UT_VERIFY(a.first == parser__make_literal(value_t::make_int(3)));
}

QUARK_TEST("parser", "parse_lhs_atom()", "", ""){
	types_t temp;
	const auto a = parse_lhs_atom(seq_t("[3]"));
	QUARK_UT_VERIFY(a.first == make_parser_node(
		floyd::k_no_location,
		parse_tree_expression_opcode_t::k_value_constructor,
		{
			type_to_json(temp, make_vector(temp, make_undefined())),
			std::vector<json_t>{ parser__make_literal(value_t::make_int(3)) }
		}
	));
}



void ut_verify__parse_expression(const quark::call_context_t& context, const std::string& input, const std::string& expected_json_s, const std::string& expected_rest){
	const auto result = parse_expression(seq_t(input));
	const std::string result_json_s = expr_to_string(result.first);

	//	Generate the expted JSON using json_to_compact_string() so manual-input differences in expected_json don't matter for result being OK.
	const auto expected_json = parse_json(seq_t(expected_json_s)).first;
	if(result.first == expected_json && result.second.get_s() == expected_rest){
	}
	else{
		const std::string expected_json2_s = json_to_compact_string(expected_json);
		ut_verify(context, result.first, expected_json);
		ut_verify(context, result.second.str(), expected_rest);
		fail_test(context);
	}
}



//////////////////////////////////			EMPTY

QUARK_TEST("parser", "parse_expression()", "", ""){
	try{
		parse_expression(seq_t(""));
		fail_test(QUARK_POS);
	}
	catch(const std::runtime_error& e){
		ut_verify(QUARK_POS, e.what(), "Unexpected end of program.");
	}
}

//////////////////////////////////			CONSTANTS

QUARK_TEST("parser", "parse_expression()", "", ""){
	ut_verify__parse_expression(QUARK_POS, "0", R"(["k", 0, "int"])", "");
}
QUARK_TEST("parser", "parse_expression()", "", ""){
	ut_verify__parse_expression(QUARK_POS, "0 xxx", R"(["k", 0, "int"])", " xxx");
}
QUARK_TEST("parser", "parse_expression()", "", ""){
//???
//	ut_verify__expression(parse_expression(seq_t("1234567890")), "[\"k\", 1234567890, \"int\"]", "");
}
QUARK_TEST("parser", "parse_expression()", "", ""){
	ut_verify__parse_expression(QUARK_POS, R"___("hello, world!")___", R"(["k", "hello, world!", "string"])", "");
}


//////////////////////////////////			ARITHMETICS

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(QUARK_POS, "10 + 4", R"(["+", ["k", 10, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 + 2 + 3 + 4",
		R"(["+", ["+", ["+", ["k", 1, "int"], ["k", 2, "int"]], ["k", 3, "int"]], ["k", 4, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(QUARK_POS, "10 * 4", R"(["*", ["k", 10, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"10 * 4 * 3",
		R"(["*", ["*", ["k", 10, "int"], ["k", 4, "int"]], ["k", 3, "int"]])",
		""
	);
}
QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(QUARK_POS, "40 / 4", R"(["/", ["k", 40, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"40 / 5 / 2",
		R"(["/", ["/", ["k", 40, "int"], ["k", 5, "int"]], ["k", 2, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(QUARK_POS, "41 % 5", R"(["%", ["k", 41, "int"], ["k", 5, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"413 % 50 % 10",
		R"(["%", ["%", ["k", 413, "int"], ["k", 50, "int"]], ["k", 10, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 + 3 * 2 + 100",
		R"(["+", ["+", ["k", 1, "int"], ["*", ["k", 3, "int"], ["k", 2, "int"]]], ["k", 100, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 + 8 + 7 + 2 * 3 + 4 * 5 + 6",
		R"(["+", ["+", ["+", ["+", ["+", ["k", 1, "int"], ["k", 8, "int"]], ["k", 7, "int"]], ["*", ["k", 2, "int"], ["k", 3, "int"]]], ["*", ["k", 4, "int"], ["k", 5, "int"]]], ["k", 6, "int"]])",
		""
	);
}


//////////////////////////////////			PARANTHESES

QUARK_TEST("parser","parse_expression()", "parantheses", ""){
	ut_verify__parse_expression(QUARK_POS, "(3)", R"(["k", 3, "int"])", "");
}
QUARK_TEST("parser", "parse_expression()", "parantheses", ""){
	ut_verify__parse_expression(QUARK_POS, "(3 * 8)", R"(["*", ["k", 3, "int"], ["k", 8, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "parantheses", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"(3 * 2 + (8 * 2)) - (((1))) * 2",
		R"(["-", ["+", ["*", ["k", 3, "int"], ["k", 2, "int"]], ["*", ["k", 8, "int"], ["k", 2, "int"]]], ["*", ["k", 1, "int"], ["k", 2, "int"]]])",
		""
	);
}


//////////////////////////////////			VECTORS


QUARK_TEST("parser", "parse_expression()", "vector", ""){
	ut_verify__parse_expression(QUARK_POS, "[]", R"(["value-constructor", ["vector", "undef"], []])", "");
}

QUARK_TEST("parser", "parse_expression()", "vector", ""){
	ut_verify__parse_expression(QUARK_POS, "[1,2,3]", R"(["value-constructor", ["vector", "undef"], [["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]]])", "");
}



//////////////////////////////////			DICTIONARIES

QUARK_TEST("parser", "parse_expression()", "dict", ""){
	ut_verify__parse_expression(QUARK_POS, "{:}", R"(["value-constructor", ["dict", "undef"], []])", "");
}
QUARK_TEST("parser", "parse_expression()", "dict", ""){
	ut_verify__parse_expression(QUARK_POS, "{}", R"(["value-constructor", ["dict", "undef"], []])", "");
}

QUARK_TEST("parser", "parse_expression()", "dict definition", ""){
	ut_verify__parse_expression(QUARK_POS,
		R"({"one": 1, "two": 2, "three": 3})",
		R"(["value-constructor", ["dict", "undef"], [["k", "one", "string"], ["k", 1, "int"], ["k", "two", "string"], ["k", 2, "int"], ["k", "three", "string"], ["k", 3, "int"]]])", ""
	);
}


//////////////////////////////////			BENCHMARK


QUARK_TEST("parser", "parse_expression()", "benchmark", ""){
	ut_verify__parse_expression(QUARK_POS, "benchmark { let a = 10 }", R"___(	["benchmark", [[12, "init-local", "undef", "a", ["k", 10, "int"]]]]	)___", "");
}




//////////////////////////////////			NEG

QUARK_TEST("parser", "parse_expression()", "unary minus", ""){
	ut_verify__parse_expression(QUARK_POS, "-2 xxx", R"(["unary-minus", ["k", 2, "int"]])", " xxx");
}

QUARK_TEST("parser", "parse_expression()", "unary minus", ""){
	ut_verify__parse_expression(QUARK_POS,
		"-(3)",
		R"(["unary-minus", ["k", 3, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "unary minus", ""){
	ut_verify__parse_expression(QUARK_POS,
		"2---2 xxx",
		R"(["-", ["k", 2, "int"], ["unary-minus", ["unary-minus", ["k", 2, "int"]]]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "unary minus", ""){
	ut_verify__parse_expression(QUARK_POS,
		"2-+-2 xxx",
		R"(["-", ["k", 2, "int"], ["unary-minus", ["k", 2, "int"]]])",
		" xxx"
	);
}


//////////////////////////////////			LOGICAL EQUALITY


QUARK_TEST("parser", "parse_expression()", "<=", ""){
	ut_verify__parse_expression(QUARK_POS, "3 <= 4", R"(["<=", ["k", 3, "int"], ["k", 4, "int"]])", "");
}
QUARK_TEST("parser", "parse_expression()", "<", ""){
	ut_verify__parse_expression(QUARK_POS, "3 < 4", R"(["<", ["k", 3, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", ">=", ""){
	ut_verify__parse_expression(QUARK_POS, "3 >= 4", R"([">=", ["k", 3, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", ">", ""){
	ut_verify__parse_expression(QUARK_POS, "3 > 4", R"([">", ["k", 3, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "==", ""){
	ut_verify__parse_expression(QUARK_POS, "3 == 4", R"(["==", ["k", 3, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "==", ""){
	ut_verify__parse_expression(QUARK_POS, "1==3", R"(["==", ["k", 1, "int"], ["k", 3, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "!=", ""){
	ut_verify__parse_expression(QUARK_POS, "3 != 4", R"(["!=", ["k", 3, "int"], ["k", 4, "int"]])", "");
}


QUARK_TEST("parser", "parse_expression()", "&&", ""){
	ut_verify__parse_expression(QUARK_POS, "3 && 4", R"(["&&", ["k", 3, "int"], ["k", 4, "int"]])", "");
}

QUARK_TEST("parser", "parse_expression()", "&&", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"3 && 4 && 5",
		R"(["&&", ["&&", ["k", 3, "int"], ["k", 4, "int"]], ["k", 5, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "&&", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 * 1 && 0 * 1",
		R"(["&&", ["*", ["k", 1, "int"], ["k", 1, "int"]], ["*", ["k", 0, "int"], ["k", 1, "int"]]])",
		""
	);
}


QUARK_TEST("parser", "parse_expression()", "||", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"3 || 4",
		R"(["||", ["k", 3, "int"], ["k", 4, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "||", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"3 || 4 || 5",
		R"(["||", ["||", ["k", 3, "int"], ["k", 4, "int"]], ["k", 5, "int"]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "||", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 * 1 || 0 * 1",
		R"(["||", ["*", ["k", 1, "int"], ["k", 1, "int"]], ["*", ["k", 0, "int"], ["k", 1, "int"]]])",
		""
	);
}


//??? Check combos of || and &&


//////////////////////////////////			IDENTIFIERS


QUARK_TEST("parser", "parse_expression()", "identifier", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"hello xxx",
		R"(["@", "hello"])",
		" xxx"
	);
}


//////////////////////////////////			COMPARISON OPERATOR

QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 ? 2 : 3 xxx",
		R"(["?:", ["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1==3 ? 4 : 6 xxx",
		R"(["?:", ["==", ["k", 1, "int"], ["k", 3, "int"]], ["k", 4, "int"], ["k", 6, "int"]])",
		" xxx"
	);
}
QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 ? \"true!!!\" : \"false!!!\" xxx",
		R"(["?:", ["k", 1, "int"], ["k", "true!!!", "string"], ["k", "false!!!", "string"]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 + 2 ? 3 + 4 : 5 + 6 xxx",
		R"(["?:", ["+", ["k", 1, "int"], ["k", 2, "int"]], ["+", ["k", 3, "int"], ["k", 4, "int"]], ["+", ["k", 5, "int"], ["k", 6, "int"]]])",
		" xxx"
	);
}

//??? Add more test to see precedence works as it should!


QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"input_flag ? \"123\" : \"456\"",
		R"(["?:", ["@", "input_flag"], ["k", "123", "string"], ["k", "456", "string"]])",
		""
	);
}


QUARK_TEST("parser", "parse_expression()", "?:", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"input_flag ? 100 + 10 * 2 : 1000 - 3 * 4",
		R"(["?:", ["@", "input_flag"], ["+", ["k", 100, "int"], ["*", ["k", 10, "int"], ["k", 2, "int"]]], ["-", ["k", 1000, "int"], ["*", ["k", 3, "int"], ["k", 4, "int"]]]])",
		""
	);
}


//////////////////////////////////			FUNCTION CALLS

QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f() xxx",
		R"(["call", ["@", "f"], []])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "function call, one simple arg", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f(3)",
		R"(["call", ["@", "f"], [["k", 3, "int"]]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "call with expression-arg", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f(x+10) xxx",
		R"(["call", ["@", "f"], [["+", ["@", "x"], ["k", 10, "int"]]]])",
		" xxx"
	);
}
QUARK_TEST("parser", "parse_expression()", "call with expression-arg", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f(1,2) xxx",
		R"(["call", ["@", "f"], [["k", 1, "int"], ["k", 2, "int"]]])",
		" xxx"
	);
}
QUARK_TEST("parser", "parse_expression()", "call with expression-arg -- whitespace", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f ( 1 , 2 ) xxx",
		R"(["call", ["@", "f"], [["k", 1, "int"], ["k", 2, "int"]]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "function call with expression-args", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f(3 + 4, 4 * g(1000 + 2345), \"hello\", 5)",
		R"(["call", ["@", "f"], [["+", ["k", 3, "int"], ["k", 4, "int"]], ["*", ["k", 4, "int"], ["call", ["@", "g"], [["+", ["k", 1000, "int"], ["k", 2345, "int"]]]]], ["k", "hello", "string"], ["k", 5, "int"]]])",
		""
	);
}



QUARK_TEST("parser", "parse_expression()", "function call, expression argument", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"1 == 2)",
		R"(["==", ["k", 1, "int"], ["k", 2, "int"]])",
		")"
	);
}


QUARK_TEST("parser", "parse_expression()", "function call, expression argument", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f(1 == 2)",
		R"(["call", ["@", "f"], [["==", ["k", 1, "int"], ["k", 2, "int"]]]])",
		""
	);
}


QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"((3))))",
		R"(["k", 3, "int"])",
		"))"
	);
}
QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"print((3))))",
		R"(["call", ["@", "print"], [["k", 3, "int"]]])",
		"))"
	);
}

QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"print(1 < 2)",
		R"(["call", ["@", "print"], [["<", ["k", 1, "int"], ["k", 2, "int"]]]])",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"print(1 < color(1, 2, 3))",
		R"(["call", ["@", "print"], [["<", ["k", 1, "int"], ["call", ["@", "color"], [["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]]]]]])",
		""
	);
}


QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"print(color(1, 2, 3) < color(1, 2, 3))",
		R"(
			[
				"call",
				["@", "print"],
				[
					[
						"<",
						["call", ["@", "color"], [["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]]],
						["call", ["@", "color"], [["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]]]
					]
				]
			]
		)",
		""
	);
}

QUARK_TEST("parser", "parse_expression()", "function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"print(color(1, 2, 3) == file(404)) xxx",
		R"___(["call", ["@", "print"], [["==", ["call", ["@", "color"], [["k", 1, "int"], ["k", 2, "int"], ["k", 3, "int"]]], ["call", ["@", "file"], [["k", 404, "int"]]]]]])___",
		" xxx"
	);
}


//////////////////////////////////			MEMBER ACCESS

QUARK_TEST("parser", "parse_expression()", "struct member access", ""){
	ut_verify__parse_expression(QUARK_POS, "hello.kitty xxx", R"(["->", ["@", "hello"], "kitty"])", " xxx");
}

QUARK_TEST("parser", "parse_expression()", "struct member access", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"hello.kitty.cat xxx",
		R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "struct member access -- whitespace", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"hello . kitty . cat xxx",
		R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
		" xxx"
	);
}





//////////////////////////////////			LOOKUP

QUARK_TEST("parser", "parse_expression()", "lookup with int", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"hello[10] xxx",
		R"(["[]", ["@", "hello"], ["k", 10, "int"]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "lookup with string", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		R"(hello["troll"] xxx)",
		R"(["[]", ["@", "hello"], ["k", "troll", "string"]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "lookup with string -- whitespace", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		R"(hello [ "troll" ] xxx)",
		R"(["[]", ["@", "hello"], ["k", "troll", "string"]])",
		" xxx"
	);
}


//////////////////////////////////			COMBOS


QUARK_TEST("parser", "parse_expression()", "combo - function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"poke.mon.f() xxx",
		R"(["call", ["->", ["->", ["@", "poke"], "mon"], "f"], []])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "combo - function call", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		"f().g() xxx",
		R"(["call", ["->", ["call", ["@", "f"], []], "g"], []])",
		" xxx"
	);
}


QUARK_TEST("parser", "parse_expression()", "complex chain", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		R"(hello["troll"].kitty[10].cat xxx)",
		R"(["->", ["[]", ["->", ["[]", ["@", "hello"], ["k", "troll", "string"]], "kitty"], ["k", 10, "int"]], "cat"])",
		" xxx"
	);
}


QUARK_TEST("parser", "parse_expression()", "chain", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		R"(poke.mon.v[10].a.b.c["three"] xxx)",
		R"(["[]", ["->", ["->", ["->", ["[]", ["->", ["->", ["@", "poke"], "mon"], "v"], ["k", 10, "int"]], "a"], "b"], "c"], ["k", "three", "string"]])",
		" xxx"
	);
}

QUARK_TEST("parser", "parse_expression()", "combo arithmetics", ""){
	ut_verify__parse_expression(
		QUARK_POS,
		" 5 - 2 * ( 3 ) xxx",
		R"(["-", ["k", 5, "int"], ["*", ["k", 2, "int"], ["k", 3, "int"]]])",
		" xxx"
	);
}


//////////////////////////////////			EXPRESSION ERRORS


void test__parse_expression__throw(const std::string& expression, const std::string& exception_message){
	try{
		const auto result = parse_expression(seq_t(expression));
		fail_test(QUARK_POS);
	}
	catch(const std::runtime_error& e){
		const std::string es(e.what());
		if(!exception_message.empty()){
			ut_verify(QUARK_POS, es, exception_message);
		}
	}
}


QUARK_TEST("parser", "parse_expression()", "Parentheses error", ""){
	test__parse_expression__throw("5*((1+3)*2+1", "");
}

QUARK_TEST("parser", "parse_expression()", "Parentheses error", ""){
//	test__parse_expression__throw("5*((1+3)*2)+1)", "");
}

QUARK_TEST("parser", "parse_expression()", "Repeated operators (wrong)", ""){
	test__parse_expression__throw("5*/2", "");
}

QUARK_TEST("parser", "parse_expression()", "Wrong position of an operator", ""){
	test__parse_expression__throw("*2", "");
}

QUARK_TEST("parser", "parse_expression()", "Wrong position of an operator", ""){
	test__parse_expression__throw("2+", "Unexpected end of program.");
}
QUARK_TEST("parser", "parse_expression()", "Wrong position of an operator", ""){
	test__parse_expression__throw("2*", "Unexpected end of program.");
}


/*
QUARK_TEST("parser", "parse_expression()", "Invalid characters", ""){
	test__parse_expression__throw("~5", "Illegal characters.");
}

QUARK_TEST("parser", "parse_expression()", "Invalid characters", ""){
	test__parse_expression__throw("~5", "");
}

QUARK_TEST("parser", "parse_expression()", "Invalid characters", ""){
//	test__parse_expression__throw("5x", "EEE_WRONG_CHAR");
}


QUARK_TEST("parser", "parse_expression()", "Invalid characters", ""){
	test__parse_expression__throw("2/", "Unexpected end of string.");
}
*/


std::pair<json_t, seq_t> parse_expression_deep(const seq_t& p, const eoperator_precedence precedence){
	QUARK_ASSERT(p.check_invariant());

	auto lhs = parse_lhs_atom(p);
	const auto r = parse_optional_operation_rightward(lhs.second, lhs.first, precedence);
	return r;
}

std::pair<json_t, seq_t> parse_expression(const seq_t& p){
#if DEBUG
	const auto illegal_char = read_while(p, k_valid_expression_chars);
	QUARK_ASSERT(illegal_char.second.empty());
#endif

	try{
		const auto r = parse_expression_deep(p, eoperator_precedence::k_super_weak);
		return { r.first, r.second };
	}

	//	If an exception other than compiler_error is thrown, make a compiler error with location info.
	catch(const compiler_error& e){
		if(e.location == k_no_location){
			QUARK_ASSERT(e.location2.loc == k_no_location);
		}
		throw_compiler_error(location_t(p.pos()), e.what());
	}
	catch(const std::runtime_error& e){
		throw_compiler_error(location_t(p.pos()), e.what());
	}
	catch(const std::exception& e){
		throw_compiler_error(location_t(p.pos()), "Failed to parse expression.");
	}
}

}	//	parser
}	//	floyd
