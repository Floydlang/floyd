//
//  parse_expression.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_expression.hpp"


#include "text_parser.h"
#include "parser_statement.hpp"

#include <cmath>


namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


QUARK_UNIT_TEST("", "math_operation2_expr_t==()", "", ""){
	const auto a = make_math_operation2_expr(math_operation2_expr_t::add, make_constant(3), make_constant(4));
	const auto b = make_math_operation2_expr(math_operation2_expr_t::add, make_constant(3), make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}




pair<expression_t, string> parse_summands(const parser_i& parser, const string& s, int depth);


expression_t negate_expression(const expression_t& e){
	//	Shortcut: directly negate numeric constants. This makes parse tree cleaner and is non-lossy.
	if(e._constant){
		const value_t& value = *e._constant;
		if(value.get_type() == make_type_identifier("int")){
			return make_constant(value_t{-value.get_int()});
		}
		else if(value.get_type() == make_type_identifier("float")){
			return make_constant(value_t{-value.get_float()});
		}
	}

	return make_math_operation1(math_operation1_expr_t::negate, e);
}

float parse_float(const string& pos){
	size_t end = -1;
	auto res = std::stof(pos, &end);
	if(isnan(res) || end == 0){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return res;
}



/*
	Constant literal
		3
		3.0
		"three"
	Function call
		f ()
		f(g())
		f(a + "xyz")
		f(a + "xyz", 1000 * 3)
	Variable
		x1
		hello2

	FUTURE
		- Add member access: my_data.name.first_name
		- Add lambda / local function
*/
pair<expression_t, string> parse_single_internal(const parser_i& parser, const string& s) {
	QUARK_ASSERT(s.size() > 0);

	string pos = s;

	//	" => string constant.
	if(peek_string(pos, "\"")){
		pos = pos.substr(1);
		const auto string_constant_pos = read_until(pos, "\"");

		pos = string_constant_pos.second;
		pos = pos.substr(1);
		return { value_t{ string_constant_pos.first }, pos };
	}

	// [0-9] and "."  => numeric constant.
	else if((number_chars + ".").find(pos[0]) != string::npos){
		const auto number_pos = read_while(pos, number_chars + ".");
		if(number_pos.first.empty()){
			throw std::runtime_error("EEE_WRONG_CHAR");
		}

		//	If it contains a "." its a float, else an int.
		if(number_pos.first.find('.') != string::npos){
			pos = pos.substr(number_pos.first.size());
			float value = parse_float(number_pos.first);
			return { value_t{value}, pos };
		}
		else{
			pos = pos.substr(number_pos.first.size());
			int value = atoi(number_pos.first.c_str());
			return { value_t{value}, pos };
		}
	}

	/*
		Start of identifier: can be variable access or function call.

		"hello"
		"f ()"
		"f(x + 10)"
	*/
	else if(identifier_chars.find(pos[0]) != string::npos){
		const auto identifier_pos = read_required_identifier(pos);

		string p2 = skip_whitespace(identifier_pos.second);

		//	Function call.
		if(!p2.empty() && p2[0] == '('){
			//	Lookup function!
			if(!parser.parser_i__is_declared_function(identifier_pos.first)){
				throw std::runtime_error("Unknown function \"" + identifier_pos.first + "\"");
			}

			const auto arg_list_pos = get_balanced(p2);
			const auto args = trim_ends(arg_list_pos.first);

			p2 = args;
			vector<std::shared_ptr<expression_t>> args_expressions;
			while(!p2.empty()){
				const auto p3 = read_until(skip_whitespace(p2), ",");
				expression_t arg_expre = parse_expression(parser, p3.first);
				args_expressions.push_back(std::make_shared<expression_t>(arg_expre));
				p2 = p3.second[0] == ',' ? p3.second.substr(1) : p3.second;
			}
			//	??? Check types of arguments and count.

			return { function_call_expr_t{identifier_pos.first, args_expressions }, arg_list_pos.second };
		}

		//	Variable-read.
		else{
			//	Lookup value!
			if(!parser.parser_i__is_declared_constant_value(identifier_pos.first)){
				throw std::runtime_error("Unknown identifier \"" + identifier_pos.first + "\"");
			}
			return { variable_read_expr_t{identifier_pos.first }, p2 };
		}
	}
	else{
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
}

pair<expression_t, string> parse_single(const parser_i& parser, const string& s){
	const auto result = parse_single_internal(parser, s);
	trace(result.first);
	return result;
}




struct test_parser : public parser_i {
	public: virtual bool parser_i__is_declared_function(const std::string& s) const{
		return s == "log" || s == "log2" || s == "f" || s == "return5";
	}
	public: virtual bool parser_i__is_declared_constant_value(const std::string& s) const{
		return false;
	}

	bool parser_i__is_known_type(const std::string& s) const{
		return true;
	}
};


//??? Do we perform proper type checking when building parse tree? Or will parse tree contain syntax errors?

QUARK_UNIT_TESTQ("parse_single", "number"){
	test_parser parser;
	QUARK_TEST_VERIFY((parse_single(parser, "9.0") == pair<expression_t, string>(value_t{ 9.0f }, "")));
}

QUARK_UNIT_TESTQ("parse_single", "function call"){
	test_parser parser;
	const auto a = parse_single(parser, "log(34.5)");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function_expr->_inputs[0]->_constant == value_t(34.5f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "function call with two args"){
	test_parser parser;
	const auto a = parse_single(parser, "log2(\"123\" + \"xyz\", 1000 * 3)");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[0]->_math_operation2_expr);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_math_operation2_expr);
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "nested function calls"){
	test_parser parser;
	const auto a = parse_single(parser, "log2(2.1, f(3.14))");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[0]->_constant);
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_call_function_expr->_function_name == "f");
	QUARK_TEST_VERIFY(a.first._call_function_expr->_inputs[1]->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function_expr->_inputs[1]->_call_function_expr->_inputs[0] == value_t(3.14f));
	QUARK_TEST_VERIFY(a.second == "");
}


