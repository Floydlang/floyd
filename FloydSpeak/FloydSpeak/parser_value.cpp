//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "parser_statement.h"


namespace floyd_parser {
	using std::string;


	//////////////////////////////////////////////////		struct_instance_t

		bool struct_instance_t::check_invariant() const{
			for(const auto m: _member_values){
				QUARK_ASSERT(m.second.check_invariant());
				//??? check type of member value is the same as in the type_def.
			}
			return true;
		}

		bool struct_instance_t::operator==(const struct_instance_t& other){
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return __def == other.__def && _member_values == other._member_values;
		}


		std::string to_preview(const struct_instance_t& instance){
			string r;
			for(const auto m: instance._member_values){
				r = r + (string("<") + m.first + ">" + m.second.plain_value_to_string());
			}
			return string("{") + r + "}";
		}

	//////////////////////////////////////////////////		value_t


void trace(const value_t& e){
	QUARK_ASSERT(e.check_invariant());

	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}


QUARK_UNIT_TESTQ("value_t()", "null"){
	QUARK_TEST_VERIFY(value_t().plain_value_to_string() == "<null>");
}

QUARK_UNIT_TESTQ("value_t()", "bool"){
	QUARK_TEST_VERIFY(value_t(true).plain_value_to_string() == "true");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	QUARK_TEST_VERIFY(value_t(13).plain_value_to_string() == "13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	QUARK_TEST_VERIFY(value_t(13.5f).plain_value_to_string() == "13.500000");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	QUARK_TEST_VERIFY(value_t("hello").plain_value_to_string() == "'hello'");
}

//??? more


}	//	floyd_parser
