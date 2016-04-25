//
//  runtime.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 31/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef runtime_hpp
#define runtime_hpp

#include <stdio.h>
#include <memory>
#include <vector>


//////////////////////////////////////////////////		floydrt__state


enum floydrt__exception_type {
	exception,
	runtime_exception,
	defect_exception,
	out_of_bounds,
	out_of_resource,
	invalid_format,
	valid_but_unsupported_format
};

struct floydrt__active_exception {
	floydrt__exception_type _type;
	char _target[128];
	char _throw_file_name[255 + 1];
	uint32_t _throw_line;
};

struct floydrt__state {
	//	pools
	//	collections

	/*
		active exceptions. Array.
		### Use setjmp() to throw exceptions: this frees return value from error handling.
	*/
	floydrt__active_exception* _active_exceptions;
	int _active_exception_count;
};


floydrt__state* make_state();
void delete_state(floydrt__state** /*state*/);



//////////////////////////////////////////////////		floydrt__string



struct floydrt__string {
	uint32_t _rc;
	char* _s;
	size_t _length;
};




#endif /* runtime_hpp */
