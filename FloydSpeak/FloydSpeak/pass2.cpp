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
#include "json_to_ast.h"

using namespace floyd_parser;
using namespace std;



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




string make_type_id_string(int id){
	return std::string("$" + std::to_string(id));
}

//	#Basic-types
bool is_basic_type(const std::string& s){
	return s == "<null>" || s == "<bool>" || s == "<int>" || s == "<float>" || s == "<string>";
}

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



json_t get_array_back(const json_t& array){
	if(!array.is_array()){
		throw;
	}
	else if(array.get_array_size() == 0){
		throw;
	}
	return array.get_array_n(array.get_array_size() - 1);
}


/*
	Converts output from parser - which is used lots of nested statements - into scope_defs.
*/

/*
	WARNING! Stores an **ARRAY** of types that is called "name"!

	scope_def: this is a scope_def. It's name and type will be used in type_entry.
*/
static json_t add_scope_type(const json_t& scope, const json_t& subscope){
	QUARK_ASSERT(scope.check_invariant());
	QUARK_ASSERT(subscope.check_invariant());

	const auto name = subscope.get_object_element("name").get_string();
	const auto type = subscope.get_object_element("type").get_string();
	const auto type_entry = json_t::make_object({
		{ "base_type", type },
		{ "scope_def", subscope }
	});
	if(exists_in(scope, make_vec({ "types", name }))){
		const auto index = get_in(scope, make_vec({ "types", name })).get_array_size();
		return assoc_in(scope, make_vec({"types", name, index }), type_entry);
	}
	else{
		return assoc_in(scope, make_vec({"types", name }), json_t::make_array({ type_entry }));
	}
}




/*
	Resolves all uses types into "$1234" syntax, deeply. Also puts the expression's type at end of each expression.
*/
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
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto op2 = string_to_math2_op(op);
		const auto lhs_expr = parser_expression_to_ast(e.get_array_n(1));
		const auto rhs_expr = parser_expression_to_ast(e.get_array_n(2));
		return expression_t::make_math2_operation(op2, lhs_expr, rhs_expr);
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
		const auto member = parser_expression_to_ast(e.get_array_n(2));
		return expression_t::make_resolve_member(base_expr, member.get_symbol(), typeid_t::make_null());
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
//		const auto variable = parser_expression_to_ast(e.get_array_n(1));
		const auto variable_symbol = e.get_array_n(1).get_string();
		return expression_t::make_resolve_variable(variable_symbol, typeid_t::make_null());
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
//		QUARK_ASSERT(arg_type[0] == '$');

		const auto arg_type2 = resolve_type_name(arg_type);
		members2.push_back(member_t(arg_type2, arg_name));
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
	public: std::map<std::string, symbol_t> _symbols;
	public: std::map<std::string, std::shared_ptr<const scope_def_t> > _objects;
};

/*
	Input is an array of statements from parser. A function has its own list of statements.
	There are no scope_defs, types, members, base_type etc.

	Returns a scope_def.
*/
pair<body_t, int> parser_statements_to_ast(const json_t& p, int id_generator){
	QUARK_SCOPED_TRACE("parser_statements_to_ast()");
	QUARK_ASSERT(p.check_invariant());

	vector<shared_ptr<statement_t>> statements2;
	std::map<std::string, symbol_t> symbols;
	std::map<std::string, std::shared_ptr<const scope_def_t> > objects;

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
			symbols[local_name2] = symbol_t{ symbol_t::k_constant, {}, make_shared<expression_t>(expr2), bind_type2 };
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
			const auto s2 = scope_def_t::make_function_object(
				args2,
				{},
				r.first._statements,
				return_type2,
				r.first._symbols,
				r.first._objects
			);

			//	Make symbol refering to function object.
			const auto function_id = to_string(id_generator);
			id_generator +=1;

			const auto function_constant = expression_t::make_function_value_constant(function_typeid, function_id);
			const auto symbol_data = symbol_t{ symbol_t::k_constant, {}, make_shared<expression_t>(function_constant), {} };

			symbols[name.get_string()] = symbol_data;
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

	return { body_t{ statements2,symbols, objects }, id_generator };
}




