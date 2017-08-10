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



pair<typeid_t, bool> resolve_type_name_int(const string& t){
	QUARK_ASSERT(t.size() > 2);

	if(t == "<null>"){
		return { typeid_t::make_null(), true };
	}
	else if(t == "<bool>"){
		return { typeid_t::make_bool(), true };
	}
	else if(t == "<int>"){
		return { typeid_t::make_int(), true };
	}
	else if(t == "<float>"){
		return { typeid_t::make_float(), true };
	}
	else if(t == "<string>"){
		return { typeid_t::make_string(), true };
	}
	else {
		return { typeid_t::make_string(), false };
	}
	QUARK_ASSERT(false);
}

typeid_t resolve_type_name(const string& t){
	QUARK_ASSERT(t.size() > 2);

	const auto a = resolve_type_name_int(t);
	if(a.second){
		return a.first;
	}
	else {
		return typeid_t::make_unresolved_symbol(trim_ends(t));
	}
}


expression_t parser_expression_to_ast(const json_t& e){
	QUARK_ASSERT(e.check_invariant());
	QUARK_TRACE(json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = type.get_string();
		if(type2 == "<null>"){
			return expression_t::make_constant_null();
		}
		else if(type2 == "<bool>"){
			return expression_t::make_constant_bool(value.is_false() ? false : true);
		}
		else if(type2 == "<int>"){
			return expression_t::make_constant_int((int)value.get_number());
		}
		else if(type2 == "<float>"){
			return expression_t::make_constant_float((float)value.get_number());
		}
		else if(type2 == "<string>"){
			return expression_t::make_constant_string(value.get_string());
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
		return expression_t::make_function_call(function_expr, args2, typeid_t::make_null());
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto member = e.get_array_n(2).get_string();
		return expression_t::make_resolve_member(base_expr, member, typeid_t::make_null());
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
		const auto variable_symbol = e.get_array_n(1).get_string();
		return expression_t::make_variable_expression(variable_symbol, typeid_t::make_null());
	}
	else{
		QUARK_ASSERT(false);
	}
}


/*
	Example:
		[
			{ "name": "g_version", "type": "<int>" },
			{ "name": "message", "type": "<string>" },
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

std::vector<typeid_t> get_member_types(const vector<member_t>& m){
	vector<typeid_t> r;
	for(const auto a: m){
		r.push_back(a._type);
	}
	return r;
}

struct body_t {
	public: const std::vector<std::shared_ptr<statement_t> > _statements;
	public: const std::vector<member_t> _locals;
	public: const std::map<int, std::shared_ptr<const lexical_scope_t> > _objects;
};

/*
	Input is an array of statements from parser.
	A function has its own list of statements.
*/
pair<body_t, int> parser_statements_to_ast(const json_t& p, int id_generator){
	QUARK_SCOPED_TRACE("parser_statements_to_ast()");
	QUARK_ASSERT(p.check_invariant());

	vector<shared_ptr<statement_t>> statements2;
	std::vector<member_t> locals;
	std::map<int, std::shared_ptr<const lexical_scope_t> > objects;

	for(const auto statement: p.get_array()){

		//	[ "return", [ "k", 3, "<int>" ] ]
		if(statement.get_array_n(0) == "return"){
			QUARK_ASSERT(statement.get_array_size() == 2);
			const auto expr = parser_expression_to_ast(statement.get_array_n(1));
			statements2.push_back(make_shared<statement_t>(make__return_statement(expr)));
		}

		//	[ "bind", "<float>", "x", EXPRESSION ],
		else if(statement.get_array_n(0) == "bind"){
			QUARK_ASSERT(statement.get_array_size() == 4);
			const auto bind_type = statement.get_array_n(1);
			const auto local_name = statement.get_array_n(2);
			const auto expr = statement.get_array_n(3);

			const auto bind_type2 = resolve_type_name(bind_type.get_string());
			const auto local_name2 = local_name.get_string();
			const auto expr2 = parser_expression_to_ast(expr);
			statements2.push_back(make_shared<statement_t>(make__bind_statement(local_name2, bind_type2, expr2)));
		}

		/*
			INPUT:
				[
					"def-func",
					{
						"args": [
							
						],
						"name": "main",
						"return_type": "<int>",
						"statements": [
							[ "return", [ "k", 3, "<int>" ] ]
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
							"return_type": "<int>",
							"statements": [???],
							"type": "function",
							"types": {}
						}
					}
				]
		*/
		else if(statement.get_array_n(0) == "def-func"){
			QUARK_ASSERT(statement.get_array_size() == 2);

			const auto def = statement.get_array_n(1);
			const auto name = def.get_object_element("name");
			const auto args = def.get_object_element("args");
			const auto statements = def.get_object_element("statements");
			const auto return_type = def.get_object_element("return_type");
			const auto r = parser_statements_to_ast(statements, id_generator);
			id_generator = r.second;

			const auto args2 = conv_members(args);
			const auto return_type2 = resolve_type_name(return_type.get_string());

			const auto function_typeid = typeid_t::make_function(return_type2, get_member_types(args2));

			//	Build function object.
			const auto s2 = lexical_scope_t::make_function_object(
				args2,
				{},
				r.first._statements,
				return_type2,
				r.first._objects
			);

			//	Make symbol refering to function object.
			const auto function_id = id_generator;
			id_generator +=1;

			value_t f = make_function_value(function_typeid, function_id);
			const auto function_constant = expression_t::make_constant_value(f);

			//	### Support overloading the same symbol name with different types.
			locals.push_back(member_t{ function_typeid, make_shared<value_t>(f), name.get_string() });
			objects[function_id] = s2;

			//### Key symbols with their type too. Support function overloading & struct named as function.
		}

		/*
			INPUT:
				[
					"def-struct",
					{
						"members": [
							{ "name": "red", "type": "<float>" },
							{ "name": "green", "type": "<float>" },
							{ "name": "blue", "type": "<float>" }
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
								{ "name": "red", "type": "<float>" },
								{ "name": "green", "type": "<float>" },
								{ "name": "blue", "type": "<float>" }
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
		else if(statement.get_array_n(0) == "def-struct"){
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
		else{
			throw std::runtime_error("Illegal statement.");
		}
	}

	return { body_t{ statements2, locals, objects }, id_generator };
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

	const auto program_body = parser_statements_to_ast(parse_tree, 1000);
	const auto a = lexical_scope_t::make_global_scope(
		program_body.first._statements,
		program_body.first._locals,
		program_body.first._objects
	);
	return ast_t(a);
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
