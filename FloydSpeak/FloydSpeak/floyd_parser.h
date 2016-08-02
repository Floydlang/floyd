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

#include "parser_primitives.h"
#include "parser_statement.h"

namespace floyd_parser {


    struct statement_result_t {
        statement_t _statement;
        ast_t _ast;
        std::string _rest;
    };

    statement_result_t read_statement(const ast_t& ast1, const std::string& pos);

	ast_t program_to_ast(const ast_t& init, const std::string& program);



//	Proves we can instantiate a struct.
const std::string k_test_program_100 =
	"struct pixel { string s; };"
	"string main(){\n"
	"	return \"\";"
	"}\n";

//	Proves we can read a struct member variable.
const std::string k_test_program_101 =
	"return p.s + a;";


/*
//	Proves we can read a struct member variable.
const std::string k_test_program_102 =
	"struct pixel { string s; };"
	"string main(){\n"
	"	pixel p = pixel( \"hiya\" );"
	"	return p.s + a;"
	"}\n";
*/


}	//	floyd_parser



#endif /* floyd_parser_h */
