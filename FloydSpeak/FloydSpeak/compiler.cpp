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





//////////////////////////////////////////////////		Text parsing primitives



const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";
const string type_chars = identifier_chars + brackets;



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

	return seq(
		s.substr(0, pos),
		s.substr(pos)
	);
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
	Skip leading whitespace, get string while type-char.
*/
seq get_type(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_until(a, type_chars);
	return b;
}

/*
	Skip leading whitespace, get string while identifier char.
*/
seq get_identifier(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_until(a, identifier_chars);
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



//////////////////////////////////////////////////		value_t



struct value_type_t {
	public: static value_type_t make_type(string s){
		value_type_t result;
		result._type_magic = s;
		return result;
	}

	public: bool operator==(const value_type_t other) const{
		return other._type_magic == _type_magic;
	}
	value_type_t() : _type_magic(""){}
	value_type_t(const char s[]) :
		_type_magic(s)
	{
	}
	value_type_t(string s) :
		_type_magic(s)
	{
	}

	/*
		### Use the same compatible technique / API  in the runtime and in the language itself!
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

value_type_t resolve_type(string node_type){
	if(node_type == "int"){
		return value_type_t::make_type("int");
	}
	else if(node_type == "string"){
		return value_type_t::make_type("string");
	}
	else if(node_type == "float"){
		return value_type_t::make_type("float");
	}
	else if(node_type == "function"){
		return value_type_t::make_type("function");
	}
	else if(node_type == "value_type"){
		return value_type_t::make_type("value_type");
	}
	else{
		QUARK_ASSERT(false);
		return "";
	}
}

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

	public: value_t(const value_type_t& s) :
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


	const value_type_t _type;

	const bool _bool = false;
	const int _int = 0;
	const float _float = 0.0f;
	const string _string = "";
	const string _function_id = "";
	const string _value_type__type_magic;
};





//////////////////////////////////////////////////		node1




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

	arg_list,
	arg,

	value_type,
	identifier

	/*
		(a + b)
		(a - b)
		(a * b)
		(a / b)
		(a % b)
	*/
//	arithmetic_expression
};

struct node1_t {
	node1_t(node1_type type, const std::vector<node1_t>& children) :
		_type(type),
		_children(children)
	{
	}
	node1_t(node1_type type, const value_t& value) :
		_type(type),
		_value(value)
	{
	}

	public: bool operator==(const node1_t& other) const{
		return _type == other._type && _children == other._children && _value == other._value;
	}


	node1_type _type;

	//	Contains childen OR a value.
	vector<node1_t> _children;
	value_t _value;
};

node1_t make_node1(node1_type type, const std::vector<node1_t>& children){
	return node1_t(type, children);
}

node1_t make_node1(node1_type type, const value_t& value){
	return node1_t(type, value);
}

void trace_node_int(const node1_t& node);

void trace_children(const node1_t& node){
	for(auto i: node._children){
		trace_node_int(i);
	}
}

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

void trace_node_int(const node1_t& node){
	const auto type_name = node_type_to_string(node._type);
	if(!node._children.empty()){
		QUARK_SCOPED_TRACE("[" + type_name + "]");
		trace_children(node);
	}
	if(!(node._value._type == value_type_t("null"))){
		QUARK_TRACE_SS("[" + type_name + "] = [" + node._value.value_to_string() + "]");
	}
}

void trace_node(const node1_t& node){
	QUARK_SCOPED_TRACE("Nodes:");
	trace_node_int(node);
}



//////////////////////////////////////////////////		pass1



struct pass1 {
	pass1(const node1_t& program_body_node) :
		_program_body_node(program_body_node)
	{
	}
	node1_t _program_body_node;
};



/*
	Examples:
		""
		"int x"
		"int x, string y, float z"

	[arg_list]
		[arg0]
			[value_type]
			[name]
		[arg1]
			[value_type]
			[name]
*/

node1_t parse_arg_list(const string& s){
	vector<node1_t> arg_nodes;

	auto pos = seq("", s);
	while(!pos.second.empty()){
		const seq arg_type = get_type(pos.second);
		const seq arg_name = get_identifier(arg_type.second);
		const seq optional_comma = read_while(arg_name.second, ",");

		const node1_t arg_type_node = make_node1(node1_type::value_type, resolve_type(arg_type.first));
		const node1_t arg_name_node = make_node1(node1_type::identifier, arg_name.first);
		arg_nodes.push_back(make_node1(node1_type::arg, vector<node1_t>{ arg_type_node, arg_name_node }));

		pos = optional_comma;
	}

	const auto result = make_node1(node1_type::arg_list, arg_nodes);
	trace_node(result);
	return result;
}


/*
	[arg_list]
		[arg0]
			[value_type]
			[name]
		[arg1]
			[value_type]
			[name]
*/
QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_arg_list("") == node1_t(node1_type::arg_list, std::vector<node1_t>{})));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wanted = node1_t(node1_type::arg_list, std::vector<node1_t>{
		make_node1(node1_type::arg, vector<node1_t>{
			make_node1(node1_type::value_type, value_type_t::make_type("int")),
			make_node1(node1_type::identifier, value_t(string("x")))
		})
	});
	trace_node(wanted);

	const auto r1 = parse_arg_list("int x");
	QUARK_TEST_VERIFY(r1 == wanted);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wanted = node1_t(node1_type::arg_list, std::vector<node1_t>{
		make_node1(node1_type::arg, vector<node1_t>{
			make_node1(node1_type::value_type, value_type_t::make_type("int")),
			make_node1(node1_type::identifier, value_t(string("x")))
		}),
		make_node1(node1_type::arg, vector<node1_t>{
			make_node1(node1_type::value_type, value_type_t::make_type("string")),
			make_node1(node1_type::identifier, value_t(string("y")))
		}),
		make_node1(node1_type::arg, vector<node1_t>{
			make_node1(node1_type::value_type, value_type_t::make_type("float")),
			make_node1(node1_type::identifier, value_t(string("z")))
		})
	});
	trace_node(wanted);

	const auto r1 = parse_arg_list("int x, string y, float z");
	QUARK_TEST_VERIFY(r1 == wanted);
}


