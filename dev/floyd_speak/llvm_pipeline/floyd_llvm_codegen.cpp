//
//  floyd_llvm_codegen.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen.h"
#include "ast_value.h"

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
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

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "host_functions.h"
#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "pass3.h"

#include "quark.h"

//http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html


/*
# ACCESSING INTEGER INSIDE GENERICVALUE

//const int x = value.IntVal.U.VAL;
//	const int64_t x = llvm::cast<llvm::ConstantInt>(value);
//	QUARK_TRACE_SS("Fib = " << x);

#if 0
if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt*>(value)) {
  if (CI->getBitWidth() <= 32) {
    const auto constIntValue = CI->getSExtValue();
    QUARK_TRACE_SS("Fib: " << constIntValue);
  }
}
#endif
//	llvm::CreateGenericValueOfInt(value);
//	int value2 = value.as_float;
*/

namespace floyd {

/*
http://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/
With LLVM, we also have some things to be careful about. The first is the LLVM context we created before needs to stay alive as long as we use anything from this compilation unit. This is important because everything that is generated with the JIT will have to stay alive after this function and registers itself in the context until it is explicitly deleted.

*/

struct llvmgen_t {
	public: llvmgen_t(llvm_ir_program_t& program_acc0, llvm::IRBuilder<>& builder0) :
		program_acc(program_acc0),
		builder(builder0)
	{
	}
	public: bool check_invariant() const {
		QUARK_ASSERT(program_acc.check_invariant());
		return true;
	}


	llvm_ir_program_t& program_acc;
	llvm::IRBuilder<>& builder;

	//	One element for each global symbol in AST. Same indexes as in symbol table.
	std::vector<llvm::Value*> globals;
};


llvm::Value* genllvm_expression(llvmgen_t& gen_acc, const expression_t& e);


std::unique_ptr<llvm_ir_program_t> make_empty_program(const std::string& module_name){
	return std::make_unique<llvm_ir_program_t>(module_name);
}




std::string print_program(const llvm_ir_program_t& program){
	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	program.module->print(stream2, nullptr);
	return dump;
}


static const std::string k_expected_ir_fibonacci_text = R"ABC(
; ModuleID = 'fibonacciModule'
source_filename = "fibonacciModule"

define i32 @FibonacciFnc() {
entry:
  %next = alloca i32
  %first = alloca i32
  %second = alloca i32
  %count = alloca i32
  store i32 0, i32* %next
  store i32 0, i32* %first
  store i32 1, i32* %second
  store i32 0, i32* %count
  br label %loopEntry

loopEntry:                                        ; preds = %merge, %entry
  %countVal = load i32, i32* %count
  %enterLoopCond = icmp ult i32 %countVal, 21
  br i1 %enterLoopCond, label %loop, label %exitLoop

loop:                                             ; preds = %loopEntry
  br label %if

exitLoop:                                         ; preds = %loopEntry
  %finalNext = load i32, i32* %next
  ret i32 %finalNext

if:                                               ; preds = %loop
  %ifStmt = icmp ult i32 %countVal, 2
  br i1 %ifStmt, label %ifTrue, label %else

ifTrue:                                           ; preds = %if
  %nextVal = load i32, i32* %count
  store i32 %nextVal, i32* %next
  br label %merge

else:                                             ; preds = %if
  %firstVal = load i32, i32* %first
  %secondVal = load i32, i32* %second
  %nextVal1 = add i32 %firstVal, %secondVal
  store i32 %nextVal1, i32* %next
  store i32 %secondVal, i32* %first
  store i32 %nextVal1, i32* %second
  br label %merge

merge:                                            ; preds = %else, %ifTrue
  %0 = add i32 %countVal, 1
  store i32 %0, i32* %count
  br label %loopEntry
}
)ABC";



