#  FLOYD PARSE TREE

This is the data outputted by the Floyd parser. It is a JSON tree like this:

//	Array of statements. Each statement in an array.
[
	[ loc, statement-type-string, optional data ],
	[ loc, statement-type-string, optional data ],
	[ loc, statement-type-string, optional data ]
]


PARSE_TREE:		STATEMENTS
STATEMENTS:	[ STATEMENT ]

STATEMENT:		[ loc, STATEMENT_TYPE, OPTIONAL_DATA ]


[ loc, "block", STATEMENTS ]
[ loc, "return", EXPRESSION ]


EXPRESSION:	[ loc, EXPRESSION_TYPE, OPTIONAL_DATA ]

[ loc, "unary-minus", EXPRESSION ]
[ loc, "value-constructor", EXPRESSION, ELEMENT_TYPE, [ EXPRESSION ] ]
[ loc, "benchmark", EXPRESSION, STATEMENTS ] ]
[ loc, "k", VALUE, TYPE ]
[ loc, "@", IDENTIFIER_STRING ]			//	load = access variable using name
[ loc, "@i", IDENTIFIER_STRING ]			//	load2 = access variable using index
[ loc, "call", EXPRESSION, [ EXPRESSION ] ]


[ loc, "->", EXPRESSION, IDENTIFIER ]	//	member_access, address, member_name
[ loc, "[]", EXPRESSION, EXPRESSION ] 	//	lookup_member(object, key)

[ loc, "+", EXPRESSION, EXPRESSION ]
[ loc, "-", EXPRESSION, EXPRESSION ]
[ loc, "*", EXPRESSION, EXPRESSION ]
[ loc, "/", EXPRESSION, EXPRESSION ]
[ loc, "%", EXPRESSION, EXPRESSION ]
[ loc, "%", EXPRESSION, EXPRESSION ]

[ loc, "?:", EXPRESSION, EXPRESSION, EXPRESSION ]	//	conditional_operator(condition, a, b)

[ loc, "%", EXPRESSION, EXPRESSION ]

[ loc, "==", EXPRESSION, EXPRESSION ]
[ loc, "!=", EXPRESSION, EXPRESSION ]
[ loc, "<=", EXPRESSION, EXPRESSION ]
[ loc, "<", EXPRESSION, EXPRESSION ]
[ loc, ">=", EXPRESSION, EXPRESSION ]
[ loc, ">", EXPRESSION, EXPRESSION ]
[ loc, "&&", EXPRESSION, EXPRESSION ]
[ loc, "||", EXPRESSION, EXPRESSION ]


ELEMENT_TYPE: TYPE
TYPE:
	"undefined",
	"any",
	"void",
	"bool", "int", "double", "string", json","typeid",
	[ "struct", [ MEMBER ] ]
	[ "vector", ELEMENT_TYPE ]
	[ "dict", ELEMENT_TYPE ]
	[ "func", ELEMENT_TYPE, [ ELEMENT_TYPE ], true / false ]
	"#ABC*"


CORECALLS

