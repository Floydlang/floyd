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


namespace floyd {

//	Compiles and runs the program.
int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args);

}	//	floyd

#endif /* floyd_llvm_hpp */
