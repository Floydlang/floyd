//
//  runtime_core.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef runtime_core_hpp
#define runtime_core_hpp

#include <stdio.h>

#include <cstdio>
#include <vector>


namespace runtime_core {

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


}	//	runtime_core


#endif /* runtime_core_h */
