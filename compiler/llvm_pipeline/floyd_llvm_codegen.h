//
//  floyd_llvm_codegen.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_codegen_hpp
#define floyd_llvm_codegen_hpp

#include "ast_value.h"
#include "statement.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_types.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
//#include "llvm/Target/TargetMachine.h"

#include <string>

namespace floyd {
	struct semantic_ast_t;
	struct llvm_function_link_entry_t;
	struct llvm_execution_engine_t;
}
namespace llvm {
	struct Module;
}
namespace floyd {



////////////////////////////////		llvm_ir_program_t


struct llvm_ir_program_t {
	llvm_ir_program_t(const llvm_ir_program_t& other) = delete;
	llvm_ir_program_t& operator=(const llvm_ir_program_t& other) = delete;

	explicit llvm_ir_program_t(
		llvm_instance_t* instance,
		std::unique_ptr<llvm::Module>& module2_swap,
		const llvm_type_lookup& type_lookup,
		const symbol_table_t& globals,
		const std::vector<llvm_function_link_entry_t>& function_link_map,
		const compiler_settings_t& settings
	);

	public: bool check_invariant() const;


	////////////////////////////////		STATE

	llvm_instance_t* instance;

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	std::unique_ptr<llvm::Module> module;

	llvm_type_lookup type_lookup;
	symbol_table_t debug_globals;
	std::vector<llvm_function_link_entry_t> function_link_map;

	container_t container_def;
	software_system_t software_system;
	compiler_settings_t settings;
};


//	Converts the semantic AST to LLVM IR code.
std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast, const std::string& module_name, const compiler_settings_t& settings);

std::string print_llvm_ir_program(const llvm_ir_program_t& program);

std::vector<uint8_t> write_object_file(llvm_ir_program_t& program, const target_t& target);
std::string write_ir_file(llvm_ir_program_t& program, const target_t& target);



}	//	floyd

#endif /* floyd_llvm_codegen_hpp */
