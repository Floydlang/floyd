//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//



/*
PROBLEM: How to store links to resolved to types or variables?

TODO: Augument existing data, don't replace typename / variable name.
TODO: Generate type IDs like this: "$global/pixel_t"

- Need to be able to insert new types, function, variables without breaking links.

A) { int scope_up_count, int item_index }. Problem - item_index breaks if we insert / reorder parent scope members.
B) Resolve by checking types exist, but do not store new info.
C) Generate IDs for types. Use type-lookup from ID to correct entry. Store ID in each resolved place. Put all
CHOSEN ==>>>> D) Create global type-lookup table. Use original static-scope-path as ID. Use these paths to refer from client to type.

Solution D algo steps:
Pass A) Scan tree, give each found type a unique ID. (Tag each scope and assign parent-scope ID.)
Pass A5) Insert constructors for each struct.
Pass B) Scan tree: resolve type references by storing the type-ID. Tag expressions with their output-type. Do this will we can see the scope of each type.
Pass C) Scan tree: move all types to global list for fast finding from ID.
Pass D) Scan tree: bind variables to type-ID + offset.

Now we can convert to a typesafe AST!


Result after transform C.
======================================
"lookup": {
	"$1": { "name": "bool", "base_type": "bool" },
	"$2": { "name": "int", "base_type": "int" },
	"$3": { "name": "string", "base_type": "string" },
	"$4": { "name": "pixel_t", "base_type": "function", "scope_def":
		{
			"parent_scope": "$5",		//	Parent scope.
			"name": "pixel_t_constructor",
			"type": "function",
			"args": [],
			"locals": [
				["$5", "it"],
				["$4", "x2"]
			],
			"statements": [
				["call", "___body"],
				["return", ["k", "$5", 100]]
			],
			"return_type": "$4"
		}
	},
	"$5": { "name": "pixel_t", "base_type": "struct", "scope_def":
		{
			"name": "pixel_t",
			"type": "struct",
			"members": [
				{ "type": "$4", "name": "red"},
				{ "type": "$4", "name": "green"},
				{ "type": "$4", "name": "blue"}
			],
			"types": {},
			"statements": [],
			"return_type": null
		}
	},
	"$6": { "name": "main", "base_type": "function", "scope_def":
		{
			"name": "main",
			"type": "function",
			"args": [
				["$3", "args"]
			],
			"locals": [
				["$5", "p1"],
				["$5", "p2"]
			],
			"types": {},
			"statements": [
				["bind", "<pixel_t>", "p1", ["call", "pixel_t_constructor"]],
				["return", ["+", ["@", "p1"], ["@", "g_version"]]]
			],
			"return_type": "$4"
		}
	}
},
"global_scope": {
	"name": "global",
	"type": "global",
	"members": [
		{ "type": "$4", "name": "g_version", "expr": "1.0" },
		{ "type": "$3", "name": "message", "expr": "Welcome!"}
	],
}
*/

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





	//////////////////////		Traversal of parse tree

	struct parser_path_t {
		//	Returns a scope_def in json format.
		public: json_t get_leaf() const;

		public: bool check_invariant() const;


		public: std::vector<json_t> _scopes;
	};



//////////////////////////////////////////////////		parser_path_t


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




string make_type_id_string(int id){
	return std::string("$" + std::to_string(id));
}



///////////////////////////////////////////		statements_to_scope()


