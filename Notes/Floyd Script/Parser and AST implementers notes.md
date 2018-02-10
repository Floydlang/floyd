# BUILDING BLOCKS & TERMS & TERMS
- expression
- member/arg/local
- static_scope: function, globals, struct, statement_scope
- statement
- type
- function_value_constant = the instructions making up a function.


# PASS 1
Pass 1: convert text to AST using syntax

Parser reads source code and generates an abstract syntax tree (AST) with nodes directly mapped to each bit of the source code. It also figures out expressions and paranthesis according to Floyd's evaluation rules. Evaluation order is completely defined in parser's output.

Nested functions and structs makes a potentially deep tree.

All statements are recorded.

Parser's jobb is just to get away from source code into the world of nodes. It has no further intelligients or logic.

It's important that the parser do not optimize of change code, we support round trip back to source code.

TODO: record line numbers
TODO: record comments


# PASS 2
Pass 2: Semantic analysis of AST, collect types, connect dots, find errors, figure out types of expressions, collect member variables, local variables. Bind things together (using temporary symbol table). Insert generated functions and code.

- Generate compiler errors.
- Verifies all expression and statement semantics
- Resolves types and track them explicitly.
- Checks that all types are compatible.
- Resolves variable accesses (when possible).
- Assign a result-type to each expression.

OUTPUT FORMAT GOALS
- Easy to read for humans
- Easy to transform
- Lose no information -- keep all original names and comments.
- Not simplified or optimized in any way.

TODO
- Use *index* of member, not the name.
- Check input as if user input -- allows pass2 to be run on external JSON of AST.



# TYPES & SYMBOLS

Example program:

	int my_glob = 3;

	struct coord_t {
		float x;
		float y;
	}

	int f(int a, int b){
		return a + b;
	}

	let a = coord_t(10.0, 20.0)
	
	typedef coord_t pixel_coord_t

RESULT

1. symbol "my_glob" that maps to variable containing 3
2. struct definition 8000:

	```
	"coord_t/8000"
	{
		float x;
		float y;
	}
	```
3. symbol "coord_t" that maps to typeid "coord_t/8000"
4. function definition 8001:


	```
	return: int
	arguments: (int a, int b)
	instructions: [ return a + b ]
	```
5. symbol "f" of typeid "int (float a, float b)" that maps to function constant 8001

6. symbol "pixel_coord_t" that maps to typeid "pixel_coord_t = coord_t/8000". This is a unique ID that shares definition with struct "coord_t/8000" and is still its own type.



"bool"	-	Built in bool type id
"int"	-	Built in integer type id
"float"	-	Built in integer type id
"string"	-	Built in integer type id

"struct/coord_t/8000"		-	this is a struct with definition object 8000. The struct definition was called "coord_t".

"[int]"		-	Vector of integers
"[function/int(float,float)]"	--	Vector of funtions of type int(float,float)
"function/bool()"		-- function with bool return and no arguments.
"function/int(float,float)"		-- function with int return and two float arguments.

		[ "int" ]
		[ "struct", "coord_t", "$8000" ]
		[ "function", "int (float, float)" ]
		[ "vector", "[string]" ]

		[ "<int>" ]

		//	"coord_t/8000" is the unique ID for this struct definition
		[ "struct", "<coord_t/8000>" ]
		[ "function", "<int> (<float>, <float>)" ]
		[ "function", "<int> (<coord_t/8000>, <[<coord_t/8000>]>)" ]

		[ "function", "[ "int", [(<coord_t/8000>, <[<coord_t/8000>]>]" ]

		[ "vector", "<[<string>]>" ]
		[ "vector", "<[<coord_t/8000>]>" ]

	??? Use recurisve JSON to describe nested signatures?
		[ "vector", "[string]" ]


RULES:

