//
//  parser2.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

//	Based on Magnus snippet.

#include "quark.h"

#include <string>
#include <memory>
#include <map>
#include "text_parser.h"


#include "parser2.h"

using namespace std;



namespace parser2 {


QUARK_UNIT_TESTQ("C++ operators", ""){
	const int a = 2-+-2;
	QUARK_TEST_VERIFY(a == 4);
}



QUARK_UNIT_TESTQ("C++ enum class()", ""){
	enum class my_enum {
		k_one = 1,
		k_four = 4
	};

	QUARK_UT_VERIFY(my_enum::k_one == my_enum::k_one);
	QUARK_UT_VERIFY(my_enum::k_one != my_enum::k_four);
	QUARK_UT_VERIFY(static_cast<int>(my_enum::k_one) == 1);
}


bool is_valid_chars(const std::string& s){
	const auto allowed = k_identifier_chars + k_c99_number_chars + k_c99_whitespace_chars + "+-*/%" + "\"[]().?:=!<>&,|";
	for(auto i = 0 ; i < s.size() ; i++){
		const char ch = s[i];
		if(allowed.find(ch) == string::npos){
			return false;
		}
	}
	return true;
}



seq_t skip_whitespace(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	return read_while(p, k_c99_whitespace_chars).second;
}



pair<std::string, seq_t> parse_string_literal(const seq_t& p){
	QUARK_ASSERT(!p.empty());
	QUARK_ASSERT(p.first1_char() == '\"');

	const auto pos = p.rest();
	const auto s = read_until(pos, "\"");
	return { s.first, s.second.rest() };
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\"\" xxx")), pair<std::string, seq_t>("", seq_t(" xxx")));
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\"hello\" xxx")), pair<std::string, seq_t>("hello", seq_t(" xxx")));
}

QUARK_UNIT_TESTQ("parse_string_literal()", ""){
	quark::ut_compare(parse_string_literal(seq_t("\".5\" xxx")), pair<std::string, seq_t>(".5", seq_t(" xxx")));
}



