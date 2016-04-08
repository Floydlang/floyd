//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	floydrt -- floyd runtime.
	floydgen -- generated code from user program.
	floydc -- floyd compiler
*/

#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true

#include "quark.h"
#include "steady_vector.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

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




typedef pair<string, string> seq;

struct seq2 {
	seq2 substr(size_t pos, size_t count = string::npos){
		return seq2();
	}
	
	const shared_ptr<const string> _str;
	std::size_t _pos;
};

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




void IncreaseIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(1);
}

void DecreateIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(-1);
}


//////////////////////////////////////////////////		Text parsing primitives



const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";
const string type_chars = identifier_chars + brackets;
const string number_chars = "0123456789.";
const string operator_chars = "+-*/.";


seq read_while(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) != string::npos){
		pos++;
	}

	return seq(
		s.substr(0, pos),
		s.substr(pos)
	);
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY(read_while("", whitespace_chars) == seq("", ""));
	QUARK_TEST_VERIFY(read_while(" ", whitespace_chars) == seq(" ", ""));
	QUARK_TEST_VERIFY(read_while("    ", whitespace_chars) == seq("    ", ""));

	QUARK_TEST_VERIFY(read_while("while", whitespace_chars) == seq("", "while"));
	QUARK_TEST_VERIFY(read_while(" while", whitespace_chars) == seq(" ", "while"));
	QUARK_TEST_VERIFY(read_while("    while", whitespace_chars) == seq("    ", "while"));
}

seq read_until(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) == string::npos){
		pos++;
	}

	return { s.substr(0, pos), s.substr(pos) };
}

QUARK_UNIT_TEST("", "read_until()", "", ""){
	QUARK_TEST_VERIFY(read_until("", ",.") == seq("", ""));
	QUARK_TEST_VERIFY(read_until("ab", ",.") == seq("ab", ""));
	QUARK_TEST_VERIFY(read_until("ab,cd", ",.") == seq("ab", ",cd"));
	QUARK_TEST_VERIFY(read_until("ab.cd", ",.") == seq("ab", ".cd"));
}


/*
	first: skipped whitespaces
	second: all / any text after whitespace.
*/
std::string skip_whitespace(const string& s){
	return read_while(s, whitespace_chars).second;
}

QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("") == "");
	QUARK_TEST_VERIFY(skip_whitespace(" ") == "");
	QUARK_TEST_VERIFY(skip_whitespace("\t") == "");
	QUARK_TEST_VERIFY(skip_whitespace("int blob()") == "int blob()");
	QUARK_TEST_VERIFY(skip_whitespace("\t\t\t int blob()") == "int blob()");
}


pair<char, string> read_char(const std::string& s){
	if(!s.empty()){
		return { s[0], s.substr(1) };
	}
	else{
		throw std::runtime_error("expected character.");
	}
}

/*
	Returns "rest" if ch is found, else throws exceptions.
*/
std::string read_required_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return s.substr(1);
	}
	else{
		throw std::runtime_error("expected character '" + string(1, ch)  + "'.");
	}
}

pair<bool, std::string> read_optional_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return { true, s.substr(1) };
	}
	else{
		return { false, s };
	}
}


bool peek_compare_char(const std::string& s, char ch){
	return s.size() > 0 && s[0] == ch;
}

bool peek_string(const std::string& s, const std::string& peek){
	return s.size() >= peek.size() && s.substr(0, peek.size()) == peek;
}

QUARK_UNIT_TEST("", "peek_string()", "", ""){
	QUARK_TEST_VERIFY(peek_string("", "") == true);
	QUARK_TEST_VERIFY(peek_string("a", "a") == true);
	QUARK_TEST_VERIFY(peek_string("a", "b") == false);
	QUARK_TEST_VERIFY(peek_string("", "b") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "abc") == true);
	QUARK_TEST_VERIFY(peek_string("abc", "abx") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "ab") == true);
}

bool is_whitespace(char ch){
	return whitespace_chars.find(string(1, ch)) != string::npos;
}

QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}


/*
	Skip leading whitespace, get string while type-char.
*/
seq get_type(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, type_chars);
	return b;
}

/*
	Skip leading whitespace, get string while identifier char.
*/
seq get_identifier(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);
	return b;
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

	return { s.substr(0, pos + 1), s.substr(pos + 1) };
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced("()") == seq("()", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)") == seq("(abc)", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)def") == seq("(abc)", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc))def") == seq("((abc))", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc)[])def") == seq("((abc)[])", "def"));
}




//////////////////////////////////////////////////		NODES



#if false
enum node1_type {

	////////////////////////////////		STATEMENTS

	/*
		Example source code:
			{
				int a = 10;
				int b = random();
				if(b > 2){
					return a;
				}
				return a + b;
			}

		[body_node]
			[statement]
			[statement 2]
			[statement 3]
	*/
	body_node = 10,

	/*
		Example source code: return x * y;

		[return_value_statement]
			[expression]
	*/
	return_value_statement,

	/*
		Example source code:
			if(a == 1){
				a = a + 2;
			}
			else if (b == a){
				a = a + 1;
			}
			else {
				return 100;
			}

		[if__else_if__else_statement]
				[expression]
				[body_node]
			[else if]
				[expression]
				[body_node]
			[else if]
				[expression]
				[body_node]
			[else]
				[body_node]
	*/
//	if__else_if__else_statement,

