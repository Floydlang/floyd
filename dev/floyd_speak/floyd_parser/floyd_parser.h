#ifndef floyd_parser_h
#define floyd_parser_h
/*
	Converts source code text to an AST, encoded in a JSON.
	Not much validation is going on, except the syntax itself.
	Result may contain unresolvable references to indentifers, illegal names etc.
*/

#include "quark.h"
#include "json_support.h"
#include <string>

struct seq_t;

namespace floyd {
	struct parse_result_t;

	struct parse_tree_t {
		//	Holds a complete program, as a JSON-array with one element per statement.
		json_t _value;
	};

	//	"a = 1; print(a)"
	parse_result_t parse_statements_no_brackets(const seq_t& s);

	//	"{ a = 1; print(a) }"
	parse_result_t parse_statements_bracketted(const seq_t& s);

	//	returns json-array of statements.
	parse_tree_t parse_program2(const std::string& program);

}	//	floyd namespace

#endif /* floyd_parser_h */