- Several different functions with different implementations can have the same type-definition.
- NOTICE: Each struct will get a unique type definition, even when several structs have the same layout.
- Several functions can have the same symbol, but differ on type. Can have same symbol as a struct.
- Many typeid:s can share the same function definition or struct definition.
- Important: function defintions and struct definitions need to stay related to the static scope they were defined in, for runtime symbol resolving.
- Only keep symbol table for collecting and resolving names, then lose it! Keep original symbols embedded in IDs for humans.

- In AST, store symbols as a string, like "f". When resolved, store as "f/1234" were 1234 is the unique ID of the definition. requires no extra fields to store resolved type id and we don't lose original name. We pack typename / typeid into same string. Notice: only use typeid 8000 to find stuff.
	OLD: typetag = "$pixel" (unresolved typename) or "$pixel/8000" (resolved typeid). 
- Paths make ast fragile, better to store data in their hiearchy.
- Keep program in a tree, keeping static scopes nested.
- Idea: Reverse scope_def thinking and use a scopeed symbol table instead. Makes it easier to process scopes.
- Separate type (function prototype) from function_value_constant (its implemenation). 
- Make a few uniform C++ classes to store AST, so it's easy to traverse using compact logic.



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

# Algorithm for resolving references to types & variables

PROBLEM: How to store links to resolved to types or variables?


TODO: Augument existing data, don't replace typename / variable name.
TODO: Generate type IDs like this: "global/pixel_t"


- Need to be able to change nesting, insert new types, function, variables without breaking links in the AST.

ALTERNATIVES:

**A**. { int scope_up_count, int item_index }. Problem - item_index breaks if we insert / reorder parent scope members.

**B**. Resolve by checking types exist, but do not store new info.

**C**. Generate IDs for types. Use type-lookup from ID to correct entry. Store ID in each resolved place. Put all

CHOSEN ==>>>> **D**. Create global type-lookup table. Use original static-scope-path as ID. Use these paths to refer from client to type.

Solution D algo steps:

1. Scan tree, give each found type a unique ID. (Tag each scope and assign parent-scope ID.)
2. Insert constructors for each struct.
3. Scan tree: resolve type references by storing the type-ID. Tag expressions with their output-type. Do this will we can see the scope of each type.
4. Scan tree: move all types to global list for fast finding from ID.
5. Scan tree: bind variables to type-ID + offset.

Now we can convert to a typesafe AST!

Result after transform C:

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


#	ADDRESSING

Addressing is used by code to specify *which* variable or member variable or global variable to read or write. One expression can consist of a chain of expressions leading up to the target.

It can address:

 1. a local variable in the current function's scope
 2. a function argument in the current function's scope
 3. a global variable
 4. a member of a struct
 5. an unnamed temp variables, as in: print get_window().title. Here 

Example adressing expressions:

 1. "my_local_x"
 2. "my_glob33"
 3. "window.title"
 4. "hello.test.a[10 + x].next.last.get_ptr().title"
 
The expressions are broken down by parser to several different primitive expression nodes that then gets combined:

- "@"		- resolve variable in current function's scope. Only used ONCE for one address.
- "->"		- resolve member of struct

Related expression nodes:

- "[-]"		- lookup element, used for [] as in "my_array[3]".
- "call"	- call a function

The AST only contains the addressing expression, no explicit expression node for "store" or "load. ??? Maybe change this!

	a = my_global_int;
	["bind", "<int>", "a", ["@", "my_global_int"]]

	"my_global.next"
	["->", ["@", "my_global"], "next"]

	c = my_global_obj.all[3].f(10).prev;


###	PROBLEM: Addressing
How to resolve a complex address expression tree into something you can read a value from (or store a value to, or call as a function etc). Current AST design doesn't have any value we can return from each expression in tree. In C or ASM we can return a typed pointer.

Alternatives:

**A** [CHOSEN SOLUTION]
Have dedicated expression types:

- resolve-variable "xyz"
- resolve-member "xyz"
- lookup x

**B**	Have value_t of type struct_member_spec_t { string member_name, value_t} so a value_t can point to a specific member variable.

**C**	parse address in special function that resolves the expression and keeps the actual address on the side. Address can be raw C++ pointer.
