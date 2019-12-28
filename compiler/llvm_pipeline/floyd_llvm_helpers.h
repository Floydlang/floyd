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
#include "llvm/Target/TargetMachine.h"


namespace floyd {

struct llvm_type_lookup;
struct link_name_t;
struct value_backend_t;


struct target_t {
	std::string target_triple;
	llvm::TargetMachine* target_machine;


	////////////////////////////////	STATE

	bool check_invariant() const;
};

target_t make_default_target();


//	Must LLVMContext be kept while using the execution engine? Yes!
struct llvm_instance_t {
	llvm_instance_t();

	bool check_invariant() const;


	////////////////////////////////	STATE

	llvm::LLVMContext context;
	target_t target;
};

bool check_invariant__function(const llvm::Function* f);
bool check_invariant__module(llvm::Module* module);
bool check_invariant__builder(llvm::IRBuilder<>* builder);

std::string print_module(llvm::Module& module);
std::string print_type(llvm::Type* type);
std::string print_function(const llvm::Function* f);
std::string print_value(llvm::Value* value);


////////////////////////////////	floyd_runtime_ptr



//	This pointer is passed as argument 0 to all compiled floyd functions and all runtime functions.

bool check_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);
bool check_emitting_function(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);
llvm::Value* get_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f);


////////////////////////////////		CODEGEN


void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg);


////////////////////////////////		VALUES


//	Converts the LLVM value into a uint64_t for storing vector, pass as DYN value.
llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const type_t& floyd_type);

//	Returns the specific LLVM type for the value, like VECTOR_CARRAY_T* etc.
llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const type_t& type);


}	//	floyd

#endif /* floyd_llvm_helpers_hpp */
