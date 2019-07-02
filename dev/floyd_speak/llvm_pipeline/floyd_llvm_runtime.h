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
	struct semantic_ast_t;
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

	function_id_t floyd_function_id;
	function_definition_t floyd_fundef;
};






////////////////////////////////		llvm_execution_engine_t


//https://en.wikipedia.org/wiki/Hexspeak
const uint64_t k_debug_magic = 0xFACEFEED05050505;


struct llvm_execution_engine_t {
	~llvm_execution_engine_t();
	bool check_invariant() const;



	////////////////////////////////		STATE

	//	Must be first member, checked by LLVM code.
	uint64_t debug_magic;

	llvm_instance_t* instance;
	std::shared_ptr<llvm::ExecutionEngine> ee;
	llvm_type_interner_t type_interner;
	symbol_table_t global_symbols;
	std::vector<function_def_t> function_defs;
	public: std::vector<std::string> _print_output;

	public: runtime_handler_i* _handler;

	public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
	public: heap_t heap;

	std::pair<void*, typeid_t> main_function;
	bool inited;
};


typedef int64_t (*FLOYD_RUNTIME_INIT)(floyd_runtime_t* frp);
typedef int64_t (*FLOYD_RUNTIME_DEINIT)(floyd_runtime_t* frp);
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(floyd_runtime_t* frp, int64_t arg);




//	func int main([string] args) impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_IMPURE)(floyd_runtime_t* frp, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_IMPURE)(floyd_runtime_t* frp);

//	func int main([string] args) pure
typedef int64_t (*FLOYD_RUNTIME_MAIN_ARGS_PURE)(floyd_runtime_t* frp, runtime_value_t args);

//	func int main() impure
typedef int64_t (*FLOYD_RUNTIME_MAIN_NO_ARGS_PURE)(floyd_runtime_t* frp);




//		func my_gui_state_t my_gui__init() impure { }
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_INIT)(floyd_runtime_t* frp);

//		func my_gui_state_t my_gui(my_gui_state_t state, json_value message) impure{
typedef runtime_value_t (*FLOYD_RUNTIME_PROCESS_MESSAGE)(floyd_runtime_t* frp, runtime_value_t state, runtime_value_t message);




typedef runtime_value_t (*FLOYD_RUNTIME_F)(floyd_runtime_t* frp, const char* args);


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


//	These are the support function built into the runtime, like RC primitives.
std::vector<host_func_t> get_runtime_functions(llvm::LLVMContext& context, const llvm_type_interner_t& interner);
std::map<std::string, void*> get_c_function_ptrs();

uint64_t call_floyd_runtime_init(llvm_execution_engine_t& ee);
uint64_t call_floyd_runtime_deinit(llvm_execution_engine_t& ee);

int64_t llvm_call_main(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f, const std::vector<std::string>& main_args);


//	Calls init() and will perform deinit() when engine is destructed later.
std::unique_ptr<llvm_execution_engine_t> start_program(llvm_ir_program_t& program);


std::map<std::string, value_t> run_llvm_container(llvm_ir_program_t& program_breaks, const std::vector<std::string>& main_args, const std::string& container_key);





//	Helper that goes directly from source to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const compilation_unit_t& cu);

//	Compiles and runs the program.
int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<std::string>& main_args);



}	//	namespace floyd


#endif /* floyd_llvm_runtime_hpp */
