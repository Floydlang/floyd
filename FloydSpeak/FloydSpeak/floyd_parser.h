//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"
#include <vector>
#include <string>
#include <map>

struct statement_t;





//////////////////////////////////////////////////		data_type_t

/*
	Internal object representing a Floyd data type.
		### Use the same compatible technique / API  in the runtime and in the language itself!

	!!! Right now we use an internal string, but this will change in future.
*/


struct data_type_t {
	public: static data_type_t make_type(std::string s){
		const data_type_t result(s);
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
	data_type_t(std::string s) :
		_type_magic(s)
	{
	}

	void swap(data_type_t& other){
		_type_magic.swap(other._type_magic);
	}

	std::string to_string() const {
		return _type_magic;
	}

	bool check_invariant() const {
		QUARK_ASSERT(_type_magic != "");
		return true;
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
	private: std::string _type_magic;
};


/*
	returns "" on unknown type.
*/
data_type_t resolve_type(std::string node_type);



//////////////////////////////////////////////////		value_t

/*
	Hold a value with an explicit type.
*/

struct value_t {
	public: bool check_invariant() const{
		return true;
	}

	public: value_t() :
		_type("null")
	{
	}
	public: explicit value_t(const char s[]) :
		_type("string"),
		_string(s)
	{
	}
	public: explicit value_t(const std::string& s) :
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


	value_t(const value_t& other):
		_type(other._type),

		_bool(other._bool),
		_int(other._int),
		_float(other._float),
		_string(other._string),
		_function_id(other._function_id),
		_data_type(other._data_type)
	{
	}

	value_t& operator=(const value_t& other){
		value_t temp(other);
		temp.swap(*this);
		return *this;
	}

	public: bool operator==(const value_t& other) const{
		return _type == other._type && _bool == other._bool && _int == other._int && _float == other._float && _string == other._string && _function_id == other._function_id && _data_type == other._data_type;
	}
	std::string plain_value_to_string() const {
		if(_type == "null"){
			return "<null>";
		}
		else if(_type == "bool"){
			return _bool ? "true" : "false";
		}
		else if(_type == "int"){
			char temp[200 + 1];//??? Use C++ function instead.
			sprintf(temp, "%d", _int);
			return std::string(temp);
		}
		else if(_type == "float"){
			char temp[200 + 1];//??? Use C++ function instead.
			sprintf(temp, "%f", _float);
			return std::string(temp);
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

	std::string value_and_type_to_string() const {
		return "<" + _type.to_string() + "> " + plain_value_to_string();
	}

	public: data_type_t get_type() const{
		return _type;
	}
	public: int get_int() const{
		return _int;
	}
	public: float get_float() const{
		return _float;
	}
	public: std::string get_string() const{
		return _string;
	}

	public: void swap(value_t& other){
		_type.swap(other._type);

		std::swap(_bool, other._bool);
		std::swap(_int, other._int);
		std::swap(_float, other._float);
		std::swap(_string, other._string);
		std::swap(_function_id, other._function_id);
		std::swap(_data_type, other._data_type);
	}

	private: data_type_t _type;

	private: bool _bool = false;
	private: int _int = 0;
	private: float _float = 0.0f;
	private: std::string _string = "";
	private: std::string _function_id = "";
	private: data_type_t _data_type;
};


/*
inline value_t make_dummy_value(const data_type_t& type){
	const auto t = type.to_string();
	if(t == "null"){
		return value_t();	//??? Remove concept of null??
	}
	else if(t == "string"){
		return value_t("...");
	}
	else if(t == "int"){
		return value_t(666);
	}
	else if(t == "float"){
		return value_t(667.667f);
	}
	else if(t == "value_type"){
		return value_t(data_type_t("null"));
	}
	else{
		QUARK_ASSERT(false);
		return value_t();
	}
}
*/



//////////////////////////////////////////////////		AST types



struct statement_t;
struct expression_t;
struct math_operation2_expr_t;
struct math_operation1_expr_t;

struct arg_t {
	bool check_invariant() const {
		QUARK_ASSERT(_type.check_invariant());
		QUARK_ASSERT(_identifier.size() > 0);
		return true;
	}
	bool operator==(const arg_t& other) const{
		return _type == other._type && _identifier == other._identifier;
	}

	const data_type_t _type;
	const std::string _identifier;
};

struct function_body_t {
	bool operator==(const function_body_t& other) const{
		return _statements == other._statements;
	}

	const std::vector<statement_t> _statements;
};

struct function_def_expr_t {
	bool check_invariant() const {
		QUARK_ASSERT(_return_type.check_invariant());
		return true;
	}

	bool operator==(const function_def_expr_t& other) const{
		return _return_type == other._return_type && _args == other._args && _body == other._body;
	}

	const data_type_t _return_type;
	const std::vector<arg_t> _args;
	const function_body_t _body;
};

struct math_operation2_expr_t {
	bool operator==(const math_operation2_expr_t& other) const;


	enum operation {
		add = 10,
		subtract,
		multiply,
		divide
	};

	const operation _operation;

	const std::shared_ptr<expression_t> _left;
	const std::shared_ptr<expression_t> _right;
};

struct math_operation1_expr_t {
	bool operator==(const math_operation1_expr_t& other) const;


	enum operation {
		negate = 20
	};

	const operation _operation;

	const std::shared_ptr<expression_t> _input;
};

struct function_call_expr_t {
	bool operator==(const function_call_expr_t& other) const{
		return _function_name == other._function_name && _inputs == other._inputs;
	}


	const std::string _function_name;
	const std::vector<std::shared_ptr<expression_t>> _inputs;
};

struct variable_read_expr_t {
	bool operator==(const variable_read_expr_t& other) const{
		return _variable_name == other._variable_name ;
	}


	const std::string _variable_name;
};


struct expression_t {
	public: bool check_invariant() const{
		return true;
	}

	expression_t() :
		_nop_expr(true)
	{
		QUARK_ASSERT(false);
	}

	expression_t(const function_def_expr_t& value) :
		_function_def_expr(std::make_shared<const function_def_expr_t>(value))
	{
	}

	expression_t(const value_t& value) :
		_constant(std::make_shared<value_t>(value))
	{
		QUARK_ASSERT(value.check_invariant());

		QUARK_ASSERT(check_invariant());
	}
	expression_t(const math_operation2_expr_t& value) :
		_math_operation2_expr(std::make_shared<math_operation2_expr_t>(value))
	{
		QUARK_ASSERT(check_invariant());
	}
	expression_t(const math_operation1_expr_t& value) :
		_math_operation1_expr(std::make_shared<math_operation1_expr_t>(value))
	{
		QUARK_ASSERT(check_invariant());
	}

	expression_t(const function_call_expr_t& value) :
		_call_function_expr(std::make_shared<function_call_expr_t>(value))
	{
		QUARK_ASSERT(check_invariant());
	}

	expression_t(const variable_read_expr_t& value) :
		_variable_read_expr(std::make_shared<variable_read_expr_t>(value))
	{
		QUARK_ASSERT(check_invariant());
	}

/*
	void swap(expression_t& other) throw(){
		_function_def_expr.swap(other._function_def_expr);
		_constant.swap(other._constant);
		_math_operation2.swap(other._math_operation2);
		_math_operation1.swap(other._math_operation1);
		std::swap(_nop, other._nop);
	}

	const expression_t& operator=(const expression_t& other){
		auto expression_t temp = other;
		temp.swap(*this);
		return *this;
	}
*/

	bool operator==(const expression_t& other) const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(_function_def_expr){
			return other._function_def_expr && *_function_def_expr == *other._function_def_expr;
		}
		else if(_constant){
			return other._constant && *_constant == *other._constant;
		}
		else if(_math_operation2_expr){
			return other._math_operation2_expr && *_math_operation2_expr == *other._math_operation2_expr;
		}
		else if(_math_operation1_expr){
			return other._math_operation1_expr && *_math_operation1_expr == *other._math_operation1_expr;
		}
		else if(_call_function_expr){
			return other._call_function_expr && *_call_function_expr == *other._call_function_expr;
		}
		else if(_variable_read_expr){
			return other._variable_read_expr && *_variable_read_expr == *other._variable_read_expr;
		}
		else{
			QUARK_ASSERT(false);
			return false;
		}
	}

	//////////////////////////		STATE

	/*
		Only one of there are used at any time.
	*/
	std::shared_ptr<const function_def_expr_t> _function_def_expr;
	std::shared_ptr<value_t> _constant;
	std::shared_ptr<math_operation2_expr_t> _math_operation2_expr;
	std::shared_ptr<math_operation1_expr_t> _math_operation1_expr;
	std::shared_ptr<function_call_expr_t> _call_function_expr;
	std::shared_ptr<variable_read_expr_t> _variable_read_expr;
	bool _nop_expr = false;
};


inline bool math_operation2_expr_t::operator==(const math_operation2_expr_t& other) const {
	return _operation == other._operation && *_left == *other._left && *_right == *other._right;
}
inline bool math_operation1_expr_t::operator==(const math_operation1_expr_t& other) const {
	return _operation == other._operation && *_input == *other._input;
}


inline expression_t make__function_def_expr(const function_def_expr_t& value){
	return expression_t(value);
}

inline expression_t make_constant(const value_t& value){
	return expression_t(value);
}

inline expression_t make_math_operation2_expr(const math_operation2_expr_t& value){
	return expression_t(value);
}

inline expression_t make_math_operation2_expr(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right){
	return expression_t(math_operation2_expr_t{op, std::make_shared<expression_t>(left), std::make_shared<expression_t>(right)});
}

inline expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input){
	return expression_t(math_operation1_expr_t{op, std::make_shared<expression_t>(input) });
}



struct bind_statement_t {
	bool operator==(const bind_statement_t& other) const {
		return _identifier == other._identifier && _expression == other._expression;
	}

	std::string _identifier;
	expression_t _expression;
};

struct return_statement_t {
	bool operator==(const return_statement_t& other) const {
		return _expression == other._expression;
	}

	expression_t _expression;
};

struct statement_t {
	statement_t(const bind_statement_t& value) :
		_bind_statement(std::make_shared<bind_statement_t>(value))
	{
	}

	statement_t(const return_statement_t& value) :
		_return_statement(std::make_shared<return_statement_t>(value))
	{
	}

	bool operator==(const statement_t& other) const {
		if(_bind_statement){
			return other._bind_statement && *_bind_statement == *other._bind_statement;
		}
		else if(_return_statement){
			return other._return_statement && *_return_statement == *other._return_statement;
		}
		else{
			QUARK_ASSERT(false);
			return false;
		}
	}

	const std::shared_ptr<bind_statement_t> _bind_statement;
	const std::shared_ptr<return_statement_t> _return_statement;
};











inline statement_t make__bind_statement(const bind_statement_t& value){
	return statement_t(value);
}

inline statement_t make__bind_statement(const std::string& identifier, const expression_t& e){
	return statement_t(bind_statement_t{identifier, e});
}

inline statement_t make__return_statement(const return_statement_t& value){
	return statement_t(value);
}





struct identifiers_t {
	identifiers_t(){
	}

	public: bool check_invariant() const {
		return true;
	}

	//### Function names should have namespace etc.
	std::map<std::string, std::shared_ptr<const function_def_expr_t> > _functions;

	std::map<std::string, std::shared_ptr<const value_t> > _constant_values;
};


struct ast_t {
	public: bool check_invariant() const {
		return true;
	}


	/////////////////////////////		STATE
	identifiers_t _identifiers;
	std::vector<statement_t> _top_level_statements;
};


ast_t program_to_ast(const identifiers_t& builtins, const std::string& program);

/*
	Parses the expression sttring
	Checks syntax
	Validates that called functions exists and has correct type.
	Validates that accessed variables exists and has correct types.

	No optimization or evalution of any constant expressions etc.
*/
expression_t parse_expression(const identifiers_t& identifiers, std::string expression);

/*
*/
expression_t evaluate3(const identifiers_t& identifiers, const expression_t& e);


/*
	functions: if null, all function calls are just stored in as expression-nodes.
		If NOT null, the function is called if possible
		
		??? Not OK: function calls should be validated even in non-optimizing pass.
*/
expression_t parse_expression_evaluate(const identifiers_t& identifiers, std::string expression);

value_t run_function(const identifiers_t& identifiers, const function_def_expr_t& f, const std::vector<value_t>& args);



#endif /* floyd_parser_h */
