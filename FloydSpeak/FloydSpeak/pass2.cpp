//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//



/*

??? make a pass that inserts generated functions, like constructors.
??? Turn function_defs "int f(){ return 42; }" into a variable "f" referencing the function. Then all @ are the same - function name or other variable.



PROBLEM: How to store links to resolved to types or variables?

Augument existing data, don't replace typename / variable name.

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
			"_name": "pixel_t_constructor",
			"_type": "function",
			"_args": [],
			"_locals": [
				["$5", "it"],
				["$4", "x2"]
			],
			"_statements": [
				["call", "___body"],
				["return", ["k", "$5", 100]]
			],
			"_return_type": "$4"
		}
	},
	"$5": { "name": "pixel_t", "base_type": "struct", "scope_def":
		{
			"_name": "pixel_t",
			"_type": "struct",
			"_members": [
				["$4", "red"],
				["$4", "green"],
				["$4", "blue"]
			],
			"_types": {},
			"_statements": [],
			"_return_type": null
		}
	},
	"$6": { "name": "main", "base_type": "function", "scope_def":
		{
			"_name": "main",
			"_type": "function",
			"_args": [
				["$3", "args"]
			],
			"_locals": [
				["$5", "p1"],
				["$5", "p2"]
			],
			"_types": {},
			"_statements": [
				["bind", "<pixel_t>", "p1", ["call", "pixel_t_constructor"]],
				["return", ["+", ["@", "p1"], ["@", "g_version"]]]
			],
			"_return_type": "$4"
		}
	}
},
"global_scope": {
	"_name": "global",
	"_type": "global",
	"_members": [
		["$4", "g_version", "1.0"],
		["$3", "message", "Welcome!"]
	],
}
*/




#include "pass2.h"

#include "statements.h"
#include "floyd_parser.h"
#include "floyd_interpreter.h"
#include "parser_value.h"
#include "ast_utils.h"
#include "utils.h"
#include "json_support.h"
#include "json_parser.h"
#include "text_parser.h"
#include "parser_primitives.h"

using namespace floyd_parser;
using namespace std;


parser_path_t make_parser_path(const json_value_t& scope){
	parser_path_t path;
	path._scopes.push_back(scope);
	return path;
}

parser_path_t go_down(const parser_path_t& path, const json_value_t& child){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(child.check_invariant());

	auto result = path;
	result._scopes.push_back(child);
	return result;
}




//??? PASS_0 should validate tree and json-objects.



///////////////////////////////////////////		PASS A


//Pass A) Scan tree, give each found type a unique ID. (Tag each scope and assign parent-scope ID.)


string make_type_id_string(int id){
	return std::string("$" + std::to_string(id));
}




pair<json_value_t, int> pass_a__scope_def(const parser_path_t& path, int type_id_count){
	QUARK_ASSERT(path.check_invariant());
	QUARK_SCOPED_TRACE("PASS A");
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto type_id_count2 = type_id_count;
	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_value_t::make_object();
	{
		const auto types = get_in(scope1, { "_types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_js = type_entry_pair.second;
			const auto type_defs_vec = type_defs_js.get_array();

			std::vector<json_value_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				if(!exists_in(type_def, make_vec({ "base_type"}))){
					throw std::runtime_error("No base type.");
				}

				const auto type_def2 = assoc(type_def, "id", json_value_t(make_type_id_string(type_id_count2)));
				type_id_count2++;

				const auto type_def3 = [&](){
					const json_value_t subscope = type_def2.get_optional_object_element("scope_def");
					if(!subscope.is_null()){
						const auto r = pass_a__scope_def(go_down(path, subscope), type_id_count2);
						type_id_count2 = r.second;
						auto temp = assoc(type_def2, "scope_def", r.first);
//						temp = assoc(temp, "parent_scope_id", json_value_t(make_type_id_string(this_scope_id)));
//						type_id_count2++;
						return temp;
					}
					else{
						return type_def2;
					}
				}();
				type_defs2.push_back(type_def3);
			}
			types_collector2 = assoc(types_collector2, type_name, json_value_t(type_defs2));
		}
	}

	const auto scope2 = assoc(scope1, "_types", types_collector2);
	QUARK_TRACE(json_to_pretty_string(scope2));
	return { scope2, type_id_count2 };
}

QUARK_UNIT_TESTQ("pass_a__scope_def()", ""){
	const auto test = parse_json(seq_t(
		R"(
			{
				"_name": "global",
				"_type": "global",
				"_members": [
					{ "type": "<int>", "name": "g_version", "expr": 1.0 },
					{ "type": "<string>", "name": "message", "expr": "Welcome!" }
				],
				"_types": {
					"bool": [ { "base_type": "bool" } ],
					"int": [ { "base_type": "int" } ],
					"string": [ { "base_type": "string" } ],
					"pixel_t": [
						{
							"base_type": "function",
							"scope_def": {
								"_name": "pixel_t_constructor",
								"_type": "function",
								"_args": [],
								"_locals": [
									{ "type": "<pixel_t>", "name": "it" },
									{ "type": "<int>", "name": "x2" }
								],
								"_types": {},
								"_statements": [
									["return", ["call", ["@", "___body"], [ ["k", "<pixel_t>", 100] ]]]
								],
								"_return_type": "<int>"
							}
						},
						{
							"base_type": "struct",
							"scope_def": {
								"_name": "pixel_t",
								"_type": "struct",
								"_members": [
									{ "type": "<int>", "name": "red" },
									{ "type": "<int>", "name": "green" },
									{ "type": "<int>", "name": "blue" }
								],
								"_types": {},
								"_statements": [],
								"_return_type": null
							}
						}
					],
					"main": [
						{
							"base_type": "function",
							"scope_def": {
								"_name": "main",
								"_type": "function",
								"_args": [
									{ "type": "<string>", "name": "args" }
								],
								"_locals": [
									{ "type": "<pixel_t>", "name": "p1" },
									{ "type": "<pixel_t>", "name": "p2" }
								],
								"_types": {},
								"_statements": [
									["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_t"], [["k", "<int>", 42]]]],
									["return", ["+", ["@", "p1"], ["@", "g_version"]]]
								],
								"_return_type": "<int>"
							}
						}
					]
				}
			}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = pass_a__scope_def(make_parser_path(test), 1000);
	QUARK_UT_VERIFY(result.first.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result.first));
}