/*
	WARNING! Stores an ARRAY of types that is called "name"!

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
		return assoc_in(
			scope,
			make_vec({"types", name }),
			json_t::make_array2({ type_entry })
		);
	}
}


/*
	Input is an array of statements from parser. A function has its own list of statements.
	There are no scope_defs, types, members, base_type etc.

	Returns a scope_def.
*/
json_t statements_to_scope(const json_t& p){
	QUARK_SCOPED_TRACE("statements_to_scope()");
	QUARK_ASSERT(p.check_invariant());

	auto scope2 = make_scope_def();
	for(const auto statement: p.get_array()){
		const auto statement_type = statement.get_array_n(0);

		if(statement_type == "return"){
			scope2 = store_object_member(scope2, "statements", push_back(scope2.get_object_element("statements"), statement));
		}
		else if(statement_type == "bind"){
			const auto bind_type = statement.get_array_n(1);
			const auto local_name = statement.get_array_n(2);
			const auto expr = statement.get_array_n(3);
			const auto loc = make_member_def(bind_type.get_string(), local_name.get_string(), json_t());

			//	Reserve an entry in _members-vector for our variable.
			scope2 = store_object_member(scope2, "locals", push_back(scope2.get_object_element("locals"), loc));
			scope2 = store_object_member(scope2, "statements", push_back(scope2.get_object_element("statements"), statement));
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
							[
								"return",
								[
									"k",
									3,
									"<int>"
								]
							]
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
		else if(statement_type == "def-func"){
			const auto function_def = statement.get_array_n(1);
			const auto name = function_def.get_object_element("name");
			const auto args = function_def.get_object_element("args");
			const auto statements = function_def.get_object_element("statements");
			const auto return_type = function_def.get_object_element("return_type");

			const auto function_body_scope = statements_to_scope(statements);
			const auto locals = function_body_scope.get_object_element("locals");
			const auto types = function_body_scope.get_object_element("types");

			auto function_scope = make_scope_def();
			function_scope = store_object_member(function_scope, "type", "function");
			function_scope = store_object_member(function_scope, "name", name);
			function_scope = store_object_member(function_scope, "args", args);
			function_scope = store_object_member(function_scope, "locals", locals);
			function_scope = store_object_member(function_scope, "statements", statements);
			function_scope = store_object_member(function_scope, "types", types);
			function_scope = store_object_member(function_scope, "return_type", return_type);
			scope2 = add_scope_type(scope2, function_scope);
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
		else if(statement_type == "def-struct"){
			const auto struct_def = statement.get_array_n(1);
			const auto name = struct_def.get_object_element("name");
			const auto members = struct_def.get_object_element("members");

			json_t struct_scope = make_scope_def();
			struct_scope = store_object_member(struct_scope, "type", "struct");
			struct_scope = store_object_member(struct_scope, "name", name);
			struct_scope = store_object_member(struct_scope, "members", members);
			scope2 = add_scope_type(scope2, struct_scope);
		}
		else{
			throw std::runtime_error("Illegal statement.");
		}
	}

	QUARK_TRACE(json_to_pretty_string(scope2));
	return scope2;
}




///////////////////////////////////////////		assign_unique_type_ids()


/*
	Scan tree, give each found type a unique ID.
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


//	Pass A5) Insert constructors for each struct.


/*
	Insert dummy-hook for constructors. Implementation needs to be provided by later pass.

	??? Calling all constructor function "pixel_t" requires us to select function from its signature.
		Alternative use one function with var-args.

	??? How to call host code. Is host code represented by a scope_def? I does need args/return value.
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

					types_collector2 = assoc(types_collector2, constructor_name, json_t::make_array2({ constructor_type_def }));
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







///////////////////////////////////////////		PASS B


/*
	Pass B) Scan tree: resolve type references by storing the type-ID.
	Turns "<int>" to "$1041" etc.

	Also tags each expression with its output type. It is appended to back of each expression.
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
pair<std::string, json_t> pass_b__type_to_id_xxx(const parser_path_t& path, const string& type_name0, eresolve_types types){
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

//	#Basic-types
bool is_basic_type(const std::string& s){
	return s == "<null>" || s == "<bool>" || s == "<int>" || s == "<float>" || s == "<string>";
}


std::string pass_b__type_to_id(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);

	if(type_name0.front() == '$'){
		return type_name0;
	}
	else{
		//	#Basic-types
		if(is_basic_type(type_name0)){
			return "$" + trim_ends(type_name0);
		}
		else{
			return pass_b__type_to_id_xxx(path, type_name0, types).first;
		}
	}
}


//??? Should already have separated functions into outer + body-scopes before doing this!

//	Resolve a variable is done by scanning the compile-time scopes upwards towards globals.
//	Returns the member-list/arg_list etc. that holds the variable AND the index inside _members where it is found.
pair<json_t, int> pass_b__resolve_scoped_variable(const parser_path_t& path, const string& variable_name){
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

json_t pass_b__expression(const parser_path_t& path, const json_t& e);


json_t pass_b__members(const parser_path_t& path, const json_t& members){
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
				const auto type2 = pass_b__type_to_id(path, type, eresolve_types::k_all_but_function);
				member2 = assoc(member2, "type", type2);
			}
			else{
			}

			const auto expression = member.get_optional_object_element("expr");
			if(!expression.is_null()){
				const auto expression2 = pass_b__expression(path, expression);
				member2 = assoc(member2, "expr", expression2);
			}

			member_vec2.push_back(member2);
		}
		return json_t(member_vec2);
	}
}



//	Array of arguments, part of function call.
json_t pass_b__arguments(const parser_path_t& path, const json_t& args1){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(args1.check_invariant());

	vector<json_t> args2;
	for(const auto& arg: args1.get_array()){
		args2.push_back(pass_b__expression(path, arg));
	}
	return args2;
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


//??? Just search tree for "<...>" and replace - no need to understand semantics of tree!

/*
	Resolves all uses types into "$1234" syntax, deeply. Also puts the expression's type at end of each expression.
*/
json_t pass_b__expression(const parser_path_t& path, const json_t& e){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_TRACE(json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = pass_b__type_to_id(path, type.get_string(), eresolve_types::k_all_but_function);

		return json_t::make_array2({
			op,
			value,
			type2,
		});
	}
	else if(is_math1_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto expr = pass_b__expression(path, e.get_array_n(1));
		const auto type = expr.get_array_n(3);
		return json_t::make_array2({ op, expr, type });
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto lhs_expr = pass_b__expression(path, e.get_array_n(1));
		const auto rhs_expr = pass_b__expression(path, e.get_array_n(2));

		const auto lhs_type = get_array_back(lhs_expr);
		const auto rhs_type = get_array_back(rhs_expr);
		if(lhs_type != rhs_type){
			throw std::runtime_error("1001 - Left & right side of math2 must have same type.");
		}
		return json_t::make_array2({ op, lhs_expr, rhs_expr, lhs_type });
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto condition_expr = pass_b__expression(path, e.get_array_n(1));
		const auto a_expr = pass_b__expression(path, e.get_array_n(2));
		const auto b_expr = pass_b__expression(path, e.get_array_n(3));

		const auto condition_type = get_array_back(condition_expr);
//		const auto condition_type2 = pass_b__type_to_id(path, condition_type.get_string(), eresolve_types::k_all);

		//??? Find bool ID.
/*
		if(condition_type2 != "bool"){
			throw std::runtime_error("xxxx - condition of (?:) must evaluate to bool.");
		}
*/

		const auto a_type = get_array_back(a_expr);
		const auto b_type = get_array_back(b_expr);
		if(a_type != b_type){
			throw std::runtime_error("1001 - Left & right side of (?:) must have same type.");
		}
		return json_t::make_array2({ op, condition_expr, a_expr, b_expr, a_type });
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const json_t function_expr = pass_b__expression(path, e.get_array_n(1));
		const json_t args_expr = pass_b__arguments(path, e.get_array_n(2));

		//	The call-expression's output type will be the same as the return-type of the function-def.
		const auto return_type = pass_b__type_to_id(path, get_array_back(function_expr).get_string(), eresolve_types::k_only_function);
//			throw std::runtime_error("1002 - Undefined function \"" + call._function.to_string() + "\".");
//		const auto function_def; // = function_type_info


		//??? Check arguments too.
/*
	else if(e._call){
		const auto& call = *e._call;

		//	Resolve function name.
		const auto f = resolve_type_to_def(path, call._function);
		if(!f || f->get_type() != base_type::k_function){
			throw std::runtime_error("1002 - Undefined function \"" + call._function.to_string() + "\".");
		}
		const auto f2 = f->get_function_def();

		const auto return_type = resolve_type_to_id(path, f2->_return_type);
		QUARK_ASSERT(return_type.is_resolved());

		if(f2->_type == scope_def_t::k_function_scope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			if(f2->_members.size() != call._inputs.size()){
				throw std::runtime_error("1003 - Wrong number of argument to function \"" + call._function.to_string() + "\".");
			}

			vector<expression_t> args2;
			for(int argument_index = 0 ; argument_index < call._inputs.size() ; argument_index++){
				const auto call_arg = call._inputs[argument_index];
				const auto call_arg2 = resolve_types__expression(path, call_arg);
				const auto call_arg2_type = call_arg2.get_expression_type();
				QUARK_ASSERT(call_arg2_type.is_resolved());

				const auto function_arg_type = *f2->_members[argument_index]._type;

				if(!(call_arg2_type.to_string() == function_arg_type.to_string())){
					throw std::runtime_error("1004 - Argument " + std::to_string(argument_index) + " to function \"" + call._function.to_string() + "\" mismatch.");
				}
				args2.push_back(call_arg2);
			}

			return expression_t::make_function_call(type_identifier_t::resolve(f), args2, return_type);
		}
		else if(f2->_type == scope_def_t::k_subscope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			QUARK_ASSERT(call._inputs.empty());
			//??? Throw exceptions -- treat JSON-AST as user input.

			return expression_t::make_function_call(type_identifier_t::resolve(f), vector<expression_t>{}, return_type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
*/

		return json_t::make_array2({ op, function_expr, args_expr, return_type });
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = pass_b__expression(path, e.get_array_n(1));
		const auto type = get_array_back(base_expr).get_string();
		QUARK_ASSERT(type.front() == '$');
		return json_t::make_array2({ op, base_expr, e.get_array_n(2), type });
	}
	else if(op == "@"){
		//	###: Idea: Functions are found via a variable called the function name, pointing to the function_def.
		QUARK_ASSERT(e.get_array_size() == 2);

		const auto variable_name = e.get_array_n(1).get_string();
		pair<json_t, int> found_variable = pass_b__resolve_scoped_variable(path, variable_name);
		if(!found_variable.first.is_null()){
			json_t member = found_variable.first.get_array_n(found_variable.second);
			const auto type = member.get_object_element("type").get_string();
			const auto type_id = pass_b__type_to_id(path, type, eresolve_types::k_all);
			return json_t::make_array2({ op, variable_name, type_id });
		}
		else{
			auto found_function = pass_b__type_to_id_xxx(path, "<" + variable_name + ">", eresolve_types::k_only_function);

			//	OK, we're resolving a function, not a variable. ### Code this differently?
			const auto return_type = found_function.second.get_object_element("scope_def").get_object_element("return_type").get_string();
			QUARK_ASSERT(return_type.back() == '>');
			const auto return_type_id = pass_b__type_to_id(path, return_type, eresolve_types::k_all);
			return json_t::make_array2({ op, variable_name, return_type_id });
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}

parser_path_t update_leaf(const parser_path_t& path, const json_t& leaf){
	vector<json_t> temp(path._scopes.begin(), path._scopes.end() - 1);
	temp.push_back(leaf);

	parser_path_t temp2;
	temp2._scopes.swap(temp);
	return temp2;
}

json_t replace_type_name_references_with_type_ids(const parser_path_t& path0){
	QUARK_SCOPED_TRACE("PASS B");
	QUARK_ASSERT(path0.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path0._scopes.back()));

	const auto scope1 = path0._scopes.back();

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
						const auto subscope2 = replace_type_name_references_with_type_ids(go_down(update_leaf(path0, scope1), subscope));
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

	auto path1 = update_leaf(path0, scope2);
	scope2 = assoc(scope2, {"members"}, pass_b__members(path1, scope2.get_optional_object_element("members")));
	scope2 = assoc(scope2, {"args"}, pass_b__members(path1, scope2.get_optional_object_element("args")));
	scope2 = assoc(scope2, {"locals"}, pass_b__members(path1, scope2.get_optional_object_element("locals")));


	if(scope2.does_object_element_exist("return_type") && scope2.get_object_element("return_type").get_string() != ""){
		scope2 = assoc(
			scope2,
			{"return_type"},
			pass_b__type_to_id(path1, scope2.get_optional_object_element("return_type", "<null>").get_string(), eresolve_types::k_all)
		);
	}

	const auto statements = scope2.get_optional_object_element("statements");
	if(!statements.is_null()){
		vector<json_t> statements2;
		const auto statements_vec = statements.get_array();
		for(const auto& s: statements_vec){
			json_t op = s.get_array_n(0);
			if(op == "bind"){
				const auto type = s.get_array_n(1);
				const auto type2 = pass_b__type_to_id(path1, type.get_string(), eresolve_types::k_all);

				const auto identifier = s.get_array_n(2);

				const auto expr = s.get_array_n(3);
				const auto expr2 = pass_b__expression(path1, expr);
				statements2.push_back(json_t::make_array2({ op, type2, identifier, expr2 }));
			}
			else if(op == "define_struct"){
				QUARK_ASSERT(false);
			}
			else if(op == "define_function"){
				QUARK_ASSERT(false);
			}
			else if(op == "return"){
				const auto expr = pass_b__expression(path1, s.get_array_n(1));
				statements2.push_back(json_t::make_array2({ op, expr }));
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

QUARK_UNIT_TESTQ("replace_type_name_references_with_type_ids()", "Test minimal program -- are all references to types resolved?"){
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

	const auto result = replace_type_name_references_with_type_ids(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}

QUARK_UNIT_TESTQ("replace_type_name_references_with_type_ids()", "Are types of locals resolved?"){
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

	const auto result = replace_type_name_references_with_type_ids(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}


QUARK_UNIT_TESTQ("replace_type_name_references_with_type_ids()", ""){
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

	const auto result = replace_type_name_references_with_type_ids(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());


	const string result_str = json_to_pretty_string(result);
	QUARK_TRACE(result_str);
	const auto it = result_str.find(">\"");
	QUARK_ASSERT(it == string::npos);

}




///////////////////////////////////////////		PASS C


//	Pass C) Make central type lookup.

//??? Store full path for each type.

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
//				scope1 = res.first;
				lookup1 = res.second;
			}
		}
	}

	QUARK_TRACE(json_to_pretty_string(scope1));
	QUARK_TRACE(json_to_pretty_string(lookup1));
	return { scope1, lookup1 };
}

json_t pass_c__scope_def(const parser_path_t& path){
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

QUARK_UNIT_TESTQ("pass_c__scope_def()", ""){
	const auto test = make_test_pass_c();
	QUARK_TRACE(json_to_pretty_string(test));

	const auto c = pass_c__scope_def(make_parser_path(test));

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




json_t run_pass2(const json_t& parse_tree){
	QUARK_TRACE(json_to_pretty_string(parse_tree));

	const auto pass_m1 = statements_to_scope(parse_tree);
	auto pass_0 = store_object_member(pass_m1, "type", "global");
	pass_0 = store_object_member(pass_0, "name", "global");

	const auto pass_a = assign_unique_type_ids(make_parser_path(pass_0), 1000);
	const auto pass_a5 = insert_generated_functions(make_parser_path(pass_a.first), pass_a.second);
	const auto pass_b = replace_type_name_references_with_type_ids(make_parser_path(pass_a5.first));
	QUARK_ASSERT(!has_unresolved_types(pass_b));

	const auto pass_c = pass_c__scope_def(make_parser_path(pass_b));
	const auto pass_d = pass_c;//pass_d__scope_def(make_parser_path(pass_c));

	QUARK_ASSERT(get_in(pass_c, {"global", "args"}) == json_t::make_array());
	QUARK_ASSERT(get_in(pass_c, {"global", "locals"}) == json_t::make_array());
	QUARK_ASSERT(get_in(pass_c, {"global", "members"}) == json_t::make_array());
	QUARK_ASSERT(get_in(pass_c, {"global", "statements"}) == json_t::make_array());

	//	Make sure there are no unresolved types left in program.
	QUARK_ASSERT(!has_unresolved_types(pass_d));

	return pass_d;
}





///////////////////////////////////////			TESTS



	const auto kMinimalProgram200 = R"(
		int main(){
			return 3;
		}
		)";


/*
	These tests verify that:
	- the transform of the AST worked as expected
	- Correct errors are emitted.
*/


QUARK_UNIT_TESTQ("run_pass2()", "Minimum program"){
	const auto pass1 = parse_program2(kMinimalProgram200);
	const auto pass2 = run_pass2(pass1);
	const auto ast = json_to_ast(pass2);

/*
	const auto found_it = find_if(
		ast._symbols.begin(),
		ast._symbols.end(),
		[&] (const std::pair<std::string, std::shared_ptr<type_def_t>>& e) { return e.second->to_string() == "int"; }
	);
	QUARK_ASSERT(found_it != ast._symbols.end());
*/
}

//	This program uses all features of pass2.???
QUARK_UNIT_TESTQ("run_pass2()", "Maxium program"){
	const auto a = R"(
		string get_s(pixel p){ return p.s; }
		struct pixel { string s = "two"; }
		string main(){
			pixel p = pixel_constructor();
			return get_s(p);
		}
		)";
	const auto pass1 = parse_program2(a);
	const auto pass2 = run_pass2(pass1);
}



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
#endif

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

/*
Can't get this test past the parser...
Make a json_to_ast() so we can try arbitrary asts with run_pass2().

QUARK_UNIT_TESTQ("run_pass2()", "1011"){
	test_error(
		R"(
			struct pixel { rgb s = "two"; }
			string main(){
				pixel p = pixel_constructor();
				int a = p.s;
				return 1;
			}
		)",
		"Unresolved member \"xyz\""
	);
}
*/