static llvm::Function* InitFibonacciFnc(llvm_ir_program_t& program, llvm::IRBuilder<>& builder, int targetFibNum){
	llvm::Function* f = llvm::cast<llvm::Function>(
		program.module->getOrInsertFunction("FibonacciFnc", llvm::Type::getInt32Ty(program.context))
	);

	llvm::Value* zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);
	llvm::Value* one = llvm::ConstantInt::get(builder.getInt32Ty(), 1);
	llvm::Value* two = llvm::ConstantInt::get(builder.getInt32Ty(), 2);
	llvm::Value* N = llvm::ConstantInt::get(builder.getInt32Ty(), targetFibNum);


	////////////////////////		Create all basic blocks first, so we can branch between them when we start emitting instructions

	llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(program.context, "entry", f);
	llvm::BasicBlock* LoopEntryBB = llvm::BasicBlock::Create(program.context, "loopEntry", f);

	llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(program.context, "loop", f);
		llvm::BasicBlock* IfBB = llvm::BasicBlock::Create(program.context, "if"); 			//floating
		llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(program.context, "ifTrue"); 	//floating
		llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(program.context, "else"); 		//floating
		llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(program.context, "merge"); 	//floating
	llvm::BasicBlock* ExitLoopBB = llvm::BasicBlock::Create(program.context, "exitLoop", f);


	////////////////////////		EntryBB

	builder.SetInsertPoint(EntryBB);
	llvm::Value* next = builder.CreateAlloca(llvm::Type::getInt32Ty(program.context), nullptr, "next");
	llvm::Value* first = builder.CreateAlloca(llvm::Type::getInt32Ty(program.context), nullptr, "first");
	llvm::Value* second = builder.CreateAlloca(llvm::Type::getInt32Ty(program.context), nullptr, "second");
	llvm::Value* count = builder.CreateAlloca(llvm::Type::getInt32Ty(program.context), nullptr, "count");
	builder.CreateStore(zero, next);
	builder.CreateStore(zero, first);
	builder.CreateStore(one, second);
	builder.CreateStore(zero, count);

	// continue to loop entry
	builder.CreateBr(LoopEntryBB);


	////////////////////////		LoopEntryBB

	builder.SetInsertPoint(LoopEntryBB);
	llvm::Value* countVal = builder.CreateLoad(count, "countVal");
	llvm::Value* ifCountLTN = builder.CreateICmpULT(countVal, N, "enterLoopCond");
	builder.CreateCondBr(ifCountLTN, LoopBB, ExitLoopBB);


	////////////////////////		LoopBB

	builder.SetInsertPoint(LoopBB);
	builder.CreateBr(IfBB);


		////////////////////////		IfBB

		// Nested statements are attached just before adding to the block, so that
		// their insertion point in LoopBB is certain.
		f->getBasicBlockList().push_back(IfBB);
		builder.SetInsertPoint(IfBB);
		llvm::Value* ifCountLTTwo = builder.CreateICmpULT(countVal, two, "ifStmt");
		builder.CreateCondBr(ifCountLTTwo, ThenBB, ElseBB);

		////////////////////////		ThenBB

		f->getBasicBlockList().push_back(ThenBB);
		builder.SetInsertPoint(ThenBB);
		llvm::Value* nextVal = builder.CreateLoad(count, "nextVal");
		builder.CreateStore(nextVal, next);

		// terminate ThenBB
		builder.CreateBr(MergeBB);


		////////////////////////		ElseBB

		f->getBasicBlockList().push_back(ElseBB);
		builder.SetInsertPoint(ElseBB);
	
		llvm::Value* firstVal = builder.CreateLoad(first, "firstVal");
		llvm::Value* secondVal = builder.CreateLoad(second, "secondVal");
		nextVal = builder.CreateAdd(firstVal, secondVal, "nextVal");
		builder.CreateStore(nextVal, next);
		builder.CreateStore(secondVal, first);
		builder.CreateStore(nextVal, second);

		// terminate ElseBB
		builder.CreateBr(MergeBB);


		////////////////////////		MergeBB

		f->getBasicBlockList().push_back(MergeBB);
		builder.SetInsertPoint(MergeBB);
		countVal = builder.CreateAdd(countVal, one); //increment
		builder.CreateStore(countVal, count);
		builder.CreateBr(LoopEntryBB);


	////////////////////////		ExitLoopBB

	builder.SetInsertPoint(ExitLoopBB);
	llvm::Value* finalFibNum = builder.CreateLoad(next, "finalNext");
	llvm::ReturnInst::Create(program.context, finalFibNum, ExitLoopBB);

	QUARK_TRACE_SS("result = " << floyd::print_program(program));

	return f;
}