///////////////////////////////////////////		assign_unique_type_ids()


/*
	Scan tree, give each found type a unique ID, like "$1000" and "$1001". ID is unique
	in entire tree, not just in its own scope.
*/
pair<json_t, int> assign_unique_type_ids(const parser_path_t& path, int type_id_count){
	QUARK_SCOPED_TRACE("assign_unique_type_ids()");
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto type_id_count2 = type_id_count;
	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_t::make_object();

	const auto types = get_in(scope1, { "types" }).get_object();
	QUARK_TRACE(json_to_pretty_string(types));

	for(const auto t: types){
		const string type_name = t.first;
		const auto type_defs_vec = t.second.get_array();

		std::vector<json_t> type_defs2;
		for(const auto& type_def: type_defs_vec){
			if(!exists_in(type_def, make_vec({ "base_type"}))){
				throw std::runtime_error("No base type.");
			}

			const auto type_def2 = assoc(type_def, "id", json_t(make_type_id_string(type_id_count2)));
			type_id_count2++;

			const auto type_def3 = [&](){
				const json_t subscope = type_def2.get_optional_object_element("scope_def");
				if(!subscope.is_null()){
					const auto r = assign_unique_type_ids(go_down(path, subscope), type_id_count2);
					type_id_count2 = r.second;
					auto temp = assoc(type_def2, "scope_def", r.first);
					return temp;
				}
				else{
					return type_def2;
				}
			}();
			type_defs2.push_back(type_def3);
		}
		types_collector2 = assoc(types_collector2, type_name, json_t(type_defs2));
	}

	const auto scope2 = assoc(scope1, "types", types_collector2);
	QUARK_TRACE(json_to_pretty_string(scope2));
	return { scope2, type_id_count2 };
}

QUARK_UNIT_TESTQ("assign_unique_type_ids()", ""){
	const auto test = parse_json(seq_t(
		R"(
			{
				"name": "global",
				"type": "global",
				"members": [
					{ "type": "<int>", "name": "g_version", "expr": 1.0 },
					{ "type": "<string>", "name": "message", "expr": "Welcome!" }
				],
				"types": {
					"bool": [ { "base_type": "bool" } ],
					"int": [ { "base_type": "int" } ],
					"string": [ { "base_type": "string" } ],
					"pixel_t": [
						{
							"base_type": "function",
							"scope_def": {
								"name": "pixel_t_constructor",
								"type": "function",
								"args": [],
								"locals": [
									{ "type": "<pixel_t>", "name": "it" },
									{ "type": "<int>", "name": "x2" }
								],
								"types": {},
								"statements": [
									["return", ["call", ["@", "___body"], [ ["k", "<pixel_t>", 100] ]]]
								],
								"return_type": "<int>"
							}
						},
						{
							"base_type": "struct",
							"scope_def": {
								"name": "pixel_t",
								"type": "struct",
								"members": [
									{ "type": "<int>", "name": "red" },
									{ "type": "<int>", "name": "green" },
									{ "type": "<int>", "name": "blue" }
								],
								"types": {},
								"statements": [],
								"return_type": null
							}
						}
					],
					"main": [
						{
							"base_type": "function",
							"scope_def": {
								"name": "main",
								"type": "function",
								"args": [
									{ "type": "<string>", "name": "args" }
								],
								"locals": [
									{ "type": "<pixel_t>", "name": "p1" },
									{ "type": "<pixel_t>", "name": "p2" }
								],
								"types": {},
								"statements": [
									["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_t"], [["k", "<int>", 42]]]],
									["return", ["+", ["@", "p1"], ["@", "g_version"]]]
								],
								"return_type": "<int>"
							}
						}
					]
				}
			}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = assign_unique_type_ids(make_parser_path(test), 1000);
	QUARK_UT_VERIFY(result.first.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result.first));
}




///////////////////////////////////////////		insert_generated_functions()


/*
	Insert dummy-hook for constructors. Implementation needs to be provided by later pass.
	??? How to call host code. Is host code represented by a scope_def? I does need args/return value.
	??? Add function body here too!
*/

