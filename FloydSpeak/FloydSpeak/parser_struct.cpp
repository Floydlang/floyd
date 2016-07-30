//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_struct.h"


#include "parser_types.h"
#include "parser_expression.hpp"
#include "parser_function.h"
#include "parser_struct.h"


namespace floyd_parser {

	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;





	struct_def_t make_test_struct0(){
		return {
			vector<arg_t>
			{
				{ make_type_identifier("int"), "x" },
				{ make_type_identifier("string"), "y" },
				{ make_type_identifier("float"), "z" }
			}
		};
	}


}