///////////////////////////////////////////		PASS A5


//	Pass A5) Insert constructors for each struct.


/*
	Insert dummy-hook for constructors. Implementation needs to be provided by later pass.

	??? Calling all constructor function "pixel_t" requires us to select function from its signature. Alternative use one function with var-args.

	??? How to call host code. Is host code represented by a scope_def? I does need args/return value.
*/


pair<json_value_t, int> pass_a5__scope_def(const parser_path_t& path, int type_id_count){
	QUARK_SCOPED_TRACE("PASS A5");
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto type_id_count2 = type_id_count;
	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_value_t::make_object();
	{
		const auto types = get_in(scope1, { "_types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_js = type_entry_pair.second;
			const auto type_defs_vec = type_defs_js.get_array();

			std::vector<json_value_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				json_value_t type_def2 = type_def;

				//	If this is a struct, insert contructor(s).
				const auto base_type = type_def2.get_object_element("base_type");
				if(base_type == "struct"){
					const json_value_t struct_scope = type_def2.get_object_element("scope_def");

					const string struct_name = struct_scope.get_object_element("_name").get_string();
					const string constructor_name = struct_name + "_constructor";

					/*
						Make default constructor for the struct.
					*/
					json_value_t constructor_def = floyd_parser::make_scope_def();
					constructor_def = store_object_member(constructor_def, "_type", "function");
					constructor_def = store_object_member(constructor_def, "_name", constructor_name);
					constructor_def = store_object_member(constructor_def, "_args", json_value_t::make_array());
					constructor_def = store_object_member(constructor_def, "_locals", json_value_t::make_array());
					constructor_def = store_object_member(constructor_def, "_statements", json_value_t::make_array());
					constructor_def = store_object_member(constructor_def, "_types", json_value_t::make_object());
					constructor_def = store_object_member(constructor_def, "_return_type", "<" + struct_name + ">");
					const auto constructor_type_def = json_value_t::make_object({
						{ "id", json_value_t(make_type_id_string(type_id_count2)) },
						{ "base_type", "function" },
						{ "scope_def", constructor_def }
					});
					type_id_count2++;

					//	### Add call to pass_a5__scope_def() on new constructor function!

					types_collector2 = assoc(types_collector2, constructor_name, json_value_t::make_array2({ constructor_type_def }));
				}

				//	Propage down to all nodes -> leaves in tree.
				const json_value_t subscope = type_def2.get_optional_object_element("scope_def");
				if(!subscope.is_null()){
					const auto r = pass_a5__scope_def(go_down(path, subscope), type_id_count2);
					type_id_count2 = r.second;
					auto temp = assoc(type_def2, "scope_def", r.first);
					type_def2 = temp;
				}

				type_defs2.push_back(type_def2);
			}
			types_collector2 = assoc(types_collector2, type_name, json_value_t(type_defs2));
		}
	}

	const auto scope2 = assoc(scope1, "_types", types_collector2);
	QUARK_TRACE(json_to_pretty_string(scope2));
	return { scope2, type_id_count2 };
}