std::pair<constant_value_t, seq_t> parse_numeric_constant(const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(k_c99_number_chars.find(p.first()) != std::string::npos);

	const auto number_pos = read_while(p, k_c99_number_chars);
	if(number_pos.first.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	//	If it contains a "." its a float, else an int.
	if(number_pos.first.find('.') != std::string::npos){
		const auto number = parse_float(number_pos.first);
		return { constant_value_t(number), number_pos.second };
	}
	else{
		int number = atoi(number_pos.first.c_str());
		return { constant_value_t(number), number_pos.second };
	}
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("0 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_int && a.first._int == 0);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("1234 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_int && a.first._int == 1234);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}

QUARK_UNIT_TESTQ("parse_numeric_constant()", ""){
	const auto a = parse_numeric_constant(seq_t("0.5 xxx"));
	QUARK_UT_VERIFY(a.first._type == constant_value_t::etype::k_float && a.first._float == 0.5f);
	QUARK_UT_VERIFY(a.second.get_s() == " xxx");
}



static const std::map<eoperation, string> k_2_operator_to_string{
//	{ eoperation::k_x_member_access, "->" },

	{ eoperation::k_2_looup, "[-]" },

	{ eoperation::k_2_add, "+" },
	{ eoperation::k_2_subtract, "-" },
	{ eoperation::k_2_multiply, "*" },
	{ eoperation::k_2_divide, "/" },
	{ eoperation::k_2_remainder, "%" },

	{ eoperation::k_2_smaller_or_equal, "<=" },
	{ eoperation::k_2_smaller, "<" },
	{ eoperation::k_2_larger_or_equal, ">=" },
	{ eoperation::k_2_larger, ">" },

	{ eoperation::k_2_logical_equal, "==" },
	{ eoperation::k_2_logical_nonequal, "!=" },
	{ eoperation::k_2_logical_and, "&&" },
	{ eoperation::k_2_logical_or, "||" },
};


/*
	Generates expressions encode as JSON in std::string values. Use for testing.


	public: virtual const string maker__make2(const eoperation op, const string& lhs, const string& rhs) const{
		const auto op_str = k_2_operator_to_string.at(op);
		return make3(quote(op_str), lhs, rhs);
	}
*/

	static string make2(string s0, string s1){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << "]";
		return ss.str();
	}

	static string make3(string s0, string s1, string s2){
		std::ostringstream ss;
		ss << "[" << s0 << ", " << s1 << ", " << s2 << "]";
		return ss.str();
	}

std::string expr_to_string(const expr_t& e);

	static string op_to_string(eoperation op, const expr_t& expr0, const expr_t& expr1){
		const auto op_string = quote(k_2_operator_to_string.at(op));
		const auto expr0_str = expr_to_string(expr0);
		const auto expr1_str = expr_to_string(expr1);

		std::ostringstream ss;
		ss << "[" << op_string << ", " << expr0_str << ", " << expr1_str << "]";
		return ss.str();
}

std::string expr_to_string(const expr_t& e){
	if(e._op == eoperation::k_0_number_constant){
		const auto value = *e._constant;
		if(value._type == constant_value_t::etype::k_bool){
			return make3("\"k\"", "\"bool\"", value._bool ? "true" : "false");
		}
		else if(value._type == constant_value_t::etype::k_int){
			return make3("\"k\"", "\"int\"", std::to_string(value._int));
		}
		else if(value._type == constant_value_t::etype::k_float){
			return make3("\"k\"", "\"float\"", float_to_string(value._float));
		}
		else if(value._type == constant_value_t::etype::k_string){
			//	 Use k_0_string_literal!
			QUARK_ASSERT(false);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(e._op == eoperation::k_0_resolve){
		return make2(quote("@"), quote(e._identifier));
	}
	else if(e._op == eoperation::k_0_string_literal){
		const auto value = *e._constant;
		QUARK_ASSERT(value._type == constant_value_t::etype::k_string);
		return make3("\"k\"", "\"string\"", quote(value._string));
	}
	else if(e._op == eoperation::k_x_member_access){
		return make3(quote("->"), expr_to_string(e._exprs[0]), quote(e._identifier));
	}
	else if(e._op == eoperation::k_2_looup){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_add){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_subtract){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_multiply){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_divide){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_remainder){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller_or_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_smaller){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger_or_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_larger){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_equal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_nonequal){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_and){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_2_logical_or){
		return op_to_string(e._op, e._exprs[0], e._exprs[1]);
	}
	else if(e._op == eoperation::k_3_conditional_operator){
		std::ostringstream ss;
		ss << "[\"?:\", " << expr_to_string(e._exprs[0]) << ", " << expr_to_string(e._exprs[1]) << ", " << expr_to_string(e._exprs[2]) + "]";
		return ss.str();
	}
	else if(e._op == eoperation::k_n_call){
		std::ostringstream ss;
		ss << "[\"call\", " + expr_to_string(e._exprs[0]) + ", [";
		for(auto i = 0 ; i < e._exprs.size() - 1 ; i++){
			const auto& arg = expr_to_string(e._exprs[i + 1]);
			ss << arg;
			if(i != (e._exprs.size() - 2)){
				ss << ", ";
			}
		}
		ss << "]]";
		return ss.str();

	}
	else if(e._op == eoperation::k_1_unary_minus){
		return "[\"unary_minus\", " + expr_to_string(e._exprs[0]) + "]";
	}
	else{
		QUARK_ASSERT(false)
		return "";
	}
}



/*
	Generates expressions encode as JSON in std::string values. Use for testing.
*/
struct json_helper : public maker<expr_t> {

	public: virtual const expr_t maker__make_identifier(const std::string& s) const{
		return expr_t{ eoperation::k_0_resolve, {}, {}, s };
	}

	public: virtual const expr_t maker__make1(const eoperation op, const expr_t& expr) const{
		return expr_t{ op, { expr }, {}, "" };
	}

	public: virtual const expr_t maker__make2(const eoperation op, const expr_t& lhs, const expr_t& rhs) const{
		return expr_t{ op, { lhs, rhs }, {}, "" };
	}

	public: virtual const expr_t maker__make3(const eoperation op, const expr_t& e1, const expr_t& e2, const expr_t& e3) const{
		return expr_t{ op, { e1, e2, e3 }, {}, "" };
	}

	public: virtual const expr_t maker__call(const expr_t& f, const std::vector<expr_t>& args) const{
		std::vector<expr_t> exprs;
		exprs.push_back(f);
		exprs.insert(exprs.end(), args.begin(), args.end());
		return expr_t{ eoperation::k_n_call, exprs, {}, "" };
	}

	public: virtual const expr_t maker__member_access(const expr_t& address, const std::string& member_name) const{
		return expr_t{ eoperation::k_x_member_access, { address }, {}, member_name };
	}

	public: virtual const expr_t maker__make_constant(const constant_value_t& value) const{
		if(value._type == constant_value_t::etype::k_string){
			return expr_t{ eoperation::k_0_string_literal, {}, std::make_shared<constant_value_t>(value), "" };
		}
		else{
			return expr_t{ eoperation::k_0_number_constant, {}, std::make_shared<constant_value_t>(value), "" };
		}
	}
};



bool test__parse_single(const std::string& expression, const std::string& expected_value, const std::string& expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	json_helper helper;
	const auto result = parse_single(helper, seq_t(expression));
	const string json_s = expr_to_string(result.first);
	QUARK_TRACE_SS("result:" << json_s);
	if(json_s != expected_value){
		return false;
	}
	else if(result.second.get_s() != expected_seq){
		return false;
	}
	return true;
}


QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"123 xxx",
	R"(["k", "int", 123])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"123.5 xxx",
	R"(["k", "float", 123.5])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"0.0 xxx",
	R"(["k", "float", 0])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"\"world!\" xxx",
	R"(["k", "string", "world!"])",
	" xxx"
));

QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"\"\" xxx",
	R"(["k", "string", ""])",
	" xxx"
));


QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"true xxx",
	R"(["k", "bool", true])",
	" xxx"
));
QUARK_UNIT_1("parse_single()", "identifier", test__parse_single(
	"false xxx",
	R"(["k", "bool", false])",
	" xxx"
));



std::pair<std::string, seq_t> test_parse_expression2(const seq_t& expression){
	json_helper helper;
	const auto result = parse_expression_template(helper, seq_t(expression));
	return { expr_to_string(result.first), result.second };
}


bool test__parse_expression(const std::string& expression, string expected_value, string expected_seq){
	QUARK_TRACE_SS("input:" << expression);
	QUARK_TRACE_SS("expect:" << expected_value);

	const auto result = test_parse_expression2(seq_t(expression));
	QUARK_TRACE_SS("result:" << result.first);
	if(result.first != expected_value){
		return false;
	}
	else if(result.second.get_s() != expected_seq){
		return false;
	}
	return true;
}



//////////////////////////////////			EMPTY

QUARK_UNIT_TESTQ("parse_expression()", ""){
	try{
		test__parse_expression("", "", "");
		QUARK_UT_VERIFY(false);
	}
	catch(...){
	}
}

//////////////////////////////////			CONSTANTS

QUARK_UNIT_1("parse_expression()", "", test__parse_expression("0", "[\"k\", \"int\", 0]", ""));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("0 xxx", "[\"k\", \"int\", 0]", " xxx"));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("1234567890", "[\"k\", \"int\", 1234567890]", ""));
QUARK_UNIT_1("parse_expression()", "", test__parse_expression("\"hello, world!\"", "[\"k\", \"string\", \"hello, world!\"]", ""));


//////////////////////////////////			ARITHMETICS

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 + 4",
	R"(["+", ["k", "int", 10], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 2 + 3 + 4",
	R"(["+", ["+", ["+", ["k", "int", 1], ["k", "int", 2]], ["k", "int", 3]], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 * 4",
	R"(["*", ["k", "int", 10], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"10 * 4 * 3",
	R"(["*", ["*", ["k", "int", 10], ["k", "int", 4]], ["k", "int", 3]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"40 / 4",
	R"(["/", ["k", "int", 40], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"40 / 5 / 2",
	R"(["/", ["/", ["k", "int", 40], ["k", "int", 5]], ["k", "int", 2]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"41 % 5",
	R"(["%", ["k", "int", 41], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"413 % 50 % 10",
	R"(["%", ["%", ["k", "int", 413], ["k", "int", 50]], ["k", "int", 10]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 3 * 2 + 100",
	R"(["+", ["+", ["k", "int", 1], ["*", ["k", "int", 3], ["k", "int", 2]]], ["k", "int", 100]])",
	""
));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"1 + 8 + 7 + 2 * 3 + 4 * 5 + 6",
	R"(["+", ["+", ["+", ["+", ["+", ["k", "int", 1], ["k", "int", 8]], ["k", "int", 7]], ["*", ["k", "int", 2], ["k", "int", 3]]], ["*", ["k", "int", 4], ["k", "int", 5]]], ["k", "int", 6]])",
