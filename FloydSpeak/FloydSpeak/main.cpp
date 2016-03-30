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
#include <memory>
#include <map>
#include <iostream>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;


/*
https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/

/*
	Compiler version #1
	===============================
	main()
	functions
	bool, int, string
	true, false
	return
	if-then-else
	foreach
	map
	seq
	(?:)
	assert()

	Later version
	===============================
	struct
	map
	while
	mutable
*/

const string kProgram1 =
"int main(string args){\n"
"	return 3;\n"
"}\n";






typedef pair<string, string> seq;


const char* basic_types[] = {
	"bool",
	"char???",
	"-code_point",
	"-double",
	"float",
	"float32",
	"float80",
	"-hash",
	"int",
	"int16",
	"int32",
	"int64",
	"int8",
	"-path",
	"string",
	"-text"
};

const char* _advanced_types[] = {
	"-clock",
	"-defect_exception",
	"-dyn",
	"-dyn**<>",
	"-enum",
	"-exception",
	"map",
	"-protocol",
	"-rights",
	"-runtime_exception",
	"seq",
	"struct",
	"-typedef",
	"-vector"
};

const char* _keywords[] = {
	"assert",
	"-catch",
	"-deserialize()",
	"-diff()",
	"else",
	"-ensure",
	"false",
	"foreach",
	"-hash()",
	"if",
	"-invariant",
	"log",
	"mutable",
	"-namespace???",
	"-null",
	"-private",
	"-property",
	"-prove",
	"-require",
	"return",
	"-serialize()",
	"-swap",
	"-switch",
	"-tag",
	"-test",
	"-this",
	"true",
	"-try",
	"-typecast",
	"-typeof",
	"while"
};

const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";



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


struct type_t {
	public: static type_t make_type(string s){
		type_t result;
		result._name = s;
		return result;
	}

	public: bool operator==(const type_t other) const{
		return other._name == _name;
	}
	type_t() : _name(""){}
	type_t(const char s[]) :
		_name(s)
	{
	}
	type_t(string s) :
		_name(s)
	{
	}


	/*
		The name of the type, including its path using :
		"int"
		"float"
		"bool"
		"metronome"
		"map<string, metronome>"
		"game_engine:sprite"
		"vector<game_engine:sprite>"
		"int (string, vector<game_engine:sprite>)"
	*/
	public: string _name;
};


////////////////////////////////////		Nodes


/*
	14
	my_constant_x
	my_mutable_y
	( 14 + my_mutable_y + some_function("hi", 100) )
	some_function("hello", 4000)
		expression

*/
enum expression_type {
	inline_constant,
	local_constant,
	mutable_variable,
	function_call,

	/*
		(a + b)
		(a - b)
		(a * b)
		(a / b)
		(a % b)
	*/
arithmetic_operation
};
struct expression_t {
	expression_type _type;
};


/*
*/
enum EStatementType {
	/*
		return x * y;

		[return_value] [expression]
	*/
	return_value,

	/*
		if(a == 1){
			a = a + 2;
		}
		else if (b == a){
			a = a + 1;
		}
		else {
			return 100;
		}

		[if]
			[expression]
			[body]
		[else if]
			[expression]
			[body]
		[else if]
			[expression]
			[body]
		[else]
			[body]
	*/
	if__else_if__else,

	/*
		[for]
			[define_mutable] x N
				[expression]
			[expression]
			[update_mutabl] x M
			[body]
	*/
	for_,

	foreach_,

	/*
		[while] [expression] [body]
	*/
	while_,

	/*
		let a = 3 + 4 + 5;

		[define_local_constant] [type] [name] [expression]
	*/
	define_constant,

	/*
		mutable a = 3 + 4 + 5;

		Create a mutable local variable and name it. Assign it an initial value.
		[define_mutable]
			[mutable_name]
			[expression]
	*/
	define_mutable_local,

	/*
		a = 6 * 4;

		Rebind a local mutable variable to a new value.
		[update_mutable]
			[mutable_name]
			[expression]
	*/
	update_mutable_local,