QUARK_UNIT_TESTQ("pass_a5__scope_def()", ""){
	const auto test = parse_json(seq_t(
		R"(
			{
				"_members": [
					{ "expr": 1, "name": "g_version", "type": "<int>" },
					{ "expr": "Welcome!", "name": "message", "type": "<string>" }
				],
				"_name": "global",
				"_type": "global",
				"_types": {
					"bool": [{ "base_type": "bool", "id": "$1000" }],
					"int": [{ "base_type": "int", "id": "$1001" }],
					"main": [
						{
							"base_type": "function",
							"id": "$1002",
							"scope_def": {
								"_args": [{ "name": "args", "type": "<string>" }],
								"_locals": [
									{ "name": "p1", "type": "<pixel_t>" },
									{ "name": "p2", "type": "<pixel_t>" }
								],
								"_name": "main",
								"_return_type": "<int>",
								"_statements": [
									["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_t"], [["k", "<int>", 42]]]],
									["return", ["+", ["@", "p1"], ["@", "g_version"]]]
								],
								"_type": "function",
								"_types": {}
							}
						}
					],
					"pixel_t": [
						{
							"base_type": "struct",
							"id": "$1004",
							"scope_def": {
								"_members": [
									{ "name": "red", "type": "<int>" },
									{ "name": "green", "type": "<int>" },
									{ "name": "blue", "type": "<int>" }
								],
								"_name": "pixel_t",
								"_return_type": null,
								"_statements": [],
								"_type": "struct",
								"_types": {}
							}
						}
					],
					"string": [{ "base_type": "string", "id": "$1005" }]
				}
			}		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = pass_a5__scope_def(make_parser_path(test), 100000);
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
pair<std::string, json_value_t> pass_b__resolve_type_xxx(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);
	QUARK_ASSERT(type_name0.front() == '<' && type_name0.back() == '>');
	QUARK_TRACE(json_to_pretty_string(path._scopes.front()));

	const string type_name = type_name0.substr(1, type_name0.size() - 2);
	for(auto i = path._scopes.size() ; i > 0 ; i--){
		const auto& scope = path._scopes[i - 1];
		const auto type = scope.get_object_element("_types").get_optional_object_element(type_name);
		if(!type.is_null()){
			const auto hits0 = type.get_array();
			vector<json_value_t> hits1;
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
std::string pass_b__resolve_type(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);

	if(type_name0.front() == '$'){
		return type_name0;
	}
	else{
		return pass_b__resolve_type_xxx(path, type_name0, types).first;
	}
}


//??? Should already have separated functions into outer + body-scopes before doing this!

//	Resolve a variable is done by scanning the compile-time scopes upwards towards globals.
//	Returns the member-list/arg_list etc. that holds the variable AND the index inside _members where it is found.
pair<json_value_t, int> pass_b__resolve_scoped_variable(const parser_path_t& path, const string& variable_name){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(variable_name.size() > 0);

	//	Look s in each scope's members, one at a time until we searched the global scope.
	for(auto i = path._scopes.size() ; i > 0 ; i--){
		const auto& scope_def = path._scopes[i - 1];

		if(scope_def.does_object_element_exist("_members")){
			const auto members = scope_def.get_object_element("_members").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("_members"), index };
				}
			}
		}

		if(scope_def.does_object_element_exist("_args")){
			const auto members = scope_def.get_object_element("_args").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("_args"), index };
				}
			}
		}

		if(scope_def.does_object_element_exist("_locals")){
			const auto members = scope_def.get_object_element("_locals").get_array();
			for(auto index = 0 ; index < members.size() ; index++){
				const auto& member = members[index];
				const auto member_name = member.get_object_element("name").get_string();
				if(member_name == variable_name){
					return { scope_def.get_object_element("_locals"), index };
				}
			}
		}
	}
	return { {}, -1 };
}

json_value_t pass_b__expression(const parser_path_t& path, const json_value_t& e);