llvm::Type* intern_type(llvmgen_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_void()){
		return llvm::Type::getVoidTy(gen_acc.program_acc.context);
	}
	else if(type.is_int()){
		return llvm::Type::getInt64Ty(gen_acc.program_acc.context);
	}
	else if(type.is_bool()){
		return llvm::Type::getInt1Ty(gen_acc.program_acc.context);
	}

	else if(type.is_string()){
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 10);
	}
	else if(type.is_json_value()){
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 14);
	}
	else if(type.is_vector()){
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 11);
	}
	else if(type.is_typeid()){
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 15);
	}
	else if(type.is_undefined()){
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 17);
	}
	else if(type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 18);
	}
	else if(type.is_double()){
		return llvm::Type::getDoubleTy(gen_acc.program_acc.context);
	}
	else if(type.is_struct()){
		std::vector<llvm::Type*> members;
		for(const auto& m: type.get_struct_ref()->_members){
			const auto m2 = intern_type(gen_acc, m._type);
			members.push_back(m2);
		}
//ArrayRef<Type*> Elements

  		llvm::StructType* s = llvm::StructType::get(gen_acc.program_acc.context, members, false);

//		return llvm::StructType::get(gen_acc.program_acc.context);
//		return llvm::Type::getInt32Ty(gen_acc.program_acc.context);
		return s;
	}

	else if(type.is_internal_dynamic()){
		//	Use int16ptr as placeholder for **dyn**. Maybe pass a struct instead?
		return llvm::Type::getIntNTy(gen_acc.program_acc.context, 13);
	}
	else if(type.is_function()){
		const auto& return_type = type.get_function_return();
		const auto args = type.get_function_args();

		const auto return_type2 = intern_type(gen_acc, return_type);

		// Make the function type:  double(double,double) etc.
		std::vector<llvm::Type*> args2;

		for(const auto& arg: args){
			auto arg_type = intern_type(gen_acc, arg);
			args2.push_back(arg_type);
		}

		llvm::FunctionType* function_type = llvm::FunctionType::get(return_type2, args2, false);

/*
		llvm::Function* F = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "dummy_function_name???", gen_acc.program_acc.module.get());

		// Set names for all arguments.
		unsigned Idx = 0;
		for (auto &Arg : F->args()){
			Arg.setName(Args[Idx++]);
		}
*/
		return function_type;
	}
	else{
		QUARK_TRACE_SS(floyd::typeid_to_compact_string(type));
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

llvm::Value* add_local_temp(llvmgen_t& gen_acc, const typeid_t& type, const std::string& name){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	llvm::Value* dest = gen_acc.builder.CreateAlloca(intern_type(gen_acc, type), nullptr, name);
	return dest;
}

variable_address_t add_local_const(llvmgen_t& gen_acc, const value_t& value, const std::string& name){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(name.empty() == false);

/*
	int id = add_constant_literal(body_acc._symbol_table, name, value);
	return variable_address_t::make_variable_address(0, id);
*/
	return {};
}


llvm::Function* make_function_stub(llvmgen_t& gen_acc, const std::string& function_name, const floyd::typeid_t& function_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_type.check_invariant());

	floyd::typeid_t hacked_function_type = floyd::typeid_t::make_function(function_type.get_function_return(), { floyd::typeid_t::make_int() }, floyd::epure::impure);

	llvm::Type* ftype = intern_type(gen_acc, hacked_function_type);
	llvm::Function* f = llvm::cast<llvm::Function>(
		gen_acc.program_acc.module->getOrInsertFunction(function_name, (llvm::FunctionType*)ftype)
	);
	llvm::verifyFunction(*f);
	return f;
}

