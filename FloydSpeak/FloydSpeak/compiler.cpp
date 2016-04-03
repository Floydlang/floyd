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
	std::size_t pos;
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





//////////////////////////////////////////////////		Text parsing primitives



const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";
const string type_chars = identifier_chars + brackets;
const string number_chars = "0123456789.";


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






//////////////////////////////////////////////////		data_type_t

/*
	Internal object representing a Floyd data type.
		### Use the same compatible technique / API  in the runtime and in the language itself!
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
	public: string _type_magic;
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
	public: value_t(string s) :
		_type("string"),
		_string(s)
	{
	}

	public: value_t(int value) :
		_type("int"),
		_int(value)
	{
	}

	public: value_t(const data_type_t& s) :
		_type("value_type"),
		_value_type__type_magic(s._type_magic)
	{
	}

	public: bool operator==(const value_t& other) const{
		return _type == other._type && _bool == other._bool && _int == other._int && _float == other._float && _string == other._string && _function_id == other._function_id && _value_type__type_magic == other._value_type__type_magic;
	}
	string value_to_string() const {
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
			return _value_type__type_magic;
		}
		else{
			return "???";
		}
	}


	const data_type_t _type;

	const bool _bool = false;
	const int _int = 0;
	const float _float = 0.0f;
	const string _string = "";
	const string _function_id = "";
	const string _value_type__type_magic;
};




//////////////////////////////////////////////////		AST types



struct statement_t;

struct arg_t {
	bool operator==(const arg_t& other) const{
		return _type == other._type && _identifier == other._identifier;
	}

	data_type_t _type;
	string _identifier;
};

struct function_body_t {
	vector<statement_t> _statements;
};

struct make_function_expression_t {
	data_type_t _return_type;
	vector<arg_t> _args;
	function_body_t _body;
};

struct constant_expression_t {
	value_t _constant;
};


struct expression_t {
	expression_t() :
		_nop(true)
	{
	}

	expression_t(make_function_expression_t value) :
		_make_function_expression(make_shared<make_function_expression_t>(value))
	{
	}

	expression_t(constant_expression_t value) :
		_constant_expression_t(make_shared<constant_expression_t>(value))
	{
	}

	shared_ptr<make_function_expression_t> _make_function_expression;
	shared_ptr<constant_expression_t> _constant_expression_t;

	bool _nop;
};





struct bind_global_constant {
	string _global_identifier;
	expression_t _expression;
};

struct return_value {
	expression_t _expression;
};

struct statement_t {
	statement_t(bind_global_constant value) :
		_bind_global_constant(make_shared<bind_global_constant>(value))
	{
	}

	statement_t(return_value value) :
		_return_value(make_shared<return_value>(value))
	{
	}

	shared_ptr<bind_global_constant> _bind_global_constant;
	shared_ptr<return_value> _return_value;
};



struct program_t {
	vector<statement_t> _top_level_statements;
};

struct visitor_i {
	virtual void visitor_interface__on_make_function_expression(make_function_expression_t& expression) = 0;
};

void visit_program(const program_t& program, visitor_i& visit){
}







//////////////////////////////////////////////////		TEMP





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




//////////////////////////////////////////////////		trace



void trace_node(const arg_t& arg){
}

template<typename T> void trace_vec(const vector<T>& v){
	for(auto i: v){
		trace_node(i);
	}
}

void trace_node(const program_t& v){
}

void trace_node(const statement_t& s){
}

/*

string node_type_to_string(node1_type type){
	if(type == node1_type::body_node){
		return "body_node";
	}
	else if(type == node1_type::return_value_statement){
		return "return_value_statement";
	}
	else if(type == node1_type::define_local_constant_statement){
		return "define_local_constant_statement";
	}
	else if(type == node1_type::define_global_constant_statement){
		return "define_global_constant_statement";
	}
	else if(type == node1_type::make_function_expression){
		return "make_function_expression";
	}
	else if(type == node1_type::expression){
		return "expression";
	}
	else if(type == node1_type::function_call_expression){
		return "function_call_expression";
	}
	else if(type == node1_type::arg_list){
		return "arg_list";
	}
	else if(type == node1_type::arg){
		return "arg";
	}
	else if(type == node1_type::value_type){
		return "value_type";
	}
	else if(type == node1_type::identifier){
		return "identifier";
	}

	else{
		QUARK_ASSERT(false);
	}
}


void trace_node_int(const node1_t& node);

void trace_children(const node1_t& node){
	trace_vec(_children);
}



void trace_node_int(const node1_t& node){
	const auto type_name = node_type_to_string(node._type);
	if(!node._children.empty()){
		QUARK_SCOPED_TRACE("[" + type_name + "]");
		trace_children(node);
	}
	if(!(node._value._type == data_type_t("null"))){
		QUARK_TRACE_SS("[" + type_name + "] = [" + node._value.value_to_string() + "]");
	}
}

void trace_node(const node1_t& node){
	QUARK_SCOPED_TRACE("Nodes:");
	trace_node_int(node);
}
*/





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

	trace_vec(args);
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










