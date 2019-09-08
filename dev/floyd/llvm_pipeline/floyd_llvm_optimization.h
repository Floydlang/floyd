//
//  floyd_llvm_optimization.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-09-08.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_optimization_hpp
#define floyd_llvm_optimization_hpp

namespace llvm {
	struct Module;
}

#include <memory>


namespace floyd {

struct llvm_instance_t;

void optimize_module_mutating(llvm_instance_t& instance, std::unique_ptr<llvm::Module>& module);

} // floyd


#endif /* floyd_llvm_optimization_hpp */
