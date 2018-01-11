	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"

#include "statements.h"
#include "floyd_parser.h"
#include "parser_value.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "parser_primitives.h"

using namespace floyd_ast;
using namespace std;


/*
//////////////////////////////////////////////////		parser_path_t

//	Traversal of parse tree, one level of static scope at a time.

struct parser_path_t {
	//	Returns a scope_def in json format.
	public: json_t get_leaf() const;

	public: bool check_invariant() const;


	public: std::vector<json_t> _scopes;
};

json_t parser_path_t::get_leaf() const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(!_scopes.empty());

	return _scopes.back();
}

bool parser_path_t::check_invariant() const {
	for(const auto i: _scopes){
		QUARK_ASSERT(i.check_invariant());
	}
	return true;
};

parser_path_t make_parser_path(const json_t& scope){
	parser_path_t path;
	path._scopes.push_back(scope);
	return path;
}

parser_path_t go_down(const parser_path_t& path, const json_t& child){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(child.check_invariant());

	auto result = path;
	result._scopes.push_back(child);
	return result;
}

parser_path_t replace_leaf(const parser_path_t& path, const json_t& leaf){
	vector<json_t> temp(path._scopes.begin(), path._scopes.end() - 1);
	temp.push_back(leaf);

	parser_path_t temp2;
	temp2._scopes.swap(temp);
	return temp2;
}

string make_path_string(const parser_path_t& path, const string& node_name){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(node_name.size() > 0);

	string result;
	for(int i = 0 ; i < path._scopes.size() ; i++){
		const auto name = path._scopes[i].get_object_element("name").get_string();
		result = result + name + "/";
	}
	result = result + node_name;
	return result;
}
*/


floyd_basics::typeid_t resolve_type_name(const string& t){
	QUARK_ASSERT(t.size() > 2);

	const auto t2 = floyd_basics::typeid_t::from_string(t);
	return t2;
}

