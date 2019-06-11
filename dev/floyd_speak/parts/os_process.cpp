//
//  os_process.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-05-15.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "os_process.h"


#include <thread>
#include <pthread.h>
#include <condition_variable>

#include <cstring>

std::string get_current_thread_name(){
	char name[16];

#ifndef __EMSCRIPTEN_PTHREADS__
	//pthread_setname_np(pthread_self(), s.c_str()); // set the name (pthread_self() returns the pthread_t of the current thread)
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