std::string make_unique_internal_function_name(const value_t& value){
	QUARK_ASSERT(value.check_invariant());

	const auto function_id = value.get_function_value();
	const auto s = std::string() + "floyd_internal_" + std::to_string(function_id);
	return s;
}


llvm::Value* make_constant(llvmgen_t& gen_acc, const value_t& value){
	const auto type = value.get_type();
	const auto itype = intern_type(gen_acc, type);

	if(type.is_undefined()){
		return llvm::ConstantInt::get(itype, 17);
	}
	else if(type.is_internal_dynamic()){
		return llvm::ConstantInt::get(itype, 13);
	}
	else if(type.is_void()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_bool()){
		return llvm::ConstantInt::get(itype, value.get_int_value());
	}
	else if(type.is_int()){
		return llvm::ConstantInt::get(itype, value.get_int_value());
	}
	else if(type.is_double()){
		return llvm::ConstantFP::get(itype, value.get_double_value());
	}
	else if(type.is_string()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_json_value()){
		return llvm::ConstantInt::get(itype, 14);
	}
	else if(type.is_typeid()){
		return llvm::ConstantInt::get(itype, 15);
	}
	else if(type.is_struct()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_protocol()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_vector()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_dict()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else if(type.is_function()){
		const auto unique_function_str = make_unique_internal_function_name(value);
		return make_function_stub(gen_acc, unique_function_str, type);
	}
	else if(type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		return nullptr;
	}
	else{
		QUARK_ASSERT(false);
		return nullptr;
	}
}


llvm::Value* genllvm_literal_expression(llvmgen_t& gen_acc, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());

	const auto literal = e.get_literal();
	return make_constant(gen_acc, literal);
}

llvm::Value* genllvm_arithmetic_expression(llvmgen_t& gen_acc, expression_type op, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto type = e._input_exprs[0].get_output_type();

	auto lhs_temp = genllvm_expression(gen_acc, e._input_exprs[0]);
	auto rhs_temp = genllvm_expression(gen_acc, e._input_exprs[1]);

/*
	if(type.is_bool()){
		static const std::map<expression_type, bc_opcode> conv_opcode = {
			{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_bool },
			{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

			{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_bool },
			{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_bool }
		};
		return conv_opcode.at(e._operation);
	}
	else*/ if(type.is_int()){
		if(e._operation == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateAdd(lhs_temp, rhs_temp, "addtmp");
		}
		else{
			QUARK_ASSERT(false);
		}
/*
		static const std::map<expression_type, bc_opcode> conv_opcode = {
			{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_int },
			{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_subtract_int },
			{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_multiply_int },
			{ expression_type::k_arithmetic_divide__2, bc_opcode::k_divide_int },
			{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_remainder_int },

			{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_int },
			{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_int }
		};
		return conv_opcode.at(e._operation);
*/

	}
/*		else if(type.is_double()){
		static const std::map<expression_type, bc_opcode> conv_opcode = {
			{ expression_type::k_arithmetic_add__2, bc_opcode::k_add_double },
			{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_subtract_double },
			{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_multiply_double },
			{ expression_type::k_arithmetic_divide__2, bc_opcode::k_divide_double },
			{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

			{ expression_type::k_logical_and__2, bc_opcode::k_logical_and_double },
			{ expression_type::k_logical_or__2, bc_opcode::k_logical_or_double }
		};
		return conv_opcode.at(e._operation);
	}
	else if(type.is_string()){
		static const std::map<expression_type, bc_opcode> conv_opcode = {
			{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_strings },
			{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
			{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

			{ expression_type::k_logical_and__2, bc_opcode::k_nop },
			{ expression_type::k_logical_or__2, bc_opcode::k_nop }
		};
		return conv_opcode.at(e._operation);
	}
	else if(type.is_vector()){
		if(encode_as_vector_w_inplace_elements(type)){
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_vectors_w_inplace_elements },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_nop },
				{ expression_type::k_logical_or__2, bc_opcode::k_nop }
			};
			return conv_opcode.at(e._operation);
		}
		else{
			static const std::map<expression_type, bc_opcode> conv_opcode = {
				{ expression_type::k_arithmetic_add__2, bc_opcode::k_concat_vectors_w_external_elements },
				{ expression_type::k_arithmetic_subtract__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_multiply__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_divide__2, bc_opcode::k_nop },
				{ expression_type::k_arithmetic_remainder__2, bc_opcode::k_nop },

				{ expression_type::k_logical_and__2, bc_opcode::k_nop },
				{ expression_type::k_logical_or__2, bc_opcode::k_nop }
			};
			return conv_opcode.at(e._operation);
		}
	}
*/		else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}

	return nullptr;
}


