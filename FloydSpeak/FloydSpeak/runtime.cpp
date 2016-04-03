//
//  runtime.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 31/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "runtime.h"

#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true

#include "quark.h"
#include "steady_vector.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;



string emit_c_code(){
	return
		"int main(int argc, char *argv[]){\n"
		"	return 0;\n"
		"}\n";
}





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

floydrt__state* make_state(){
	floydrt__state* state = (floydrt__state*)std::calloc(1, sizeof(floydrt__state));
	return state;
}

void delete_state(floydrt__state** /*state*/){
}



/*

*/

struct floydrt__string {
	uint32_t _rc;
	char* _s;
	size_t _length;
};


floydrt__string* floydrt__string__make(const char s[], size_t length){
	floydrt__string* obj = (floydrt__string*)std::calloc(1, sizeof(floydrt__string));
	char* str = (char*)std::calloc(1, length);
	std::memcpy(str, s, length);
	obj->_rc = 1;
	obj->_s = str;
	obj->_length = length;
	return obj;
}

void floydrt__string__add_ref(floydrt__string* s){
	s->_rc++;
}
void floydrt__string__release_ref(floydrt__string** s){
	(*s)->_rc--;
	if((*s)->_rc == 0){
		std::free((*s)->_s);
		std::free(*s);
		*s = NULL;
	}
}

floydrt__string* floydrt__string__make_from_cstr(const char s[]){
	return floydrt__string__make(s, strlen(s));
}



int floydgen__myfunc(/*restrict*/ floydrt__state* state, int64_t balance, /*restrict*/ const floydrt__string* name){
	return 0;
}



/*
int main ( int argc, char *argv[] )
{
  if ( argc != 2 ) // argc should be 2 for correct execution
    // We print argv[0] assuming it is the program name
    cout<<"usage: "<< argv[0] <<" <filename>\n";
  else {
    // We assume argv[1] is a filename to open
    ifstream the_file ( argv[1] );
    // Always check to see if file opening succeeded
    if ( !the_file.is_open() )
      cout<<"Could not open file\n";
    else {
      char x;
      // the_file.get ( x ) returns false if the end of the file
      //  is reached or an error occurs
      while ( the_file.get ( x ) )
        cout<< x;
    }
    // the_file is closed implicitly here
  }
}
*/
int floydgen__main(int argc, const char* argv[]){
	floydrt__state* state = make_state();
	floydrt__string* name = floydrt__string__make_from_cstr("Hello, world");
	int error = floydgen__myfunc(state, 1000, name);
	floydrt__string__release_ref(&name);
	delete_state(&state);
	return error;
}


QUARK_UNIT_TEST("", "compiler_pass1()", "", ""){
//	QUARK_TEST_VERIFY(r._functions["main"]._return_type == type_t::make_type("int"));
}



/*
int main(int argc, const char * argv[]) {
	try {
#if QUARK_UNIT_TESTS_ON
		quark::run_tests();
#endif
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

  return 0;
}
*/

