//
//  runtime_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "runtime_value.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>



/*
	Cast a frontend_value8 to a frontend_value8_string
	Encoding
		“”
			_data[0] = 0x00

		“A”
			_data[0] = 0x01
			_data[1] = ‘A’

		“ABCDEFG”:
			_data[0] = 0x07
			_data[1] = ‘A’
			_data[2] = ‘B’
			_data[7] = ‘G’

		“ABCDEFG1”:
			_data[0] = 0xff
			_data[0 - 7] — 56 bit global string index.
*/
struct frontend_value8_string {
	uint8_t _size;
	uint8_t _char0;
	uint8_t _char1;
	uint8_t _char2;
	uint8_t _char3;
	uint8_t _char4;
	uint8_t _char5;
	uint8_t _char6;
	uint8_t _char7;
};


struct backend_string {
	uint32_t _atomic_rc;
	size_t _count;

	//	Malloced.
	const char* _raw_chars;
};

struct interned_strings {
	//!!! reallocates = illegal.
	std::vector<backend_string> _strings;
	uint64_t _unique_string_id_generator;
};




namespace runtime_value {



	//////////////////////////////////////		frontend_value_ref

	frontend_value_ref::frontend_value_ref(){
	}

	frontend_value_ref::~frontend_value_ref(){
	}

	bool frontend_value_ref::check_invariant() const{
		return true;
	}



	frontend_value_ref make_ref(){
		const frontend_value_ref a;
		return a;
	}



}	//	runtime_value



using namespace runtime_value;



//////////////////////////////////////		rt_memory_manager


QUARK_UNIT_TESTQ("frontend_value_ref()", "constructed as valid"){
	const auto a = make_ref();
	QUARK_TEST_VERIFY(a.check_invariant());
}





