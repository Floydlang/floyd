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

namespace floyd {

void optimize_module_mutating(llvm::Module& module);

} // floyd


#endif /* floyd_llvm_optimization_hpp */
