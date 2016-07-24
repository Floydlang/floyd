
//
//  runtime_types.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "runtime_types.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>



namespace runtime_types {



	using std::string;

	string to_string(const frontend_base_type t){
		if(t == k_int32){
			return "int32";
		}
		else if(t == k_string){
			return "string";
		}
		else if(t == k_struct){
			return "struct";
		}
		else if(t == k_vector){
			return "vector";
		}
		else{
			QUARK_ASSERT(false);
		}
	}

	void trace_frontend_type(const frontend_type_t& t){
		QUARK_TRACE("frontend_type_t <" + t._name + ">");


		const auto s = to_string(t._base_type);
	}


} //	runtime_types;



using namespace runtime_types;



//////////////////////////////////////		rt_memory_manager


QUARK_UNIT_TESTQ("to_string()", ""){
	QUARK_TEST_VERIFY(to_string(k_int32) == "int32");
	QUARK_TEST_VERIFY(to_string(k_string) == "string");
	QUARK_TEST_VERIFY(to_string(k_struct) == "struct");
	QUARK_TEST_VERIFY(to_string(k_vector) == "vector");
}



