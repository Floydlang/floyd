
/*
	"%1" = type_definition_t of the coord struct. Two floats, x and y etc. Static scope inside parent scope.
	"coord_t": a type_identifier_t-string.
	Type-name-lookup map binds type identifier string "coord_t" to type definition "%1". (Many type identifiers can bind to same type definition)

	"a": variable of type %1
	"pixel_coord_t": type identifiers that ALSO looks up to %1.
*/
struct coord_t {
	float x;
	float y;
}

let a = z(10.0, 20.0)

typedef coord_t pixel_coord_t

/*
	New function type %2: int (int a, int b)
	New function constant "=10" of type %2, - store instructions
	New constant variable "f" of type %2 holding function value =10
*/
int f(int a, int b){
	return a + b;
}



//////////////////////		AST


//	Defines the type: is it an int? A vector of strings? A function with prototype int (float, float)?

struct type_definition_t {
	base_type_t basetype

	type_definition_t function_return 
	vector<type_definition_t> function_args 

	type_definition_t vector_element_type 

	vector<type_definition_t> struct_members 
}

struct function_value_constant_t {
	type_definiton_t function_signature
	scope_def function_code
}

struct static_scope_def {
	type: global, function, struct, statement_subscope
	vector<member_t> _members
}

struct ast_t {
	static_scope_def globals

	//	Many typenames can link to the same type definition...
	map<type_identifier_t, string> typenames

	map<string, type_definition_t> type_definitions

	map<string, function_value_constant_t> function_value_constants
}







string get_s(pixel p){ return p.s; }
struct pixel { string s = "two"; }
string main(){
	pixel p = pixel_constructor();
	return get_s(p);
}




# AST 2017

INSIGHT: Paths make ast fragile, better to store data in their hiearchy.
INSIGHT: We want to track static scope hiearchy.
INSIGHT: Make a few uniform objects, so it's easy to traverse using compact logic.

TODO: Keep program in a tree, keeping static scopes nested.


	expression
	member/arg/local
	static_scope: function, globals, struct, statement_scope
	statement
	type
	function_value_constant = the instructions making up a function.

??? Reverse scope_def thinking and use a scopeed symbol table instead. Makes it easier to process scopes:

TODO: Separate type (function prototype) from function_value_constant (its implemenation). 


IDEA: Only keep symbol table for collecting and resolving names, then lose it! Keep original symbols embedded in IDs for humans.


INSIGHT about passes
Pass 1: convert text to AST using syntax
Pass 2: Semantic analysis of AST, collect types, connect dots, find errors, figure out types of expressions, collect member variables, local variables. Bind things together (using temporary symbol table). Insert generated functions and code.

Now program works and is "complete" and compiles. AST is in an easy-to-manipulate form. JSON version is used by people.

- Nested scopes and symbol tables instead of big flat symbol table => easier to nest and compose code.

Futher passes are about executing or optimizing or code generation.


	struct pixel { float red; float green; float blue; }
	pixel pixel(float red, float green, float blue)
	float get_grey(pixel p){ (return p.red + p.green + p.blue()) / 3; }

	float main(){
		pixel p = pixel(1, 0, 0);
		return get_grey(p);
	}


- Simplest to have two formats for types: "pixel" and "pixel/8000" -- this requires no extra fields to store resolved type id and we don't lose original name. We pack typename / typeid into same string. Notice: only use typeid 8000 to find stuff.

symbol name = "pixel"
typename = "$pixel"
typeid = 8000
typetag = "$pixel" or "$pixel/8000"



??? Functions and structs must track in which static scope they were defined in order to resolve symbols correctly.

