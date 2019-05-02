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
	struct ExecutionEngine;
}

namespace floyd {




////////////////////////////////		host_func_t


struct host_func_t {
	std::string name_key;
	llvm::FunctionType* function_type;
	void* implementation_f;
};



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


	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
};


typedef int64_t (*FLOYD_RUNTIME_INIT)(void* floyd_runtime_ptr);
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(void* floyd_runtime_ptr, int64_t arg);


//??? How to do this?
typedef runtime_value_t (*FLOYD_RUNTIME_F)(void* floyd_runtime_ptr, const char* args);


const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name);

//	Cast to uint64_t* or other the required type, then access via it.
void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name);

//	Cast to function pointer, then call it.
void* get_global_function(llvm_execution_engine_t& ee, const std::string& name);

std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name);
value_t call_function(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f);

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v);


std::vector<host_func_t> get_runtime_functions(llvm::LLVMContext& context);
std::map<std::string, void*> get_host_functions_map2();

uint64_t call_floyd_runtime_init(llvm_execution_engine_t& ee);


}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