	/*
		[assert]
			[expression]
	*/
	assert,

	/*
		Named function:

		int myfunc(string a, int b){
			...
			return b + 1;
		}

		Lambda:

		int myfunc(string a){
			() => {
			}
		}

		Just defines an unnamed function - it has no name.

		[define_function] [return_type] [args] [body]
	*/
	define_function
};


struct statement_t;

/*
	Functions always return a value.
*/
struct body_t {
	vector<statement_t> _root_statements;
};



////////////////////		Expression types


struct return_statement_t {
	expression_t _expression;
};

struct define_constant_t {
	type_t _type;
	string _identifier;
	expression_t _expression;
};

struct function_def_t {
	type_t _return_type;
	vector<pair<type_t, string>> _args;
	body_t _body;
};


////////////////////		statement_t


struct statement_t {
	EStatementType _type;

	//	Only one is used at any time.
//	shared_ptr<return_statement_t> _return_value;
//	shared_ptr<define_constant_t> _define_constant;
//	shared_ptr<function_def_t> _function_def;
};




struct pass1 {
	/*
		All defined functions, both named and unnamed. Unnamed ones get a generated name.
	*/
	std::map<string, function_def_t> _functions;
};


/*
	Makes a tree of all the program. Whitespace removed, all types of parenthesis split to sub-trees.
	No knowledget of language syntax needed to do this.
*/


pass1 compile_pass1(string program){
	const auto start = seq("", program);

//	vector<string, function_def_t> function_defs;
//	vector<string, function_def_t*> global_constants;

	const seq t1 = get_next_token(program, whitespace_chars, identifier_chars);

	string type1;
	if(t1.first == "int" || t1.first == "bool" || t1.first == "string"){
		type1 = t1.first;
	}
	else {
		throw std::runtime_error("expected type");
	}

	//	Get identifier (name of a defined function or constant variable name).
	const seq t2 = get_next_token(t1.second, whitespace_chars, identifier_chars);
	const string identifier = t2.first;

	//	Skip whitespace.
	const seq t3 = get_next_token(t2.second, whitespace_chars, "");

	//	Is this a function definition?
	if(t3.second.size() > 0 && t3.second[0] == '('){
		const auto t4 = get_balanced(t3.second);
		const auto function_return = type1;
		const auto function_args = t4.first;


/*
		function_def_t function_def;
		function_def._return_type = type_t::make_type(type1);
//		function_def._args = args;
//		function_def._body = body;
		global_constants.push_back(&function_def);


struct function_def_t {
	type_t _return_type;
	vector<pair<type_t, string>> _args;
	body_t _body;
};

		function_defs.push_back(function_def);
*/
	}
	else{
		throw std::runtime_error("expected (");
	}

	return pass1();
}

string emit_c_code(){
	return
		"int main(int argc, char *argv[]){\n"
		"	return 0;\n"
		"}\n";
}

QUARK_UNIT_TEST("", "compile_pass1()", "kProgram1", ""){
	QUARK_TEST_VERIFY(compile_pass1(kProgram1)._functions.size() == 1);
}



const string kProgram2 =
"int main(string args){\n"
"	log(args);\n"
"	return 3;\n"
"}\n";

const string kProgram3 =
"int main(string args){\n"
"	return 3 + 4;\n"
"}\n";

const string kProgram4 =
"int myfunc(){ return 5; }\n"
"int main(string args){\n"
"	return myfunc();\n"
"}\n";


QUARK_UNIT_TEST("", "compiler_pass1()", "", ""){
	auto r = compile_pass1(kProgram2);
	QUARK_TEST_VERIFY(r._functions["main"]._return_type == type_t::make_type("int"));
	vector<pair<string, string>> a{ { "string", "args" }};
	QUARK_TEST_VERIFY((r._functions["main"]._args == vector<pair<type_t, string>>{ { type_t::make_type("string"), "args" }}));
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
