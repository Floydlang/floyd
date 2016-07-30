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



	std::pair<struct_def_t, std::string> parse_struct_body(const string& s){
		const auto s2 = skip_whitespace(s);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<arg_t> members;
		auto pos = trim_ends(body_pos.first);
		while(!pos.empty()){
			const auto arg_type = read_type(pos);
			const auto arg_name = read_identifier(arg_type.second);
			const auto optional_comma = read_optional_char(skip_whitespace(arg_name.second), ';');

			const auto a = arg_t{ make_type_identifier(arg_type.first), arg_name.first };
			members.push_back(a);
			pos = optional_comma.second;
		}

		return { struct_def_t{members}, body_pos.second };
	}

	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		QUARK_TEST_VERIFY((parse_struct_body("{}") == pair<struct_def_t, string>({}, "")));
	}

	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		QUARK_TEST_VERIFY((parse_struct_body(" {} x") == pair<struct_def_t, string>({}, " x")));
	}


	QUARK_UNIT_TESTQ("parse_struct_body", ""){
		const auto r = parse_struct_body(k_test_struct0);
		QUARK_TEST_VERIFY((
			r == pair<struct_def_t, string>(make_test_struct0(), "" )
		));
	}




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