""
));



//////////////////////////////////			parantheses

QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression("(3)", "[\"k\", \"int\", 3]", ""));
QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression("(3 * 8)", R"(["*", ["k", "int", 3], ["k", "int", 8]])", ""));

/*
	["-",
		["+",
			["*", ["k", "int", 3], 	["k", "int", 2]],
			["*", ["k", "int", 8], ["k", "int", 2]]
		],
		["*",["k", "int", 1], ["k", "int", 2]]
	]
*/

QUARK_UNIT_1("parse_expression()", "parantheses", test__parse_expression(
	"(3 * 2 + (8 * 2)) - (((1))) * 2",
	R"(["-", ["+", ["*", ["k", "int", 3], ["k", "int", 2]], ["*", ["k", "int", 8], ["k", "int", 2]]], ["*", ["k", "int", 1], ["k", "int", 2]]])",
	""
));


//////////////////////////////////			NEG

QUARK_UNIT_1("parse_expression()", "", test__parse_expression("-2 xxx", "[\"unary_minus\", [\"k\", \"int\", 2]]", " xxx"));

QUARK_UNIT_1("parse_expression()", "arithmetics", test__parse_expression(
	"-(3)",
	R"(["unary_minus", ["k", "int", 3]])",
	""
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	"2---2 xxx",
	R"(["-", ["k", "int", 2], ["unary_minus", ["unary_minus", ["k", "int", 2]]]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	"2-+-2 xxx",
	R"(["-", ["k", "int", 2], ["unary_minus", ["k", "int", 2]]])",
	" xxx"
));




//////////////////////////////////			LOGICAL EQUALITY


QUARK_UNIT_1("parse_expression()", "<=", test__parse_expression(
	"3 <= 4",
	R"(["<=", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "<", test__parse_expression(
	"3 < 4",
	R"(["<", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", ">=", test__parse_expression(
	"3 >= 4",
	R"([">=", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", ">", test__parse_expression(
	"3 > 4",
	R"([">", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "==", test__parse_expression(
	"3 == 4",
	R"(["==", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "==", test__parse_expression(
	"1==3",
	R"(["==", ["k", "int", 1], ["k", "int", 3]])",
	""
));
QUARK_UNIT_1("parse_expression()", "!=", test__parse_expression(
	"3 != 4",
	R"(["!=", ["k", "int", 3], ["k", "int", 4]])",
	""
));

QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"3 && 4",
	R"(["&&", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"3 && 4 && 5",
	R"(["&&", ["&&", ["k", "int", 3], ["k", "int", 4]], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "&&", test__parse_expression(
	"1 * 1 && 0 * 1",
	R"(["&&", ["*", ["k", "int", 1], ["k", "int", 1]], ["*", ["k", "int", 0], ["k", "int", 1]]])",
	""
));

QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"3 || 4",
	R"(["||", ["k", "int", 3], ["k", "int", 4]])",
	""
));
QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"3 || 4 || 5",
	R"(["||", ["||", ["k", "int", 3], ["k", "int", 4]], ["k", "int", 5]])",
	""
));
QUARK_UNIT_1("parse_expression()", "||", test__parse_expression(
	"1 * 1 || 0 * 1",
	R"(["||", ["*", ["k", "int", 1], ["k", "int", 1]], ["*", ["k", "int", 0], ["k", "int", 1]]])",
	""
));

//??? Check combos of || and &&



//////////////////////////////////			IDENTIFIERS


QUARK_UNIT_1("parse_expression()", "identifier", test__parse_expression(
	"hello xxx",
	R"(["@", "hello"])",
	" xxx"
));


