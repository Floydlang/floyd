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
#include <string>

struct seq_t;

/*
PEG
https://en.wikipedia.org/wiki/Parsing_expression_grammar
http://craftinginterpreters.com/representing-code.html

# FLOYD SYNTAX
	??? add chaining to more expressions

	TYPE:
		NULL				"null"
		BOOL				"bool"
		INT					"int"
		FLOAT				"float"
		STRING				"string"
		TYPEID				???
		VECTOR				"[" TYPE "]"
		DICTIONARY			"{" TYPE ":" TYPE" "}"
		FUNCTION-TYPE		TYPE "(" ARGUMENT ","* ")" "{" BODY "}"
			ARGUMENT		TYPE identifier
		STRUCT-DEF			"struct" IDENTIFIER "{" MEMBER* "}"
			MEMBER			 TYPE IDENTIFIER
		UNNAMED-STRUCT		"struct" "{" TYPE* "}"

		UNRESOLVED-TYPE		IDENTIFIER-CHARS, like "hello"

	EXPRESSION:
		EXPRESSION-COMMA-LIST			EXPRESSION | EXPRESSION "," EXPRESSION-COMMA-LIST
		NAME-EXPRESSION					IDENTIFIER ":" EXPRESSION
		NAME-COLON-EXPRESSION-LIST		NAME-EXPRESSION | NAME-EXPRESSION "," NAME-COLON-EXPRESSION-LIST

		TYPE-NAME						TYPE IDENTIFIER
		TYPE-NAME-COMMA-LIST			TYPE-NAME | TYPE-NAME "," TYPE-NAME-COMMA-LIST
		TYPE-NAME-SEMICOLON-LIST		TYPE-NAME ";" | TYPE-NAME ";" TYPE-NAME-SEMICOLON-LIST

		NUMERIC-LITERAL					"0123456789" +++
		RESOLVE-IDENTIFIER				IDENTIFIER +++

		CONSTRUCTOR-CALL				TYPE "(" EXPRESSION-COMMA-LIST ")" +++

		VECTOR-DEFINITION				"[" EXPRESSION-COMMA-LIST "]" +++
		DICT-DEFINITION					"[" NAME-COLON-EXPRESSION-LIST "]" +++
		ADD								EXPRESSION "+" EXPRESSION +++
		SUB								EXPRESSION "-" EXPRESSION +++
										EXPRESSION "&&" EXPRESSION +++
		RESOLVE-MEMBER					EXPRESSION "." EXPRESSION +++
		GROUP							"(" EXPRESSION ")"" +++
		LOOKUP							EXPRESSION "[" EXPRESSION "]"" +++
		CALL							EXPRESSION "(" EXPRESSION-COMMA-LIST ")" +++
		UNARY-MINUS						"-" EXPRESSION
		CONDITIONAL-OPERATOR			EXPRESSION ? EXPRESSION : EXPRESSION +++

	STATEMENT:
		BODY = "{" STATEMENT* "}"

		RETURN							"return" EXPRESSION
		DEFINE-STRUCT					"struct" IDENTIFIER "{" TYPE-NAME-SEMICOLON-LIST "}"
		IF								"if" "(" EXPRESSION ")" BODY "else" BODY
		IF-ELSE							"if" "(" EXPRESSION ")" BODY "else" "if"(EXPRESSION) BODY "else" BODY
		FOR								"for" "(" IDENTIFIER "in" EXPRESSION "..." EXPRESSION ")" BODY
		WHILE 							"while" "(" EXPRESSOIN ")" BODY

 		BIND							TYPE IDENTIFIER "=" EXPRESSION
		DEFINE-FUNCTION				 	TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" BODY
		EXPRESSION-STATEMENT 			EXPRESSION
 		ASSIGNMENT	 					SYMBOL = EXPRESSION
*/

namespace floyd {
	struct ast_json_t;


	//////////////////////////////////////////////////		read_statement()


	/*
		Read one statement, including any expressions it uses.
		Consumes trailing semicolon, where appropriate.
		Supports all statments:
			- return statement
			- struct-definition
			- function-definition
			- define constant, with initializating.

		Never simplifes expressions- the parser is non-lossy.


		RETURN

		"return 3;"
		"return myfunc(myfunc() + 3);"


		DEFINE STRUCT

		"struct mytype_t { float a; float b; };


		BIND

		"int x = 10;"


		FUNCTION DEFINITION

		"int f(string name){ return 13; }"


		IF-ELSE
		Notice: not trailing semicolon

		"if(true){ return 1000; }else { return 1001;}


		OUTPUT

		["return", EXPRESSION ]
		["bind", "string", "local_name", EXPRESSION ]
		["def_struct", STRUCT_DEF ]
		["define_function", FUNCTION_DEF ]
	*/

	std::pair<ast_json_t, seq_t> parse_statement(const seq_t& pos0);



	//	returns json-array of statements.
	std::pair<ast_json_t, seq_t> parse_statements(const seq_t& s);

	//	returns json-array of statements.
	ast_json_t parse_program2(const std::string& program);

}	//	floyd


#endif /* floyd_parser_h */
