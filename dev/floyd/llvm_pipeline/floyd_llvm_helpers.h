//
//  floyd_llvm_helpers.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_helpers_hpp
#define floyd_llvm_helpers_hpp

#include "value_backend.h"

#include <llvm/IR/IRBuilder.h>




namespace floyd {

struct llvm_type_lookup;
struct link_name_t;
struct value_mgr_t;

//	Must LLVMContext be kept while using the execution engine? Yes!
struct llvm_instance_t {
	bool check_invariant() const {
		return true;
	}

	llvm::LLVMContext context;
};


bool check_invariant__function(const llvm::Function* f);
bool check_invariant__module(llvm::Module* module);
bool check_invariant__builder(llvm::IRBuilder<>* builder);

std::string print_module(llvm::Module& module);
std::string print_type(llvm::Type* type);
std::string print_function(const llvm::Function* f);
std::string print_value(llvm::Value* value);


////////////////////////////////	floyd_runtime_ptr


/*
	The Floyd runtime doesn't use global variables at all. Not even for memory heaps etc.
	Instead it passes around an invisible argumen to all functions, called Floyd Runtime Ptr (FRP).
*/

struct floyd_runtime_t {
};

//	This pointer is passed as argument 0 to all compiled floyd functions and all runtime functions.

bool check_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);
bool check_emitting_function(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);
llvm::Value* get_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);



////////////////////////////////		CODEGEN


void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg);


llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr);



////////////////////////////////		ENCODE / DECODE LLVM LINK NAMES



//	"hello" => "floydf_hello"
link_name_t encode_floyd_func_link_name(const std::string& name);
std::string decode_floyd_func_link_name(const link_name_t& name);


//	"hello" => "floydrt_hello"
link_name_t encode_runtime_func_link_name(const std::string& name);
std::string decode_runtime_func_link_name(const link_name_t& name);




////////////////////////////////		VALUES


VECTOR_CPPVECTOR_T* unpack_vector_cppvector_arg(const value_mgr_t& value_mgr, runtime_value_t arg_value, runtime_type_t arg_type);
DICT_CPPMAP_T* unpack_dict_cppmap_arg(const value_mgr_t& value_mgr, runtime_value_t arg_value, runtime_type_t arg_type);

//	Converts the LLVM value into a uint64_t for storing vector, pass as DYN value.
llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const typeid_t& floyd_type);

//	Returns the specific LLVM type for the value, like VECTOR_CPPVECTOR_T* etc.
llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const typeid_t& type);


}	//	floyd

#endif /* floyd_llvm_helpers_hpp */