json_value_t pass_b__members(const parser_path_t& path, const json_value_t& members){
	if(members.is_null()){
		return members;
	}
	else{
		const auto member_vec = members.get_array();
		vector<json_value_t> member_vec2;
		for(const auto& member: member_vec){
			if(!member.is_object()){
				throw std::runtime_error("Member definitions must be JSON objects.");
			}

			const auto type = member.get_object_element("type").get_string();

			auto member2 = member;
			if(type != ""){
				const auto type2 = pass_b__resolve_type(path, type, eresolve_types::k_all_but_function);
				member2 = assoc(member, "type", type2);
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
		return json_value_t(member_vec2);
	}
}

bool is_math1_op(const string& op){
	return op == "neg";
}

bool is_math2_op(const string& op){
	return op ==
		"[-]"
		|| op == "+" || op == "-" || op == "*" || op == "/" || op == "%"
		|| op == "<=" || op == "<" || op == ">=" || op == ">"
		|| op == "==" || op == "!=" || op == "&&" || op == "||";
}


//	Array of arguments, part of function call.
json_value_t pass_b__arguments(const parser_path_t& path, const json_value_t& args1){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(args1.check_invariant());

	vector<json_value_t> args2;
	for(const auto& arg: args1.get_array()){
		args2.push_back(pass_b__expression(path, arg));
	}
	return args2;
}

json_value_t get_array_back(const json_value_t& array){
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
json_value_t pass_b__expression(const parser_path_t& path, const json_value_t& e){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_TRACE(json_to_pretty_string(e));

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);
		const auto type2 = pass_b__resolve_type(path, type.get_string(), eresolve_types::k_all_but_function);

		return json_value_t::make_array2({
			op,
			value,
			type2,
		});
	}
	else if(is_math1_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto expr = pass_b__expression(path, e.get_array_n(1));
		const auto type = expr.get_array_n(3);
		return json_value_t::make_array2({ op, expr, type });
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
		return json_value_t::make_array2({ op, lhs_expr, rhs_expr, lhs_type });
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto condition_expr = pass_b__expression(path, e.get_array_n(1));
		const auto a_expr = pass_b__expression(path, e.get_array_n(2));
		const auto b_expr = pass_b__expression(path, e.get_array_n(3));

		const auto condition_type = get_array_back(condition_expr);

		//??? Find bool ID.
		if(condition_type != "bool"){
			throw std::runtime_error("xxxx - condition of (?:) must evaluate to bool.");
		}

		const auto a_type = get_array_back(a_expr);
		const auto b_type = get_array_back(b_expr);
		if(a_type != b_type){
			throw std::runtime_error("1001 - Left & right side of (?:) must have same type.");
		}
		return json_value_t::make_array2({ op, condition_expr, a_expr, b_expr, a_type });
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);

		const json_value_t function_expr = pass_b__expression(path, e.get_array_n(1));
		const json_value_t args_expr = pass_b__arguments(path, e.get_array_n(2));

		//	The call-expression's output type will be the same as the return-type of the function-def.
		const auto return_type = pass_b__resolve_type(path, get_array_back(function_expr).get_string(), eresolve_types::k_only_function);
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

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), args2, return_type);
		}
		else if(f2->_type == scope_def_t::k_subscope){
			//	Verify & resolve all arguments in the call vs the actual function definition.
			QUARK_ASSERT(call._inputs.empty());
			//??? Throw exceptions -- treat JSON-AST as user input.

			return floyd_parser::expression_t::make_function_call(type_identifier_t::resolve(f), vector<expression_t>{}, return_type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
*/

		return json_value_t::make_array2({ op, function_expr, args_expr, return_type });
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto base_expr = pass_b__expression(path, e.get_array_n(1));
		const auto type = get_array_back(base_expr).get_string();
		QUARK_ASSERT(type.front() == '$');
		return json_value_t::make_array2({ op, base_expr, e.get_array_n(2), type });
	}
	else if(op == "@"){
		//	###: Idea: Functions are found via a variable called the function name, pointing to the function_def.
		QUARK_ASSERT(e.get_array_size() == 2);

		const auto variable_name = e.get_array_n(1).get_string();
		pair<json_value_t, int> found_variable = pass_b__resolve_scoped_variable(path, variable_name);
		if(!found_variable.first.is_null()){
			json_value_t member = found_variable.first.get_array_n(found_variable.second);
			const auto type = member.get_object_element("type").get_string();
			const auto type_id = pass_b__resolve_type(path, type, eresolve_types::k_all);
			return json_value_t::make_array2({ op, variable_name, type_id });
		}
		else{
			auto found_function = pass_b__resolve_type_xxx(path, "<" + variable_name + ">", eresolve_types::k_only_function);

			//	OK, we're resolving a function, not a variable. ### Code this differently?
			const auto return_type = found_function.second.get_object_element("scope_def").get_object_element("_return_type").get_string();
			QUARK_ASSERT(return_type.back() == '>');
			const auto return_type_id = pass_b__resolve_type(path, return_type, eresolve_types::k_all);
			return json_value_t::make_array2({ op, variable_name, return_type_id });
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}

parser_path_t update_leaf(const parser_path_t& path, const json_value_t& leaf){
	vector<json_value_t> temp(path._scopes.begin(), path._scopes.end() - 1);
	temp.push_back(leaf);

	parser_path_t temp2;
	temp2._scopes.swap(temp);
	return temp2;
}

//	All types have already been tagged.
json_value_t pass_b__scope_def(const parser_path_t& path0){
	QUARK_SCOPED_TRACE("PASS B");
	QUARK_ASSERT(path0.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path0._scopes.back()));

	const auto scope1 = path0._scopes.back();

	auto types_collector2 = json_value_t::make_object();
	{
		const auto types = get_in(scope1, { "_types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_vec = type_entry_pair.second.get_array();

			std::vector<json_value_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				const auto type_def3 = [&](){
					const json_value_t subscope = type_def.get_optional_object_element("scope_def");
					if(!subscope.is_null()){
						const auto subscope2 = pass_b__scope_def(go_down(update_leaf(path0, scope1), subscope));
						return assoc(type_def, "scope_def", subscope2);
					}
					else{
						return type_def;
					}
				}();
				type_defs2.push_back(type_def3);
			}
			types_collector2 = assoc(types_collector2, type_name, json_value_t(type_defs2));
		}
	}
	auto scope2 = assoc(scope1, "_types", types_collector2);

	auto path1 = update_leaf(path0, scope2);
	scope2 = assoc(scope2, {"_members"}, pass_b__members(path1, scope2.get_optional_object_element("_members")));
	scope2 = assoc(scope2, {"_args"}, pass_b__members(path1, scope2.get_optional_object_element("_args")));
	scope2 = assoc(scope2, {"_locals"}, pass_b__members(path1, scope2.get_optional_object_element("_locals")));


	if(scope2.does_object_element_exist("_return_type") && scope2.get_object_element("_return_type").get_string() != ""){
		scope2 = assoc(
			scope2,
			{"_return_type"},
			pass_b__resolve_type(path1, scope2.get_optional_object_element("_return_type", "<null>").get_string(), eresolve_types::k_all)
		);
	}

	const auto statements = scope2.get_optional_object_element("_statements");
	if(!statements.is_null()){
		vector<json_value_t> statements2;
		const auto statements_vec = statements.get_array();
		for(const auto& s: statements_vec){
			json_value_t op = s.get_array_n(0);
			if(op == "bind"){

				const auto type = s.get_array_n(1);
				const auto type2 = pass_b__resolve_type(path1, type.get_string(), eresolve_types::k_all);

				const auto identifier = s.get_array_n(2);

				const auto expr = s.get_array_n(3);
				const auto expr2 = pass_b__expression(path1, expr);
				statements2.push_back(json_value_t::make_array2({ op, type2, identifier, expr2 }));
			}
			else if(op == "define_struct"){
				QUARK_ASSERT(false);
			}
			else if(op == "define_function"){
				QUARK_ASSERT(false);
			}
			else if(op == "return"){
				const auto expr = pass_b__expression(path1, s.get_array_n(1));
				statements2.push_back(json_value_t::make_array2({ op, expr }));
			}
			else{
				throw std::runtime_error("Unknown statement operation");
			}
		}
		scope2 = assoc(scope2, {"_statements"}, statements2);
	}

	QUARK_TRACE(json_to_pretty_string(scope2));
	return scope2;
}

QUARK_UNIT_TESTQ("pass_b__scope_def()", "Test minimal program -- are all references to types resolved?"){
	const auto test = parse_json(seq_t(
		R"(
		{
			"_members": [],
			"_name": "global",
			"_type": "global",
			"_types": {
				"bool": [{ "base_type": "bool", "id": "$100001" }],
				"int": [{ "base_type": "int", "id": "$100002" }],
				"main": [
					{
						"base_type": "function",
						"id": "$100003",
						"scope_def": {
							"_args": [],
							"_locals": [],
							"_name": "main",
							"_return_type": "<int>",
							"_statements": [ ["return", ["k", "hello!", "<string>"]]],
							"_type": "function",
							"_types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$100004" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = pass_b__scope_def(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}

QUARK_UNIT_TESTQ("pass_b__scope_def()", "Are types of locals resolved?"){
	const auto test = parse_json(seq_t(
		R"(
		{
			"_members": [],
			"_name": "global",
			"_type": "global",
			"_types": {
				"bool": [{ "base_type": "bool", "id": "$100001" }],
				"int": [{ "base_type": "int", "id": "$100002" }],
				"main": [
					{
						"base_type": "function",
						"id": "$100003",
						"scope_def": {
							"_args": [],
							"_locals": [
								{ "name": "p1", "type": "<int>" },
								{ "name": "p2", "type": "<string>" },
								{ "name": "p3", "type": "<bool>" }
							],
							"_name": "main",
							"_return_type": "<int>",
							"_statements": [ ["return", ["k", "hello!", "<string>"]]],
							"_type": "function",
							"_types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$100004" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = pass_b__scope_def(make_parser_path(test));
	QUARK_UT_VERIFY(result.check_invariant());
	QUARK_TRACE(json_to_pretty_string(result));
}


QUARK_UNIT_TESTQ("pass_b__scope_def()", ""){
	const auto test = parse_json(seq_t(
		R"(
		{
			"_members": [
				{ "expr": ["k", 1, "<int>"], "name": "g_version", "type": "<int>" },
				{ "expr": ["k", 1, "<int>"], "name": "g_base_version", "type": "<int>" },
				{ "expr": ["k", "Welcome!", "<string>"], "name": "message", "type": "<string>" }
			],
			"_name": "global",
			"_type": "global",
			"_types": {
				"bool": [{ "base_type": "bool", "id": "$1000" }],
				"int": [{ "base_type": "int", "id": "$1001" }],
				"main": [
					{
						"base_type": "function",
						"id": "$1002",
						"scope_def": {
							"_args": [{ "name": "args", "type": "<string>" }],
							"_locals": [
								{ "name": "p1", "type": "<pixel_t>" },
								{ "name": "p2", "type": "<pixel_t>" }
							],
							"_name": "main",
							"_return_type": "<int>",
							"_statements": [
								["bind", "<pixel_t>", "p1", ["call", ["@", "pixel_constructor"], [["k", 42, "<int>"]]]],
								["return", ["+", ["@", "g_base_version"], ["@", "g_version"]]]
							],
							"_type": "function",
							"_types": {
							}
						}
					}
				],
				"pixel_constructor": [
					{
						"base_type": "function",
						"id": "$1003",
						"scope_def": {
							"_args": [],
							"_locals": [{ "name": "it", "type": "<pixel_t>" }, { "name": "x2", "type": "<int>" }],
							"_name": "pixel_t_constructor",
							"_return_type": "<int>",
							"_statements": [
							],
							"_type": "function",
							"_types": {}
						}
					},
				],

				"pixel_t": [
					{
						"base_type": "struct",
						"id": "$1004",
						"scope_def": {
							"_members": [
								{ "name": "red", "type": "<int>" },
								{ "name": "green", "type": "<int>" },
								{ "name": "blue", "type": "<int>" }
							],
							"_name": "pixel_t",
							"_statements": [],
							"_type": "struct",
							"_types": {}
						}
					}
				],
				"string": [{ "base_type": "string", "id": "$1005" }]
			}
		}
		)"
	)).first;

	QUARK_TRACE(json_to_pretty_string(test));

	const auto result = pass_b__scope_def(make_parser_path(test));
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
		const auto name = path._scopes[i].get_object_element("_name").get_string();
		result = result + name + "/";
	}
	result = result + node_name;
	return result;
}

//	Returns new scope + lookup.
pair<json_value_t, json_value_t> pass_c__scope_def_internal(const json_value_t& lookup, const parser_path_t& path){
	QUARK_ASSERT(lookup.check_invariant());
	QUARK_ASSERT(lookup.is_object());
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	auto scope1 = path._scopes.back();
	const auto types = get_in(scope1, { "_types" }).get_object();
	auto lookup1 = lookup;
	scope1 = dissoc(scope1, "_types");

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

			const json_value_t subscope = type_def.get_optional_object_element("scope_def");
			if(!subscope.is_null()){
				const auto res = pass_c__scope_def_internal(lookup1, go_down(path, subscope));
				scope1 = res.first;
				lookup1 = res.second;
			}
		}
	}

	QUARK_TRACE(json_to_pretty_string(scope1));
	QUARK_TRACE(json_to_pretty_string(lookup1));
	return { scope1, lookup1 };
}

json_value_t pass_c__scope_def(const parser_path_t& path){
	QUARK_SCOPED_TRACE("PASS C");
	QUARK_TRACE(json_to_pretty_string(path._scopes.front()));

	QUARK_ASSERT(path.check_invariant());

	const auto a = pass_c__scope_def_internal(json_value_t::make_object(), path);

	const auto result = json_value_t::make_object({
		{ "global", a.first },
		{ "lookup", a.second }
	});

	QUARK_TRACE(json_to_pretty_string(result));
	return result;
}

const json_value_t make_test_pass_c(){
std::pair<json_value_t, seq_t> k_test = parse_json(seq_t(
	R"(
			{
				"_args": [],
				"_locals": [],
				"_members": [
					{ "expr": 1, "name": "g_version", "type": "$1001" },
					{ "expr": 1, "name": "g_base_version", "type": "$1001" },
					{ "expr": "Welcome!", "name": "message", "type": "$1005" }
				],
				"_name": "global",
				"_type": "global",
				"_types": {
					"bool": [{ "base_type": "bool", "id": "$1000" }],
					"int": [{ "base_type": "int", "id": "$1001" }],
					"main": [
						{
							"base_type": "function",
							"id": "$1002",
							"scope_def": {
								"_args": [{ "name": "args", "type": "$1005" }],
								"_locals": [{ "name": "p1", "type": "$1004" }, { "name": "p2", "type": "$1004" }],
								"_members": [],
								"_name": "main",
								"_return_type": "$1001",
								"_statements": [
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
								"_type": "function",
								"_types": {}
							}
						}
					],
					"pixel_constructor": [
						{
							"base_type": "function",
							"id": "$1003",
							"scope_def": {
								"_args": [],
								"_locals": [{ "name": "it", "type": "$1004" }, { "name": "x2", "type": "$1001" }],
								"_members": [],
								"_name": "pixel_t_constructor",
								"_return_type": "$1001",
								"_statements": [],
								"_type": "function",
								"_types": {}
							}
						}
					],
					"pixel_t": [
						{
							"base_type": "struct",
							"id": "$1004",
							"scope_def": {
								"_args": [],
								"_locals": [],
								"_members": [
									{ "name": "red", "type": "$1001" },
									{ "name": "green", "type": "$1001" },
									{ "name": "blue", "type": "$1001" }
								],
								"_name": "pixel_t",
								"_statements": [],
								"_type": "struct",
								"_types": {}
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







///////////////////////////////////////////		PASS D




//	Pass D) Scan tree: bind variables to type-ID + offset.



std::string pass_d__resolve_type(const parser_path_t& path, const string& type_name0, eresolve_types types){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(type_name0.size() > 2);

	const string type_name = type_name0.substr(1, type_name0.size() - 2);
	for(auto i = path._scopes.size() ; i > 0 ; i--){
		const auto& scope = path._scopes[i - 1];
		const auto type = scope.get_object_element("_types").get_optional_object_element(type_name);
		if(!type.is_null()){
			const auto hits0 = type.get_array();
			vector<json_value_t> hits1;
			if(types == eresolve_types::k_all){
				hits1 = hits0;
			}
			else{
				for(const auto& hit: hits0){
					const auto base_type = hit.get_object_element("base_type").get_string();
					if(base_type == "function"){
					}
					else{
						hits1.push_back(hit);
					}
				}
			}

			if(hits1.size() == 0){
			}
			else if(hits1.size() == 1){
				return type.get_array_n(0).get_object_element("id").get_string();
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

json_value_t pass_d__members(const parser_path_t& path, const json_value_t& members){
	if(members.is_null()){
		return members;
	}
	else{
		const auto member_vec = members.get_array();
		vector<json_value_t> member_vec2;
		for(const auto& member: member_vec){
			if(!member.is_object()){
				throw std::runtime_error("Member definitions must be JSON objects.");
			}

			const auto type = member.get_object_element("type").get_string();

			if(type != ""){
				const auto type2 = pass_d__resolve_type(path, type, eresolve_types::k_all_but_function);
				const auto member2 = assoc(member, "type", type2);
				member_vec2.push_back(member2);
			}
			else{
				member_vec2.push_back(member);
			}
		}
		return json_value_t(member_vec2);
	}
}

json_value_t pass_d__expression(const parser_path_t& path, const json_value_t& e);

//	Array of arguments, part of function call.
json_value_t pass_d__arguments(const parser_path_t& path, const json_value_t& args1){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(args1.check_invariant());

	vector<json_value_t> args2;
	for(const auto& arg: args1.get_array()){
		args2.push_back(pass_d__expression(path, arg));
	}
	return args2;
}
json_value_t pass_d__expression(const parser_path_t& path, const json_value_t& e){
	QUARK_ASSERT(path.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_array_n(0).get_string();
	if(op == "k"){
		QUARK_ASSERT(e.get_array_size() == 3);
		return json_value_t::make_array2({
			op,
			pass_d__resolve_type(path, e.get_array_n(1).get_string(), eresolve_types::k_all_but_function),
			e.get_array_n(2)
		});
	}
	else if(is_math1_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		return json_value_t::make_array2({ op, pass_d__expression(path, e.get_array_n(1))});
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 3);
		return json_value_t::make_array2({ op, pass_d__expression(path, e.get_array_n(1)), pass_d__expression(path, e.get_array_n(2))});
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 4);
		return json_value_t::make_array2({
			op,
			pass_d__expression(path, e.get_array_n(1)),
			pass_d__expression(path, e.get_array_n(2)),
			pass_d__expression(path, e.get_array_n(3)),
		});
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 3);
		return json_value_t::make_array2({ op, pass_d__expression(path, e.get_array_n(1)), pass_d__arguments(path, e.get_array_n(2))});
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 3);
		return json_value_t::make_array2({ op, pass_d__expression(path, e.get_array_n(1)), e.get_array_n(2)});
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 2);
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}

json_value_t pass_d__scope_def(const parser_path_t& path){
	QUARK_SCOPED_TRACE("PASS D");
	QUARK_ASSERT(path.check_invariant());
	QUARK_TRACE(json_to_pretty_string(path._scopes.back()));

	const auto scope1 = path._scopes.back();

	auto types_collector2 = json_value_t::make_object();
	if(scope1.does_object_element_exist("_types")){
		const auto types = get_in(scope1, { "_types" }).get_object();
		QUARK_TRACE(json_to_pretty_string(types));

		for(const auto type_entry_pair: types){
			const string type_name = type_entry_pair.first;
			const auto type_defs_js = type_entry_pair.second;
			const auto type_defs_vec = type_defs_js.get_array();

			std::vector<json_value_t> type_defs2;
			for(const auto& type_def: type_defs_vec){
				QUARK_TRACE(json_to_pretty_string(type_def));

				const auto type_def3 = [&](){
					const json_value_t subscope = type_def.get_optional_object_element("scope_def");
					if(!subscope.is_null()){
						const auto subscope2 = pass_d__scope_def(go_down(path, subscope));
						return assoc(type_def, "scope_def", subscope2);
					}
					else{
						return type_def;
					}
				}();
				type_defs2.push_back(type_def3);
			}
			types_collector2 = assoc(types_collector2, type_name, json_value_t(type_defs2));
		}
	}

	auto scope2 = assoc(scope1, "_types", types_collector2);
	scope2 = assoc(scope2, {"_members"}, pass_d__members(path, scope2.get_optional_object_element("_members")));
	scope2 = assoc(scope2, {"_args"}, pass_d__members(path, scope2.get_optional_object_element("_args", json_value_t::make_array())));
	scope2 = assoc(scope2, {"_locals"}, pass_d__members(path, scope2.get_optional_object_element("_locals", json_value_t::make_array())));

	const auto statements = scope2.get_optional_object_element("_statements");
	if(!statements.is_null()){
		vector<json_value_t> statements_vec2;
		const auto statements_vec = statements.get_array();
		for(const auto& s: statements_vec){
			json_value_t op = s.get_array_n(0);
			if(op == "bind"){
				statements_vec2.push_back(json_value_t::make_array2({ op, s.get_array_n(1), s.get_array_n(2), pass_d__expression(path, s.get_array_n(3))}));
			}
			else if(op == "define_struct"){
				QUARK_ASSERT(false);
			}
			else if(op == "define_function"){
				QUARK_ASSERT(false);
			}
			else if(op == "return"){
				statements_vec2.push_back(json_value_t::make_array2({ op, pass_d__expression(path, s.get_array_n(1))}));
			}
			else{
				throw std::runtime_error("Unknown statement operation");
			}
		}
	}

	QUARK_TRACE(json_to_pretty_string(scope2));
	return scope2;
}

const json_value_t make_test_pass_d(){
std::pair<json_value_t, seq_t> k_test = parse_json(seq_t(
	R"(
		{
			"_name": "global",
			"_type": "global",
			"_members": [
				{ "type": "<string>", "name": "message", "expr": "Welcome!" }
			],
			"_types": {
				"bool": [ { "id": "$1", "base_type": "bool" } ],
				"int": [ { "id": "$2", "base_type": "int" } ],
				"string": [ { "id": "$3", "base_type": "string" } ],
				"pixel_t": [
					{
						"id": "$4",
						"base_type": "function",
						"scope_def": {
							"_name": "pixel_t_constructor",
							"_type": "function",
							"_args": [],
							"_locals": [
								{ "type": "<int>", "name": "x2" }
							],
							"_types": {},
							"_statements": [
								["return", ["call", ["@", "___body"], [ ["k", "<pixel_t>", 100] ]]]
							],
							"_return_type": "<int>"
						}
					},
					{
						"id": "$5",
						"base_type": "struct",
						"scope_def": {
							"_name": "pixel_t",
							"_type": "struct",
							"_members": [
								{ "type": "<int>", "name": "blue" }
							],
							"_types": {},
							"_statements": [],
							"_return_type": null
						}
					}
				],
				"main": [
					{
						"id": "$6",
						"base_type": "function",
						"scope_def": {
							"_name": "main",
							"_type": "function",
							"_args": [
							],
							"_locals": [
							],
							"_types": {},
							"_statements": [
							],
							"_return_type": "<int>"
						}
					}
				]
			}
		}
	)"
));

	return k_test.first;
}

QUARK_UNIT_TESTQ("pass_d__scope_def()", ""){
	const auto test = make_test_pass_d();
	QUARK_TRACE(json_to_pretty_string(test));

	const auto c = pass_d__scope_def(make_parser_path(test));

	QUARK_UT_VERIFY(c.check_invariant());
	QUARK_TRACE(json_to_pretty_string(c));
}





bool has_unresolved_types(const json_value_t& obj){
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



type_identifier_t resolve_type123(const string& id, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	const auto it = temp_type_defs.find(id);
	if(it == temp_type_defs.end()){
		QUARK_ASSERT(false);
	}

	return type_identifier_t::resolve(it->second);
}
/*
	{
		"_args": [],
		"_locals": [],
		"_members": [],
		"_name": "main",
		"_return_type": "$1001",
		"_statements": [["return", ["k", 3, "$1001"]]],
		"_type": "function",
		"_types": {}
	}
*/
std::shared_ptr<const scope_def_t> conv_scope_def(const json_value_t& scope_def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	const string type = scope_def.get_object_element("_type").get_string();
	const string name = scope_def.get_object_element("_name").get_string();
	const string return_type_id = scope_def.get_object_element("_return_type").get_string();
	const auto args = scope_def.get_optional_object_element("_args", json_value_t::make_array()).get_array();
	const auto locals = scope_def.get_optional_object_element("_locals", json_value_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("_members", json_value_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("_statements").get_array();

	/*
		{ "expr": 1, "name": "g_version", "type": "<int>" },
		{ "expr": "Welcome!", "name": "message", "type": "<string>" }
	*/
	std::vector<member_t> args2;
	for(const auto i: args){
		const string arg_name = i.get_object_element("name").get_string();
		const string arg_type = i.get_object_element("type").get_string();
		QUARK_ASSERT(arg_type[0] == '$');

		const auto arg_type2 = resolve_type123(arg_type, temp_type_defs);
		args2.push_back(member_t(arg_type2, arg_name));
	}

	/*
		{ "expr": 1, "name": "g_version", "type": "<int>" },
		{ "expr": "Welcome!", "name": "message", "type": "<string>" }
	*/
	std::vector<member_t> members2;
	for(const auto i: members){
		const string member_name = i.get_object_element("name").get_string();
		const string member_type = i.get_object_element("type").get_string();
		QUARK_ASSERT(member_type[0] == '$');

		const auto member_type2 = resolve_type123(member_type, temp_type_defs);
		members2.push_back(member_t(member_type2, member_name));
	}

	if(type == "function"){
		return scope_def_t::make_function_def(
			type_identifier_t::make(name),
			resolve_type123(return_type_id, temp_type_defs),
			{},
			executable_t(),
			std::vector<member_t>{}
		);
	}
	else if(type == "struct"){
		return scope_def_t::make_struct(
			type_identifier_t::make(name),
			std::vector<member_t>{}
		);
	}
	else if(type == "global"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
	return {};
}

std::shared_ptr<type_def_t> conv_type_def(const json_value_t& def, const map<string, shared_ptr<type_def_t>>& temp_type_defs){
	QUARK_ASSERT(def.check_invariant());

//	const auto name = def.get_object_element("_name");
	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");
	const auto path = def.get_object_element("path");

	if(base_type == "null"){
		return make_shared<type_def_t>(type_def_t::make(k_null));
	}
	else if(base_type == "bool"){
		return make_shared<type_def_t>(type_def_t::make_bool());
	}
	else if(base_type == "int"){
		return make_shared<type_def_t>(type_def_t::make_int());
	}
	else if(base_type == "string"){
		return make_shared<type_def_t>(type_def_t::make(k_string));
	}
	else if(base_type == "struct"){
		return make_shared<type_def_t>(type_def_t::make_struct_def(conv_scope_def(scope_def, temp_type_defs)));
	}
	else if(base_type == "vector"){
		QUARK_ASSERT(false);
	}
	else if(base_type == "function"){
		return make_shared<type_def_t>(type_def_t::make_function_def(conv_scope_def(scope_def, temp_type_defs)));
	}
	else if(base_type == "subscope"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

/*
	"$3": { "name": "string", "base_type": "string" },
	"$4": { "name": "pixel_t", "base_type": "function", "scope_def":
		{
			"parent_scope": "$5",		//	Parent scope.
			"base_type": "function",
			"scope_def": {
				"_name": "pixel_t_constructor",
				"_type": "function",
*/

/*
	Input tree has all types and variables resolved.
	Returns immutable AST.
*/
ast_t json_to_ast(const json_value_t& program){
	//	Make placeholder type-defs for each symbol. We use pointer to type_def as its identity.
	map<string, shared_ptr<type_def_t>> temp_type_defs;
	for(const auto& s: program.get_object_element("lookup").get_object()){
		temp_type_defs[s.first] = make_shared<type_def_t>(type_def_t());
	}

	for(const auto& s: program.get_object_element("lookup").get_object()){
		const auto id = s.first;
		const json_value_t& def = s.second;
		std::shared_ptr<type_def_t> type_def = conv_type_def(def, temp_type_defs);

		shared_ptr<type_def_t> identity_type_def = temp_type_defs.at(id);

		//	Update our identify type defs.
		identity_type_def->swap(*type_def.get());
	}

	const auto global_scope = conv_scope_def(program.get_object_element("global"), temp_type_defs);

	const auto global_scope2 = scope_def_t::make2(
		scope_def_t::etype::k_global_scope,
		type_identifier_t("global"),
		{},//	const std::vector<member_t>& members,
		executable_t(),
		type_identifier_t()
	);

	ast_t result;
	result._symbols = temp_type_defs;
	result._global_scope = global_scope2;
	QUARK_ASSERT(result.check_invariant());
	return result;
}

floyd_parser::ast_t run_pass2(const json_value_t& parse_tree){
	QUARK_TRACE(json_to_pretty_string(parse_tree));

	const ast_t dummy;
	const auto pass_a = pass_a__scope_def(make_parser_path(parse_tree), 1000);
	const auto pass_a5 = pass_a5__scope_def(make_parser_path(pass_a.first), pass_a.second);
	const auto pass_b = pass_b__scope_def(make_parser_path(pass_a5.first));
	QUARK_ASSERT(!has_unresolved_types(pass_b));

	const auto pass_c = pass_c__scope_def(make_parser_path(pass_b));
	const auto pass_d = pass_d__scope_def(make_parser_path(pass_c));

	//	Make sure there are no unresolved types left in program.
	QUARK_ASSERT(!has_unresolved_types(pass_d));

	const ast_t ast = json_to_ast(pass_d);
	trace(ast);
	return ast;
}




//??? pass1 shall insert int, bool etc. Built-in types.



///////////////////////////////////////			TESTS

/*
	These tests verify that:
	- the transform of the AST worked as expected
	- Correct errors are emitted.
*/


QUARK_UNIT_TESTQ("run_pass2()", "Minimum program"){
	const auto a = R"(
		int main(){
			return 3;
		}
		)";
	const auto pass1 = floyd_parser::program_to_ast(a);
	const ast_t pass2 = run_pass2(pass1);



	const auto found_it = find_if(
		pass2._symbols.begin(),
		pass2._symbols.end(),
		[&] (const std::pair<std::string, std::shared_ptr<type_def_t>>& e) { return e.second->to_string() == "int"; }
	);
	QUARK_ASSERT(found_it != pass2._symbols.end());
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
	const auto pass1 = floyd_parser::program_to_ast(a);
	const ast_t pass2 = run_pass2(pass1);
}



void test_error(const string& program, const string& error_string){
	const auto pass1 = floyd_parser::program_to_ast(program);
	try{
		const ast_t pass2 = run_pass2(pass1);
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