global static_scope
{
	"name": "global",
	"type": "global",

	//	Each introduces its own static_scope within parent scope.

	"struct_defs": {
		//	"pixel"
		"9000", {
			"state": [
				{ "name": "red", "typetag": "$float" },
				{ "name": "green", "typetag": "$float" },
				{ "name": "blue", "typetag": "$float" }
			]
		},
	},

	//??? Can hold subscopes for statements like while(true) { subscope }.
	//??? How to easily traverse AST when subscopes can hide inside statements?

	"function_constant_values": {
		//	"pixel pixel(float, float, float)"
		"7999", {
			"args": [
				{ "name": "red", "typetag": "$float" }, { "name": "green", "typetag": "$float" }, { "name": "blue", "typetag": "$float" }
			],
			"state": [],
			"return_typetag": "$float",
			"statements": [
				["bind", "$pixel", "p",
					["call", ["@", "pixel"], [ [ "k", 1 ], [ "k", 0 ], [ "k", 0 ]]]
				],
				["while",
					[ "k", "false" ],
					[ "statements"
						["bind", "$pixel", "test", ["call", ["@", "pixel"], []]],
					]
				]
				["return", ["call", ["@", "get_grey"], [["@", "p"]]]]
			],
		}

		//	"get_grey()"
		"8000", {
			"args": [{ "name": "p", "typentag": "$pixel" }],
			"return_typetag": "$float",
			"state": []
			"statements": [
				["return",
					["/",
						[ "+"
							["->", ["@", "p"], "red"],
							[ "+"
								["->", ["@", "p"], "green"], 
								["->", ["@", "p"], "blue"]
							],
						],
						3
					]
				]
			]
		},

		//	"main()"
		"8001", {
			"args": [],
			"state": [{ "name": "p", "typetag": "$pixel" }],
			"return_typetag": "$float",
			"statements": [
				["bind", "$pixel", "p", ["call", ["@", "pixel"], []]],
				["return", ["call", ["@", "get_grey"], [["@", "p"]]]]
			],
			"type": "function",
			"types": {}
		}
	}

	"state": [
		{ "name": "main", "value": "function_constant_values/main/8001" }
	],
	"symbols": {
		"get_grey": { "value": "function_constant_values/8000" },
		"pixel": { "struct_def": "struct_defs/9000" },
		"pixel": { "value": "function_constant_values/7999" },
		"main": { "value": "function_constant_values/main/8001" }
	}

	"statements": [],
}



-------------------------


/*
	ABOUT ADDRESSING AND CHAINS

	Each value is refered to via an address which is a construct like this: { scope + member }.
	Encoded as { value_t scope, string member_name } or { value_t scope int _member_offset }.

	This address can specifify any value in the system:

	- To a local variable in the function-body-scope
	- a function argument in the function-scope
	- a global variable as a value in the global scope
	- point to a struct + which member.


	CALL is made on a function_scope


		PROBLEM: How to resolve a complex address expression tree into something you can read a value from (or store a value to or call as a function etc.
		We don't have any value we can return from each expression in tree.
		Alternatives:

		A) [CHOSEN SOLUTION]
			Have dedicated expression types:
			struct_member_address_t { expression_t _parent_address, struct_def* _def, shared_ptr<struct_instance_t> _instance, string _member_name; }
			collection_lookup { vector_def* _def, shared_ptr<vector_instance_t> _instance, value_t _key };

			resolve-variable "xyz"
			resolve-member "xyz"
			lookup x

		B)	Have value_t of type struct_member_spec_t { string member_name, value_t} so a value_t can point to a specific member variable.
		C)	parse address in special function that resolves the expression and keeps the actual address on the side. Address can be raw C++ pointer.


	CHAINS
	"hello.test.a[10 + x].next.last.get_ptr().title"

	call						"call"
	resolve_variable			"@"
	resolve_member				"->"
	lookup						"[-]"

	!!! AST DOES NOT GENERATE LOADs, ONLY IDENTIFIER, FOR EXAMPLE.

		a = my_global_int;
		["bind", "<int>", "a", ["@", "my_global_int"]]

		"my_global.next"
		["->", ["@", "my_global"], "next"]

		c = my_global_obj.all[3].f(10).prev;
*/
