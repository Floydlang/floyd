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
#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace floyd {
	struct semantic_ast_t;
}
namespace llvm {
	struct Module;
	struct ExecutionEngine;
}
namespace floyd {

struct llvm_ir_program_t {
	llvm_ir_program_t(const llvm_ir_program_t& other) = delete;
	llvm_ir_program_t& operator=(const llvm_ir_program_t& other) = delete;

	explicit llvm_ir_program_t(const std::string& module_name) :
		context(),
		module(std::make_unique<llvm::Module>(module_name.c_str(), context))
	{
	}

	public: bool check_invariant() const {
		return true;
	}

	llvm::LLVMContext context;

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	std::unique_ptr<llvm::Module> module;
};



//	Converts the semantic AST to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> generate_llvm_ir(const semantic_ast_t& ast, const std::string& module_name);


//	Runs the LLVM IR program.
int64_t run_llvm_program(llvm_ir_program_t& program, const std::vector<floyd::value_t>& args);


//??? Temp - should be:
//func int main([string] args){...}
typedef int64_t (*FLOYD_RUNTIME_MAIN)();

struct llvm_execution_engine_t {
	std::shared_ptr<llvm::ExecutionEngine> ee;
};

llvm_execution_engine_t make_engine_break_program_no_init(llvm_ir_program_t& program);
llvm_execution_engine_t make_engine_break_program(llvm_ir_program_t& program);

//	Cast to uint64_t* or other the required type, then access via it.
void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name);

//	Cast to function pointer, then call it.
void* get_global_function(llvm_execution_engine_t& ee, const std::string& name);




//	Helper that goes directly from source to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(const std::string& program_source, const std::string& file);

//	Compiles and runs the program.
int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args);


}	//	floyd

#endif /* floyd_llvm_codegen_hpp */