llvm::Value* genllvm_expression(llvmgen_t& gen_acc, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();
	if(op == expression_type::k_literal){
		return genllvm_literal_expression(gen_acc, e);
	}
/*
	else if(op == expression_type::k_resolve_member){
		return bcgen_resolve_member_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_lookup_element){
		return bcgen_lookup_element_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_load2){
		return bcgen_load2_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_call){
		return bcgen_call_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_value_constructor){
		return bcgen_construct_value_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return bcgen_arithmetic_unary_minus_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_conditional_operator3){
		return bcgen_conditional_operator_expression(gen_acc, target_reg, e, body);
	}
*/
	else if (is_arithmetic_expression(op)){
		return genllvm_arithmetic_expression(gen_acc, op, e);
	}
/*
	else if (is_comparison_expression(op)){
		return bcgen_comparison_expression(gen_acc, target_reg, op, e, body);
	}
*/
	else{
		QUARK_ASSERT(false);
	}
	quark::throw_exception();
}


llvm::Value* find_value_slot(llvmgen_t& gen_acc, const std::vector<llvm::Value*>& local_symbols, const variable_address_t& reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(reg._parent_steps == -1 || reg._parent_steps == 0);

	if(reg._parent_steps == -1){
		QUARK_ASSERT(reg._index >=0 && reg._index < gen_acc.globals.size());
		return gen_acc.globals[reg._index];
	}
	else if(reg._parent_steps == 0){
		QUARK_ASSERT(reg._index >=0 && reg._index < local_symbols.size());
		return local_symbols[reg._index];
	}
	else{
		QUARK_ASSERT(false);
	}
}

void genllvm_store2_statement(llvmgen_t& gen_acc, const statement_t::store2_t& s, const std::vector<llvm::Value*>& local_symbols){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = genllvm_expression(gen_acc, s._expression);

	llvm::Value* dest = find_value_slot(gen_acc, local_symbols, s._dest_variable);
	gen_acc.builder.CreateStore(value, dest);

/*
	//	Shortcut: if destination is a local variable, have the expression write directly to that register.
	if(statement._dest_variable._parent_steps != -1){
		const auto expr = bcgen_expression(gen_acc, statement._dest_variable, statement._expression, body_acc);
		body_acc = expr._body;
		QUARK_ASSERT(body_acc.check_invariant());
	}
	else{
		const auto expr = bcgen_expression(gen_acc, {}, statement._expression, body_acc);
		body_acc = expr._body;
		body_acc = copy_value(statement._expression.get_output_type(), statement._dest_variable, expr._out, body_acc);
		QUARK_ASSERT(body_acc.check_invariant());
	}
	return body_acc;
*/

	QUARK_ASSERT(gen_acc.check_invariant());
}


/*
	All Floyd's global statements becomes instructions in floyd_init()-function that is called by Floyd runtime before any other function is called.
*/

