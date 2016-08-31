//
//  parser_struct.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_struct_hpp
#define parser_struct_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <tuple>
#include "parser_ast.h"

namespace floyd_parser {
	struct scope_def_t;


	/*
		"struct pixel { int red; int green; int blue; }"
		"struct pixel { int red = 255; int green = 255; int blue = 255; }"
		
	*/
	std::pair<scope_ref_t, std::string> parse_struct_definition(scope_ref_t scope_def, const std::string& pos);


	const std::string k_test_struct0_body = "{int x; string y; float z;}";
	scope_ref_t make_test_struct0(scope_ref_t scope_def);

}

#endif /* parser_struct_hpp */