// Parse a constant or an expression in parenthesis
pair<expression_t, string> parse_atom(const parser_i& parser, const string& s, int depth) {
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

		auto res = parse_summands(parser, pos, depth);
		pos = skip_whitespace(res.second);
		if(!(res.second.size() > 0 && res.second[0] == ')')) {
			// Unmatched opening parenthesis
			throw std::runtime_error("EEE_PARENTHESIS");
		}
		pos = pos.substr(1);
		const auto expr = res.first;
		return { negative ? negate_expression(expr) : expr, pos };
	}

	//	Parse constant literal / function call / variable-access.
	if(pos.size() > 0){
		const auto single_pos = parse_single(parser, pos);
		return { negative ? negate_expression(single_pos.first) : single_pos.first, single_pos.second };
	}

	throw std::runtime_error("Expected number");
}





//??? more tests here!
QUARK_UNIT_TEST("", "parse_atom", "", ""){
	test_parser parser;

	QUARK_TEST_VERIFY((parse_atom(parser, "0.0", 0) == pair<expression_t, string>(value_t{ 0.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "9.0", 0) == pair<expression_t, string>(value_t{ 9.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "12345.0", 0) == pair<expression_t, string>(value_t{ 12345.0f }, "")));

	QUARK_TEST_VERIFY((parse_atom(parser, "10.0", 0) == pair<expression_t, string>(value_t{ 10.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "-10.0", 0) == pair<expression_t, string>(value_t{ -10.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(parser, "+10.0", 0) == pair<expression_t, string>(value_t{ 10.0f }, "")));

	QUARK_TEST_VERIFY((parse_atom(parser, "4.0+", 0) == pair<expression_t, string>(value_t{ 4.0f }, "+")));


	QUARK_TEST_VERIFY((parse_atom(parser, "\"hello\"", 0) == pair<expression_t, string>(value_t{ "hello" }, "")));
}


pair<expression_t, string> parse_factors(const parser_i& parser, const string& s, int depth) {
	const auto num1_pos = parse_atom(parser, s, depth);
	auto result_expression = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	while(!pos.empty() && (pos[0] == '*' || pos[0] == '/')){
		const auto op_pos = read_char(pos);
		const auto expression2_pos = parse_atom(parser, op_pos.second, depth);
		if(op_pos.first == '/') {
			result_expression = make_math_operation2_expr(math_operation2_expr_t::divide, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation2_expr(math_operation2_expr_t::multiply, result_expression, expression2_pos.first);
		}
		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

pair<expression_t, string> parse_summands(const parser_i& parser, const string& s, int depth) {
	const auto num1_pos = parse_factors(parser, s, depth);
	auto result_expression = num1_pos.first;
	string pos = num1_pos.second;
	while(!pos.empty() && (pos[0] == '-' || pos[0] == '+')) {
		const auto op_pos = read_char(pos);

		const auto expression2_pos = parse_factors(parser, op_pos.second, depth);
		if(op_pos.first == '-'){
			result_expression = make_math_operation2_expr(math_operation2_expr_t::subtract, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation2_expr(math_operation2_expr_t::add, result_expression, expression2_pos.first);
		}

		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}




string operation_to_string(const math_operation2_expr_t::operation& op){
	if(op == math_operation2_expr_t::add){
		return "add";
	}
	else if(op == math_operation2_expr_t::subtract){
		return "subtract";
	}
	else if(op == math_operation2_expr_t::multiply){
		return "multiply";
	}
	else if(op == math_operation2_expr_t::divide){
		return "divide";
	}
	else{
		QUARK_ASSERT(false);
	}
}

string operation_to_string(const math_operation1_expr_t::operation& op){
	if(op == math_operation1_expr_t::negate){
		return "negate";
	}
	else{
		QUARK_ASSERT(false);
	}
}




void trace(const function_def_expr_t& e){
	QUARK_SCOPED_TRACE("function_def_expr_t");

	{
		QUARK_SCOPED_TRACE("return");
		trace(e._return_type);
	}
	{
		trace_vec("arguments", e._args);
	}
	{
		trace(e._body);
	}
}

void trace(const struct_def_expr_t& e){
	QUARK_SCOPED_TRACE("struct_def_expr_t");

	{
		trace_vec("arguments", e._members);
	}
}

void trace(const math_operation2_expr_t& e){
	string s = "math_operation2_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._left);
	trace(*e._right);
}
void trace(const math_operation1_expr_t& e){
	string s = "math_operation1_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._input);
}

void trace(const function_call_expr_t& e){
	string s = "function_call_expr_t: " + e._function_name;
	QUARK_SCOPED_TRACE(s);
	for(const auto i: e._inputs){
		trace(*i);
	}
}
void trace(const variable_read_expr_t& e){
	string s = "variable_read_expr_t: " + e._variable_name;
}


void trace(const expression_t& e){
	if(e._constant){
		trace(*e._constant);
	}
	else if(e._function_def_expr){
		const function_def_expr_t& temp = *e._function_def_expr;
		trace(temp);
	}
	else if(e._struct_def_expr){
		const struct_def_expr_t& temp = *e._struct_def_expr;
		trace(temp);
	}
	else if(e._math_operation2_expr){
		trace(*e._math_operation2_expr);
	}
	else if(e._math_operation1_expr){
		trace(*e._math_operation1_expr);
	}
	else if(e._nop_expr){
		QUARK_TRACE("NOP");
	}
	else if(e._call_function_expr){
		trace(*e._call_function_expr);
	}
	else if(e._variable_read_expr){
		trace(*e._variable_read_expr);
	}
	else{
		QUARK_ASSERT(false);
	}
}






vector<arg_t> parse_functiondef_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<arg_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = read_type(str);
		const auto arg_name = read_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ make_type_identifier(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_functiondef_arguments("()") == vector<arg_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto r = parse_functiondef_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<arg_t>{
		{ make_type_identifier("int"), "x" },
		{ make_type_identifier("string"), "y" },
		{ make_type_identifier("float"), "z" }
	}
	));
}



/*
	Never simplifes - the parser is non-lossy.

	Must not have whitespace before / after {}.
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

	FUTURE
	- Include comments
	- Split-out parse_statement().
	- Add struct {}
	- Add variables
	- Add local functions
*/
//??? Need concept of parsing stack-frame to store local variables.



function_body_t parse_function_body(const ast_t& ast, const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	vector<statement_t> statements;

	ast_t local_scope = ast;

	string pos = body_str;
	while(!pos.empty()){

		//	Examine function body, one statement at a time. Only statements are allowed.
		const auto token_pos = read_until(pos, whitespace_chars);

		//	return statement?
		if(token_pos.first == "return"){
			const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
			const auto expression1 = parse_expression(local_scope, expression_pos.first);
//			const auto expression2 = evaluate3(local_scope, expression1);
			const auto statement = statement_t(return_statement_t{ make_shared<expression_t>(expression1) });
			statements.push_back(statement);

			//	Skip trailing ";".
			pos = skip_whitespace(expression_pos.second.substr(1));
		}

		//	Define local variable?
		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/
		else if(ast.parser_i__is_known_type(token_pos.first)){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(local_scope, pos);
			const string& identifier = assignment_statement.first._bind_statement->_identifier;

			const auto it = local_scope._constant_values.find(identifier);
			if(it != local_scope._constant_values.end()){
				throw std::runtime_error("Variable name already in use!");
			}

			shared_ptr<const value_t> blank;
			local_scope._constant_values[identifier] = blank;

			statements.push_back(assignment_statement.first);

			//	Skips trailing ";".
			pos = skip_whitespace(assignment_statement.second);
		}
		else{
			throw std::runtime_error("syntax error");
		}
	}
	const auto result = function_body_t{ statements };
	trace(result);
	return result;
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY((parse_function_body({}, "{}")._statements.empty()));
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{return 3;}")._statements.size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{\n\treturn 3;\n}")._statements.size() == 1);
}

QUARK_UNIT_TESTQ("parse_function_body()", ""){
	const auto a = parse_function_body(make_test_ast(),
		"{	float test = log(10.11);\n"
		"	return 3;\n}"
	);
	QUARK_TEST_VERIFY(a._statements.size() == 2);
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression->_call_function_expr->_function_name == "log");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression->_call_function_expr->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a._statements[0]._bind_statement->_expression->_call_function_expr->_inputs[0]->_constant == value_t(10.11f));

	QUARK_TEST_VERIFY(*a._statements[1]._return_statement->_expression->_constant == value_t(3));
}

/*
QUARK_UNIT_TEST("", "", "", ""){
	const auto identifiers = make_test_ast();
	const auto a = parse_function_body(identifiers,
		"{ return return5() + return5() * 2;\n}"
	);
	QUARK_TEST_VERIFY(a._statements.size() == 1);
//	QUARK_TEST_VERIFY(a._statements[0]._return_statement->_expression._math_operation2);

	const auto b = evaluate3(identifiers, *a._statements[0]._return_statement->_expression);
	QUARK_TEST_VERIFY(b._constant && *b._constant == value_t(15));

//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_identifier == "test");
//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_function_name == "log");
//	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_inputs.size() == 1);
//	QUARK_TEST_VERIFY(*a._statements[0]._bind_statement->_expression._call_function->_inputs[0]->_constant == value_t(10.11f));
}
*/




expression_t parse_expression(const parser_i& parser, string expression){
	if(expression.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	auto result = parse_summands(parser, expression, 0);
	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	QUARK_TRACE("Expression: \"" + expression + "\"");
	trace(result.first);
	return result.first;
}






//////////////////////////////////////////////////		test rig



shared_ptr<const function_def_expr_t> make_log_function(){
	vector<arg_t> args{ {make_type_identifier("float"), "value"} };
	function_body_t body{
		{
			make__return_statement(
				return_statement_t{ std::make_shared<expression_t>(make_constant(value_t(123.f))) }
			)
		}
	};

	return make_shared<const function_def_expr_t>(function_def_expr_t{ make_type_identifier("float"), args, body });
}

shared_ptr<const function_def_expr_t> make_log2_function(){
	vector<arg_t> args{ {make_type_identifier("string"), "s"}, {make_type_identifier("float"), "v"} };
	function_body_t body{
		{
			make__return_statement(
				return_statement_t{ make_shared<expression_t>(make_constant(value_t(456.7f))) }
			)
		}
	};

	return make_shared<const function_def_expr_t>(function_def_expr_t{ make_type_identifier("float"), args, body });
}

shared_ptr<const function_def_expr_t> make_return5(){
	vector<arg_t> args{};
	function_body_t body{
		{
			make__return_statement(
				return_statement_t{ make_shared<expression_t>(make_constant(value_t(5))) }
			)
		}
	};

	return make_shared<const function_def_expr_t>(function_def_expr_t{ make_type_identifier("int"), args, body });
}



shared_ptr<const struct_def_expr_t> make_struct0(){
	vector<arg_t> members{};

	return make_shared<const struct_def_expr_t>(struct_def_expr_t{ members });
}

shared_ptr<const struct_def_expr_t> make_struct1(){
	vector<arg_t> members{
		{ make_type_identifier("float"), "x" },
		{ make_type_identifier("float"), "y" },
		{ make_type_identifier("string"), "name" }
	};

	return make_shared<const struct_def_expr_t>(struct_def_expr_t{ members });
}



ast_t make_test_ast(){
	ast_t result;
	result._functions["log"] = make_log_function();
	result._functions["log2"] = make_log2_function();
	result._functions["f"] = make_log_function();
	result._functions["return5"] = make_return5();

	result._structs["test_struct0"] = make_struct0();
	result._structs["test_struct1"] = make_struct1();
	return result;
}

QUARK_UNIT_TESTQ("make_test_ast()", ""){
	auto a = make_test_ast();
	QUARK_TEST_VERIFY(a._structs.size() == 2);

	QUARK_TEST_VERIFY(*a._structs["test_struct0"] == *make_struct0());
	QUARK_TEST_VERIFY(*a._structs["test_struct1"] == *make_struct1());
}



//??? where are unit tests???
}



