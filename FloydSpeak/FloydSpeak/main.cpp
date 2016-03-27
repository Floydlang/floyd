//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true

#include "quark.h"
#include "steady_vector.h"
#include <string>
#include <map>
#include <iostream>

using std::string;
using std::pair;


const string kProgram1 =
"int main(string args){"
"	log(args);"
"	return 3;"
"}";

typedef pair<string, string> seq;

bool is_whitespace(char c){
	return c == ' ' || c == '\t' || c == '\n';
}

/*
	example
		input " \t\tint blob ()"
		result "int", " blob ()"
	example
		input "test()"
		result "test", "()"
*/

string skip_whitespace(const string& s){
	size_t pos = 0;
	while(pos < s.size() && is_whitespace(s[pos])){
		pos++;
	}
	return s.substr(pos);
}

const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";

seq get_next_token(const string& s, string first_skip, string then_use){
	size_t pos = 0;
	while(pos < s.size() && first_skip.find(s[pos]) != string::npos){
		pos++;
	}

	size_t pos2 = pos;
	while(pos2 < s.size() && then_use.find(s[pos2]) != string::npos){
		pos2++;
	}

	return seq(
		s.substr(pos, pos2 - pos),
		s.substr(pos2)
	);
}

bool is_start_char(char c){
	return c == '(' || c == '[' || c == '{' || c == '<';
}

QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('('));
	QUARK_TEST_VERIFY(is_start_char('['));
	QUARK_TEST_VERIFY(is_start_char('{'));
	QUARK_TEST_VERIFY(is_start_char('<'));

	QUARK_TEST_VERIFY(!is_start_char(')'));
	QUARK_TEST_VERIFY(!is_start_char(']'));
	QUARK_TEST_VERIFY(!is_start_char('}'));
	QUARK_TEST_VERIFY(!is_start_char('>'));

	QUARK_TEST_VERIFY(!is_start_char(' '));
}

bool is_end_char(char c){
	return c == ')' || c == ']' || c == '}' || c == '>';
}

QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('('));
	QUARK_TEST_VERIFY(!is_end_char('['));
	QUARK_TEST_VERIFY(!is_end_char('{'));
	QUARK_TEST_VERIFY(!is_end_char('<'));

	QUARK_TEST_VERIFY(is_end_char(')'));
	QUARK_TEST_VERIFY(is_end_char(']'));
	QUARK_TEST_VERIFY(is_end_char('}'));
	QUARK_TEST_VERIFY(is_end_char('>'));

	QUARK_TEST_VERIFY(!is_end_char(' '));
}


char start_char_to_end_char(char start_char){
	QUARK_ASSERT(is_start_char(start_char));

	if(start_char == '('){
		return ')';
	}
	else if(start_char == '['){
		return ']';
	}
	else if(start_char == '{'){
		return '}';
	}
	else if(start_char == '<'){
		return '>';
	}
	else {
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('(') == ')');
	QUARK_TEST_VERIFY(start_char_to_end_char('[') == ']');
	QUARK_TEST_VERIFY(start_char_to_end_char('{') == '}');
	QUARK_TEST_VERIFY(start_char_to_end_char('<') == '>');
}

/*
	First char is the start char, like '(' or '{'.
*/

seq get_balanced(const string& s){
	QUARK_ASSERT(s.size() > 0);

	const char start_char = s[0];
	QUARK_ASSERT(is_start_char(start_char));

	const char end_char = start_char_to_end_char(start_char);

	int depth = 0;
	size_t pos = 0;
	while(pos < s.size() && !(depth == 1 && s[pos] == end_char)){
		const char c = s[pos];
		if(is_start_char(c)) {
			depth++;
		}
		else if(is_end_char(c)){
			if(depth == 0){
				throw std::runtime_error("unbalanced ([{< >}])");
			}
			depth--;
		}
		pos++;
	}

	return seq(
		s.substr(0, pos + 1),
		s.substr(pos + 1)
	);
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced("()") == seq("()", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)") == seq("(abc)", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)def") == seq("(abc)", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc))def") == seq("((abc))", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc)[])def") == seq("((abc)[])", "def"));
}


struct pass1 {
	std::map<string, string> _functions;
};


pass1 compiler_pass1(string program){
	const auto start = seq("", program);

	const seq t1 = get_next_token(program, whitespace_chars, identifier_chars);

	string type1;
	if(t1.first == "int" || t1.first == "bool" || t1.first == "string"){
		type1 = t1.first;
	}
	else {
		throw std::runtime_error("expected type");
	}

	const seq t2 = get_next_token(t1.second, whitespace_chars, identifier_chars);
	const string identifier = t2.first;

	const seq t3 = get_next_token(t2.second, whitespace_chars, "");

	//	Is this a function definition?
	if(t3.second.size() > 0 && t3.second[0] == '('){
		const auto t4 = get_balanced(t3.second);

		const auto function_return = type1;
		const auto function_args = std::string("(") + t4.first;
		QUARK_TRACE("");
	}
	else{
		throw std::runtime_error("expected (");
	}

	return pass1();
}

QUARK_UNIT_TEST("", "generate_numbers()", "5 numbers", "correct vector"){
	QUARK_TEST_VERIFY(compiler_pass1(kProgram1)._functions.size() == 1);
}



int main(int argc, const char * argv[]) {
	try {
#if QUARK_UNIT_TESTS_ON
		quark::run_tests();
#endif
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

  return 0;
}
