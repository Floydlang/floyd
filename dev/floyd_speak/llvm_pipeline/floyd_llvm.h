//
//  floyd_llvm.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_hpp
#define floyd_llvm_hpp

#include "floyd_llvm_codegen.h"
#include "floyd_llvm_runtime.h"


namespace floyd {

//	Helper that goes directly from source to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const compilation_unit_t& cu);

//	Compiles and runs the program.
int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<std::string>& main_args);


}	//	floyd

#endif /* floyd_llvm_hpp */
