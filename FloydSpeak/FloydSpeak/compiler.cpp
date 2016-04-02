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


bool peek_required_char(const std::string& s, char ch){
	return s.size() > 0 && s[0] == ch;
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


struct expression_t {
	expression_t(make_function_expression_t value) :
		_make_function_expression(make_shared<make_function_expression_t>(value))
	{
	}

	shared_ptr<make_function_expression_t> _make_function_expression;
};





struct bind_global_constant {
	string _global_identifier;
	expression_t _expression;
};

struct statement_t {
	statement_t(bind_global_constant value) :
		_bind_global_constant(make_shared<bind_global_constant>(value))
	{
	}

	shared_ptr<bind_global_constant> _bind_global_constant;
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

pair<vector<arg_t>, string> read_function_arguments(const string& s){
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
	return { args, str };
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((read_function_arguments("") == pair<vector<arg_t>, string>{ {}, ""}));
}

QUARK_UNIT_TEST("", "", "", ""){
	//### include the ( and )!!!"
	const auto r = read_function_arguments("int x, string y, float z");
	QUARK_TEST_VERIFY((r == pair<vector<arg_t>, string>{
		{
			{ resolve_type("int"), "x" },
			{ resolve_type("string"), "y" },
			{ resolve_type("float"), "z" }
		},
		""
		}
	));
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

	if(!peek_required_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	//### crete pair<string, string> get_required_balanced(string, '(', ')') that returns empty string if not found.

	const auto arg_list_pos = get_balanced(rest);
	const auto arg_list_chars(arg_list_pos.first.substr(1, arg_list_pos.first.length() - 2));
	const auto arg_list2 = read_function_arguments(arg_list_chars);

	if(!peek_required_char(arg_list_pos.second, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(arg_list_pos.second);
	const auto a = make_function_expression_t{ type_pos.first, arg_list2.first, function_body_t{} };
//	trace_node(a);

	return { { identifier_pos.first, a }, body_pos.second };
}



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
