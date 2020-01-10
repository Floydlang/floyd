//
//  os_process.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-05-15.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef os_process_hpp
#define os_process_hpp

#include "quark.h"
#include <string>

std::string get_current_thread_name();
void set_current_threads_name(const std::string& s);


//#define TRACE_STACK() trace_psthread_stack_info(QUARK_SOURCE_LOCATION())
#define TRACE_STACK()

void trace_psthread_stack_info(const quark::source_code_location& location);

struct stack_info_light_t {
	const uint8_t* stack_pos;
	const uint8_t* stack_base_high;
	const uint8_t* stack_end_low;
	uint64_t thread_id;
};

stack_info_light_t sample_stack_light();

struct stack_info_t {
	stack_info_light_t light;
	char thread_name[100 + 1];
	bool is_main_thread;
};

stack_info_t sample_stack();


void stack_test(int x, uint8_t* ptr);


#endif /* os_process_hpp */