/*
	10 + 20 + 30 * (40 + 50 / 60) - 70
	10 + 20 + (30 * (40 + (50 / 60))) - 70
*/

struct expression_i {
	virtual void expression_i__on_open() = 0;
	virtual void expression_i__on_close() = 0;

	virtual void expression_i__on_int_constant(int number) = 0;
	virtual void expression_i__on_float_constant(float number) = 0;
	virtual void expression_i__on_string_constant(const string& s) = 0;
	virtual void expression_i__on_constant_variable(const value_t& value) = 0;
	virtual void expression_i__on_function_call(const value_t& value) = 0;

	virtual void expression_i__on_plus() = 0;
	virtual void expression_i__on_minus() = 0;
	virtual void expression_i__on_multiply() = 0;
	virtual void expression_i__on_divide() = 0;
};


struct open_stacker_t {
	open_stacker_t(expression_i& i) :
		_i(i)
	{
		_i.expression_i__on_open();
	}

	~open_stacker_t(){
		_i.expression_i__on_close();
	}


	expression_i& _i;
};



struct test_expression_i : public expression_i {
	virtual void expression_i__on_open(){
		QUARK_TRACE_SS("OPEN");
		auto r = quark::get_runtime();
		r->runtime_i__add_log_indent(1);
	}
	virtual void expression_i__on_close(){
		auto r = quark::get_runtime();
		r->runtime_i__add_log_indent(-1);
		QUARK_TRACE_SS("CLOSE");
	}

	virtual void expression_i__on_int_constant(int number){
	}
	virtual void expression_i__on_float_constant(float number){
		QUARK_TRACE_SS("float: " << number);
	}
	virtual void expression_i__on_string_constant(const string& s){
	}
	virtual void expression_i__on_constant_variable(const value_t& value){
	}
	virtual void expression_i__on_function_call(const value_t& value){
	}

	virtual void expression_i__on_plus(){
	}
	virtual void expression_i__on_minus(){
	}
	virtual void expression_i__on_multiply(){
	}
	virtual void expression_i__on_divide(){
	}
};






pair<float, string> ParseSummands(const string& s, expression_i& i);



// Parse a number or an expression in parenthesis
pair<float, string> ParseAtom(const string& s, expression_i& i) {
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

		open_stacker_t open(i);
			auto res = ParseSummands(pos, i);
			pos = skip_whitespace(res.second);
			bool close = res.second.size() > 0 && res.second[0] == ')';
			if(!close) {
				// Unmatched opening parenthesis
				throw std::runtime_error("EEE_PARENTHESIS");
			}
			pos = pos.substr(1);
			return { negative ? -res.first : res.first, pos };
	}

	if(pos.size() > 0){
		// It should be a number; convert it to double
		const auto float_string_pos = read_while(pos, number_chars);
		if(float_string_pos.first.empty()){
			throw std::runtime_error("EEE_WRONG_CHAR");
		}

		size_t end = -1;
		auto res = std::stof(float_string_pos.first, &end);
		if(isnan(res) || end == 0){
			throw std::runtime_error("EEE_WRONG_CHAR");
		}
		pos = pos.substr(float_string_pos.first.size());
		float result_number = negative ? -res : res;
		i.expression_i__on_float_constant(result_number);
		return { result_number, pos };
	}

	throw std::runtime_error("Expected number");
}

