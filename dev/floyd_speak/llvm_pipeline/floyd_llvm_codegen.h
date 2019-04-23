//
//  floyd_llvm_codegen.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_codegen_hpp
#define floyd_llvm_codegen_hpp

#include "ast_value.h"
#include "statement.h"
#include "ast.h"
#include "floyd_llvm_helpers.h"
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

namespace floyd {
	struct semantic_ast_t;
}
namespace llvm {
	struct Module;
	struct ExecutionEngine;
}
namespace floyd {


struct resolved_symbol_t {
	llvm::Value* value_ptr;
	std::string debug_str;
	enum class esymtype { k_global, k_local, k_function_argument } symtype;

	std::string symbol_name;
	symbol_t symbol;
};




struct function_def_t {
	std::string def_name;
	llvm::Function* llvm_f;

	int floyd_function_id;
	function_definition_t floyd_fundef;
};



////////////////////////////////		llvm_ir_program_t


struct llvm_ir_program_t {
	llvm_ir_program_t(const llvm_ir_program_t& other) = delete;
	llvm_ir_program_t& operator=(const llvm_ir_program_t& other) = delete;

	explicit llvm_ir_program_t(llvm_instance_t* instance, std::unique_ptr<llvm::Module>& module2_swap, const type_interner_t& interner, const symbol_table_t& globals, const std::vector<function_def_t>& function_defs) :
		instance(instance),
		type_interner(interner),
		debug_globals(globals),
		function_defs(function_defs)
	{
		module.swap(module2_swap);
		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
		QUARK_ASSERT(instance != nullptr);
		QUARK_ASSERT(instance->check_invariant());
		QUARK_ASSERT(module);
		QUARK_ASSERT(check_invariant__module(module.get()));
		return true;
	}


	////////////////////////////////		STATE

	llvm_instance_t* instance;

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	std::unique_ptr<llvm::Module> module;

	type_interner_t type_interner;
	symbol_table_t debug_globals;
	std::vector<function_def_t> function_defs;
};

const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name);

//	Converts the semantic AST to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast, const std::string& module_name);

//	Runs the LLVM IR program.
int64_t run_llvm_program(llvm_instance_t& instance, llvm_ir_program_t& program_breaks, const std::vector<floyd::value_t>& args);



////////////////////////////////		NATIVE FUNCTION PROTOYPES


typedef int64_t (*FLOYD_RUNTIME_INIT)(void* floyd_runtime_ptr);
typedef void (*FLOYD_RUNTIME_HOST_FUNCTION)(void* floyd_runtime_ptr, int64_t arg);

typedef int64_t (*FLOYD_RUNTIME_F)(void* floyd_runtime_ptr, const char* args);


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


llvm_execution_engine_t make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program);
llvm_execution_engine_t make_engine_run_init(llvm_instance_t& instance, llvm_ir_program_t& program);

//	Cast to uint64_t* or other the required type, then access via it.
void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name);

//	Cast to function pointer, then call it.
void* get_global_function(llvm_execution_engine_t& ee, const std::string& name);


std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name);
value_t call_function(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& f);

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name);
value_t load_global(llvm_execution_engine_t& ee, const std::pair<void*, typeid_t>& v);

value_t runtime_llvm_to_value(const llvm_execution_engine_t& runtime, const uint64_t encoded_value, const typeid_t& type);


//	Helper that goes directly from source to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const std::string& program_source, const std::string& file);

//	Compiles and runs the program.
int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args);


}	//	floyd

#endif /* floyd_llvm_codegen_hpp */
