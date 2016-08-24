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
#include <map>
#include <iostream>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;



namespace floyd_runtime {


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




	//////////////////////////////////////		rt_memory_manager


	QUARK_UNIT_TESTQ("frontend_value_ref()", "constructed as valid"){
		const auto a = make_ref();
		QUARK_TEST_VERIFY(a.check_invariant());
	}



	/*
		Virtual memory page. Platform dependant.
	*/
	struct rt_vpage {
		rt_cache_line_t _cache_lines[FLOYD_VIRTUAL_PAGE_SIZE / FLOYD_CACHE_LINE_SIZE];
	};


	/*
	*/
	struct rt_huge_block {
		rt_vpage _pages[FLOYD_HUGE_SIZE / FLOYD_VIRTUAL_PAGE_SIZE];
	};



	//////////////////////////////////////		rt_memory_manager


	rt_cache_line_t* rt_memory_manager::allocate_cl(uint32_t thread_id){
		auto cl = new rt_cache_line_t;

		for(int i = 0 ; i < FLOYD_CACHE_LINE_SIZE ; i++){
			cl->_data[i] = static_cast<uint8_t>(0x00 + i);
		}
		cl->_data[0] = 0xfa;
		cl->_data[FLOYD_CACHE_LINE_SIZE - 1] = 0xfb;
		return cl;
	}

	void rt_memory_manager::free_cl(uint32_t thread_id, rt_cache_line_t* cl){
		delete cl;
		cl = nullptr;
	}

	bool rt_memory_manager::check_invariant() const{
		return true;
	}



	//////////////////////////////////////		rt_memory_manager


	QUARK_UNIT_TESTQ("rt_memory_manager()", "constructed as valid"){
		auto a = std::make_shared<rt_memory_manager>();
		QUARK_TEST_VERIFY(a->check_invariant());
	}

	QUARK_UNIT_TESTQ("allocate_cl()", "allocate a cache line"){
		auto a = std::make_shared<rt_memory_manager>();

		auto cl = a->allocate_cl(0);
		QUARK_TEST_VERIFY(cl != nullptr);

		QUARK_ASSERT(cl->_data[0] == 0xfa);
		QUARK_ASSERT(cl->_data[1] == 0x01);
		QUARK_ASSERT(cl->_data[FLOYD_CACHE_LINE_SIZE - 1] == 0xfb);

		//	!!! cache lines are not cleaned up, manual delete it here to avoid leak.
		a->free_cl(0, cl);
	}




	string emit_c_code(){
		return
			"int main(int argc, char *argv[]){\n"
			"	return 0;\n"
			"}\n";
	}




	floydrt__state* make_state(){
		floydrt__state* state = (floydrt__state*)std::calloc(1, sizeof(floydrt__state));
		return state;
	}

	void delete_state(floydrt__state** /*state*/){
	}



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



}	//	floyd_runtime