bool is_valid_type(const string& s){
	return s == "int" || s == "bool" || s == "string" || s == "float";
}

pair<value_type_t, string> read_required_type(const string& s){
	const seq type_pos = get_type(s);
	if(!is_valid_type(type_pos.first)){
		throw std::runtime_error("expected type");
	}
	const auto type = resolve_type(type_pos.first);
	return { type, type_pos.second };
}

//	Get identifier (name of a defined function or constant variable name).
pair<value_type_t, string> read_required_identifier(const string& s){
	const seq t2 = get_identifier(s);
	if(t2.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	const string identifier = t2.first;
	return { identifier, type_pos.second };
}

pair<node1_t, string> read_toplevel(const string& pos){
	const auto type_pos = read_required_type(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	//	Skip whitespace.
	const seq t3 = get_next_token(t2.second, whitespace_chars, "");

	//	Is this a function definition?
	if(t3.second.size() > 0 && t3.second[0] == '('){
		/*
			[make_function_expression]
				[value_type] = "int"
				[arg_list]
					[arg0]
						[value_type]
						[name]
					[arg1]
						[value_type]
						[name]
				[body_node]
		*/
		const auto t4 = get_balanced(t3.second);
		const auto function_return = resolve_type(type1);;
		const auto arg_list(t4.first.substr(1, t4.first.length() - 2));
		const auto arg_list_node = parse_arg_list(arg_list);

		const auto function_return_node = make_node1(node1_type::value_type, function_return);

		const auto body_node = make_node1(node1_type::body_node, vector<node1_t>{});
		//??? Also bind it to a global constant.
		const auto r = node1_t(node1_type::make_function_expression, {function_return_node, arg_list_node, body_node});
		trace_node(r);

		top_childen.push_back(r);
	}
	else{
		throw std::runtime_error("expected (");
	}
}

QUARK_UNIT_TEST("", "read_toplevel()", "three arguments", ""){
	const string kInput =
		"int f(int x, int y, string z){\n"
		"	return 3;\n"
		"}\n";

	const auto result = read_toplevel(kInput);
	QUARK_TEST_VERIFY(result.first._type == node1_type::make_function_expression);
	QUARK_TEST_VERIFY(result.second == "");
}


/*
	Makes a tree of all the program. Whitespace removed, all types of parenthesis split to sub-trees.
	No knowledget of language syntax needed to do this.
*/

pass1 compile_pass1(string program){
	vector<node1_t> top_childen;
	const auto start = seq("", program);




	node1_t program_body_node(node1_type::body_node, top_childen);
	trace_node(program_body_node);
	pass1 result(program_body_node);

	return result;
}


QUARK_UNIT_TEST("", "compile_pass1()", "kProgram1", ""){
	QUARK_TEST_VERIFY(compile_pass1(kProgram1)._program_body_node._type == node1_type::body_node);
}


QUARK_UNIT_TEST("", "compile_pass1()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	QUARK_TEST_VERIFY(compile_pass1(kProgram)._program_body_node._type == node1_type::body_node);
}


QUARK_UNIT_TEST("", "compile_pass1()", "two functions", ""){
	const string kProgram =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"
	;

	QUARK_TEST_VERIFY(compile_pass1(kProgram)._program_body_node._type == node1_type::body_node);
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
//	QUARK_TEST_VERIFY(r._functions["main"]._return_type == value_type_t::make_type("int"));
	vector<pair<string, string>> a{ { "string", "args" }};
//	QUARK_TEST_VERIFY((r._functions["main"]._args == vector<pair<value_type_t, string>>{ { value_type_t::make_type("string"), "args" }}));
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
