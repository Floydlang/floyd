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

	struct llvm_ir_program_t;
	struct runtime_handler_i;


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

//	This header is followed by a number of uint64_t elements in the same heap block.
//	This header represents a sharepoint of many clients and holds an RC to count clients.
//	If you want to change the size of the allocation, allocate 0 following elements and make separate dynamic allocation and stuff its pointer into data1.
struct heap_alloc_64_t {
	public: virtual ~heap_alloc_64_t(){};


	////////////////////////////////		STATE
	uint64_t allocation_word_count;
	uint64_t element_count;
	int32_t rc;
	int32_t data0;
	uint64_t data1;
	uint64_t data2;
	uint64_t data3;
	char debug_info[16];
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

	public: runtime_handler_i* _handler;

	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


	public: std::vector<heap_alloc_64_t> heap_allocs;
};


typedef int64_t (*FLOYD_RUNTIME_INIT)(void* floyd_runtime_ptr);
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(void* floyd_runtime_ptr, int64_t arg);




//	func int main([string] args) impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_IMPURE)(void* floyd_runtime_ptr, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE)(void* floyd_runtime_ptr);

//	func int main([string] args) pure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_PURE)(void* floyd_runtime_ptr, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_PURE)(void* floyd_runtime_ptr);




//		func my_gui_state_t my_gui__init() impure { }
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_INIT)(void* floyd_runtime_ptr);

//		func my_gui_state_t my_gui(my_gui_state_t state, json_value message) impure{
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_MESSAGE)(void* floyd_runtime_ptr, runtime_value_t state, runtime_value_t message);




typedef runtime_value_t (*FLOYD_RUNTIME_F)(void* floyd_runtime_ptr, const char* args);


const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name);

//	Cast to uint64_t* or other the required type, then access via it.
void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name);


void* get_global_function(llvm_execution_engine_t& ee, const std::string& name);

std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name);

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v);




struct llvm_bind_t {
	std::string name;
	void* address;
	typeid_t type;
};

llvm_bind_t bind_function2(llvm_execution_engine_t& ee, const std::string& name);


int64_t call_main(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f, const std::vector<std::string>& main_args);

std::vector<host_func_t> get_runtime_functions(llvm::LLVMContext& context);
std::map<std::string, void*> get_host_functions_map2();

uint64_t call_floyd_runtime_init(llvm_execution_engine_t& ee);


llvm_execution_engine_t make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program);
llvm_execution_engine_t make_engine_run_init(llvm_instance_t& instance, llvm_ir_program_t& program);

std::map<std::string, value_t> run_llvm_container(llvm_ir_program_t& program_breaks, const std::vector<std::string>& main_args, const std::string& container_key);


}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
