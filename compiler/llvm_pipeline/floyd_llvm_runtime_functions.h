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
#include "floyd_llvm_execution_engine.h"
#include "floyd_llvm_helpers.h"

namespace floyd {

struct llvm_function_generator_t;


//	Make link entries for all runtime functions, like floydrt_retain_vec().
//	These have no floyd-style function type, only llvm function type, since they use parameters not expressable with type_t.
std::vector<func_link_t> make_runtime_function_link_map(llvm::LLVMContext& context, const llvm_type_lookup& type_lookup);

llvm::Value* generate_allocate_vector(llvm_function_generator_t& gen_acc, const type_t& vector_type, int64_t element_count, vector_backend vector_backend);
llvm::Value* generate_lookup_dict(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, dict_backend dict_mode);
void generate_store_dict_mutable(llvm_function_generator_t& gen_acc, llvm::Value& dict_reg, const type_t& dict_type, llvm::Value& key_reg, llvm::Value& value_reg, dict_backend dict_mode);
llvm::Value* generate_update_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index, llvm::Value& value_reg);

llvm::Value* generate_load_struct_member(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& struct_type, int member_index);


void generate_retain(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type);
void generate_release(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const type_t& type);


} // floyd



#endif /* floyd_llvm_runtime_functions_hpp */
