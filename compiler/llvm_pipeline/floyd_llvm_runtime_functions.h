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
	runtime_functions_t(const std::vector<function_link_entry_t>& function_defs);


	////////////////////////////////		STATE

	const function_link_entry_t floydrt_init;
	const function_link_entry_t floydrt_deinit;

	const function_link_entry_t floydrt_alloc_kstr;
	const function_link_entry_t floydrt_allocate_vector_fill;
	const function_link_entry_t floydrt_store_vector_element_hamt_mutable;
	const function_link_entry_t floydrt_concatunate_vectors;
	const function_link_entry_t floydrt_load_vector_element_hamt;
	
	const function_link_entry_t floydrt_allocate_dict;

	const function_link_entry_t floydrt_allocate_json;
	const function_link_entry_t floydrt_lookup_json;
	const function_link_entry_t floydrt_json_to_string;

	const function_link_entry_t floydrt_allocate_struct;

	const function_link_entry_t floydrt_compare_values;

	const function_link_entry_t floydrt_get_profile_time;
	const function_link_entry_t floydrt_analyse_benchmark_samples;
};


llvm::Value* generate_allocate_vector(llvm_function_generator_t& gen_acc, const type_t& vector_type, int64_t element_count, vector_backend vector_backend);
llvm::Value* generate_lookup_dict(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, dict_backend dict_mode);
void generate_store_dict_mutable(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, llvm::Value& value_reg, dict_backend dict_mode);
llvm::Value* generate_update_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index, llvm::Value& value_reg);

llvm::Value* generate_load_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index);


void generate_retain(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type);
void generate_release(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type);


} // floyd



#endif /* floyd_llvm_runtime_functions_hpp */
