//
//  floyd_basics.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2017-08-09.
//  Copyright © 2017 Marcus Zetterquist. All rights reserved.
//

#include "floyd_basics.h"


using std::string;

namespace floyd_basics {

	//////////////////////////////////////////////////		base_type


	string base_type_to_string(const base_type t){
		if(t == base_type::k_null){
			return "null";
		}
		else if(t == base_type::k_bool){
			return "bool";
		}
		else if(t == base_type::k_int){
			return "int";
		}
		else if(t == base_type::k_float){
			return "float";
		}
		else if(t == base_type::k_string){
			return "string";
		}
		else if(t == base_type::k_struct){
			return "struct";
		}
		else if(t == base_type::k_vector){
			return "vector";
		}
		else if(t == base_type::k_function){
			return "function";
		}
		else{
			QUARK_ASSERT(false);
		}
	}

}



	//////////////////////////////////////		base_type_to_string(base_type)


	QUARK_UNIT_TESTQ("base_type_to_string(base_type)", ""){
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_bool) == "bool");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_int) == "int");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_float) == "float");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_string) == "string");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_struct) == "struct");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_vector) == "vector");
		QUARK_TEST_VERIFY(base_type_to_string(floyd_basics::base_type::k_function) == "function");
	}

