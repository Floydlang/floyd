//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_statement_hpp
#define parse_statement_hpp


#include "quark.h"
#include <vector>
#include <string>

namespace floyd_parser {
	struct return_statement_t;
	struct statement_t;

	/*
		s:
			Must start with "return".

			Examples:
				return 0;
				return x + y;

	*/
	std::pair<return_statement_t, std::string> parse_return_statement(const std::string& s);


	/*
		"int a = 10;"
		"float b = 0.3;"
		"int c = a + b;"
		"int b = f(a);"
		"string hello = f(a) + \"_suffix\";";

		...can contain trailing whitespace.
	*/
	std::pair<statement_t, std::string> parse_assignment_statement(const std::string& s);

}	//	floyd_parser


#endif /* parser_statement_hpp */