	/*
		[for]
			[define_mutable N]
				[expression]
			[define_mutable N2]
				[expression]
			[expression]
			[update_mutable M]
			[update_mutable M2]
			[body_node]
	*/
//	for_statement,

//	foreach_statement,

	/*
		Example source code:
			while(x < 3 && y == 2){
				x++;
				y++;
			}

		[while_statement]
			[expression]
			[body_node]
	*/
//	while_statement,

	/*
		Define constant using expression and bind it to an identifier.

		let a = 3 + 4 + 5;

		[define_local_constant] [value_type] [name] [expression]
	*/
	define_local_constant_statement,

	/*
		mutable a = 3 + 4 + 5;

		Create a mutable local variable and name it. Assign it an initial value.
		[define_mutable]
			[mutable_name]
			[expression]
	*/
//	define_mutable_local_statement,

	/*
		a = 6 * 4;

		Rebind a local mutable variable to a new value.
		[update_mutable]
			[mutable_name]
			[expression]
	*/
//	update_mutable_local_statement,

	/*
		[assert]
			[expression]
	*/
//	assert_statement,

	define_global_constant_statement,



	////////////////////////////////		EXPRESSIONS



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

		[make_function_expression]
			[value_type]
			[arg_list]
				[arg0]
					[value_type]
					[name]
				[arg1]
					[value_type]
					[name]
			[body_node]
	*/
	make_function_expression,

	/*
		( 14 + my_mutable_y + some_function("hi", 100) )
	*/
	expression,

	/*
		14
		"hello"
	*/
//	inline_constant_expression,

	/*
		my_constant_x
	*/
//	local_constant_expression,

	/*
		my_mutable_y
	*/
//	mutable_variable_expression,

	/*
		some_function("hello", 4000)
	*/
	function_call_expression,

	/*
		(a + b)
		(a - b)
		(a * b)
		(a / b)
		(a % b)
	*/
//	arithmetic_expression
};
#endif





//////////////////////////////////////////////////		data_type_t

/*
	Internal object representing a Floyd data type.
		### Use the same compatible technique / API  in the runtime and in the language itself!

	!!! Right now we use an internal string, but this will change in future.
*/


struct data_type_t {
	public: static data_type_t make_type(string s){
		data_type_t result;
		result._type_magic = s;
		return result;
	}

	public: bool operator==(const data_type_t other) const{
		return other._type_magic == _type_magic;
	}
	public: bool operator!=(const data_type_t other) const{
		return !(*this == other);
	}
	data_type_t() : _type_magic(""){}
	data_type_t(const char s[]) :
		_type_magic(s)
	{
	}
	data_type_t(string s) :
		_type_magic(s)
	{
	}

	string to_string() const {
		return _type_magic;
	}

	/*
		The name of the type, including its path using :
		"null"

		"bool"
		"int"
		"float"
		"function"

		//	Specifies a data type.
		"value_type"


		"metronome"
		"map<string, metronome>"
		"game_engine:sprite"
		"vector<game_engine:sprite>"
		"int (string, vector<game_engine:sprite>)"
	*/
	private: string _type_magic;
};


/*
	returns "" on unknown type.
*/
data_type_t resolve_type(string node_type){
	if(node_type == "int"){
		return data_type_t::make_type("int");
	}
	else if(node_type == "string"){
		return data_type_t::make_type("string");
	}
	else if(node_type == "float"){
		return data_type_t::make_type("float");
	}
	else if(node_type == "function"){
		return data_type_t::make_type("function");
	}
	else if(node_type == "value_type"){
		return data_type_t::make_type("value_type");
	}
	else{
		return "";
	}
}



//////////////////////////////////////////////////		value_t

/*
	Hold a value with an explicit type.
*/

struct value_t {
	public: value_t() :
		_type("null")
	{
	}
	public: value_t(const char s[]) :
		_type("string"),
		_string(s)
	{
	}
	public: value_t(const string& s) :
		_type("string"),
		_string(s)
	{
	}

	public: value_t(int value) :
		_type("int"),
		_int(value)
	{
	}

	public: value_t(float value) :
		_type("float"),
		_float(value)
	{
	}

	public: value_t(const data_type_t& s) :
		_type("value_type"),
		_data_type(s)
	{
	}

	public: bool operator==(const value_t& other) const{
		return _type == other._type && _bool == other._bool && _int == other._int && _float == other._float && _string == other._string && _function_id == other._function_id && _data_type == other._data_type;
	}
	string plain_value_to_string() const {
		if(_type == "null"){
			return "<null>";
		}
		else if(_type == "bool"){
			return _bool ? "true" : "false";
		}
		else if(_type == "int"){
			char temp[200 + 1];//??? Use C++ function instead.
			sprintf(temp, "%d", _int);
			return string(temp);
		}
		else if(_type == "float"){
			char temp[200 + 1];//??? Use C++ function instead.
			sprintf(temp, "%f", _float);
			return string(temp);
		}
		else if(_type == "string"){
			return _string;
		}
		else if(_type == "function_id"){
			return _function_id;
		}
		else if(_type == "value_type"){
			return _data_type.to_string();
		}
		else{
			return "???";
		}
	}

	string value_and_type_to_string() const {
		return "<" + _type.to_string() + "> " + plain_value_to_string();
	}


	const data_type_t _type;

	const bool _bool = false;
	const int _int = 0;
	const float _float = 0.0f;
	const string _string = "";
	const string _function_id = "";
	const data_type_t _data_type;
};




//////////////////////////////////////////////////		AST types



struct statement_t;
struct expression_t;
struct math_operation_t;
struct math_operation1_t;

