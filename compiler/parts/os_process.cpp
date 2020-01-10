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
	TRACE_STACK();
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
	TRACE_STACK();

	stack_test(9000, nullptr);
}
#endif



/*
	x86 stack grows from high -> low addresses
*/

static const size_t k_low_stack = 100000;



stack_info_light_t sample_stack_light(){
	uint64_t probe = 0x0123456789abcdef;

	const auto p = pthread_self();

	__uint64_t thread_id = 0;
	const int err = pthread_threadid_np(p, &thread_id);
	QUARK_ASSERT(err == 0);

	const size_t size = pthread_get_stacksize_np(p);
	const uint8_t* stack_base_high = (uint8_t*)pthread_get_stackaddr_np(p);
	const uint8_t* stack_end_low = stack_base_high - size;

	stack_info_light_t result;
	result.stack_pos = (uint8_t*)&probe;
	result.stack_base_high = stack_base_high;
	result.stack_end_low = stack_end_low;
	result.thread_id = thread_id;
	return result;
}

stack_info_t sample_stack(){
	const auto light = sample_stack_light();

//	pthread_id_np_t tid = pthread_getthreadid_np();

	const auto p = pthread_self();

	char name_buffer[100 + 1];
	int	name_x = pthread_getname_np(p, name_buffer, 100);
	(void)name_x;

	/* returns non-zero if the current thread is the main thread */
	const int is_main = pthread_main_np();

	stack_info_t result;
	result.light = light;
	strcpy(result.thread_name, name_buffer);
	result.is_main_thread = is_main == 1;
	return result;
}


//	Result is not a valid FS path -- it's for preview only.
std::string shorten_path(const std::string& path, int chars){
	const auto size = path.size();
	if(size < chars){
		return path;
	}
	else{
		return path.substr(path.size() - chars, chars);
	}
}

QUARK_TEST("", "shorten_path()", "", ""){
	QUARK_VERIFY(shorten_path("", 10) == "");
}
QUARK_TEST("", "shorten_path()", "", ""){
	QUARK_VERIFY(shorten_path("hello", 10) == "hello");
}
QUARK_TEST("", "shorten_path()", "", ""){
	QUARK_VERIFY(shorten_path("hello", 4) == "ello");
}


void trace_psthread_stack_info(const quark::source_code_location& location){
	const auto s = sample_stack();

	if(false && s.is_main_thread){
		return;
	}

	const auto size = s.light.stack_base_high - s.light.stack_end_low;
	const auto used_bytes = s.light.stack_base_high - s.light.stack_pos;
	const auto free_bytes = s.light.stack_pos - s.light.stack_end_low;
#if 1
	QUARK_TRACE_SS(
		"PTHREAD INFO "
		<< s.light.thread_id
		<< " " << shorten_path(location._source_file, 40) << ":" << location._line_number << ":" << std::string(location._func)

		<< "\t probe addr:" << ptr_to_hexstring(s.light.stack_pos)

		<< " (" << ptr_to_hexstring(s.light.stack_base_high)
		<< " - " << ptr_to_hexstring(s.light.stack_end_low) << ")"

		<< ", size: " << size
		<< ", used: " << used_bytes
		<< ", free: " << free_bytes
		<< ", name: " << std::string(s.thread_name)
		<< ", main: " << (s.is_main_thread ? "YES" : "NO")
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