void genllvm_statements(llvmgen_t& gen_acc, const std::vector<statement_t>& statements, const std::vector<llvm::Value*>& local_symbols){
	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());

			struct visitor_t {
				llvmgen_t& acc0;
				const std::vector<llvm::Value*>& local_symbols0;

				void operator()(const statement_t::return_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_return_statement(_gen_acc, s, body_acc);
				}
				void operator()(const statement_t::define_struct_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				void operator()(const statement_t::define_protocol_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				void operator()(const statement_t::define_function_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}

				void operator()(const statement_t::bind_local_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				void operator()(const statement_t::store_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
				}
				void operator()(const statement_t::store2_t& s) const{
					genllvm_store2_statement(acc0, s, local_symbols0);
				}
				void operator()(const statement_t::block_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_block_statement(_gen_acc, s, body_acc);
				}

				void operator()(const statement_t::ifelse_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_ifelse_statement(_gen_acc, s, body_acc);
				}
				void operator()(const statement_t::for_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_for_statement(_gen_acc, s, body_acc);
				}
				void operator()(const statement_t::while_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_while_statement(_gen_acc, s, body_acc);
				}


				void operator()(const statement_t::expression_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return bcgen_expression_statement(_gen_acc, s, body_acc);
				}
				void operator()(const statement_t::software_system_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return body_acc;
				}
				void operator()(const statement_t::container_def_statement_t& s) const{
					QUARK_ASSERT(false);
					quark::throw_exception();
//					return body_acc;
				}
			};

			std::visit(visitor_t{ gen_acc, local_symbols }, statement._contents);
		}
	}
}

std::vector<llvm::Value*> genllvm_local_make_symbols(llvmgen_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	std::vector<llvm::Value*> result;

	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(gen_acc, type);

		llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

		if(e.second._const_value.is_void() == false){
			llvm::Value* c = make_constant(gen_acc, e.second._const_value);
			gen_acc.builder.CreateStore(c, dest);
		}
		else{
		}
		result.push_back(dest);
	}
	return result;
}

std::vector<llvm::Value*> genllvm_make_globals(llvmgen_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	std::vector<llvm::Value*> result;

	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(gen_acc, type);

		llvm::Value* init = nullptr;
		if(e.second._const_value.is_void() == false){
			llvm::Value* c = make_constant(gen_acc, e.second._const_value);
//				dest->setInitializer(constant_value);
			init = c;
		}

		llvm::Value* gv = new llvm::GlobalVariable(
			*gen_acc.program_acc.module,
			itype,
			false,
			llvm::GlobalValue::ExternalLinkage,
//			init,
			llvm::Constant::getNullValue(itype),
			e.first
		);

/*
		llvm::Value* gv = GlobalVariable(
			Module &M,
			Type *Ty,
			bool isConstant,
			LinkageTypes Linkage,
			Constant *Initializer,
			const Twine &Name = "",
			GlobalVariable *InsertBefore = nullptr,
			ThreadLocalMode = NotThreadLocal,
			unsigned AddressSpace = 0,
			bool isExternallyInitialized = false
		);
*/

		result.push_back(gv);
	}
	return result;
}



void genllvm_function_def(llvmgen_t& gen_acc, const floyd::function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	llvm::Function* f = make_function_stub(gen_acc, "???function name???", function_def._function_type);

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.program_acc.context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	auto symbol_table_values = genllvm_local_make_symbols(gen_acc, function_def._body->_symbol_table);
	genllvm_statements(gen_acc, function_def._body->_statements, symbol_table_values);

	llvm::verifyFunction(*f);
}

void genllvm_make_function_defs(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());


	for(int function_id = 0 ; function_id < semantic_ast._checked_ast._function_defs.size() ; function_id++){
		const auto& function_def = *semantic_ast._checked_ast._function_defs[function_id];

		if(function_def._host_function_id != k_no_host_function_id){
//			QUARK_ASSERT(false);
/*
			const auto function_def2 = bc_function_definition_t{
				function_def._function_type,
				function_def._args,
				nullptr,
				function_def._host_function_id
			};
*/

		}
		else{
//???			genllvm_function_def(gen_acc, function_def);
		}
	}
}