struct arg_t {
	bool operator==(const arg_t& other) const{
		return _type == other._type && _identifier == other._identifier;
	}

	data_type_t _type;
	string _identifier;
};

struct function_body_t {
	bool operator==(const function_body_t& other) const{
		return _statements == other._statements;
	}

	vector<statement_t> _statements;
};

struct make_function_expression_t {
	bool operator==(const make_function_expression_t& other) const{
		return _return_type == other._return_type && _args == other._args && _body == other._body;
	}

	data_type_t _return_type;
	vector<arg_t> _args;
	function_body_t _body;
};

struct constant_expression_t {
	bool operator==(const constant_expression_t& other) const{
		return _constant == other._constant;
	}

	value_t _constant;
};

struct math_operation_t {
	bool operator==(const math_operation_t& other) const;


	enum operation {
		add = 10,
		subtract,
		multiply,
		divide
	};

	operation _operation;

	shared_ptr<expression_t> _left;
	shared_ptr<expression_t> _right;
};

struct math_operation1_t {
	bool operator==(const math_operation1_t& other) const;


	enum operation {
		negate = 20
	};

	operation _operation;

	shared_ptr<expression_t> _input;
};


struct expression_t {
	expression_t() :
		_nop(true)
	{
	}

	expression_t(const make_function_expression_t& value) :
		_make_function_expression(make_shared<make_function_expression_t>(value))
	{
	}

	expression_t(const constant_expression_t& value) :
		_constant_expression(make_shared<constant_expression_t>(value))
	{
	}
	expression_t(const math_operation_t& value) :
		_math_operation(make_shared<math_operation_t>(value))
	{
	}
	expression_t(const math_operation1_t& value) :
		_math_operation1(make_shared<math_operation1_t>(value))
	{
	}

	bool operator==(const expression_t& other) const {
		if(_make_function_expression){
			return other._make_function_expression && *_make_function_expression == *other._make_function_expression;
		}
		else if(_constant_expression){
			return other._constant_expression && *_constant_expression == *other._constant_expression;
		}
		else if(_math_operation){
			return other._math_operation && *_math_operation == *other._math_operation;
		}
		else if(_math_operation1){
			return other._math_operation1 && *_math_operation1 == *other._math_operation1;
		}
		else{
			QUARK_ASSERT(false);
			return false;
		}
	}

	shared_ptr<make_function_expression_t> _make_function_expression;
	shared_ptr<constant_expression_t> _constant_expression;
	shared_ptr<math_operation_t> _math_operation;
	shared_ptr<math_operation1_t> _math_operation1;

	bool _nop;
};


bool math_operation_t::operator==(const math_operation_t& other) const {
	return _operation == other._operation && *_left == *other._left && *_right == *other._right;
}
bool math_operation1_t::operator==(const math_operation1_t& other) const {
	return _operation == other._operation && *_input == *other._input;
}


expression_t make__make_function_expression(const make_function_expression_t& value){
	return expression_t(value);
}
expression_t make_constant_expression(const constant_expression_t& value){
	return expression_t(value);
}
expression_t make_constant_expression(const int value){
	return expression_t(constant_expression_t{ value_t{value} });
}
expression_t make_constant_expression(const float value){
	return expression_t(constant_expression_t{ value_t{value} });
}
expression_t make_constant_expression(const string& value){
	return expression_t(constant_expression_t{ value_t{value} });
}
expression_t make_math_operation(const math_operation_t& value){
	return expression_t(value);
}
expression_t make_math_operation(math_operation_t::operation op, const expression_t& left, const expression_t& right){
	return expression_t(math_operation_t{op, make_shared<expression_t>(left), make_shared<expression_t>(right)});
}

expression_t make_math_operation1(math_operation1_t::operation op, const expression_t& input){
	return expression_t(math_operation1_t{op, make_shared<expression_t>(input) });
}



QUARK_UNIT_TEST("", "math_operation_t==()", "", ""){
	const auto a = make_math_operation(math_operation_t::add, make_constant_expression(3), make_constant_expression(4));
	const auto b = make_math_operation(math_operation_t::add, make_constant_expression(3), make_constant_expression(4));
	QUARK_TEST_VERIFY(a == b);
}



struct bind_global_constant_t {
	bool operator==(const bind_global_constant_t& other) const {
		return _global_identifier == other._global_identifier && _expression == other._expression;
	}

	string _global_identifier;
	expression_t _expression;
};

struct return_statement_t {
	bool operator==(const return_statement_t& other) const {
		return _expression == other._expression;
	}

	expression_t _expression;
};

struct statement_t {
	statement_t(const bind_global_constant_t& value) :
		_bind_global_constant(make_shared<bind_global_constant_t>(value))
	{
	}

	statement_t(const return_statement_t& value) :
		_return_statement(make_shared<return_statement_t>(value))
	{
	}

	bool operator==(const statement_t& other) const {
		if(_bind_global_constant){
			return other._bind_global_constant && *_bind_global_constant == *other._bind_global_constant;
		}
		else if(_return_statement){
			return other._return_statement && *_return_statement == *other._return_statement;
		}
		else{
			QUARK_ASSERT(false);
			return false;
		}
	}

	shared_ptr<bind_global_constant_t> _bind_global_constant;
	shared_ptr<return_statement_t> _return_statement;
};

statement_t make__bind_global_constant(const bind_global_constant_t& value){
	return statement_t(value);
}

statement_t make__return_statement(const return_statement_t& value){
	return statement_t(value);
}