pair<json_t, int> insert_generated_functions(const parser_path_t& path, int type_id_count){
	QUARK_SCOPED_TRACE("PASS A5");
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto type_id_count2 = type_id_count;
	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_t::make_object();
	{
		const auto types = get_in(scope1, { "types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_js = type_entry_pair.second;
			const auto type_defs_vec = type_defs_js.get_array();

			std::vector<json_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				json_t type_def2 = type_def;

				//	If this is a struct, insert contructor(s).
				const auto base_type = type_def2.get_object_element("base_type");
				if(base_type == "struct"){
					const json_t struct_scope = type_def2.get_object_element("scope_def");

					const string struct_name = struct_scope.get_object_element("name").get_string();
					const string constructor_name = struct_name + "_constructor";

					/*
						Make default constructor for the struct.
					*/
					json_t constructor_def = make_scope_def();
					constructor_def = store_object_member(constructor_def, "type", "function");
					constructor_def = store_object_member(constructor_def, "name", constructor_name);
					constructor_def = store_object_member(constructor_def, "args", json_t::make_array());
					constructor_def = store_object_member(constructor_def, "locals", json_t::make_array());
					constructor_def = store_object_member(constructor_def, "statements", json_t::make_array());
					constructor_def = store_object_member(constructor_def, "types", json_t::make_object());
					constructor_def = store_object_member(constructor_def, "return_type", "<" + struct_name + ">");
					constructor_def = store_object_member(constructor_def, "function_type", "def-constructor");
					const auto constructor_type_def = json_t::make_object({
						{ "id", json_t(make_type_id_string(type_id_count2)) },
						{ "base_type", "function" },
						{ "scope_def", constructor_def }
					});
					type_id_count2++;

					//	### Add call to insert_generated_functions() on new constructor function!

					types_collector2 = assoc(types_collector2, constructor_name, json_t::make_array({ constructor_type_def }));
				}

				//	Propage down to all nodes -> leaves in tree.
				const json_t subscope = type_def2.get_optional_object_element("scope_def");
				if(!subscope.is_null()){
					const auto r = insert_generated_functions(go_down(path, subscope), type_id_count2);
					type_id_count2 = r.second;
					auto temp = assoc(type_def2, "scope_def", r.first);
					type_def2 = temp;
				}

				type_defs2.push_back(type_def2);
			}
			types_collector2 = assoc(types_collector2, type_name, json_t(type_defs2));
		}
	}

	const auto scope2 = assoc(scope1, "types", types_collector2);
	QUARK_TRACE(json_to_pretty_string(scope2));
	return { scope2, type_id_count2 };
}

QUARK_UNIT_TESTQ("insert_generated_functions()", ""){
	const auto test = parse_json(seq_t(
		R"(
			{
				"members": [
					{ "expr": 1, "name": "g_version", "type": "<int>" },
					{ "expr": "Welcome!", "name": "message", "type": "<string>" }
				],
				"name": "global",
				"type": "global",
				"types": {
					"bool": [{ "base_type": "bool", "id": "$1000" }],
					"int": [{ "base_type": "int", "id": "$1001" }],
					"main": [
						{
							"base_type": "function",
							"id": "$1002",
							"scope_def": {
								"args": [{ "name": "args", "type": "<string>" }],
								"locals": [
									{ "name": "p1", "type": "<pixel_t>" },
									{ "name": "p2", "type": "<pixel_t>" }
								],
								"name": "main",
								"return_type": "<int>",
								"statements": [
									["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_t"], [["k", "<int>", 42]]]],
									["return", ["+", ["@", "p1"], ["@", "g_version"]]]
								],
								"type": "function",
								"types": {}
							}
						}
					],
					"pixel_t": [
						{
							"base_type": "struct",
							"id": "$1004",
							"scope_def": {
								"members": [
									{ "name": "red", "type": "<int>" },
									{ "name": "green", "type": "<int>" },
									{ "name": "blue", "type": "<int>" }
								],
								"name": "pixel_t",
								"return_type": null,
								"statements": [],
								"type": "struct",
								"types": {}
							}
						}
					],
					"string": [{ "base_type": "string", "id": "$1005" }]
				}
			}		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = insert_generated_functions(make_parser_path(test), 100000);
	QUARK_UT_VERIFY(result.first.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result.first));
}



