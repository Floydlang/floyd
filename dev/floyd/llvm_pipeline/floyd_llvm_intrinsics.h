//
//  floyd_llvm_intrinsics.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_intrinsics_hpp
#define floyd_llvm_intrinsics_hpp


#include "floyd_llvm_types.h"
#include "floyd_llvm_runtime.h"

namespace floyd {

std::map<std::string, void*> get_intrinsic_binds();


} // floyd


#endif /* floyd_llvm_intrinsics_hpp */
