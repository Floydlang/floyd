//
//  floyd_llvm_runtime.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_corelib_hpp
#define floyd_llvm_corelib_hpp

#include "ast_value.h"
#include "floyd_llvm_helpers.h"
#include "ast.h"


#include <string>
#include <vector>

namespace llvm {
	struct ExecutionEngine;
}

namespace floyd {

struct floyd_runtime_t;

STRUCT_T* floyd_funcdef__calc_string_sha1(floyd_runtime_t* frp, runtime_value_t s0);
STRUCT_T* floyd_funcdef__calc_binary_sha1(floyd_runtime_t* frp, STRUCT_T* binary_ptr);

int64_t floyd_funcdef__get_time_of_day(floyd_runtime_t* frp);

runtime_value_t floyd_funcdef__read_text_file(floyd_runtime_t* frp, runtime_value_t arg);
void floyd_funcdef__write_text_file(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t data0);

void floyd_funcdef__create_directory_branch(floyd_runtime_t* frp, runtime_value_t path0);
void floyd_funcdef__delete_fsentry_deep(floyd_runtime_t* frp, runtime_value_t path0);
uint8_t floyd_funcdef__does_fsentry_exist(floyd_runtime_t* frp, runtime_value_t path0);
runtime_value_t floyd_funcdef__get_fs_environment(floyd_runtime_t* frp);
VEC_T* floyd_funcdef__get_fsentries_deep(floyd_runtime_t* frp, runtime_value_t path0);
VEC_T* floyd_funcdef__get_fsentries_shallow(floyd_runtime_t* frp, runtime_value_t path0);
STRUCT_T* floyd_funcdef__get_fsentry_info(floyd_runtime_t* frp, runtime_value_t path0);
void floyd_funcdef__rename_fsentry(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t name0);


}	//	namespace floyd


#endif /* floyd_llvm_corelib_hpp */