///////////////////////////////////////////		resolve_scope_typenames()


/*
	Scan tree: resolve type references by storing the type-ID.
	Turns "<int>" to "$1041" etc.

	- Replaces "pixel" with "$1008" -- the ID of the type.
	- Stamps each expression with its output type.
	- Checks for type mismatches. ??? Better to do this inseparate pass!
*/

enum class eresolve_types {
	k_all,
	k_all_but_function,
	k_only_function,
};

/*
	returns "$1003"-style ID-string.
	Throws if not found.
*/
pair<std::string, json_t> resolve_custom_typename(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);
	QUARK_ASSERT(type_name0.front() == '<' && type_name0.back() == '>');
	QUARK_TRACE(json_to_pretty_string(path._scopes.front()));

	const string type_name = trim_ends(type_name0);
	for(auto i = path._scopes.size() ; i > 0 ; i--){
		const auto& scope = path._scopes[i - 1];
		const auto type = scope.get_object_element("types").get_optional_object_element(type_name);
		if(!type.is_null()){
			const auto hits0 = type.get_array();
			vector<json_t> hits1;
			if(types == eresolve_types::k_all){
				hits1 = hits0;
			}
			else if(types == eresolve_types::k_all_but_function){
				for(const auto& hit: hits0){
					const auto base_type = hit.get_object_element("base_type").get_string();
					if(base_type != "function"){
						hits1.push_back(hit);
					}
				}
			}
			else if(types == eresolve_types::k_only_function){
				for(const auto& hit: hits0){
					const auto base_type = hit.get_object_element("base_type").get_string();
					if(base_type == "function"){
						hits1.push_back(hit);
					}
				}
			}
			else{
				QUARK_ASSERT(false);
			}

			if(hits1.size() == 0){
			}
			else if(hits1.size() == 1){
				const auto id = type.get_array_n(0).get_object_element("id").get_string();
				return { id, type.get_array_n(0) };
			}
			else {
				QUARK_TRACE(json_to_pretty_string(scope));

				//??? DO better matching - if a function is wanted, see if there is only one function.
				throw std::runtime_error("Multiple definitions for type-identifier \"" + type_name + "\".");
			}
		}
		else{
		}
	}
	throw std::runtime_error("Undefined type \"" + type_name + "\"");
}

std::string resolve_typename(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);

	//	Already resolved?
	if(type_name0.front() == '$'){
		return type_name0;
	}
	else{
		//	Basic-type?
		if(is_basic_type(type_name0)){
			return "$" + trim_ends(type_name0);
		}
		else{

			//	Custom type -- look it up.
			return resolve_custom_typename(path, type_name0, types).first;
		}
	}
}

//	Resolve a variable is done by scanning the compile-time scopes upwards towards globals.
//	Returns the member-list/arg_list etc. that holds the variable AND the index inside _members where it is found.
pair<json_t, int> resolve_variable(const parser_path_t& path, const string& variable_name){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(variable_name.size() > 0);

	//	Look s in each scope's members, one at a time until we searched the global scope.
	for(auto i = path._scopes.size() ; i > 0 ; i--){
		const auto& scope_def = path._scopes[i - 1];

		if(scope_def.does_object_element_exist("members")){
			const auto members = scope_def.get_object_element("members").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("members"), index };
				}
			}
		}

		if(scope_def.does_object_element_exist("args")){
			const auto members = scope_def.get_object_element("args").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("args"), index };
				}
			}
		}

		if(scope_def.does_object_element_exist("locals")){
			const auto members = scope_def.get_object_element("locals").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("locals"), index };
				}
			}
		}
	}
	return { {}, -1 };
}

json_t resolve_expression_typenames(const parser_path_t& path, const json_t& expression);