QUARK_UNIT_TEST("", "ParseAtom", "", ""){
	test_expression_i i;
	QUARK_TEST_VERIFY((ParseAtom("0", i) == pair<float, string>(0.0f, "")));
	QUARK_TEST_VERIFY((ParseAtom("9", i) == pair<float, string>(9.0f, "")));
	QUARK_TEST_VERIFY((ParseAtom("12345", i) == pair<float, string>(12345.0f, "")));
	QUARK_TEST_VERIFY((ParseAtom("10 ", i) == pair<float, string>(10.0f, " ")));
	QUARK_TEST_VERIFY((ParseAtom("-10", i) == pair<float, string>(-10.0f, "")));
	QUARK_TEST_VERIFY((ParseAtom("+10", i) == pair<float, string>(10.0f, "")));

	QUARK_TEST_VERIFY((ParseAtom("4+", i) == pair<float, string>(4.0f, "+")));
}


// Parse multiplication and division
pair<float, string> ParseFactors(const string& s, expression_i& i) {
	const auto num1_pos = ParseAtom(s, i);
	auto value = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	for(;;) {
		if(pos.empty()){
			return { value, "" };
		}
		const char op = pos[0];
		if(op != '/' && op != '*'){
			return { value, pos };
		}
		pos = pos.substr(1);

		const auto num2_pos = ParseAtom(pos, i);
		const auto value2 = num2_pos.first;
		if(op == '/') {
			// Handle division by zero
			if(value2 == 0) {
				throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			value = value / value2;
		}
		else{
			value = value * value2;
		}

		pos = skip_whitespace(num2_pos.second);
	}
}

// Parse addition and subtraction
pair<float, string> ParseSummands(const string& s, expression_i& i) {
	const auto num1_pos = ParseFactors(s, i);
	auto value = num1_pos.first;
	string pos = num1_pos.second;
	for(;;) {
		if(pos.empty()){
			return { value, "" };
		}
		const auto op = pos[0];
		if(op != '-' && op != '+'){
			return { value, pos };
		}
		pos = pos.substr(1);

		const auto num2_pos = ParseFactors(pos, i);
		const auto value2 = num2_pos.first;
		if(op == '-'){
			value = value - value2;
		}
		else{
			value = value + value2;
		}

		pos = skip_whitespace(num2_pos.second);
	}
}

float visit_evaluator(string expression, expression_i& i){
	auto result = ParseSummands(expression, i);

	// Now, expr should point to '\0', and _paren_count should be zero
//	if(result.second._paren_count != 0 || result.second._tokens[result.second._pos] == ')') {
//		throw "EEE_PARENTHESIS";
//	}

	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return result.first;
}

float evaluate(string expression) {
	struct x : public expression_i {
		virtual void expression_i__on_open(){
		}
		virtual void expression_i__on_close(){
		}

		virtual void expression_i__on_int_constant(int number){
		}
		virtual void expression_i__on_float_constant(float number){
		}
		virtual void expression_i__on_string_constant(const string& s){
		}
		virtual void expression_i__on_constant_variable(const value_t& value){
		}
		virtual void expression_i__on_function_call(const value_t& value){
		}

		virtual void expression_i__on_plus(){
		}
		virtual void expression_i__on_minus(){
		}
		virtual void expression_i__on_multiply(){
		}
		virtual void expression_i__on_divide(){
		}
	};

	x i;
	return visit_evaluator(expression, i);
}

bool compare_float_approx(float value, float expected){
	float diff = fabs(value - expected);
	return diff < 0.00001;
}


QUARK_UNIT_TEST("", "visit_evaluator()", "", "") {
	test_expression_i i;

	// Some simple expressions
	QUARK_TEST_VERIFY(visit_evaluator("1234", i) == 1234);
	QUARK_TEST_VERIFY(visit_evaluator("1+2*3", i) == 7);

	// Parenthesis
	QUARK_TEST_VERIFY(visit_evaluator("5*(4+4+1)", i) == 45);
	QUARK_TEST_VERIFY(visit_evaluator("5*(2*(1+3)+1)", i) == 45);
	QUARK_TEST_VERIFY(visit_evaluator("5*((1+3)*2+1)", i) == 45);

	// Spaces
	QUARK_TEST_VERIFY(visit_evaluator(" 5 * ((1 + 3) * 2 + 1) ", i) == 45);
	QUARK_TEST_VERIFY(visit_evaluator(" 5 - 2 * ( 3 ) ", i) == -1);
	QUARK_TEST_VERIFY(visit_evaluator(" 5 - 2 * ( ( 4 )  - 1 ) ", i) == -1);

	// Sign before parenthesis
	QUARK_TEST_VERIFY(visit_evaluator("-(2+1)*4", i) == -12);
	QUARK_TEST_VERIFY(visit_evaluator("-4*(2+1)", i) == -12);
	
	// Fractional numbers
	QUARK_TEST_VERIFY(compare_float_approx(visit_evaluator("5.5/5", i), 1.1f));
//	QUARK_TEST_VERIFY(visit_evaluator("1/5e10", i) == 2e-11);
	QUARK_TEST_VERIFY(compare_float_approx(visit_evaluator("(4-3)/(4*4)", i), 0.0625f));
	QUARK_TEST_VERIFY(visit_evaluator("1/2/2", i) == 0.25);
	QUARK_TEST_VERIFY(visit_evaluator("0.25 * .5 * 0.5", i) == 0.0625f);
	QUARK_TEST_VERIFY(visit_evaluator(".25 / 2 * .5", i) == 0.0625f);
	
	// Repeated operators
	QUARK_TEST_VERIFY(visit_evaluator("1+-2", i) == -1);
	QUARK_TEST_VERIFY(visit_evaluator("--2", i) == 2);
	QUARK_TEST_VERIFY(visit_evaluator("2---2", i) == 0);
	QUARK_TEST_VERIFY(visit_evaluator("2-+-2", i) == 4);

	// === Errors ===

	if(true){
		//////////////////////////		Parenthesis error
		try{
			visit_evaluator("5*((1+3)*2+1", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_PARENTHESIS");
		}

		try{
			visit_evaluator("5*((1+3)*2)+1)", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Repeated operators (wrong)
		try{
			visit_evaluator("5*/2", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		//////////////////////////		Wrong position of an operator
		try{
			visit_evaluator("*2", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		try{
			visit_evaluator("2+", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		try{
			visit_evaluator("2*", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Division by zero

		try{
			visit_evaluator("2/0", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			visit_evaluator("3+1/(5-5)+4", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			visit_evaluator("2/", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Invalid characters
		try{
			visit_evaluator("~5", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}
		try{
			visit_evaluator("5x", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Multiply errors
			//	Multiple errors not possible/relevant now that we use exceptions for errors.
	/*
		//////////////////////////		Only one error will be detected (in this case, the last one)
		QUARK_TEST_VERIFY(visit_evaluator("3+1/0+4$") == EEE_WRONG_CHAR);

		QUARK_TEST_VERIFY(visit_evaluator("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

		// ...or the first one
		QUARK_TEST_VERIFY(visit_evaluator("q+1/0)") == EEE_WRONG_CHAR);
		QUARK_TEST_VERIFY(visit_evaluator("+1/0)") == EEE_PARENTHESIS);
		QUARK_TEST_VERIFY(visit_evaluator("+1/0") == EEE_DIVIDE_BY_ZERO);
	*/

		// An emtpy string
		try{
			visit_evaluator("", i);
			QUARK_TEST_VERIFY(false);
		}
		catch(...){
//			QUARK_TEST_VERIFY(error == "EEE_WRONG_CHAR");
		}
	}
}



























/*
	Add variable as sources (arguments and locals and globals).
*/

/*
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
*/
expression_t parse_expression(const string& s){
//	return expression_t(constant_expression_t());
	return expression_t(constant_expression_t{ value_t(3) });
}

QUARK_UNIT_TEST("", "parse_expression", "", ""){
	QUARK_TEST_VERIFY((parse_expression("()")._constant_expression_t->_constant == value_t(3)));
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
			const auto statement = statement_t(return_value{ expression });
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
	return function_body_t{ statements };
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
	const auto bind = bind_global_constant{ function.first.first, function.first.second };
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
	trace_node(result);

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
