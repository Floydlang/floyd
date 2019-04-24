//
//  floyd_llvm_runtime.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_runtime_hpp
#define floyd_llvm_runtime_hpp

#include "ast_value.h"
#include "floyd_llvm_helpers.h"
#include "ast.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

#include <string>
#include <vector>

namespace llvm {
	struct Module;
	struct ExecutionEngine;
}

namespace floyd {

struct llvm_ir_program_t;



////////////////////////////////		function_def_t




struct function_def_t {
	std::string def_name;
	llvm::Function* llvm_f;

	int floyd_function_id;
	function_definition_t floyd_fundef;
};

////////////////////////////////		llvm_execution_engine_t


//https://en.wikipedia.org/wiki/Hexspeak
const uint64_t k_debug_magic = 0xFACEFEED05050505;

struct llvm_execution_engine_t {
	bool check_invariant() const {
		QUARK_ASSERT(ee);
		return true;
	}

	//	Must be first member, checked by LLVM code.
	uint64_t debug_magic;

	llvm_instance_t* instance;
	std::shared_ptr<llvm::ExecutionEngine> ee;
	type_interner_t type_interner;
	symbol_table_t global_symbols;
	std::vector<function_def_t> function_defs;
	public: std::vector<std::string> _print_output;
};



typedef int64_t (*FLOYD_RUNTIME_INIT)(void* floyd_runtime_ptr);
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(void* floyd_runtime_ptr, int64_t arg);

typedef int64_t (*FLOYD_RUNTIME_F)(void* floyd_runtime_ptr, const char* args);





const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name);


//	Cast to uint64_t* or other the required type, then access via it.
void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name);

//	Cast to function pointer, then call it.
void* get_global_function(llvm_execution_engine_t& ee, const std::string& name);


std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name);
value_t call_function(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f);

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v);

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const uint64_t encoded_value, const typeid_t& type);

VEC_T* unpack_vec_arg(const llvm_execution_engine_t& runtime, int64_t arg_value, int64_t arg_type);


value_t llvm_global_to_value(const llvm_execution_engine_t& runtime, const void* global_ptr, const typeid_t& type);

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const uint64_t encoded_value, const typeid_t& type);




typeid_t unpack_itype(const llvm_execution_engine_t& runtime, int64_t itype);
int64_t pack_itype(const llvm_execution_engine_t& runtime, const typeid_t& type);


value_t llvm_valueptr_to_value(const llvm_execution_engine_t& runtime, const void* value_ptr, const typeid_t& type);






llvm_execution_engine_t& get_floyd_runtime(void* floyd_runtime_ptr);
void hook(const std::string& s, void* floyd_runtime_ptr, int64_t arg);
std::string gen_to_string(llvm_execution_engine_t& runtime, int64_t arg_value, int64_t arg_type);

//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!

void floyd_runtime__unresolved_func(void* floyd_runtime_ptr);




}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
