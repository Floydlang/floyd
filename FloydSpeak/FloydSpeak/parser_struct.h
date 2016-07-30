//
//  parser_struct.hpp
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

namespace floyd_parser {
	struct struct_def_t;

	/*
		{}
		{int a;}
	*/
	std::pair<struct_def_t, std::string> parse_struct_body(const std::string& s);


	const std::string k_test_struct0 = "{int x; string y; float z;}";
	struct_def_t make_test_struct0();

}

#endif /* parser_struct_hpp */
