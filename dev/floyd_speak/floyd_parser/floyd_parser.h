
#ifndef floyd_parser_h
#define floyd_parser_h

/*
	Converts source code text to an AST, encoded in a JSON.
	Not much validation is going on, except the syntax itself.
	Result may contain unresolvable references to indentifers, illegal names etc.
*/

#include "quark.h"
#include <string>


struct seq_t;

namespace floyd {
	struct ast_json_t;
	struct parse_result_t;


	std::pair<ast_json_t, seq_t> parse_statement(const seq_t& pos0);

	//	"a = 1; print(a)"
	parse_result_t parse_statements_no_brackets(const seq_t& s);

	//	"{ a = 1; print(a) }"
	parse_result_t parse_statements_bracketted(const seq_t& s);

	//	returns json-array of statements.
	parse_result_t parse_program2(const std::string& program);

}	//	floyd


#endif /* floyd_parser_h */
