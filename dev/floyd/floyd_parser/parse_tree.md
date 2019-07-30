#  FLOYD PARSE TREE

This is the data outputted by the Floyd parser. It is a JSON tree like this:

Array of statements. Each statement in an array.
```
[
	[ LOCATION, STATEMENT-OPCODE, STATEMENT-ARG* ],
	[ LOCATION, STATEMENT-OPCODE, STATEMENT-ARG* ],
	[ LOCATION, STATEMENT-OPCODE, STATEMENT-ARG* ]
]
```

Here is the complete syntax:
```
PARSE_TREE:	STATEMENTS

STATEMENT: [ LOCATION, STATEMENT-OPCODE, * ]
	[ LOCATION, "return", EXPRESSION ]

	[ LOCATION, "init-local", [ TYPE, IDENTIFIER, EXPRESSION, { "mutable": true }* ] ]
	[ LOCATION, "assign", [ TYPE, IDENTIFIER, EXPRESSION ] ]
	[ LOCATION, "block", STATEMENTS ]
	[ LOCATION, "def-struct", { "name": NAME, "members": [ STRUCT-MEMBER ]* } ]
	[ LOCATION, "def-func", [ { "name": NAME, "args": ARGS, "statements": STATEMENTS, "return_type": TYPE, "impure": BOOL ] ]

	[ LOCATION, "if", EXPRESSION, STATEMENTS, STATEMENTS ]
	[ LOCATION, "for", "closed-range" / "open-range", IDENTIFIER, EXPRESSION, EXPRESSION, STATEMENTS ]
	[ LOCATION, "while", EXPRESSION, STATEMENTS ]

	[ LOCATION, "expression-statement", EXPRESSION ]

	[ LOCATION, "software-system-def", JSON ]
	[ LOCATION, "container-def", JSON ]
	[ LOCATION, "benchmark-def", STATEMENTS ]

EXPRESSION:	[ LOCATION, EXPRESSION-OPCODE, * ]
	[ LOCATION, "k", VALUE, TYPE ]
	[ LOCATION, "call", EXPRESSION, [ EXPRESSION ] ]
	[ LOCATION, "@", IDENTIFIER ]			//	load = access variable using name
	[ LOCATION, "->", EXPRESSION, IDENTIFIER ]	//	member_access, address, member_name
	[ LOCATION, "unary-minus", EXPRESSION ]
	[ LOCATION, "?:", EXPRESSION, EXPRESSION, EXPRESSION ]	//	conditional_operator(condition, a, b)
	[ LOCATION, "value-constructor", EXPRESSION, ELEMENT-TYPE, [ EXPRESSION ] ]
	[ LOCATION, "[]", EXPRESSION, EXPRESSION ] 	//	lookup_member(object, key)
	[ LOCATION, "benchmark", EXPRESSION, STATEMENTS ] ]

	[ LOCATION, "+", EXPRESSION, EXPRESSION ]
	[ LOCATION, "-", EXPRESSION, EXPRESSION ]
	[ LOCATION, "*", EXPRESSION, EXPRESSION ]
	[ LOCATION, "/", EXPRESSION, EXPRESSION ]
	[ LOCATION, "%", EXPRESSION, EXPRESSION ]
	[ LOCATION, "%", EXPRESSION, EXPRESSION ]

	[ LOCATION, "&&", EXPRESSION, EXPRESSION ]
	[ LOCATION, "||", EXPRESSION, EXPRESSION ]

	[ LOCATION, "<=", EXPRESSION, EXPRESSION ]
	[ LOCATION, "<", EXPRESSION, EXPRESSION ]
	[ LOCATION, ">=", EXPRESSION, EXPRESSION ]
	[ LOCATION, ">", EXPRESSION, EXPRESSION ]

	[ LOCATION, "==", EXPRESSION, EXPRESSION ]
	[ LOCATION, "!=", EXPRESSION, EXPRESSION ]

LOCATION: integer
STATEMENT-OPCODE: string
STATEMENTS: [ STATEMENT ]*

EXPRESSION-OPCODE: string
IDENTIFIER: string

STRUCT-MEMBER: { "type": TYPE, "name": IDENTIFIER }
STRUCT-MEMBERS: [ STRUCT-MEMBER]*

ARG: { "type": TYPE, "name": IDENTIFIER }
ARGS: [ ARG ]*
BOOL: true OR false

ELEMENT-TYPE: TYPE
TYPE:
	"undefined",
	"any",
	"void",
	"bool", "int", "double", "string", json","typeid",
	[ "struct", STRUCT-MEMBERS ]
	[ "vector", ELEMENT-TYPE ]
	[ "dict", ELEMENT-TYPE ]
	[ "func", ELEMENT-TYPE, [ ELEMENT-TYPE ], true / false ]
	"#ABC*"
```