expression_t parser_expression_to_ast(const json_t& e){
	QUARK_ASSERT(e.check_invariant());
	QUARK_TRACE(json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2).get_string();
		const auto type2 = resolve_type_name(type);

		if(type2.is_null()){
			return expression_t::make_literal_null();
		}
		else if(type2.get_base_type() == floyd_basics::base_type::k_bool){
			return expression_t::make_literal_bool(value.is_false() ? false : true);
		}
		else if(type2.get_base_type() == floyd_basics::base_type::k_int){
			return expression_t::make_literal_int((int)value.get_number());
		}
		else if(type2.get_base_type() == floyd_basics::base_type::k_float){
			return expression_t::make_literal_float((float)value.get_number());
		}
		else if(type2.get_base_type() == floyd_basics::base_type::k_string){
			return expression_t::make_literal_string(value.get_string());
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(op == "unary_minus"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto expr = parser_expression_to_ast(e.get_array_n(1));
		return expression_t::make_unary_minus(expr);
	}
	else if(floyd_ast::is_simple_expression__2(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto op2 = floyd_basics::token_to_expression_type(op);
		const auto lhs_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto rhs_expr = parser_expression_to_ast(e.get_array_n(2));
		return expression_t::make_simple_expression__2(op2, lhs_expr, rhs_expr);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto condition_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto a_expr = parser_expression_to_ast(e.get_array_n(2));
		const auto b_expr = parser_expression_to_ast(e.get_array_n(3));
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto function_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			args2.push_back(parser_expression_to_ast(arg));
		}
		return expression_t::make_function_call(function_expr, args2, floyd_basics::typeid_t::make_null());
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		return expression_t::make_resolve_member(base_expr, member, floyd_basics::typeid_t::make_null());
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto variable_symbol = e.get_array_n(1).get_string();
		return expression_t::make_variable_expression(variable_symbol, floyd_basics::typeid_t::make_null());
	}
	else{
		QUARK_ASSERT(false);
	}
}


/*
	Example:
		[
			{ "name": "g_version", "type": "int" },
			{ "name": "message", "type": "string" },
			{ "name": "pos", "type": "pixel" },
			{ "name": "pos", "type": "image_lib.image.pixel_t" }
		]
*/
std::vector<member_t> conv_members(const json_t& members){
	std::vector<member_t> members2;
	for(const auto i: members.get_array()){
		const string arg_name = i.get_object_element("name").get_string();
		const string arg_type = i.get_object_element("type").get_string();

		const auto arg_type2 = resolve_type_name(arg_type);
		members2.push_back(member_t{arg_type2, nullptr, arg_name });
	}
	return members2;
}




/*
	Input is an array of statements from parser.
	A function has its own list of statements.

	??? Split each statement into separate function.

	### Support overloading the same symbol name with different types.
	### Key symbols with their type too. Support function overloading & struct named as function.
*/
const std::vector<std::shared_ptr<statement_t> > parser_statements_to_ast(const json_t& p){
	QUARK_SCOPED_TRACE("parser_statements_to_ast()");
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(p.is_array());

	vector<shared_ptr<statement_t>> statements2;

	for(const auto statement: p.get_array()){
		const auto type = statement.get_array_n(0);

		//	[ "return", [ "k", 3, "int" ] ]
		if(type == "return"){
			QUARK_ASSERT(statement.get_array_size() == 2);
			const auto expr = parser_expression_to_ast(statement.get_array_n(1));
			statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
		}

		//	[ "bind", "float", "x", EXPRESSION ],
		else if(type == "bind"){
			QUARK_ASSERT(statement.get_array_size() == 4);
			const auto bind_type = statement.get_array_n(1);
			const auto name = statement.get_array_n(2);
			const auto expr = statement.get_array_n(3);

			const auto bind_type2 = resolve_type_name(bind_type.get_string());
			const auto name2 = name.get_string();
			const auto expr2 = parser_expression_to_ast(expr);
			statements2.push_back(make_shared<statement_t>(make__bind_statement(name2, bind_type2, expr2)));
		}

		//	[ "block", [ STATEMENTS ] ],
		else if(type == "block"){
			QUARK_ASSERT(statement.get_array_size() == 2);

			const auto statements = statement.get_array_n(1);
			const auto r = parser_statements_to_ast(statements);
			//??? also include locals & objects
			statements2.push_back(make_shared<statement_t>(make__block_statement(r)));
		}

		/*
			INPUT:
				[
					"def-func",
					{
						"args": [
							
						],
						"name": "main",
						"return_type": "int",
						"statements": [
							[ "return", [ "k", 3, "int" ] ]
						]
					}
				]

			OUTPUT:
				"main": [
					{
						"base_type": "function",
						"scope_def": {
							"args": [],
							"locals": [],
							"members": [],
							"name": "main",
							"return_type": "int",
							"statements": [???],
							"type": "function",
							"types": {}
						}
					}
				]
		*/
		else if(type == "def-func"){
			QUARK_ASSERT(statement.get_array_size() == 2);
			const auto def = statement.get_array_n(1);
			const auto name = def.get_object_element("name");
			const auto args = def.get_object_element("args");
			const auto fstatements = def.get_object_element("statements");
			const auto return_type = def.get_object_element("return_type");

			const auto name2 = name.get_string();
			const auto args2 = conv_members(args);
			const auto fstatements2 = parser_statements_to_ast(fstatements);
			const auto return_type2 = resolve_type_name(return_type.get_string());

			const auto function_typeid = floyd_basics::typeid_t::make_function(return_type2, get_member_types(args2));
			const auto function_def = function_definition_t(args2, fstatements2, return_type2);
			const auto function_def_expr = expression_t::make_function_definition(function_def);
			statements2.push_back(make_shared<statement_t>(make__bind_statement(name2, function_typeid, function_def_expr)));
		}

		/*
			INPUT:
				[
					"def-struct",
					{
						"members": [
							{ "name": "red", "type": "float" },
							{ "name": "green", "type": "float" },
							{ "name": "blue", "type": "float" }
						],
						"name": "pixel"
					}
				],

			OUTPUT:
				"pixel": [
					{
						"base_type": "struct",
						"scope_def": {
							"args": [],
							"locals": [],
							"members": [
								{ "name": "red", "type": "float" },
								{ "name": "green", "type": "float" },
								{ "name": "blue", "type": "float" }
							],
							"name": "pixel",
							"return_type": "",
							"statements": [],
							"type": "struct",
							"types": {}
						}
					}
				]
		*/
		else if(type == "def-struct"){
/*
			const auto struct_def = statement.get_array_n(1);
			const auto name = struct_def.get_object_element("name");
			const auto members = struct_def.get_object_element("members");

			json_t struct_scope = make_scope_def();
			struct_scope = store_object_member(struct_scope, "type", "struct");
			struct_scope = store_object_member(struct_scope, "name", name);
			struct_scope = store_object_member(struct_scope, "members", members);
			scope2 = add_scope_type(scope2, struct_scope);
*/
		}

		//	[ "if", CONDITION_EXPR, THEN_STATEMENTS, ELSE_STATEMENTS ]
		//	Else is optional.
		else if(type == "if"){
			QUARK_ASSERT(statement.get_array_size() == 3 || statement.get_array_size() == 4);
			const auto condition_expression = statement.get_array_n(1);
			const auto then_statements = statement.get_array_n(2);
			const auto else_statements = statement.get_array_size() == 4 ? statement.get_array_n(3) : json_t::make_array();

			const auto condition_expression2 = parser_expression_to_ast(condition_expression);
			const auto& then_statements2 = parser_statements_to_ast(then_statements);
			const auto& else_statements2 = parser_statements_to_ast(else_statements);

			statements2.push_back(make_shared<statement_t>(
				make__ifelse_statement(
					condition_expression2,
					then_statements2,
					else_statements2
				)
			));
		}
		else if(type == "for"){
			QUARK_ASSERT(statement.get_array_size() == 6);
			const auto for_mode = statement.get_array_n(1);
			const auto iterator_name = statement.get_array_n(2);
			const auto start_expression = statement.get_array_n(3);
			const auto end_expression = statement.get_array_n(4);
			const auto body_statements = statement.get_array_n(5);

			const auto start_expression2 = parser_expression_to_ast(start_expression);
			const auto end_expression2 = parser_expression_to_ast(end_expression);
			const auto& body_statements2 = parser_statements_to_ast(body_statements);

			statements2.push_back(make_shared<statement_t>(
				make__for_statement(
					iterator_name.get_string(),
					start_expression2,
					end_expression2,
					body_statements2
				)
			));

		}
		else{
			throw std::runtime_error("Illegal statement.");
		}
	}

	return statements2;
}







bool has_unresolved_types(const json_t& obj){
	if(obj.is_string()){
		const auto s = obj.get_string();
		if(s.size() > 2 && s[0] == '<' && s[s.size() - 1] == '>'){
			return true;
		}
		return false;
	}
	else if(obj.is_object()){
		for(const auto i: obj.get_object()){
			if(has_unresolved_types(i.second)){
				return true;
			}
		}
		return false;
	}
	else if(obj.is_array()){
		for(int i = 0 ; i < obj.get_array_size() ; i++){
			if(has_unresolved_types(obj.get_array_n(i))){
				return true;
			}
		}
		return false;
	}
	else{
		return false;
	}
}


ast_t run_pass2(const json_t& parse_tree){
	QUARK_TRACE(json_to_pretty_string(parse_tree));

	const auto program_body = parser_statements_to_ast(parse_tree);
	return ast_t(program_body);
}



///////////////////////////////////////			TESTS


/*
	These tests verify that:
	- the transform of the AST worked as expected
	- Correct errors are emitted.
*/
QUARK_UNIT_TESTQ("run_pass2()", "k_test_program_0"){
	const auto pass1 = parse_json(seq_t(floyd_parser::k_test_program_0_parserout)).first;
	const auto pass2 = run_pass2(pass1);
	const auto pass2_output = parse_json(seq_t(floyd_parser::k_test_program_0_pass2output)).first;
	ut_compare_jsons(ast_to_json(pass2), pass2_output);
}

/*
QUARK_UNIT_TESTQ("run_pass2()", "k_test_program_100"){
	const auto pass1 = parse_json(seq_t(k_test_program_100_parserout)).first;
	const auto pass2 = run_pass2(pass1);
	const auto pass2_output = parse_json(seq_t("[12344]")).first;
	ut_compare_jsons(pass2, pass2_output);
}
*/

void test_error(const string& program, const string& error_string){
	const auto pass1 = floyd_parser::parse_program2(program);
	try{
		const auto pass2 = run_pass2(pass1);
		QUARK_UT_VERIFY(false);
	}
	catch(const std::runtime_error& e){
		if(error_string != ""){
			const auto s = string(e.what());
			quark::ut_compare(s, error_string);
		}
	}
}


#if false

QUARK_UNIT_TESTQ("run_pass2()", "1001"){
	test_error(
		R"(
			string main(){
				return 10 * "hello";
			}
		)",
		"1001 - Left & right side of math2 must have same type."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1002"){
	test_error(
		R"(
			string main(){
				int p = f();
				return p;
			}
		)",
		""//"1002 - Undefined function \"f\"."
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "1003"){
	test_error(
		R"(
			string main(){
				int p = main(42);
				return p;
			}
		)",
		"1003 - Wrong number of argument to function \"main\"."
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "1004"){
	test_error(
		R"(
			int a(string p1){
				return 31;
			}
			string main(){
				int p = a(42);
				return p;
			}
		)",
		""//"1004 - Argument 0 to function \"a\" mismatch."
	);
}

QUARK_UNIT_TESTQ("run_pass2()", "1005"){
	test_error(
		R"(
			string main(){
				return p;
			}
		)",
		""//"1005 - Undefined variable \"p\"."
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "Return undefine type"){
	test_error(
		R"(
			xyz main(){
				return 10;
			}
		)",
		"Undefined type \"xyz\""
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "1006"){
	test_error(
		R"(
			int main(){
				int a = "hello";
				return 0;
			}
		)",
		"1006 - Bind type mismatch."
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "1007"){
	test_error(
		R"(
			return 4;
			int main(){
				return 0;
			}
		)",
		"1007 - Return-statement not allowed outside function definition."
	);
}
#endif

#if false
QUARK_UNIT_TESTQ("run_pass2()", "1008"){
	test_error(
		R"(
			int main(){
				return "goodbye";
			}
		)",
		"1008 - return value doesn't match function return type."
	);
}
#endif


#if false
QUARK_UNIT_TESTQ("run_pass2()", "1010"){
	test_error(
		R"(
			struct pixel { string s = "two"; }
			string main(){
				pixel p = pixel_constructor();
				int a = p.xyz;
				return 1;
			}
		)",
		"Unresolved member \"xyz\""
	);
}
#endif