//	Generate floyd_runtime_init() that runs all global statements, before main() is run.
void genllvm_make_floyd_runtime_init(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast, const std::vector<llvm::Value*>& global_symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Function* f = make_function_stub(
		gen_acc,
		"floyd_runtime_init",
		floyd::typeid_t::make_function(floyd::typeid_t::make_int(), {}, floyd::epure::impure)
	);

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.program_acc.context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	genllvm_statements(gen_acc, semantic_ast._checked_ast._globals._statements, global_symbol_table);

	llvm::Value* dummy_result = llvm::ConstantInt::get(gen_acc.builder.getInt64Ty(), 667);
	llvm::ReturnInst::Create(gen_acc.program_acc.context, dummy_result, entryBB);

	llvm::verifyFunction(*f);
}

void genllvm_all(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	Global variables and global statements.
	{
		QUARK_ASSERT(gen_acc.check_invariant());
		QUARK_ASSERT(semantic_ast.check_invariant());

		std::vector<llvm::Value*> globals = genllvm_make_globals(gen_acc, semantic_ast._checked_ast._globals._symbol_table);
		gen_acc.globals = globals;
	}

	genllvm_make_function_defs(gen_acc, semantic_ast);
	genllvm_make_floyd_runtime_init(gen_acc, semantic_ast, gen_acc.globals);
}




std::unique_ptr<llvm_ir_program_t> generate_llvm_ir(const semantic_ast_t& ast, const std::string& module_name){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(ast_to_json(ast._checked_ast)._value));

	auto p = make_empty_program(module_name);

	llvm::IRBuilder<> builder(p->context);
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	llvmgen_t acc(*p, builder);
	genllvm_all(acc, ast);

	QUARK_TRACE_SS("result = " << floyd::print_program(*p));

	return p;
}

void* get_global_ptr(llvm::ExecutionEngine& exeEng, const std::string& name){
	const auto addr = exeEng.getGlobalValueAddress(name);
	return  (void*)addr;
}

uint64_t* get_global_uint64_t(llvm::ExecutionEngine& exeEng, const std::string& name){
	return (uint64_t*)get_global_ptr(exeEng, name);
}


void* get_global_function(llvm::ExecutionEngine& exeEng, const std::string& name){
	const auto addr = exeEng.getFunctionAddress(name);
	return (void*)addr;
}



struct llvm_execution_engine_t {
	std::shared_ptr<llvm::ExecutionEngine> ee;
};

typedef int64_t (*FLOYD_RUNTIME_INIT)();


