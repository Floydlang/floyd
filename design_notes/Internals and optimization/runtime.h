//
//  runtime.h
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


namespace floyd_runtime {


	//////////////////////////////////////		frontend_value_ref


	/*
		Value type, immutable, reference counted.
		Always uses 8 bytes, optionally owns external memory too.

		Can hold *any* frontend value, ints, vectors, structs and composites etc.
		The type is external to the value.
		
	*/
	struct frontend_value_ref {
		public: frontend_value_ref();
		public: ~frontend_value_ref();
		public: bool check_invariant() const;

		public: uint8_t _data[8];
	};


	frontend_value_ref make_ref();


	#define FLOYD_CACHE_LINE_SIZE		64
	#define FLOYD_VIRTUAL_PAGE_SIZE		4096
	#define FLOYD_HUGE_SIZE				(4096 * 1024)


	//////////////////////////////////////		rt_cache_line_t


	/*
		Always maches the byte size of the targets cache line: this struct has different sizes depending on architecture.
	*/
	struct rt_cache_line_t {
		uint8_t _data[FLOYD_CACHE_LINE_SIZE];
	};


	//////////////////////////////////////		rt_memory_manager


	struct rt_memory_manager {
		/*
			Returns the hottest cache line.
		*/
		rt_cache_line_t* allocate_cl(uint32_t thread_id);

		/*
			cl == nullptr is OK, has no effect.
		*/
		void free_cl(uint32_t thread_id, rt_cache_line_t* cl);

		/*
			Returns the hottest page.
		*/
//		rt_vpage* allocate_vpage(uint32_t thread_id);
//		void free_vpage(uint32_t thread_id, rt_vpage** cl);

		public: bool check_invariant() const;


		///////////////////		STATE
//		private: std::vector<rt_cache_line_t> _cache_lines;
	};


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

}	//	floyd_runtime


#endif /* runtime_hpp */