struct program_t {
	vector<statement_t> _top_level_statements;
};




//////////////////////////////////////////////////		VISITOR




struct visitor_i {
	virtual ~visitor_i(){};

	virtual void visitor_interface__on_math_operation(const math_operation_t& e) = 0;
	virtual void visitor_interface__on_make_function_expression(const make_function_expression_t& e) = 0;

	virtual void visitor_interface__on_bind_global_constant_statement(const bind_global_constant_t& s) = 0;
	virtual void visitor_interface__on_return_statement(const return_statement_t& s) = 0;
};

void visit_statement(const statement_t& s, visitor_i& visitor){
	if(s._bind_global_constant){
		visitor.visitor_interface__on_bind_global_constant_statement(*s._bind_global_constant);
	}
	else if(s._return_statement){
		visitor.visitor_interface__on_return_statement(*s._return_statement);
	}
	else{
		QUARK_ASSERT(false);
	}
}


void visit_program(const program_t& program, visitor_i& visitor){
	for(const auto i: program._top_level_statements){
		visit_statement(i, visitor);
	}
}



//////////////////////////////////////////////////		TRACE

//	### Instead of just tracing, generate + trace the parse tree as JSON for easy tooling and debugging.

void trace(const arg_t& arg);
void trace(const expression_t& e);
void trace(const statement_t& s);
void trace(const expression_t& e);
void trace(const function_body_t& body);
void trace(const data_type_t& v);
void trace(const constant_expression_t& e);
void trace(const make_function_expression_t& e);
void trace(const math_operation_t& e);
void trace(const math_operation1_t& e);

void trace(const program_t& program);




string operation_to_string(const math_operation_t::operation& op){
	if(op == math_operation_t::add){
		return "add";
	}
	else if(op == math_operation_t::subtract){
		return "subtract";
	}
	else if(op == math_operation_t::multiply){
		return "multiply";
	}
	else if(op == math_operation_t::divide){
		return "divide";
	}
	else{
		QUARK_ASSERT(false);
	}
}

string operation_to_string(const math_operation1_t::operation& op){
	if(op == math_operation1_t::negate){
		return "negate";
	}
	else{
		QUARK_ASSERT(false);
	}
}


void trace(const arg_t& arg){
	QUARK_TRACE("<arg_t> data type: <" + arg._type.to_string() + "> indentifier: \"" + arg._identifier + "\"");
}

template<typename T> void trace_vec(const string& title, const vector<T>& v){
	QUARK_SCOPED_TRACE(title);
	for(auto i: v){
		trace(i);
	}
}

void trace(const expression_t& e);

