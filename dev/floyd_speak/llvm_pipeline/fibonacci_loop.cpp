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


using namespace llvm;

/// Function declarations
static Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module, int targetFibNum);

/// Global variables
// static std::map<std::string, AllocaInst*> NamedValues;


int main_fibonacci_loop(int argc, char* argv[])
{
	/// Collect N, for Nth fibonacci number
	if ( argv[1] == nullptr )
	{
		perror("No argument entered for the Nth fibonacci number");
		return -1;
	}
	if ( argc > 2 )
	{
		perror("Only the first argument was used");
	}

	/// Convert and check
	int targetFibNum = atol(argv[1]); //Only needed for loop case. Haven't had time to check.
	if ( targetFibNum > 48 )
	{
		perror("Argument passed was too large");
		return -1;
	}
	
	/// LLVM IR Variables
	static LLVMContext context;
	static IRBuilder<> builder(context);
	std::unique_ptr<Module> mainModule( new Module("fibonacciModule", context) );
	Module *module = mainModule.get();
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();

	// "+1" Only needed for loop case. Still need to check why.
	Function *FibonacciFnc = InitFibonacciFnc(context, builder, module, targetFibNum+1);

	/// Create a JIT
	std::string collectedErrors;
	ExecutionEngine *exeEng = 
		EngineBuilder(std::move(mainModule))
		.setErrorStr(&collectedErrors)
		.setEngineKind(EngineKind::JIT)
		.create();

	/// Execution Engine
	if ( !exeEng )
	{
		std::string error = "Unable to construct execution engine: " + collectedErrors;
		perror(error.c_str());
		return -1;
	}

	std::vector<GenericValue> Args(0); // Empty vector as no args are passed
	GenericValue value = exeEng->runFunction(FibonacciFnc, Args);

	outs() << "\n" << *module;
	outs() << "\n-----------------------------------------\n";
	switch (targetFibNum % 10) {
		case(1): 
			if (targetFibNum == 11)
			{
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
				break;
			}
			outs() << targetFibNum << "st fibonacci number = " << value.IntVal << "\n";
			break;
		case(2): 
			if (targetFibNum == 12)
			{
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
				break;
			}
			outs() << targetFibNum << "nd fibonacci number = " << value.IntVal << "\n";
			break;
		case(3): 
			if (targetFibNum == 13)
			{
				outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
				break;
			}
			outs() << targetFibNum << "rd fibonacci number = " << value.IntVal << "\n";
			break;
		default: 
			outs() << targetFibNum << "th fibonacci number = " << value.IntVal << "\n";
			break;
	}
	outs() << "-----------------------------------------\n";


	return 0;
}

static Function* InitFibonacciFnc(LLVMContext &context, IRBuilder<> &builder, Module* module, int targetFibNum)
{
	Function *FibonacciFnc = 
		cast<Function>( module->getOrInsertFunction("FibonacciFnc", Type::getInt32Ty(context)) );

	Value* zero = ConstantInt::get(builder.getInt32Ty(), 0);
	Value* one = ConstantInt::get(builder.getInt32Ty(), 1);
	Value* two = ConstantInt::get(builder.getInt32Ty(), 2);
	Value* N = ConstantInt::get(builder.getInt32Ty(), targetFibNum);

	/// BBs and outline
	BasicBlock *EntryBB = BasicBlock::Create(context, "entry", FibonacciFnc);
	BasicBlock *LoopEntryBB = BasicBlock::Create(context, "loopEntry", FibonacciFnc);
	BasicBlock *LoopBB = BasicBlock::Create(context, "loop", FibonacciFnc);
		BasicBlock *IfBB = BasicBlock::Create(context, "if"); 			//floating
		BasicBlock *ThenBB = BasicBlock::Create(context, "ifTrue"); 	//floating
		BasicBlock *ElseBB = BasicBlock::Create(context, "else"); 		//floating
		BasicBlock *MergeBB = BasicBlock::Create(context, "merge"); 	//floating
	BasicBlock *ExitLoopBB = BasicBlock::Create(context, "exitLoop", FibonacciFnc);
	
	/// Variables
	Value *next, *first, *second, *count, *next1, *next2;

	/// EntryBB
	builder.SetInsertPoint(EntryBB);
	// NamedValues["next"] = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "next");
	// Allocate and store for mutable variables
	next = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "next");
	first = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "first");
	second = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "second");
	count = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "count");
	builder.CreateStore(zero, next);
	builder.CreateStore(zero, first);
	builder.CreateStore(one, second);
	builder.CreateStore(zero, count);

	// continue to loop entry
	builder.CreateBr(LoopEntryBB);
	

	/// LoopEntryBB
	builder.SetInsertPoint(LoopEntryBB);
	Value *countVal = builder.CreateLoad(count, "countVal");
	Value *ifCountLTN = builder.CreateICmpULT(countVal, N, "enterLoopCond");
	builder.CreateCondBr(ifCountLTN, LoopBB, ExitLoopBB);

	/// LoopBB
	builder.SetInsertPoint(LoopBB);
	builder.CreateBr(IfBB);

		/// IfBB
		// Nested statements are attached just before adding to the block, so that
		// their insertion point in LoopBB is certain.
		FibonacciFnc->getBasicBlockList().push_back(IfBB);
		builder.SetInsertPoint(IfBB);
		Value *ifCountLTTwo = builder.CreateICmpULT(countVal, two, "ifStmt");
		builder.CreateCondBr(ifCountLTTwo, ThenBB, ElseBB);

		/// ThenBB
		FibonacciFnc->getBasicBlockList().push_back(ThenBB);
		builder.SetInsertPoint(ThenBB);
		Value *nextVal = builder.CreateLoad(count, "nextVal");
		builder.CreateStore(nextVal, next);
		builder.CreateBr(MergeBB); // terminate ThenBB

		/// ElseBB
		FibonacciFnc->getBasicBlockList().push_back(ElseBB);
		builder.SetInsertPoint(ElseBB);
		
		Value *firstVal = builder.CreateLoad(first, "firstVal");
		Value *secondVal = builder.CreateLoad(second, "secondVal");
		nextVal = builder.CreateAdd(firstVal, secondVal, "nextVal");
		builder.CreateStore(nextVal, next);
		builder.CreateStore(secondVal, first);
		builder.CreateStore(nextVal, second);

		builder.CreateBr(MergeBB); // terminate ElseBB

		/// MergeBB
		FibonacciFnc->getBasicBlockList().push_back(MergeBB);
		builder.SetInsertPoint(MergeBB);
		countVal = builder.CreateAdd(countVal, one); //increment
		builder.CreateStore(countVal, count);
		builder.CreateBr(LoopEntryBB);

	/// ExitLoopBB
	builder.SetInsertPoint(ExitLoopBB);
	Value *finalFibNum = builder.CreateLoad(next, "finalNext");
	ReturnInst::Create(context, finalFibNum, ExitLoopBB);


	return FibonacciFnc;
}