//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
llvm_execution_engine_t make_engine_break_program(llvm_ir_program_t& program){
	QUARK_ASSERT(program.module);

	llvm::Function* init_func = program.module->getFunction("floyd_runtime_init");
	QUARK_ASSERT(init_func != nullptr);

	init_func->print(llvm::errs());

	if(false){
		const auto& functionList = program.module->getFunctionList();
		for(const auto& e: functionList){

	/*
		std::string dump;
		llvm::raw_string_ostream stream2(dump);
		program.module->print(stream2, nullptr);
		return dump;
	*/
			e.print(llvm::errs());
		}

		const auto& globalList = program.module->getGlobalList();
		for(const auto& e: globalList){
			e.print(llvm::errs());
		}
	}

	std::string collectedErrors;

	//??? Destroys p -- uses std::move().
	llvm::ExecutionEngine* exeEng = llvm::EngineBuilder(std::move(program.module))
		.setErrorStr(&collectedErrors)
		.setOptLevel(llvm::CodeGenOpt::Level::None)
		.setVerifyModules(true)
		.setEngineKind(llvm::EngineKind::JIT)
		.create();

	if (exeEng == nullptr){
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		throw std::exception();
	}

	auto ee = std::shared_ptr<llvm::ExecutionEngine>(exeEng);

	ee->finalizeObject();


	const auto result0_ptr = get_global_uint64_t(*ee, "result");
	const auto result0 = result0_ptr ? *result0_ptr : -1;


	//	Call floyd_runtime_init().
	{
		/*
		  /// For MCJIT execution engines, clients are encouraged to use the
		  /// "GetFunctionAddress" method (rather than runFunction) and cast the
		  /// returned uint64_t to the desired function pointer type. However, for
		  /// backwards compatibility MCJIT's implementation can execute 'main-like'
		  /// function (i.e. those returning void or int, and taking either no
		  /// arguments or (int, char*[])).
		*/
		/*
		LLVM’s “eager” JIT compiler is safe to use in threaded programs. Multiple threads can call ExecutionEngine::getPointerToFunction() or ExecutionEngine::runFunction() concurrently, and multiple threads can run code output by the JIT concurrently. The user must still ensure that only one thread accesses IR in a given LLVMContext while another thread might be modifying it. One way to do that is to always hold the JIT lock while accessing IR outside the JIT (the JIT modifies the IR by adding CallbackVHs). Another way is to only call getPointerToFunction() from the LLVMContext’s thread.
		*/

		llvm::GenericValue init_result = ee->runFunction(init_func, {});
		const int64_t init_result_int = init_result.IntVal.getSExtValue();
		QUARK_ASSERT(init_result_int == 667);


		auto a_addr = ee->getFunctionAddress("floyd_runtime_init");
		auto a_func = (FLOYD_RUNTIME_INIT)a_addr;
		int64_t a_result = (*a_func)();
		QUARK_ASSERT(a_result == 667);


		llvm::Function* b_func = ee->FindFunctionNamed("floyd_runtime_init");
		llvm::GenericValue b_result = ee->runFunction(b_func, {});
		const int64_t b_result_int = b_result.IntVal.getSExtValue();
		QUARK_ASSERT(b_result_int == 667);



//		init_result.print(llvm::errs());
	}

	const auto result1_ptr = get_global_uint64_t(*ee, "result");
	const auto result1 = result1_ptr ? *result1_ptr : -1;

	return { ee };
}


//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_ir_program_t& program, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(program.module);

	auto ee = make_engine_break_program(program);

	const auto result = *get_global_uint64_t(*ee.ee, "result");
	QUARK_TRACE_SS("a_result() = " << result);

//	const int64_t x = return_value.IntVal.getSExtValue();
//	QUARK_TRACE_SS("init() = " << x);
	return 0;
}




/*
	auto gv = program.module->getGlobalVariable("result");
	const auto p3 = exeEng->getPointerToGlobal(gv);

	const auto result = *(uint64_t*)p3;

	const auto p = exeEng->getPointerToGlobalIfAvailable("result");
	llvm::GlobalVariable* p2 = exeEng->FindGlobalVariableNamed("result", true);
*/


////////////////////////////////		HELPERS



std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(const std::string& program, const std::string& file){
	const auto pass3 = compile_to_sematic_ast__errors(program, file, compilation_unit_mode::k_no_core_lib);
	auto bc = generate_llvm_ir(pass3, file);
	return bc;
}


int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args){
	const auto pass3 = compile_to_sematic_ast__errors(program_source, file, compilation_unit_mode::k_no_core_lib);
	auto program = generate_llvm_ir(pass3, file);
	const auto result = run_llvm_program(*program, args);
	QUARK_TRACE_SS("Fib = " << result);
	return result;
}



}	//	namespace floyd




////////////////////////////////		TESTS



#if 0
QUARK_UNIT_TEST_VIP("", "run_using_llvm()", "", ""){
	const auto r = floyd::run_using_llvm_helper("", "", {});
	QUARK_UT_VERIFY(r == 6765);
}
#endif


QUARK_UNIT_TEST_VIP("Floyd test suite", "+", "", ""){
//	ut_verify_global_result(QUARK_POS, "let int result = 1 + 2 + 3", value_t::make_int(6));

	const auto pass3 = compile_to_sematic_ast__errors("let int result = 1 + 2 + 3", "myfile.floyd", floyd::compilation_unit_mode::k_no_core_lib);
	auto program = generate_llvm_ir(pass3, "myfile.floyd");
	auto ee = make_engine_break_program(*program);
	const auto result = *floyd::get_global_uint64_t(*ee.ee, "result");
	QUARK_ASSERT(result == 6);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}


