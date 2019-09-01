//
//  floyd_llvm_codegen_basics.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-31.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_codegen_basics_hpp
#define floyd_llvm_codegen_basics_hpp

#include "floyd_llvm_runtime.h"
#include "floyd_llvm_runtime_functions.h"
#include "floyd_llvm_helpers.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DataLayout.h>

#include "quark.h"

#include <map>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>


namespace floyd {




////////////////////////////////		resolved_symbol_t


struct resolved_symbol_t {
	llvm::Value* value_ptr;
	std::string debug_str;
	enum class esymtype { k_global, k_local, k_function_argument } symtype;

	std::string symbol_name;
	symbol_t symbol;
};






////////////////////////////////		llvm_code_generator_t


struct llvm_code_generator_t {
	public: llvm_code_generator_t(llvm_instance_t& instance, llvm::Module* module, const type_interner_t& interner, const llvm_type_lookup& type_lookup, const std::vector<function_def_t>& function_defs) :
		instance(&instance),
		module(module),
		builder(instance.context),
		type_lookup(type_lookup),
		function_defs(function_defs),

		runtime_functions(function_defs)
	{
		QUARK_ASSERT(instance.check_invariant());

		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();

		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
//		QUARK_ASSERT(scope_path.size() >= 1);
		QUARK_ASSERT(instance);
		QUARK_ASSERT(type_lookup.check_invariant());
		QUARK_ASSERT(instance->check_invariant());
		QUARK_ASSERT(module);
		return true;
	}

	llvm::IRBuilder<>& get_builder(){
		return builder;
	}


	////////////////////////////////		STATE

	llvm_instance_t* instance;
	llvm::Module* module;
	llvm::IRBuilder<> builder;
	llvm_type_lookup type_lookup;
	std::vector<function_def_t> function_defs;

	/*
		variable_address_t::_parent_steps
			-1: global, uncoditionally.
			0: current scope. scope_path[scope_path.size() - 1]
			1: parent scope. scope_path[scope_path.size() - 2]
			2: parent scope. scope_path[scope_path.size() - 3]
	*/
	//	One element for each global symbol in AST. Same indexes as in symbol table.
	std::vector<std::vector<resolved_symbol_t>> scope_path;

	const runtime_functions_t runtime_functions;
};





////////////////////////////////		llvm_function_generator_t


//	Used while generating a function, which requires more context than llvm_code_generator_t provides.
//	Notice: uses a REFERENCE to the llvm_code_generator_t so that must outlive the llvm_function_generator_t.

struct llvm_function_generator_t {
	public: llvm_function_generator_t(llvm_code_generator_t& gen, llvm::Function& emit_f) :
		gen(gen),
		emit_f(emit_f),
		floyd_runtime_ptr_reg(*floyd::get_callers_fcp(gen.type_lookup, emit_f))
	{
		QUARK_ASSERT(gen.check_invariant());
		QUARK_ASSERT(check_emitting_function(gen.type_lookup, emit_f));

		QUARK_ASSERT(check_invariant());
	}


	public: bool check_invariant() const {
		QUARK_ASSERT(gen.check_invariant());
		QUARK_ASSERT(check_emitting_function(gen.type_lookup, emit_f));
		return true;
	}

	//	??? rename to get_fcp().
	llvm::Value* get_callers_fcp(){
		return &floyd_runtime_ptr_reg;
	}

	llvm::IRBuilder<>& get_builder(){
		return gen.builder;
	}


	////////////////////////////////		STATE
	llvm_code_generator_t& gen;
	llvm::Function& emit_f;
	llvm::Value& floyd_runtime_ptr_reg;
};


llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const typeid_t& type);


llvm::Value* generate_cast_to_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& value, const typeid_t& floyd_type);
llvm::Value* generate_cast_from_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& runtime_value_reg, const typeid_t& type);


}	//	floyd

#endif /* floyd_llvm_codegen_basics_hpp */