//////////////////////////////////			COMPARISON OPERATOR

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? 2 : 3 xxx",
	R"(["?:", ["k", "int", 1], ["k", "int", 2], ["k", "int", 3]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1==3 ? 4 : 6 xxx",
	R"(["?:", ["==", ["k", "int", 1], ["k", "int", 3]], ["k", "int", 4], ["k", "int", 6]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 ? \"true!!!\" : \"false!!!\" xxx",
	R"(["?:", ["k", "int", 1], ["k", "string", "true!!!"], ["k", "string", "false!!!"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"1 + 2 ? 3 + 4 : 5 + 6 xxx",
	R"(["?:", ["+", ["k", "int", 1], ["k", "int", 2]], ["+", ["k", "int", 3], ["k", "int", 4]], ["+", ["k", "int", 5], ["k", "int", 6]]])",
	" xxx"
));

//??? Add more test to see precedence works as it should!


QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"input_flag ? \"123\" : \"456\"",
	R"(["?:", ["@", "input_flag"], ["k", "string", "123"], ["k", "string", "456"]])",
	""
));


QUARK_UNIT_1("parse_expression()", "?:", test__parse_expression(
	"input_flag ? 100 + 10 * 2 : 1000 - 3 * 4",
	R"(["?:", ["@", "input_flag"], ["+", ["k", "int", 100], ["*", ["k", "int", 10], ["k", "int", 2]]], ["-", ["k", "int", 1000], ["*", ["k", "int", 3], ["k", "int", 4]]]])",
	""
));




//////////////////////////////////			FUNCTION CALLS

QUARK_UNIT_1("parse_expression()", "function call", test__parse_expression(
	"f() xxx",
	R"(["call", ["@", "f"], []])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "function call, one simple arg", test__parse_expression(
	"f(3)",
	R"(["call", ["@", "f"], [["k", "int", 3]]])",
	""
));

QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(x+10) xxx",
	R"(["call", ["@", "f"], [["+", ["@", "x"], ["k", "int", 10]]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg", test__parse_expression(
	"f(1,2) xxx",
	R"(["call", ["@", "f"], [["k", "int", 1], ["k", "int", 2]]])",
	" xxx"
));
QUARK_UNIT_1("parse_expression()", "call with expression-arg -- whitespace", test__parse_expression(
	"f ( 1 , 2 ) xxx",
	R"(["call", ["@", "f"], [["k", "int", 1], ["k", "int", 2]]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "function call with expression-args", test__parse_expression(
	"f(3 + 4, 4 * g(1000 + 2345), \"hello\", 5)",
	R"(["call", ["@", "f"], [["+", ["k", "int", 3], ["k", "int", 4]], ["*", ["k", "int", 4], ["call", ["@", "g"], [["+", ["k", "int", 1000], ["k", "int", 2345]]]]], ["k", "string", "hello"], ["k", "int", 5]]])",
	""
));

QUARK_UNIT_TESTQ("parse_expression()", "function call, expression argument"){
	const auto result = test_parse_expression2(seq_t("1 == 2)"));
	QUARK_UT_VERIFY((		result == pair<string, seq_t>( "[\"==\", [\"k\", \"int\", 1], [\"k\", \"int\", 2]]", ")" )		));
}



QUARK_UNIT_TESTQ("parse_expression()", "function call, expression argument"){
	const auto result = test_parse_expression2(seq_t("f(1 == 2)"));
	QUARK_UT_VERIFY((		result == pair<string, seq_t>( R"(["call", ["@", "f"], [["==", ["k", "int", 1], ["k", "int", 2]]]])", "" )		));
}


QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("((3))))"));
}
QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print((3))))"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(1 < 2)"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(1 < color(1, 2, 3))"));
}


QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	const auto result = test_parse_expression2(seq_t("print(color(1, 2, 3) < color(1, 2, 3))"));
}

