//
//  floyd_llvm_runtime_functions.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_runtime_functions_hpp
#define floyd_llvm_runtime_functions_hpp

#include "floyd_llvm_types.h"
#include "floyd_llvm_runtime.h"

namespace floyd {

struct function_bind_t;
struct llvm_function_generator_t;

//	These are the support function built into the runtime, like RC primitives.
std::vector<function_bind_t> get_runtime_function_binds(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup);





////////////////////////////////		runtime_functions_t


struct runtime_functions_t {
	runtime_functions_t(const std::vector<function_def_t>& function_defs);


	////////////////////////////////		STATE

	const function_def_t floydrt_init;
	const function_def_t floydrt_deinit;

	const function_def_t floydrt_alloc_kstr;
	const function_def_t floydrt_allocate_vector_fill;
	const function_def_t floydrt_store_vector_element_mutable;
	const function_def_t floydrt_concatunate_vectors;
	const function_def_t floydrt_push_back_hamt_pod;
	const function_def_t floydrt_load_vector_element_hamt;
	
	const function_def_t floydrt_allocate_dict;
	const function_def_t floydrt_lookup_dict;
	const function_def_t floydrt_store_dict_mutable;

	const function_def_t floydrt_allocate_json;
	const function_def_t floydrt_release_json;
	const function_def_t floydrt_lookup_json;
	const function_def_t floydrt_json_to_string;

	const function_def_t floydrt_allocate_struct;
	const function_def_t floydrt_release_struct;
	const function_def_t floydrt_update_struct_member;

	const function_def_t floydrt_compare_values;

	const function_def_t floydrt_get_profile_time;
	const function_def_t floydrt_analyse_benchmark_samples;
};


llvm::Value* generate_allocate_vector(const std::vector<function_def_t>& function_defs, llvm::IRBuilder<>& builder, llvm::Value& frp_reg, llvm::Value& vector_type_reg, int64_t element_count, vector_backend vector_backend);


void generate_retain(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const typeid_t& type);
void generate_release(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const typeid_t& type);


} // floyd



#endif /* floyd_llvm_runtime_functions_hpp */
