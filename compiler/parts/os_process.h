//
//  os_process.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-05-15.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef os_process_hpp
#define os_process_hpp

#include <string>

std::string get_current_thread_name();
void set_current_threads_name(const std::string& s);

void trace_psthread_stack_info();

void stack_test(int x, uint8_t* ptr);


#endif /* os_process_hpp */
