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



std::unique_ptr<llvm_ir_program_t> generate_llvm_ir(const semantic_ast_t& ast, const std::string& module_name){
	QUARK_ASSERT(ast.check_invariant());
//	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(ast_to_json(ast._checked_ast)._value));

	int targetFibNum = 20;

	auto p = make_empty_program(module_name);

	llvm::IRBuilder<> builder(p->context);
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	// "+1" Only needed for loop case. Still need to check why.
	llvm::Function* FibonacciFnc = InitFibonacciFnc(*p, builder, targetFibNum + 1);
	QUARK_ASSERT(FibonacciFnc != nullptr);

	QUARK_TRACE_SS("result = " << floyd::print_program(*p));

	return p;
}

//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_ir_program_t& program, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(program.module);

	llvm::Function* fib = program.module->getFunction("FibonacciFnc");
	QUARK_ASSERT(fib != nullptr);

	std::string collectedErrors;

	/// Execution Engine
	//??? Destroys p -- uses std::move().
	llvm::ExecutionEngine* exeEng = llvm::EngineBuilder(std::move(program.module))
		.setErrorStr(&collectedErrors)
		.setEngineKind(llvm::EngineKind::JIT)
		.create();

	if (exeEng == nullptr){
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		throw std::exception();
	}



	std::vector<llvm::GenericValue> args2(0);
	llvm::GenericValue value = exeEng->runFunction(fib, args2);


	const int64_t x = value.IntVal.getSExtValue();
	QUARK_TRACE_SS("Fib = " << x);
	return x;
}


////////////////////////////////		HELPERS



std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(const std::string& program, const std::string& file){
	const auto pass3 = compile_to_sematic_ast__errors(program, file);
	auto bc = generate_llvm_ir(pass3, file);
	return bc;
}


int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args){
	const auto pass3 = compile_to_sematic_ast__errors(program_source, file);
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

	auto program = floyd::compile_to_ir_helper( "let int result = 1 + 2 + 3", "myfile.floyd");

	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}


