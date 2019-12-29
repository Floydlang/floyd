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

struct llvm_function_generator_t;


//	Make link entries for all intrinsics functions, like assert() including optimized specialisations.
std::vector<func_link_t> make_intrinsics_link_map(
	llvm::LLVMContext& context,
	const llvm_type_lookup& type_lookup,
	const intrinsic_signatures_t& intrinsic_signatures
);

llvm::Value* generate_instrinsic_push_back(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type, llvm::Value& value_reg);

llvm::Value* generate_instrinsic_size(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type);

llvm::Value* generate_instrinsic_update(llvm_function_generator_t& gen_acc, const type_t& resolved_call_type, llvm::Value& collection_reg, const type_t& collection_type, llvm::Value& key_reg, llvm::Value& value_reg);

llvm::Value* generate_instrinsic_map(
	llvm_function_generator_t& gen_acc,
	const type_t& resolved_call_type,
	llvm::Value& elements_vec_reg,
	const type_t& elements_vec_type,
	llvm::Value& f_reg,
	const type_t& f_type,
	llvm::Value& context_reg,
	const type_t& context_type
);

} // floyd


#endif /* floyd_llvm_intrinsics_hpp */
