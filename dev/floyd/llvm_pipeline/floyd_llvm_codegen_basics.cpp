//
//  floyd_llvm_codegen_basics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-31.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen_basics.h"


namespace floyd {




llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto itype = lookup_itype(gen_acc.type_lookup, type).get_data();
	auto t = make_runtime_type_type(gen_acc.type_lookup);
 	return llvm::ConstantInt::get(t, itype);
}




}	//	floyd
