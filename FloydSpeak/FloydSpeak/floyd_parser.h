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

#include "parser_statement.h"
#include "parser_ast.h"

namespace floyd_parser {
	struct scope_def_t;

	/*
		Resolves the type, starting at _scope_instances.back() then moving towards to global space. This is a compile-time operation.
	*/
	//??? const version
	std::shared_ptr<floyd_parser::type_def_t> resolve_type(const scope_ref_t scope_def, const std::string& s);


	floyd_parser::value_t make_default_value(const scope_ref_t scope_def, const floyd_parser::type_identifier_t& type);

	floyd_parser::value_t make_default_value(const floyd_parser::type_def_t& t);
	floyd_parser::value_t make_default_value(const floyd_parser::struct_def_t& t);


    struct statement_result_t {
        statement_t _statement;
        std::string _rest;
    };

	statement_result_t read_statement(scope_ref_t scope_def, const std::string& pos);

	ast_t program_to_ast(const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
