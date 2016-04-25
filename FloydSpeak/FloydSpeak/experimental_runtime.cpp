//
//  experimental_runtime.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 25/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "experimental_runtime.h"



/*
	Frontend = the perspective of the Floyd program and developer
	Backend = the actual storage of the data type etc. Can chose to organize things in a different way.
*/

//////////////////////////////////////////////////		experimental


namespace experimental_runtime {



//////////////////////////////////////////////////		RUNTIME_CORE

#define FLOYD_CACHE_LINE_SIZE		64
#define FLOYD_VIRTUAL_PAGE_SIZE		4096
#define FLOYD_HUGE_SIZE				(4096 * 1024)


/*
	Always maches the byte size of the targets cache line: this struct has different sizes depending on architecture.
*/
struct rt_cache_line {
	uint8_t _data[FLOYD_CACHE_LINE_SIZE];
};


/*
	Virtual memory page. Platform dependant.
*/
struct rt_vpage {
	rt_cache_line _cache_lines[FLOYD_VIRTUAL_PAGE_SIZE / FLOYD_CACHE_LINE_SIZE];
};


/*
*/
struct rt_huge_block {
	rt_vpage _pages[FLOYD_HUGE_SIZE / FLOYD_VIRTUAL_PAGE_SIZE];
};


/*
*/
struct rt_stack_frame {
	rt_stack_frame* _prev;

	/*
		Fixed maximum size of local variables. ### Make this dynamic to unlimited stack?
	*/
	rt_cache_line* _local_variables[16384 / sizeof(rt_cache_line)];
};


struct rt_memory_manager {
	/*
		Returns the hottest cache line.
	*/
	rt_cache_line* allocate_cl(uint32_t thread_id);
	void free_cl(uint32_t thread_id, rt_cache_line** cl);

	/*
		Returns the hottest page.
	*/
	rt_vpage* allocate_vpage(uint32_t thread_id);
	void free_vpage(uint32_t thread_id, rt_vpage** cl);
};


/*
	This type is tracked by compiler, not stored in the value-type.
*/
enum frontend_base_type {
	k_int32_plain,
	k_string_plain,
	k_function,
	k_struct,
	k_vector,
	k_map
};

/*
	Recursively describes a frontend type.
*/
struct frontend_type {
	/*
		Plain types only use the _base_type. ### Add support for int-ranges etc.
	*/
	frontend_base_type _base_type;

	/*
		Function
			_output_type is function return-type.
			_input_types are function arguements.

		Struct
			_inputs_types are struct members.

		Map
			_output_type is value type
			_inputs_types[0] is key type.

		Vector
			_output_type is value type
	*/

	std::shared_ptr<const frontend_type> _output_type;
	std::vector<std::shared_ptr<frontend_type> > _input_types;
};

/*
*/
struct frontend_value8 {
	uint8_t _data[8];
};


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

struct experimental_interned_strings {
	//??? reallocates = illegal.
	std::vector<backend_string> _strings;
	uint64_t _unique_string_id_generator;
};



/*
	Stores its data in rt_cache_line.
	Leaf node directly stores values if at least 8 values fit in one leaf (requires 4 rt_cache_line to hold leaf data).Else the leaf keeps references to the values.
*/
template<typename T> struct rt_pvec {
};
	


struct experimental_floydrt__state {
	experimental_interned_strings* _strings;

	rt_stack_frame _stack_frame;

	uint32_t _thread_id;
	rt_memory_manager _memory_manager;
};


}	//	experimental_runtime