json_t resolve_members_typenames(const parser_path_t& path, const json_t& members){
	if(members.is_null()){
		return members;
	}
	else{
		const auto member_vec = members.get_array();
		vector<json_t> member_vec2;
		for(const auto& member: member_vec){
			if(!member.is_object()){
				throw std::runtime_error("Member definitions must be JSON objects.");
			}

			const auto type = member.get_object_element("type").get_string();

			auto member2 = member;
			if(type != ""){
				const auto type2 = resolve_typename(path, type, eresolve_types::k_all_but_function);
				member2 = assoc(member2, "type", type2);
			}
			else{
			}

			const auto expression = member.get_optional_object_element("expr");
			if(!expression.is_null()){
				const auto expression2 = resolve_expression_typenames(path, expression);
				member2 = assoc(member2, "expr", expression2);
			}

			member_vec2.push_back(member2);
		}
		return json_t(member_vec2);
	}
}

//	Array of arguments, part of function call.
json_t resolve_arguments_typenames(const parser_path_t& path, const json_t& args1){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(args1.check_invariant());

	vector<json_t> args2;
	for(const auto& arg: args1.get_array()){
		args2.push_back(resolve_expression_typenames(path, arg));
	}
	return args2;
}

/*
	Resolves all uses types into "$1234" syntax, deeply. Also puts the expression's type at end of each expression.
*/
json_t resolve_expression_typenames(const parser_path_t& path, const json_t& expression){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(expression.check_invariant());
	QUARK_TRACE(json_to_pretty_string(expression));

	const auto e = expression;
	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = resolve_typename(path, type.get_string(), eresolve_types::k_all_but_function);

		return json_t::make_array({
			op,
			value,
			type2,
		});
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto lhs_expr = resolve_expression_typenames(path, e.get_array_n(1));
		const auto rhs_expr = resolve_expression_typenames(path, e.get_array_n(2));

		const auto lhs_type = get_array_back(lhs_expr);
		const auto rhs_type = get_array_back(rhs_expr);
		if(lhs_type != rhs_type){
			throw std::runtime_error("1001 - Left & right side of math2 must have same type.");
		}
		return json_t::make_array({ op, lhs_expr, rhs_expr, lhs_type });
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto condition_expr = resolve_expression_typenames(path, e.get_array_n(1));
		const auto a_expr = resolve_expression_typenames(path, e.get_array_n(2));
		const auto b_expr = resolve_expression_typenames(path, e.get_array_n(3));

		const auto condition_type = get_array_back(condition_expr);

		const auto a_type = get_array_back(a_expr);
		const auto b_type = get_array_back(b_expr);
		if(a_type != b_type){
			throw std::runtime_error("1001 - Left & right side of (?:) must have same type.");
		}
		return json_t::make_array({ op, condition_expr, a_expr, b_expr, a_type });
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const json_t function_expr = resolve_expression_typenames(path, e.get_array_n(1));
		const json_t args_expr = resolve_arguments_typenames(path, e.get_array_n(2));

		//	The call-expression's output type will be the same as the return-type of the function-def.
		const auto return_type = resolve_typename(path, get_array_back(function_expr).get_string(), eresolve_types::k_only_function);
//			throw std::runtime_error("1002 - Undefined function \"" + call._function.to_string() + "\".");

		//??? Check arguments too.
		return json_t::make_array({ op, function_expr, args_expr, return_type });
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = resolve_expression_typenames(path, e.get_array_n(1));
		const auto type = get_array_back(base_expr).get_string();
		QUARK_ASSERT(type.front() == '$');
		return json_t::make_array({ op, base_expr, e.get_array_n(2), type });
	}
	else if(op == "@"){
		//	###: Idea: Functions are found via a variable called the function name, pointing to the function_def.
		QUARK_ASSERT(e.get_array_size() == 2);

		const auto variable_name = e.get_array_n(1).get_string();
		pair<json_t, int> found_variable = resolve_variable(path, variable_name);
		if(!found_variable.first.is_null()){
			json_t member = found_variable.first.get_array_n(found_variable.second);
			const auto type = member.get_object_element("type").get_string();
			const auto type_id = resolve_typename(path, type, eresolve_types::k_all);
			return json_t::make_array({ op, variable_name, type_id });
		}
		else{
			auto found_function = resolve_custom_typename(path, "<" + variable_name + ">", eresolve_types::k_only_function);

			//	OK, we're resolving a function, not a variable. ### Code this differently?
			const auto return_type = found_function.second.get_object_element("scope_def").get_object_element("return_type").get_string();
			QUARK_ASSERT(return_type.back() == '>');
			const auto return_type_id = resolve_typename(path, return_type, eresolve_types::k_all);
			return json_t::make_array({ op, variable_name, return_type_id });
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}

json_t resolve_scope_typenames(const parser_path_t& path0){
	QUARK_SCOPED_TRACE("PASS B");
	QUARK_ASSERT(path0.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path0._scopes.back()));

	const auto scope1 = path0._scopes.back();

	//	Update all nested types.
	auto types_collector2 = json_t::make_object();
	{
		const auto types = get_in(scope1, { "types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_vec = type_entry_pair.second.get_array();

			std::vector<json_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				const auto type_def3 = [&](){
					const json_t subscope = type_def.get_optional_object_element("scope_def");
					if(!subscope.is_null()){
						const auto subscope2 = resolve_scope_typenames(go_down(replace_leaf(path0, scope1), subscope));
						return assoc(type_def, "scope_def", subscope2);
					}
					else{
						return type_def;
					}
				}();
				type_defs2.push_back(type_def3);
			}
			types_collector2 = assoc(types_collector2, type_name, json_t(type_defs2));
		}
	}
	auto scope2 = assoc(scope1, "types", types_collector2);

	//	Update members, args and locals.
	auto path1 = replace_leaf(path0, scope2);
	scope2 = assoc(scope2, {"members"}, resolve_members_typenames(path1, scope2.get_optional_object_element("members")));
	scope2 = assoc(scope2, {"args"}, resolve_members_typenames(path1, scope2.get_optional_object_element("args")));
	scope2 = assoc(scope2, {"locals"}, resolve_members_typenames(path1, scope2.get_optional_object_element("locals")));

	//	Update return type.
	if(scope2.does_object_element_exist("return_type") && scope2.get_object_element("return_type").get_string() != ""){
		scope2 = assoc(
			scope2,
			{"return_type"},
			resolve_typename(path1, scope2.get_optional_object_element("return_type", "<null>").get_string(), eresolve_types::k_all)
		);
	}

	//	Update statements.
	const auto statements = scope2.get_optional_object_element("statements");
	if(!statements.is_null()){
		vector<json_t> statements2;
		const auto statements_vec = statements.get_array();
		for(const auto& s: statements_vec){
			json_t op = s.get_array_n(0);
			if(op == "bind"){
				const auto type = s.get_array_n(1);
				const auto type2 = resolve_typename(path1, type.get_string(), eresolve_types::k_all);

				const auto identifier = s.get_array_n(2);

				const auto expr = s.get_array_n(3);
				const auto expr2 = resolve_expression_typenames(path1, expr);
				statements2.push_back(json_t::make_array({ op, type2, identifier, expr2 }));
			}
			else if(op == "define_struct"){
				QUARK_ASSERT(false);
			}
			else if(op == "define_function"){
				QUARK_ASSERT(false);
			}
			else if(op == "return"){
				const auto expr = resolve_expression_typenames(path1, s.get_array_n(1));
				statements2.push_back(json_t::make_array({ op, expr }));
			}
			else{
				throw std::runtime_error("Unknown statement operation");
			}
		}
		scope2 = assoc(scope2, {"statements"}, statements2);
	}

	QUARK_TRACE(json_to_pretty_string(scope2));
	return scope2;
}

