//
//  runtime_core.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 24/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "runtime_core.h"


#include "quark.h"
#include <vector>
#include <string>
#include <map>



namespace runtime_core {



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



}	//	runtime_core






using namespace runtime_core;



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