QUARK_UNIT_TESTQ("parse_expression()", "function call"){
	QUARK_UT_VERIFY(
		test__parse_expression(
			"print(color(1, 2, 3) == file(404)) xxx",
			"[\"call\", [\"@\", \"print\"], [[\"==\", [\"call\", [\"@\", \"color\"], [[\"k\", \"int\", 1], [\"k\", \"int\", 2], [\"k\", \"int\", 3]]], [\"call\", [\"@\", \"file\"], [[\"k\", \"int\", 404]]]]]]",
			" xxx"
		)
	)
}


//////////////////////////////////			MEMBER ACCESS

QUARK_UNIT_TESTQ("parse_expression()", "struct member access"){
	QUARK_UT_VERIFY(test__parse_expression("hello.kitty xxx", R"(["->", ["@", "hello"], "kitty"])", " xxx"));
}

QUARK_UNIT_1("parse_expression()", "struct member access", test__parse_expression(
	"hello.kitty.cat xxx",
	R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "struct member access -- whitespace", test__parse_expression(
	"hello . kitty . cat xxx",
	R"(["->", ["->", ["@", "hello"], "kitty"], "cat"])",
	" xxx"
));


//////////////////////////////////			LOOKUP

QUARK_UNIT_1("parse_expression()", "lookup with int", test__parse_expression(
	"hello[10] xxx",
	R"(["[-]", ["@", "hello"], ["k", "int", 10]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string", test__parse_expression(
	"hello[\"troll\"] xxx",
	R"(["[-]", ["@", "hello"], ["k", "string", "troll"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "lookup with string -- whitespace", test__parse_expression(
	"hello [ \"troll\" ] xxx",
	R"(["[-]", ["@", "hello"], ["k", "string", "troll"]])",
	" xxx"
));



//////////////////////////////////			COMBOS


QUARK_UNIT_1("parse_expression()", "combo - function call", test__parse_expression(
	"poke.mon.f() xxx",
	R"(["call", ["->", ["->", ["@", "poke"], "mon"], "f"], []])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo - function call", test__parse_expression(
	"f().g() xxx",
	R"(["call", ["->", ["call", ["@", "f"], []], "g"], []])",
	" xxx"
));



QUARK_UNIT_1("parse_expression()", "complex chain", test__parse_expression(
	"hello[\"troll\"].kitty[10].cat xxx",
	R"(["->", ["[-]", ["->", ["[-]", ["@", "hello"], ["k", "string", "troll"]], "kitty"], ["k", "int", 10]], "cat"])",
	" xxx"
));


QUARK_UNIT_1("parse_expression()", "chain", test__parse_expression(
	"poke.mon.v[10].a.b.c[\"three\"] xxx",
	R"(["[-]", ["->", ["->", ["->", ["[-]", ["->", ["->", ["@", "poke"], "mon"], "v"], ["k", "int", 10]], "a"], "b"], "c"], ["k", "string", "three"]])",
	" xxx"
));

QUARK_UNIT_1("parse_expression()", "combo arithmetics", test__parse_expression(
	" 5 - 2 * ( 3 ) xxx",
	R"(["-", ["k", "int", 5], ["*", ["k", "int", 2], ["k", "int", 3]]])",
	" xxx"
));




//////////////////////////////////			EXPRESSION ERRORS

void test__parse_expression__throw(const std::string& expression, const std::string& exception_message){
	try{
		const auto result = parse_expression_template(json_helper(), seq_t(expression));
		QUARK_TEST_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		const std::string es(e.what());
		if(!exception_message.empty()){
			QUARK_TEST_VERIFY(es == exception_message);
		}
	}
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Parenthesis error") {
	test__parse_expression__throw("5*((1+3)*2+1", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Parenthesis error") {
//	test__parse_expression__throw("5*((1+3)*2)+1)", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Repeated operators (wrong)") {
	test__parse_expression__throw("5*/2", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("*2", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("2+", "Unexpected end of string");
}
QUARK_UNIT_TESTQ("evaluate_expression()", "Wrong position of an operator") {
	test__parse_expression__throw("2*", "Unexpected end of string");
}


QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("~5", "Illegal characters.");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("~5", "");
}

QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
//	test__parse_expression__throw("5x", "EEE_WRONG_CHAR");
}




QUARK_UNIT_TESTQ("evaluate_expression()", "Invalid characters") {
	test__parse_expression__throw("2/", "Unexpected end of string");
}


}	//	parser2