QUARK_UNIT_TESTQ("resolve_scope_typenames()", "Test minimal program -- are all references to types resolved?"){
	const auto test = parse_json(seq_t(
		R"(
		{
			"members": [],
			"name": "global",
			"type": "global",
			"types": {
				"bool": [{ "base_type": "bool", "id": "$100001" }],
				"int": [{ "base_type": "int", "id": "$100002" }],
				"main": [
					{
						"base_type": "function",
						"id": "$100003",
						"scope_def": {
							"args": [],
							"locals": [],
							"name": "main",
							"return_type": "<int>",
							"statements": [ ["return", ["k", "hello!", "<string>"]]],
							"type": "function",
							"types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$100004" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = resolve_scope_typenames(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}

QUARK_UNIT_TESTQ("resolve_scope_typenames()", "Are types of locals resolved?"){
	const auto test = parse_json(seq_t(
		R"(
		{
			"members": [],
			"name": "global",
			"type": "global",
			"types": {
				"bool": [{ "base_type": "bool", "id": "$100001" }],
				"int": [{ "base_type": "int", "id": "$100002" }],
				"main": [
					{
						"base_type": "function",
						"id": "$100003",
						"scope_def": {
							"args": [],
							"locals": [
								{ "name": "p1", "type": "<int>" },
								{ "name": "p2", "type": "<string>" },
								{ "name": "p3", "type": "<bool>" }
							],
							"name": "main",
							"return_type": "<int>",
							"statements": [ ["return", ["k", "hello!", "<string>"]]],
							"type": "function",
							"types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$100004" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = resolve_scope_typenames(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}

QUARK_UNIT_TESTQ("resolve_scope_typenames()", ""){
	const auto test = parse_json(seq_t(
		R"(
		{
			"members": [
				{ "expr": ["k", 1, "<int>"], "name": "g_version", "type": "<int>" },
				{ "expr": ["k", 1, "<int>"], "name": "g_base_version", "type": "<int>" },
				{ "expr": ["k", "Welcome!", "<string>"], "name": "message", "type": "<string>" }
			],
			"name": "global",
			"type": "global",
			"types": {
				"bool": [{ "base_type": "bool", "id": "$1000" }],
				"int": [{ "base_type": "int", "id": "$1001" }],
				"main": [
					{
						"base_type": "function",
						"id": "$1002",
						"scope_def": {
							"args": [{ "name": "args", "type": "<string>" }],
							"locals": [
								{ "name": "p1", "type": "<pixel_t>" },
								{ "name": "p2", "type": "<pixel_t>" }
							],
							"name": "main",
							"return_type": "<int>",
							"statements": [
								["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_constructor"], [["k", 42, "<int>"]]]],
								["return", ["+", ["@", "g_base_version"], ["@", "g_version"]]]
							],
							"type": "function",
							"types": {
							}
						}
					}
				],
				"pixel_constructor": [
					{
						"base_type": "function",
						"id": "$1003",
						"scope_def": {
							"args": [],
							"locals": [{ "name": "it", "type": "<pixel_t>" }, { "name": "x2", "type": "<int>" }],
							"name": "pixel_t_constructor",
							"return_type": "<int>",
							"statements": [
							],
							"type": "function",
							"types": {}
						}
					},
				],

				"pixel_t": [
					{
						"base_type": "struct",
						"id": "$1004",
						"scope_def": {
							"members": [
								{ "name": "red", "type": "<int>" },
								{ "name": "green", "type": "<int>" },
								{ "name": "blue", "type": "<int>" }
							],
							"name": "pixel_t",
							"statements": [],
							"type": "struct",
							"types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$1005" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = resolve_scope_typenames(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());


	const string result_str = json_to_pretty_string(result);
	QUARK_TRACE(result_str);
	const auto it = result_str.find(">\"");
	QUARK_ASSERT(it == string::npos);
}



///////////////////////////////////////////		make_central_type_lookup()


//	Collects all "types" tables and merge them.



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

//	Returns new scope + lookup.
pair<json_t, json_t> make_central_type_lookup(const json_t& lookup, const parser_path_t& path){
	QUARK_ASSERT(lookup.check_invariant());
	QUARK_ASSERT(lookup.is_object());
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto scope1 = path._scopes.back();
	const auto types = get_in(scope1, { "types" }).get_object();
	auto lookup1 = lookup;
	scope1 = dissoc(scope1, "types");

	for(const auto type_entry_pair: types){
		const auto type_defs_js = type_entry_pair.second;
		const auto type_defs_vec = type_defs_js.get_array();

		for(const auto& type_def: type_defs_vec){
			QUARK_TRACE(json_to_pretty_string(type_def));

			const auto type_id = type_def.get_object_element("id");
			auto v2 = dissoc(type_def, "id");
			const auto path_str = make_path_string(path, type_entry_pair.first);
			v2 = assoc(v2, "path", path_str);
			lookup1 = assoc(lookup1, type_id, v2);

			const json_t subscope = type_def.get_optional_object_element("scope_def");
			if(!subscope.is_null()){
				const auto res = make_central_type_lookup(lookup1, go_down(path, subscope));
				lookup1 = res.second;
			}
		}
	}

	QUARK_TRACE(json_to_pretty_string(scope1));
	QUARK_TRACE(json_to_pretty_string(lookup1));
	return { scope1, lookup1 };
}

json_t make_central_type_lookup(const parser_path_t& path){
	QUARK_SCOPED_TRACE("PASS C");
	QUARK_TRACE(json_to_pretty_string(path._scopes.front()));

	QUARK_ASSERT(path.check_invariant());

	const auto a = make_central_type_lookup(json_t::make_object(), path);

	const auto result = json_t::make_object({
		{ "global", a.first },
		{ "lookup", a.second }
	});

	QUARK_TRACE(json_to_pretty_string(result));
	return result;
}

const json_t make_test_pass_c(){
std::pair<json_t, seq_t> k_test = parse_json(seq_t(
	R"(
			{
				"args": [],
				"locals": [],
				"members": [
					{ "expr": 1, "name": "g_version", "type": "$1001" },
					{ "expr": 1, "name": "g_base_version", "type": "$1001" },
					{ "expr": "Welcome!", "name": "message", "type": "$1005" }
				],
				"name": "global",
				"type": "global",
				"types": {
					"bool": [{ "base_type": "bool", "id": "$1000" }],
					"int": [{ "base_type": "int", "id": "$1001" }],
					"main": [
						{
							"base_type": "function",
							"id": "$1002",
							"scope_def": {
								"args": [{ "name": "args", "type": "$1005" }],
								"locals": [{ "name": "p1", "type": "$1004" }, { "name": "p2", "type": "$1004" }],
								"members": [],
								"name": "main",
								"return_type": "$1001",
								"statements": [
									[
										"bind",
										"<pixel_t>",
										"p1",
										["call", ["@", "pixel_constructor", "$1001"], [["k", 42, "$1001"]], "$1001"]
									],
									[
										"return",
										["+", ["@", "g_base_version", "$1001"], ["@", "g_version", "$1001"], "$1001"]
									]
								],
								"type": "function",
								"types": {}
							}
						}
					],
					"pixel_constructor": [
						{
							"base_type": "function",
							"id": "$1003",
							"scope_def": {
								"args": [],
								"locals": [{ "name": "it", "type": "$1004" }, { "name": "x2", "type": "$1001" }],
								"members": [],
								"name": "pixel_t_constructor",
								"return_type": "$1001",
								"statements": [],
								"type": "function",
								"types": {}
							}
						}
					],
					"pixel_t": [
						{
							"base_type": "struct",
							"id": "$1004",
							"scope_def": {
								"args": [],
								"locals": [],
								"members": [
									{ "name": "red", "type": "$1001" },
									{ "name": "green", "type": "$1001" },
									{ "name": "blue", "type": "$1001" }
								],
								"name": "pixel_t",
								"statements": [],
								"type": "struct",
								"types": {}
							}
						}
					],
					"string": [{ "base_type": "string", "id": "$1005" }]
				}
			}
	)"
));

	return k_test.first;
}

QUARK_UNIT_TESTQ("make_central_type_lookup()", ""){
	const auto test = make_test_pass_c();
	QUARK_TRACE(json_to_pretty_string(test));

	const auto c = make_central_type_lookup(make_parser_path(test));

	QUARK_UT_VERIFY(c.check_invariant());
	QUARK_TRACE(json_to_pretty_string(c));
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
	const auto a = scope_def_t::make_global_scope(
		program_body.first._statements,
		program_body.first._symbols,
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
	const auto pass1 = parse_json(seq_t(k_test_program_0_parserout)).first;
	const auto pass2 = run_pass2(pass1);
	const auto pass2_output = parse_json(seq_t(k_test_program_0_pass2output)).first;
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
	const auto pass1 = parse_program2(program);
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