void trace(const statement_t& s){
	if(s._bind_global_constant){
		const auto s2 = s._bind_global_constant;
		string t = "bind_global_constant_t: \"" + s2->_global_identifier + "\"";
		QUARK_SCOPED_TRACE(t);
		trace(s2->_expression);
	}
	else if(s._return_statement){
		const auto s2 = s._return_statement;
		QUARK_SCOPED_TRACE("return_statement_t");
		trace(s2->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

void trace(const expression_t& e){
	if(e._constant_expression){
		trace(*e._constant_expression);
	}
	else if(e._make_function_expression){
		const make_function_expression_t& temp = *e._make_function_expression;
		trace(temp);
	}
	else if(e._math_operation){
		trace(*e._math_operation);
	}
	else if(e._math_operation1){
		trace(*e._math_operation1);
	}
	else if(e._nop){
		QUARK_TRACE("NOP");
	}
	else{
		QUARK_ASSERT(false);
	}
}

void trace(const function_body_t& body){
	QUARK_SCOPED_TRACE("function_body_t");
	trace_vec("Statements:", body._statements);
}

void trace(const data_type_t& v){
	QUARK_TRACE("data_type_t <" + v.to_string() + ">");
}

void trace(const constant_expression_t& e){
	QUARK_TRACE("constant_expression_t: " + e._constant.value_and_type_to_string());
}

void trace(const make_function_expression_t& e){
	QUARK_SCOPED_TRACE("make_function_expression_t");

	{
		QUARK_SCOPED_TRACE("return");
		trace(e._return_type);
	}
	{
		trace_vec("arguments", e._args);
	}
	{
		QUARK_SCOPED_TRACE("body");
		trace(e._body);
	}
}

void trace(const math_operation_t& e){
	string s = "math_operation_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._left);
	trace(*e._right);
}
void trace(const math_operation1_t& e){
	string s = "math_operation1_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._input);
}


void trace(const program_t& program){
	QUARK_SCOPED_TRACE("program");

	for(const auto i: program._top_level_statements){
		trace(i);
	}
}



//////////////////////////////////////////////////		Syntax-specific reading

/*
	These functions knows about the Floyd syntax.
*/


bool is_string_valid_type(const string& s){
	return s == "int" || s == "bool" || s == "string" || s == "float";
}

pair<data_type_t, string> read_required_type(const string& s){
	const seq type_pos = get_type(s);
	if(!is_string_valid_type(type_pos.first)){
		throw std::runtime_error("expected type");
	}
	const auto type = resolve_type(type_pos.first);
	return { type, type_pos.second };
}

//	Get identifier (name of a defined function or constant variable name).
pair<string, string> read_required_identifier(const string& s){
	const seq type_pos = get_identifier(s);
	if(type_pos.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	const string identifier = type_pos.first;
	return { identifier, type_pos.second };
}

/*
	()
	(int a)
	(int x, int y)
*/
vector<arg_t> parse_function_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<arg_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = get_type(str);
		const auto arg_name = get_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ resolve_type(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_function_arguments("()") == vector<arg_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	//### include the ( and )!!!"
	const auto r = parse_function_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<arg_t>{
		{ resolve_type("int"), "x" },
		{ resolve_type("string"), "y" },
		{ resolve_type("float"), "z" }
	}
	));
}






pair<expression_t, string> ParseSummands(const string& s, int depth);


expression_t negate_expression(const expression_t& e){
	//	Shortcut: directly negate numeric constants.
	if(e._constant_expression){
		const value_t& value = e._constant_expression->_constant;
		if(value._type == resolve_type("int")){
			return make_constant_expression(constant_expression_t{-value._int});
		}
		else if(value._type == resolve_type("float")){
			return make_constant_expression(constant_expression_t{-value._float});
		}
	}

	return make_math_operation1(math_operation1_t::negate, e);
}

float parse_float(const string& pos){
	size_t end = -1;
	auto res = std::stof(pos, &end);
	if(isnan(res) || end == 0){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return res;
}


// Parse a constant or an expression in parenthesis
pair<expression_t, string> ParseAtom(const string& s, int depth) {
	string pos = skip_whitespace(s);

	//	Handle the sign before parenthesis (or before number)
	auto negative = false;
	while(pos.size() > 0 && (is_whitespace(pos[0]) || (pos[0] == '-' || pos[0] == '+'))){
		if(pos[0] == '-') {
			negative = !negative;
		}
		pos = pos.substr(1);
	}

	// Check if there is parenthesis
	if(pos.size() > 0 && pos[0] == '(') {
		pos = pos.substr(1);

		auto res = ParseSummands(pos, depth);
		pos = skip_whitespace(res.second);
		if(!(res.second.size() > 0 && res.second[0] == ')')) {
			// Unmatched opening parenthesis
			throw std::runtime_error("EEE_PARENTHESIS");
		}
		pos = pos.substr(1);
		const auto expr = res.first;
		return { negative ? negate_expression(expr) : expr, pos };
	}

	//	Parse constant / function call.
	if(pos.size() > 0){
		//	" => string constant.
		if(peek_string(pos, "\"")){
			pos = pos.substr(1);
			const auto string_constant_pos = read_until(pos, "\"");

			pos = string_constant_pos.second;
			pos = pos.substr(1);
			return { constant_expression_t{ string_constant_pos.first }, pos };
		}

		// [0-9] and .  => numeric constant.
		else if((number_chars + ".").find(pos[0]) != string::npos){
			const auto number_pos = read_while(pos, number_chars + ".");
			if(number_pos.first.empty()){
				throw std::runtime_error("EEE_WRONG_CHAR");
			}

			//	If it contains a "." its a float, else an int.
			if(number_pos.first.find('.') != string::npos){
				pos = pos.substr(number_pos.first.size());

				float value = parse_float(number_pos.first);
				float result_number = negative ? -value : value;
				return { constant_expression_t{result_number}, pos };
			}
			else{
				pos = pos.substr(number_pos.first.size());

				int value = atoi(number_pos.first.c_str());
				int result_number = negative ? -value : value;
				return { constant_expression_t{result_number}, pos };
			}
		}

		//	Start of idenditifer: can be variable access or function call.
		else if(identifier_chars.find(pos[0]) != string::npos){
			QUARK_ASSERT(false);
		}
		else{
			throw std::runtime_error("EEE_WRONG_CHAR");
		}
	}

	throw std::runtime_error("Expected number");
}

QUARK_UNIT_TEST("", "ParseAtom", "", ""){
	QUARK_TEST_VERIFY((ParseAtom("0.0", 0) == pair<expression_t, string>(constant_expression_t{ 0.0f }, "")));
	QUARK_TEST_VERIFY((ParseAtom("9.0", 0) == pair<expression_t, string>(constant_expression_t{ 9.0f }, "")));
	QUARK_TEST_VERIFY((ParseAtom("12345.0", 0) == pair<expression_t, string>(constant_expression_t{ 12345.0f }, "")));

	QUARK_TEST_VERIFY((ParseAtom("10.0", 0) == pair<expression_t, string>(constant_expression_t{ 10.0f }, "")));
	QUARK_TEST_VERIFY((ParseAtom("-10.0", 0) == pair<expression_t, string>(constant_expression_t{ -10.0f }, "")));
	QUARK_TEST_VERIFY((ParseAtom("+10.0", 0) == pair<expression_t, string>(constant_expression_t{ 10.0f }, "")));

	QUARK_TEST_VERIFY((ParseAtom("4.0+", 0) == pair<expression_t, string>(constant_expression_t{ 4.0f }, "+")));


	QUARK_TEST_VERIFY((ParseAtom("\"hello\"", 0) == pair<expression_t, string>(constant_expression_t{ "hello" }, "")));
}

pair<expression_t, string> ParseFactors(const string& s, int depth) {
	const auto num1_pos = ParseAtom(s, depth);
	auto result_expression = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	while(!pos.empty() && (pos[0] == '*' || pos[0] == '/')){
		const auto op_pos = read_char(pos);
		const auto expression2_pos = ParseAtom(op_pos.second, depth);
		if(op_pos.first == '/') {
			result_expression = make_math_operation(math_operation_t::divide, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation(math_operation_t::multiply, result_expression, expression2_pos.first);
		}
		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

pair<expression_t, string> ParseSummands(const string& s, int depth) {
	const auto num1_pos = ParseFactors(s, depth);
	auto result_expression = num1_pos.first;
	string pos = num1_pos.second;
	while(!pos.empty() && (pos[0] == '-' || pos[0] == '+')) {
		const auto op_pos = read_char(pos);

		const auto expression2_pos = ParseFactors(op_pos.second, depth);
		if(op_pos.first == '-'){
			result_expression = make_math_operation(math_operation_t::subtract, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation(math_operation_t::add, result_expression, expression2_pos.first);
		}

		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

expression_t evaluate2(string expression){
	auto result = ParseSummands(expression, 0);

	// Now, expr should point to '\0', and _paren_count should be zero
//	if(result.second._paren_count != 0 || result.second._tokens[result.second._pos] == ')') {
//		throw "EEE_PARENTHESIS";
//	}

	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	trace("Expression: \"" + expression + "\"");
	trace(result.first);
	return result.first;
}

bool compare_float_approx(float value, float expected){
	float diff = static_cast<float>(fabs(value - expected));
	return diff < 0.00001;
}


expression_t evaluate_constants(const expression_t& e){
	if(e._constant_expression){
		return e;
	}
	else if(e._math_operation){
		const auto e2 = *e._math_operation;
		const auto left = evaluate_constants(*e2._left);
		const auto right = evaluate_constants(*e2._right);

		//	Both left and right are constant, replace the math_operation with a constant!
		if(left._constant_expression && right._constant_expression){
			const auto left_value = left._constant_expression->_constant;
			const auto right_value = right._constant_expression->_constant;
			if(left_value._type == resolve_type("int") && right_value._type == resolve_type("int")){
				if(e2._operation == math_operation_t::add){
					return make_constant_expression(left_value._int + right_value._int);
				}
				else if(e2._operation == math_operation_t::subtract){
					return make_constant_expression(left_value._int - right_value._int);
				}
				else if(e2._operation == math_operation_t::multiply){
					return make_constant_expression(left_value._int * right_value._int);
				}
				else if(e2._operation == math_operation_t::divide){
					if(right_value._int == 0){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant_expression(left_value._int / right_value._int);
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value._type == resolve_type("float") && right_value._type == resolve_type("float")){
				if(e2._operation == math_operation_t::add){
					return make_constant_expression(left_value._float + right_value._float);
				}
				else if(e2._operation == math_operation_t::subtract){
					return make_constant_expression(left_value._float - right_value._float);
				}
				else if(e2._operation == math_operation_t::multiply){
					return make_constant_expression(left_value._float * right_value._float);
				}
				else if(e2._operation == math_operation_t::divide){
					if(right_value._float == 0.0f){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant_expression(left_value._float / right_value._float);
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value._type == resolve_type("string") && right_value._type == resolve_type("string")){
				if(e2._operation == math_operation_t::add){
					return make_constant_expression(left_value._string + right_value._string);
				}
				else{
					throw std::runtime_error("Arithmetics failed.");
				}
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation(e2._operation, left, right);
		}
	}
	else if(e._math_operation1){
		const auto e2 = *e._math_operation1;
		const auto input = evaluate_constants(*e2._input);

		//	Replace the with a constant!
		if(input._constant_expression){
			const auto value = input._constant_expression->_constant;
			if(value._type == resolve_type("int")){
				if(e2._operation == math_operation1_t::negate){
					return make_constant_expression(-value._int);
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value._type == resolve_type("float")){
				if(e2._operation == math_operation1_t::negate){
					return make_constant_expression(-value._float);
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value._type == resolve_type("string")){
				throw std::runtime_error("Arithmetics failed.");
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation1(e2._operation, input);
		}
	}
	else if(e._nop){
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}

expression_t evaluate(string expression){
	const auto e = evaluate2(expression);
	return evaluate_constants(e);
}


//??? Add tests for strings and floats.
QUARK_UNIT_TEST("", "evaluate()", "", "") {
	// Some simple expressions
	QUARK_TEST_VERIFY(evaluate("1234") == make_constant_expression(1234));
	QUARK_TEST_VERIFY(evaluate("1+2") == make_constant_expression(3));
	QUARK_TEST_VERIFY(evaluate("1+2+3") == make_constant_expression(6));
	QUARK_TEST_VERIFY(evaluate("3*4") == make_constant_expression(12));
	QUARK_TEST_VERIFY(evaluate("3*4*5") == make_constant_expression(60));
	QUARK_TEST_VERIFY(evaluate("1+2*3") == make_constant_expression(7));

	// Parenthesis
	QUARK_TEST_VERIFY(evaluate("5*(4+4+1)") == make_constant_expression(45));
	QUARK_TEST_VERIFY(evaluate("5*(2*(1+3)+1)") == make_constant_expression(45));
	QUARK_TEST_VERIFY(evaluate("5*((1+3)*2+1)") == make_constant_expression(45));

	// Spaces
	QUARK_TEST_VERIFY(evaluate(" 5 * ((1 + 3) * 2 + 1) ") == make_constant_expression(45));
	QUARK_TEST_VERIFY(evaluate(" 5 - 2 * ( 3 ) ") == make_constant_expression(-1));
	QUARK_TEST_VERIFY(evaluate(" 5 - 2 * ( ( 4 )  - 1 ) ") == make_constant_expression(-1));

	// Sign before parenthesis
	QUARK_TEST_VERIFY(evaluate("-(2+1)*4") == make_constant_expression(-12));
	QUARK_TEST_VERIFY(evaluate("-4*(2+1)") == make_constant_expression(-12));
	
	// Fractional numbers
	QUARK_TEST_VERIFY(compare_float_approx(evaluate("5.5/5.0")._constant_expression->_constant._float, 1.1f));
//	QUARK_TEST_VERIFY(evaluate("1/5e10") == 2e-11);
	QUARK_TEST_VERIFY(compare_float_approx(evaluate("(4.0-3.0)/(4.0*4.0)")._constant_expression->_constant._float, 0.0625f));
	QUARK_TEST_VERIFY(evaluate("1.0/2.0/2.0") == make_constant_expression(0.25f));
	QUARK_TEST_VERIFY(evaluate("0.25 * .5 * 0.5") == make_constant_expression(0.0625f));
	QUARK_TEST_VERIFY(evaluate(".25 / 2.0 * .5") == make_constant_expression(0.0625f));
	
	// Repeated operators
	QUARK_TEST_VERIFY(evaluate("1+-2") == make_constant_expression(-1));
	QUARK_TEST_VERIFY(evaluate("--2") == make_constant_expression(2));
	QUARK_TEST_VERIFY(evaluate("2---2") == make_constant_expression(0));
	QUARK_TEST_VERIFY(evaluate("2-+-2") == make_constant_expression(4));


	// === Errors ===

	if(true){
		//////////////////////////		Parenthesis error
		try{
			evaluate("5*((1+3)*2+1");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_PARENTHESIS");
		}

		try{
			evaluate("5*((1+3)*2)+1)");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Repeated operators (wrong)
		try{
			evaluate("5*/2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		//////////////////////////		Wrong position of an operator
		try{
			evaluate("*2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		try{
			evaluate("2+");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		try{
			evaluate("2*");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Division by zero

		try{
			evaluate("2/0");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			evaluate("3+1/(5-5)+4");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			evaluate("2/");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Invalid characters
		try{
			evaluate("~5");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}
		try{
			evaluate("5x");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Multiply errors
			//	Multiple errors not possible/relevant now that we use exceptions for errors.
	/*
		//////////////////////////		Only one error will be detected (in this case, the last one)
		QUARK_TEST_VERIFY(evaluate("3+1/0+4$") == EEE_WRONG_CHAR);

		QUARK_TEST_VERIFY(evaluate("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

		// ...or the first one
		QUARK_TEST_VERIFY(evaluate("q+1/0)") == EEE_WRONG_CHAR);
		QUARK_TEST_VERIFY(evaluate("+1/0)") == EEE_PARENTHESIS);
		QUARK_TEST_VERIFY(evaluate("+1/0") == EEE_DIVIDE_BY_ZERO);
	*/

		// An emtpy string
		try{
			evaluate("");
			QUARK_TEST_VERIFY(false);
		}
		catch(...){
//			QUARK_TEST_VERIFY(error == "EEE_WRONG_CHAR");
		}
	}
}









/*
	### Add variable as sources (arguments and locals and globals).


	Example input:
		0
		3
		()
		(3)
		(1 + 2) * 3
		"test"
		"test number: " +

		x
		x + y

		f()
		f(10, 122)

		(my_fun1("hello, 3) + 4) * my_fun2(10))
*/
expression_t parse_expression(const string expression){
	if(expression.empty()){
		return expression_t();
	}

	const auto result = evaluate(expression);
	return result;
}


QUARK_UNIT_TEST("", "parse_expression", "", ""){
//	QUARK_TEST_VERIFY((parse_expression("")._nop));

//	QUARK_TEST_VERIFY((parse_expression("0")._constant_expression->_constant == value_t(0)));
//	QUARK_TEST_VERIFY((parse_expression("134")._constant_expression->_constant == value_t(134)));
//	QUARK_TEST_VERIFY((parse_expression("10 + 3")._constant_expression->_constant == value_t(13)));

//	QUARK_TEST_VERIFY((parse_expression("()")._nop));
}








/*
	### Split-out parse_statement().
	### Add struct {}
	### Add variables
	### Add local functions

	{}

	{
		return 3;
	}

	{
		return 3 + 4;
	}
	{
		return f(3, 4) + 2;
	}


	//	Example: binding constants to constants, result of function calls and math operations.
	{
		int a = 10;
		int b = f(a);
		int c = a + b;
		return c;
	}

	//	Local scope.
	{
		{
			int a = 10;
		}
	}
	{
		struct point2d {
			int _x;
			int _y;
		}
	}

	{
		int my_func(string a, string b){
			int c = a + b;
			return c;
		}
	}
*/
function_body_t parse_function_body(const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	vector<statement_t> statements;

	string pos = body_str;
	while(!pos.empty()){
		const auto token_pos = read_until(pos, whitespace_chars);
		if(token_pos.first == "return"){
			const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
			const auto expression = parse_expression(expression_pos.first);
			const auto statement = statement_t(return_statement_t{ expression });
			statements.push_back(statement);

			//	Skip trailing ";".
			pos = skip_whitespace(expression_pos.second.substr(1));
		}
		else if(resolve_type(token_pos.first) != ""){
//			const auto type = resolve_type(token_pos.first);
			return {};
		}
		else{
			throw std::runtime_error("syntax error");
		}
	}
	const auto result = function_body_t{ statements };
	trace(result);
	return result;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_function_body("{}")._statements.empty()));
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body("{return 3;}")._statements.size() == 1);
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body("{\n\treturn 3;\n}")._statements.size() == 1);
}




/*
	Just defines an unnamed function - it has no name.

	Named function:

	int myfunc(string a, int b){
		...
		return b + 1;
	}


	LATER:
	Lambda:

	int myfunc(string a){
		() => {
		}
	}
*/
pair<pair<string, make_function_expression_t>, string> read_function(const string& pos){
	const auto type_pos = read_required_type(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(identifier_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	//### create pair<string, string> get_required_balanced(string, '(', ')') that returns empty string if not found.

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_function_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);
	const auto body = parse_function_body(body_pos.first);
	const auto a = make_function_expression_t{ type_pos.first, args, function_body_t{} };
//	trace_node(a);

	return { { identifier_pos.first, a }, body_pos.second };
}

QUARK_UNIT_TEST("", "read_function()", "", ""){
	try{
		const auto result = read_function("int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "read_function()", "", ""){
	const auto result = read_function("int f(){}");
	QUARK_TEST_VERIFY(result.first.first == "f");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY(result.first.second._args.empty());
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TEST("", "read_function()", "Test many arguments of different types", ""){
	const auto result = read_function("int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ resolve_type("string"), "a" },
		{ resolve_type("float"), "barry" },
		{ resolve_type("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "read_function()", "Test exteme whitespaces", ""){
	const auto result = read_function("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ resolve_type("string"), "a" },
		{ resolve_type("float"), "barry" },
		{ resolve_type("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/





//////////////////////////////////////////////////		read_toplevel_statement()


//	make_function_expression,
//	??? also allow defining types and global constants.
pair<statement_t, string> read_toplevel_statement(const string& pos){
	const auto type_pos = read_required_type(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);


	pair<pair<string, make_function_expression_t>, string> function = read_function(pos);
	const auto bind = bind_global_constant_t{ function.first.first, function.first.second };
	return { bind, function.second };
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	try{
		const auto result = read_toplevel_statement("int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	const auto result = read_toplevel_statement("int f(){}");
	QUARK_TEST_VERIFY(result.first._bind_global_constant);
	QUARK_TEST_VERIFY(result.first._bind_global_constant->_global_identifier == "f");
	QUARK_TEST_VERIFY(result.first._bind_global_constant->_expression._make_function_expression);

	const auto make_function_expression = result.first._bind_global_constant->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY(make_function_expression->_args.empty());
	QUARK_TEST_VERIFY(make_function_expression->_body._statements.empty());

	QUARK_TEST_VERIFY(result.second == "");
}


//////////////////////////////////////////////////		program_to_ast()



program_t program_to_ast(string program){
	vector<statement_t> top_level_statements;
	auto pos = program;
	pos = skip_whitespace(pos);
	while(!pos.empty()){
		const auto statement_pos = read_toplevel_statement(pos);
		top_level_statements.push_back(statement_pos.first);
		pos = skip_whitespace(statement_pos.second);
	}

	const program_t result{ top_level_statements };
	trace(result);

	return result;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

	const auto result = program_to_ast(kProgram1);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_global_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression);

	const auto make_function_expression = result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{ arg_t{ data_type_t::make_type("string"), "args" }}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_global_identifier == "f");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression);

	const auto make_function_expression = result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("int"), "x" },
		arg_t{ data_type_t::make_type("int"), "y" },
		arg_t{ data_type_t::make_type("string"), "z" }
	}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"
	;
	QUARK_TRACE(kProgram);

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 2);

	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_global_identifier == "hello");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression);

	const auto hello = result._top_level_statements[0]._bind_global_constant->_expression._make_function_expression;
	QUARK_TEST_VERIFY(hello->_return_type == data_type_t::make_type("string"));
	QUARK_TEST_VERIFY((hello->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("int"), "x" },
		arg_t{ data_type_t::make_type("int"), "y" },
		arg_t{ data_type_t::make_type("string"), "z" }
	}));


	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_global_constant);
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_global_constant->_global_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_global_constant->_expression._make_function_expression);

	const auto main = result._top_level_statements[1]._bind_global_constant->_expression._make_function_expression;
	QUARK_TEST_VERIFY(main->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((main->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("string"), "args" }
	}));

}




/*
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"




	string hello(int x, int y, string z){
		return "test abc";
	}

	int main(string args){
		return 3;
	}


{
	"program_as_json" : [
		{
			"_func" : {
				"_prototype" : "string hello(int x, int y, string z)",
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}



{
	"program_as_json" : [
		{
			"_func" : {
				"return": "string",
				"name": "hello",
				"args": [
					{ "type": "int", "name": "x" },
					{ "type": "int", "name": "y" },
					{ "type": "string", "name": "z" },
				],
				"body": [
					[ "assign", "a",
						[
							[ "return",
								[ "*", "a", "y" ]
							]
						],
					[ "return", "test abc" ]
				]
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}




!!! JSON does not support multi-line strings.


*/



const string kProgram2 =
"int main(string args){\n"
"	float test = log(args);\n"
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


QUARK_UNIT_TEST("", "program_to_ast()", "", ""){
	auto r = program_to_ast(kProgram2);
//	QUARK_TEST_VERIFY(r._functions["main"]._return_type == data_type_t::make_type("int"));
	vector<pair<string, string>> a{ { "string", "args" }};
//	QUARK_TEST_VERIFY((r._functions["main"]._args == vector<pair<data_type_t, string>>{ { data_type_t::make_type("string"), "args" }}));
}



//////////////////////////////////////////////////		main()



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
