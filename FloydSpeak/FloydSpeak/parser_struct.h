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

namespace floyd_parser {
	struct struct_def_t;
	struct ast_t;
	struct scope_def_t;


	/*
		struct pixel { int red; int green; int blue; };
		struct pixel { int red = 255; int green = 255; int blue = 255; };
	*/
	std::tuple<std::string, struct_def_t, std::string> parse_struct_definition(const scope_def_t& scope_def, const std::string& pos);


	const std::string k_test_struct0_body = "{int x; string y; float z;}";
	struct_def_t make_test_struct0(const scope_def_t& scope_def);

}

#endif /* parser_struct_hpp */
