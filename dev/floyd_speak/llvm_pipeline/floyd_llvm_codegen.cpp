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

#include "quark.h"



static llvm::Function* InitFibonacciFnc(llvm::LLVMContext &context, llvm::IRBuilder<> &builder, llvm::Module* module, int targetFibNum){
	llvm::Function* f = llvm::cast<llvm::Function>(
		module->getOrInsertFunction("FibonacciFnc", llvm::Type::getInt32Ty(context))
	);

	llvm::Value* zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);
	llvm::Value* one = llvm::ConstantInt::get(builder.getInt32Ty(), 1);
	llvm::Value* two = llvm::ConstantInt::get(builder.getInt32Ty(), 2);
	llvm::Value* N = llvm::ConstantInt::get(builder.getInt32Ty(), targetFibNum);

	/// BBs and outline
	llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(context, "entry", f);
	llvm::BasicBlock* LoopEntryBB = llvm::BasicBlock::Create(context, "loopEntry", f);
	llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(context, "loop", f);
		llvm::BasicBlock* IfBB = llvm::BasicBlock::Create(context, "if"); 			//floating
		llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context, "ifTrue"); 	//floating
		llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(context, "else"); 		//floating
		llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context, "merge"); 	//floating
	llvm::BasicBlock* ExitLoopBB = llvm::BasicBlock::Create(context, "exitLoop", f);
	
	/// EntryBB
	builder.SetInsertPoint(EntryBB);

	// NamedValues["next"] = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "next");
	// Allocate and store for mutable variables
	llvm::Value* next = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "next");
	llvm::Value* first = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "first");
	llvm::Value* second = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "second");
	llvm::Value* count = builder.CreateAlloca(llvm::Type::getInt32Ty(context), nullptr, "count");
	builder.CreateStore(zero, next);
	builder.CreateStore(zero, first);
	builder.CreateStore(one, second);
	builder.CreateStore(zero, count);

	// continue to loop entry
	builder.CreateBr(LoopEntryBB);
	

	/// LoopEntryBB
	builder.SetInsertPoint(LoopEntryBB);
	llvm::Value* countVal = builder.CreateLoad(count, "countVal");
	llvm::Value* ifCountLTN = builder.CreateICmpULT(countVal, N, "enterLoopCond");
	builder.CreateCondBr(ifCountLTN, LoopBB, ExitLoopBB);

	/// LoopBB
	builder.SetInsertPoint(LoopBB);
	builder.CreateBr(IfBB);

		/// IfBB
		// Nested statements are attached just before adding to the block, so that
		// their insertion point in LoopBB is certain.
		f->getBasicBlockList().push_back(IfBB);
		builder.SetInsertPoint(IfBB);
		llvm::Value* ifCountLTTwo = builder.CreateICmpULT(countVal, two, "ifStmt");
		builder.CreateCondBr(ifCountLTTwo, ThenBB, ElseBB);

		/// ThenBB
		f->getBasicBlockList().push_back(ThenBB);
		builder.SetInsertPoint(ThenBB);
		llvm::Value* nextVal = builder.CreateLoad(count, "nextVal");
		builder.CreateStore(nextVal, next);
		builder.CreateBr(MergeBB); // terminate ThenBB

		/// ElseBB
		f->getBasicBlockList().push_back(ElseBB);
		builder.SetInsertPoint(ElseBB);
	
		llvm::Value* firstVal = builder.CreateLoad(first, "firstVal");
		llvm::Value* secondVal = builder.CreateLoad(second, "secondVal");
		nextVal = builder.CreateAdd(firstVal, secondVal, "nextVal");
		builder.CreateStore(nextVal, next);
		builder.CreateStore(secondVal, first);
		builder.CreateStore(nextVal, second);

		builder.CreateBr(MergeBB); // terminate ElseBB

		/// MergeBB
		f->getBasicBlockList().push_back(MergeBB);
		builder.SetInsertPoint(MergeBB);
		countVal = builder.CreateAdd(countVal, one); //increment
		builder.CreateStore(countVal, count);
		builder.CreateBr(LoopEntryBB);

	/// ExitLoopBB
	builder.SetInsertPoint(ExitLoopBB);
	llvm::Value* finalFibNum = builder.CreateLoad(next, "finalNext");
	llvm::ReturnInst::Create(context, finalFibNum, ExitLoopBB);

	return f;
}

int run_using_llvm(const std::string& program, const std::string& file, const std::vector<floyd::value_t>& args){
	int targetFibNum = 20;

	/// LLVM IR Variables
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder(context);
	std::unique_ptr<llvm::Module> mainModule( new llvm::Module("fibonacciModule", context) );
	llvm::Module* module = mainModule.get();
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	// "+1" Only needed for loop case. Still need to check why.
	llvm::Function* FibonacciFnc = InitFibonacciFnc(context, builder, module, targetFibNum + 1);

	/// Create a JIT
	std::string collectedErrors;
	llvm::ExecutionEngine* exeEng =
		llvm::EngineBuilder(std::move(mainModule))
		.setErrorStr(&collectedErrors)
		.setEngineKind(llvm::EngineKind::JIT)
		.create();

	/// Execution Engine
	if ( !exeEng )
	{
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		return -1;
	}

	std::vector<llvm::GenericValue> Args(0); // Empty vector as no args are passed
	llvm::GenericValue value = exeEng->runFunction(FibonacciFnc, Args);

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


	llvm::outs() << "Module: " << *module << " Fibonacci #" << targetFibNum << " = " << value.IntVal;
	return 0;
}

QUARK_UNIT_TEST_VIP("", "run_using_llvm()", "", ""){
	run_using_llvm("", "", {});
}
