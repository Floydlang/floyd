//
//  floyd_llvm_codegen.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_codegen_hpp
#define floyd_llvm_codegen_hpp

#include "ast_value.h"
#include <string>

int64_t run_using_llvm(const std::string& program, const std::string& file, const std::vector<floyd::value_t>& args);

#endif /* floyd_llvm_codegen_hpp */
