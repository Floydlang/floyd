//
//  os_process.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-05-15.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "os_process.h"


#include <thread>
#include <pthread.h>
#include <unistd.h>

#include "utils.h"
#include <condition_variable>

#include <cstring>

std::string get_current_thread_name(){
	char name[16];

#ifndef __EMSCRIPTEN_PTHREADS__
	pthread_getname_np(pthread_self(), &name[0], sizeof(name));
#else
	strcpy(name, "");
#endif
	if(strlen(name) == 0){
		return "main";
	}
	else{
		return std::string(name);
	}
}




#if QUARK_MAC

void set_current_threads_name(const std::string& s){

//			const auto native_thread = thread::native_handle();
	pthread_setname_np(/*pthread_self(),*/ s.c_str());
}

#else

void set_current_threads_name(const std::string& s){
}

#endif



void stack_test(int x, uint8_t* ptr){
	trace_psthread_stack_info();
	uint8_t data[1 * 1024];
	data[0] = (uint8_t)x;
	data[1023] = data[0] + 1;
	if(x > 0){
		stack_test(x - 1, data);
	}
}

#if 0
//	Crashes by overrunning stack-
QUARK_TEST_VIP("", "", "", ""){
	trace_psthread_stack_info();

	stack_test(9000, nullptr);
}
#endif



/*
	x86 stack grows from high -> low addresses
*/

static const size_t k_low_stack = 100000;

void trace_psthread_stack_info(){
	uint64_t probe = 0x0123456789abcdef;

	const uint8_t* stack_pos = (uint8_t*)&probe;

	const auto p = pthread_self();

	__uint64_t thread_id = 0;
	const int err = pthread_threadid_np(p, &thread_id);
	QUARK_ASSERT(err == 0);

//	pthread_id_np_t tid = pthread_getthreadid_np();

	char name_buffer[200 + 1];
	int	name_x = pthread_getname_np(p, name_buffer, 200);
	(void)name_x;
	const auto name = std::string(name_buffer);

	/* returns non-zero if the current thread is the main thread */
	const int is_main = pthread_main_np();
	if(false && is_main == 1){
		return;
	}

	const size_t size = pthread_get_stacksize_np(p);
	const uint8_t* stack_base_high = (uint8_t*)pthread_get_stackaddr_np(p);
	const uint8_t* stack_end_low = stack_base_high - size;

	const auto used_bytes = stack_base_high - stack_pos;
	const auto free_bytes = stack_pos - stack_end_low;
#if 0
	QUARK_TRACE_SS(
		"PTHREAD INFO "
		<< thread_id
		<< " probe addr:" << ptr_to_hexstring(stack_pos)

		<< " (" << ptr_to_hexstring(stack_base_high)
		<< " - " << ptr_to_hexstring(stack_end_low) << ")"

		<< ", size: " << size
		<< ", used: " << used_bytes
		<< ", free: " << free_bytes
		<< ", name: " << name
		<< ", main: " << (is_main == 1 ? "YES" : "NO")
		<< (free_bytes < k_low_stack ? "•••LOW-STACK•••" : "")
	);
#else
	QUARK_TRACE_SS(free_bytes);
#endif

	if(free_bytes < k_low_stack){
		//	Put breakpoint here.
		QUARK_ASSERT(true);
	}
}

