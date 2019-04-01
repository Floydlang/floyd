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


struct global_v_t {
	llvm::Value* value_ptr;
	std::string debug_str;
};

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
	std::vector<global_v_t> globals;
};


llvm::Value* genllvm_expression(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const expression_t& e);


std::unique_ptr<llvm_ir_program_t> make_empty_program(const std::string& module_name){
	return std::make_unique<llvm_ir_program_t>(module_name);
}

std::string print_program(const llvm_ir_program_t& program){
	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	program.module->print(stream2, nullptr);
	return dump;
}

std::string print_type(llvm::Type* type){
	if(type == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		type->print(rso);
//		std::cout<<rso.str();
		return s;
	}
}

std::string print_value0(llvm::Value* value){
	if(value == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		value->print(rso);
//		std::cout<<rso.str();
		return s;
	}
}
std::string print_value(llvm::Value* value){
	if(value == nullptr){
		return "nullptr";
	}
	else{
		const std::string type_str = print_type(value->getType());
		const auto val_str = print_value0(value);
		return "[" + type_str + ":" + val_str + "]";
	}
}

std::string print_globals(const std::vector<global_v_t>& globals){
	std::stringstream out;

	out << "{" << std::endl;
	for(const auto& e: globals){
		out << "{ " << print_value(e.value_ptr) << " : " << e.debug_str << " }" << std::endl;
	}

	return out.str();
}



std::string print_gen(const llvmgen_t& gen){
	std::stringstream out;

	out << "llvm_ir_program_t:"
		<< print_program(gen.program_acc)
		<< std::endl
	<< "globals"
		<< print_globals(gen.globals);

	return out.str();
}



global_v_t make_global(llvm::Value* value_ptr, std::string debug_str){
	return { value_ptr, debug_str };
}



/*
First-class values

LLVM has a distinction between first class values and other types of values.
First class values can be returned by instructions, passed to functions,
loaded, stored, PHI'd etc.  Currently the first class value types are:

  1. Integer
  2. Floating point
  3. Pointer
  4. Vector
  5. Opaque (which is assumed to eventually resolve to 1-4)

The non-first-class types are:

  5. Array
  6. Structure/Packed Structure
  7. Function
  8. Void
  9. Label
*/

value_t llvm_to_value(const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	//??? more types.
	if(type.is_undefined()){
	}
	else if(type.is_bool()){
		//??? How is int1 encoded by LLVM?
		const auto temp = *static_cast<const uint8_t*>(value_ptr);
		return value_t::make_bool(temp == 0 ? false : true);
	}
	else if(type.is_int()){
		const auto temp = *static_cast<const uint64_t*>(value_ptr);
		return value_t::make_int(temp);
	}
	else if(type.is_double()){
		const auto temp = *static_cast<const double*>(value_ptr);
		return value_t::make_double(temp);
	}
	else if(type.is_string()){
	}
	else if(type.is_json_value()){
	}
	else if(type.is_typeid()){
	}
	else if(type.is_struct()){
	}
	else if(type.is_vector()){
	}
	else if(type.is_dict()){
	}
	else if(type.is_function()){
	}
	else{
	}
	QUARK_ASSERT(false);
	throw std::exception();
}






global_v_t find_value_slot(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const variable_address_t& reg){
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


enum class func_encode {
	functions_are_values,
	functions_are_pointers
};

llvm::Type* intern_type(llvmgen_t& gen_acc, const typeid_t& type, func_encode encode){
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
			const auto m2 = intern_type(gen_acc, m._type, encode);
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

		const auto return_type2 = intern_type(gen_acc, return_type, encode);

		// Make the function type:  double(double,double) etc.
		std::vector<llvm::Type*> args2;

		for(const auto& arg: args){
			auto arg_type = intern_type(gen_acc, arg, encode);
			args2.push_back(arg_type);
		}

		llvm::FunctionType* function_type = llvm::FunctionType::get(return_type2, args2, false);

		if(encode == func_encode::functions_are_values){
			return function_type;
		}
		else if(encode == func_encode::functions_are_pointers){
			auto function_pointer_type = function_type->getPointerTo();
			return function_pointer_type;
		}
		else{
			QUARK_ASSERT(false);
		}

	}
	else{
		QUARK_TRACE_SS(floyd::typeid_to_compact_string(type));
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

//mJit->addGlobalMapping(fn_func, reinterpret_cast<void*>(myFunc));

//	??? temp workaround -- we don't support all argument types yet.
typeid_t hack_function_type(const typeid_t& function_type){
	floyd::typeid_t hacked_function_type = floyd::typeid_t::make_function(
		function_type.get_function_return(),
		{ floyd::typeid_t::make_int() },
		floyd::epure::impure
	);
	return hacked_function_type;
}

//	Function prototype must NOT EXIST already.
llvm::Function* make_function_stub(llvmgen_t& gen_acc, const std::string& function_name, const floyd::typeid_t& function_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_type.check_invariant());
	auto existing_f = gen_acc.program_acc.module->getFunction(function_name);
	QUARK_ASSERT(existing_f == nullptr);

	floyd::typeid_t hacked_function_type = hack_function_type(function_type);

	llvm::Type* ftype = intern_type(gen_acc, hacked_function_type, func_encode::functions_are_values);

	//	IMPORTANT: Must cast to (llvm::FunctionType*) to get correct overload of getOrInsertFunction() to be called!
	auto f3 = gen_acc.program_acc.module->getOrInsertFunction(function_name, (llvm::FunctionType*)ftype);
	llvm::Function* f = llvm::cast<llvm::Function>(f3);
	llvm::verifyFunction(*f);
	return f;
}

std::string make_unique_internal_function_name2(int function_id){
	const auto s = std::string() + "floyd_internal_" + std::to_string(function_id);
	return s;
}
std::string make_unique_internal_function_name(const value_t& function_value){
	QUARK_ASSERT(function_value.check_invariant());

	const auto function_id = function_value.get_function_value();
	return make_unique_internal_function_name2(function_id);
}
std::string make_host_function_label(int host_function_id){
	const auto s = std::string() + "floyd_host_function_" + std::to_string(host_function_id);
	return s;
}


llvm::Value* make_constant(llvmgen_t& gen_acc, const value_t& value){
	const auto type = value.get_type();
	const auto itype = intern_type(gen_acc, type, func_encode::functions_are_pointers);

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
		return llvm::ConstantInt::get(itype, value.get_bool_value() ? 1 : 0);
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
		return make_function_stub(gen_acc, unique_function_str, hack_function_type(type));
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

llvm::Value* genllvm_arithmetic_expression(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, expression_type op, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto type = e._input_exprs[0].get_output_type();

	auto lhs_temp = genllvm_expression(gen_acc, local_symbols, e._input_exprs[0]);
	auto rhs_temp = genllvm_expression(gen_acc, local_symbols, e._input_exprs[1]);

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
	else*/
	if(type.is_int()){
		if(e._operation == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_subtract__2){
			return gen_acc.builder.CreateSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_multiply__2){
			return gen_acc.builder.CreateMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_divide__2){
			return gen_acc.builder.CreateSDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_remainder__2){
			return gen_acc.builder.CreateSRem(lhs_temp, rhs_temp, "reminder_tmp");
		}

		else if(e._operation == expression_type::k_logical_and__2){
			QUARK_ASSERT(false);
			quark::throw_exception();
			return gen_acc.builder.CreateAnd(lhs_temp, rhs_temp, "logical_and_tmp");
		}
		else if(e._operation == expression_type::k_logical_or__2){
			QUARK_ASSERT(false);
			quark::throw_exception();
			return gen_acc.builder.CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(type.is_double()){
		if(e._operation == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateFAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_subtract__2){
			return gen_acc.builder.CreateFSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_multiply__2){
			return gen_acc.builder.CreateFMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_divide__2){
			return gen_acc.builder.CreateFDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(e._operation == expression_type::k_arithmetic_remainder__2){
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		else if(e._operation == expression_type::k_logical_and__2){
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		else if(e._operation == expression_type::k_logical_or__2){
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		else{
			QUARK_ASSERT(false);
		}
	}
/*

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
*/
	QUARK_ASSERT(false);
	quark::throw_exception();

	return nullptr;
}

llvm::Value* genllvm_arithmetic_unary_minus_expression(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto type = e._input_exprs[0].get_output_type();
	if(type.is_int()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), e._input_exprs[0], e._output_type);
		return genllvm_expression(gen_acc, local_symbols, e2);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_simple_expression__2(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_double(0), e._input_exprs[0], e._output_type);
		return genllvm_expression(gen_acc, local_symbols, e2);
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}



////////////////////////////////	Host functions, automatically linked into the LLVM execution engine.

/*
@variable = global i32 21
define i32 @main() {
%1 = load i32, i32* @variable ; load the global variable
%2 = mul i32 %1, 2
store i32 %2, i32* @variable ; store instruction to write to global variable
ret i32 %2
}
*/
extern "C" {

void floyd_host_function_1000(int64_t arg){
	std:: cout << "floyd_host_function_1000: " << arg << std::endl;
}
void floyd_host_function_1001(int64_t arg){
	std:: cout << "floyd_host_function_1001: " << arg << std::endl;
}

void host_print(int64_t arg){
	std:: cout << "host_print: " << arg << std::endl;
}

}


llvm::Value* llvmgen_call_expression(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	//	[0] is the callee, which is required. [1] etc are the args, which are optional.
	QUARK_ASSERT(e._input_exprs.size() >= 1);

	//	_input_exprs[0] is callee, rest are arguments.
	const auto callee_arg_count = static_cast<int>(e._input_exprs.size()) - 1;

	const auto function_type = e._input_exprs[0].get_output_type();
	const auto function_def_arg_types = function_type.get_function_args();
	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto return_type = e.get_output_type();

	QUARK_ASSERT(callee_arg_count == function_def_arg_types.size());
	const auto arg_count = callee_arg_count;

//	int host_function_id = get_host_function_id(gen_acc, e);

#if 0
	//	a = size(b)
	if(host_function_id == 1007 && arg_count == 1){
		const auto arg1_type = e._input_exprs[1].get_output_type();

		bc_opcode opcode = convert_call_to_size_opcode(arg1_type);
		if(opcode != bc_opcode::k_nop){
			const auto& arg1_expr = bcgen_expression(gen_acc, {}, e._input_exprs[1], body_acc);
			body_acc = arg1_expr._body;

			const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: result for k_get_size_vector_x") : target_reg;
			body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, make_imm_int(0)));
			QUARK_ASSERT(body_acc.check_invariant());
			return { body_acc, target_reg2, intern_type(gen_acc, return_type) };
		}
		else{
		}
	}

	//	a = push_back(b, c)
	else if(host_function_id == 1011 && arg_count == 2){
		QUARK_ASSERT(e.get_output_type() == e._input_exprs[1].get_output_type());

		const auto arg1_type = e._input_exprs[1].get_output_type();
		bc_opcode opcode = convert_call_to_pushback_opcode(arg1_type);
		if(opcode != bc_opcode::k_nop){
			//	push_back() used DYN-arguments which are resolved at runtime. When we make opcodes we need to check at compile time = now.
			if(arg1_type.is_string() && e._input_exprs[2].get_output_type().is_int() == false){
				quark::throw_runtime_error("Bad element to push_back(). Require push_back(string, int)");
			}

			const auto& arg1_expr = bcgen_expression(gen_acc, {}, e._input_exprs[1], body_acc);
			body_acc = arg1_expr._body;

			const auto& arg2_expr = bcgen_expression(gen_acc, {}, e._input_exprs[2], body_acc);
			body_acc = arg2_expr._body;

			const auto target_reg2 = target_reg.is_empty() ? add_local_temp(body_acc, e.get_output_type(), "temp: result for k_pushback_x") : target_reg;

			body_acc._instrs.push_back(bcgen_instruction_t(opcode, target_reg2, arg1_expr._out, arg2_expr._out));
			QUARK_ASSERT(body_acc.check_invariant());
			return { body_acc, target_reg2, intern_type(gen_acc, return_type) };
		}
		else{
		}
	}
#endif

//	floyd::typeid_t print_function_type = floyd::typeid_t::make_function(typeid_t::make_void(), { floyd::typeid_t::make_int() }, floyd::epure::impure);

	//	Name matches host_print()-function above.
//	llvm::Function* print_stub = make_function_stub(gen_acc, "host_print", print_function_type);


	//	Normal function call.
	{
		llvm::Value* callee0 = genllvm_expression(gen_acc, local_symbols, e._input_exprs[0]);
		QUARK_TRACE_SS("callee0: " << print_value(callee0));

		QUARK_TRACE_SS("gen_acc: " << print_gen(gen_acc));

		llvm::Function* callee = llvm::cast<llvm::Function>(callee0);

		//	Skip [0], which is callee.
		std::vector<llvm::Value*> args2;
		for(int i = 1 ; i < e._input_exprs.size() ; i++){
			llvm::Value* arg2 = genllvm_expression(gen_acc, local_symbols, e._input_exprs[i]);
			args2.push_back(arg2);
		}

		// First, see if the function has already been added to the current module.
//		auto *F = gen_acc.program_acc.module->getFunction("assdjflksjf");

/*		// If not, check whether we can codegen the declaration from some existing
		// prototype.
		auto FI = FunctionProtos.find(Name);
		if (FI != FunctionProtos.end())
		return FI->second->codegen();
*/

		//	20 = host-print().
//		const auto print_function = value_t::make_function_value(function_type, 20);
//		const auto unique_function_str = make_unique_internal_function_name(print_function);
//		llvm::Function* print_func = make_function_stub(gen_acc, unique_function_str, function_type);

		if(return_type.is_void()){
			return gen_acc.builder.CreateCall(callee, args2, "");
		}
		else{
			return gen_acc.builder.CreateCall(callee, args2, "temp_call");
		}
	}
}

llvm::Value* llvmgen_load2_expression(llvmgen_t& gen_acc, const expression_t& e, const std::vector<global_v_t>& local_symbols){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_value_slot(gen_acc, local_symbols, e._address);
	return gen_acc.builder.CreateLoad(dest.value_ptr);
}

//??? use visitor and std::variant<>
llvm::Value* genllvm_expression(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();
	if(op == expression_type::k_literal){
		return genllvm_literal_expression(gen_acc, e);
	}
	else if(op == expression_type::k_resolve_member){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		return bcgen_resolve_member_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_lookup_element){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		return bcgen_lookup_element_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_load2){
#if 0
		if(e.get_output_type().is_function()){
			llvm::Value* dest = find_value_slot(gen_acc, gen_acc.globals, e._address);
			return dest;
		}
		else{
			return llvmgen_load2_expression(gen_acc, e, local_symbols);
		}
#else
		return llvmgen_load2_expression(gen_acc, e, local_symbols);
#endif
	}
	else if(op == expression_type::k_call){
		return llvmgen_call_expression(gen_acc, local_symbols, e);
	}
	else if(op == expression_type::k_value_constructor){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		return bcgen_construct_value_expression(gen_acc, target_reg, e, body);
	}
	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return genllvm_arithmetic_unary_minus_expression(gen_acc, local_symbols, e);
	}
	else if(op == expression_type::k_conditional_operator3){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		return bcgen_conditional_operator_expression(gen_acc, target_reg, e, body);
	}
	else if (is_arithmetic_expression(op)){
		return genllvm_arithmetic_expression(gen_acc, local_symbols, op, e);
	}
	else if (is_comparison_expression(op)){
		QUARK_ASSERT(false);
		quark::throw_exception();
//		return bcgen_comparison_expression(gen_acc, target_reg, op, e, body);
	}
	else{
		QUARK_ASSERT(false);
	}
	quark::throw_exception();
}



void genllvm_store2_statement(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const statement_t::store2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = genllvm_expression(gen_acc, local_symbols, s._expression);

	auto dest = find_value_slot(gen_acc, local_symbols, s._dest_variable);
	gen_acc.builder.CreateStore(value, dest.value_ptr);

	QUARK_ASSERT(gen_acc.check_invariant());
}

void genllvm_expression_statement(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const statement_t::expression_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	genllvm_expression(gen_acc, local_symbols, s._expression);

	QUARK_ASSERT(gen_acc.check_invariant());
}

/*
	All Floyd's global statements becomes instructions in floyd_init()-function that is called by Floyd runtime before any other function is called.
*/

void genllvm_statements(llvmgen_t& gen_acc, const std::vector<global_v_t>& local_symbols, const std::vector<statement_t>& statements){
	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());

			struct visitor_t {
				llvmgen_t& acc0;
				const std::vector<global_v_t>& local_symbols0;

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
					genllvm_store2_statement(acc0, local_symbols0, s);
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
					genllvm_expression_statement(acc0, local_symbols0, s);
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

std::vector<global_v_t> genllvm_local_make_symbols(llvmgen_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	std::vector<global_v_t> result;

	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(gen_acc, type, func_encode::functions_are_pointers);

		llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

		if(e.second._const_value.is_void() == false){
			llvm::Value* c = make_constant(gen_acc, e.second._const_value);
			gen_acc.builder.CreateStore(c, dest);
		}
		else{
		}
		const auto debug_str = "name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_global(dest, debug_str));
	}
	return result;
}



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
llvm::Value* genllvm_make_global(llvmgen_t& gen_acc, const pass2_ast_t& checked_ast, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(checked_ast.check_invariant());
	QUARK_ASSERT(symbol.check_invariant());

	const auto type0 = symbol.get_type();

	if(type0.is_function()){
		const auto itype = intern_type(gen_acc, hack_function_type(type0), func_encode::functions_are_values);
		QUARK_TRACE_SS("itype: " << print_type(itype));

		const int function_id = symbol._const_value.get_function_value();
		const auto& function_def = *checked_ast._tree._function_defs[function_id];
		if(function_def._host_function_id != k_no_host_function_id){
			const auto label = make_host_function_label(function_def._host_function_id);
			llvm::Function* f = gen_acc.program_acc.module->getFunction(label);
			QUARK_ASSERT(f != nullptr);

			llvm::Value* gv = new llvm::GlobalVariable(
				*gen_acc.program_acc.module,
				itype,
				true,
				llvm::GlobalValue::ExternalLinkage,
				f,
//				llvm::Constant::getNullValue(itype),
				symbol_name
			);

			QUARK_TRACE_SS("global value: " << print_value(gv));

			return gv;
		}
		else{
			return nullptr;
		}
	}
	else{
		const auto itype = intern_type(gen_acc, type0, func_encode::functions_are_pointers);
		llvm::Value* init = nullptr;
		if(symbol._const_value.is_void() == false){
			llvm::Value* c = make_constant(gen_acc, symbol._const_value);
	//				dest->setInitializer(constant_value);
			init = c;
		}

		llvm::Value* gv = new llvm::GlobalVariable(
			*gen_acc.program_acc.module,
			itype,
			true,
			llvm::GlobalValue::ExternalLinkage,
	//			init,
			llvm::Constant::getNullValue(itype),
			symbol_name
		);

		return gv;
	}
}



std::vector<global_v_t> genllvm_make_globals(llvmgen_t& gen_acc, const pass2_ast_t& checked_ast, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	std::vector<global_v_t> result;

	for(const auto& e: symbol_table._symbols){
		llvm::Value* value = genllvm_make_global(gen_acc, checked_ast, e.first, e.second);
		const auto debug_str = "name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_global(value, debug_str));

		QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));
	}
	return result;
}



void genllvm_function_def(llvmgen_t& gen_acc, const floyd::function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	llvm::Function* f = make_function_stub(gen_acc, "generated_func_name", function_def._function_type);

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.program_acc.context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	auto symbol_table_values = genllvm_local_make_symbols(gen_acc, function_def._body->_symbol_table);
	genllvm_statements(gen_acc, symbol_table_values, function_def._body->_statements);

	llvm::verifyFunction(*f);
}

void genllvm_make_function_defs(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());


	for(int function_id = 0 ; function_id < semantic_ast._checked_ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = *semantic_ast._checked_ast._tree._function_defs[function_id];

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
void genllvm_make_floyd_runtime_init(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast, const std::vector<global_v_t>& global_symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Function* f = make_function_stub(
		gen_acc,
		"floyd_runtime_init",
		floyd::typeid_t::make_function(floyd::typeid_t::make_int(), {}, floyd::epure::impure)
	);

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.program_acc.context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	genllvm_statements(gen_acc, global_symbol_table, semantic_ast._checked_ast._tree._globals._statements);

	llvm::Value* dummy_result = llvm::ConstantInt::get(gen_acc.builder.getInt64Ty(), 667);
	llvm::ReturnInst::Create(gen_acc.program_acc.context, dummy_result, entryBB);

	llvm::verifyFunction(*f);
}


void genllvm_all(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	Register all function defs as LLVM function prototypes.
	//	host functions are later linked by LLVM execution engine, by matching the function names.
	//	Floyd functions are later filled with instructions.
	{
		const auto& defs = semantic_ast._checked_ast._tree._function_defs;
		for(int function_id = 0 ; function_id < defs.size() ; function_id++){
			const auto& function_def = *defs[function_id];
			const auto function_type = function_def._function_type;

			if(function_def._host_function_id != k_no_host_function_id){
				const auto label = make_host_function_label(function_def._host_function_id);
				auto f = make_function_stub(gen_acc, label, hack_function_type(function_type));
			}
			else{
				const auto label = make_unique_internal_function_name2(function_id);
				auto f = make_function_stub(gen_acc, label, hack_function_type(function_type));
			}
		}
	}

	//	Global variables.
	{
		QUARK_ASSERT(gen_acc.check_invariant());
		QUARK_ASSERT(semantic_ast.check_invariant());

		std::vector<global_v_t> globals = genllvm_make_globals(
			gen_acc,
			semantic_ast._checked_ast,
			semantic_ast._checked_ast._tree._globals._symbol_table
		);
		gen_acc.globals = globals;
	}

	//	Global instructions are packed into the "floyd_runtime_init"-function. LLVM does't have global instructions.
	genllvm_make_floyd_runtime_init(gen_acc, semantic_ast, gen_acc.globals);

	//	Generate instructions for all functions.
	genllvm_make_function_defs(gen_acc, semantic_ast);
}

std::unique_ptr<llvm_ir_program_t> generate_llvm_ir(const semantic_ast_t& ast, const std::string& module_name){
	QUARK_ASSERT(ast.check_invariant());
	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(pass1_ast_to_json(ast._checked_ast)._value));

	auto p = make_empty_program(module_name);

	llvm::IRBuilder<> builder(p->context);
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	llvmgen_t acc(*p, builder);
	genllvm_all(acc, ast);

	QUARK_TRACE_SS("result = " << floyd::print_program(*p));

	return p;
}



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
/*

	llvm::Function* init_func = program.module->getFunction("floyd_runtime_init");
	QUARK_ASSERT(init_func != nullptr);
	init_func->print(llvm::errs());



		llvm::GenericValue init_result = ee->runFunction(init_func, {});
		const int64_t init_result_int = init_result.IntVal.getSExtValue();
		QUARK_ASSERT(init_result_int == 667);



		llvm::Function* b_func = ee->FindFunctionNamed("floyd_runtime_init");
		llvm::GenericValue b_result = ee->runFunction(b_func, {});
		const int64_t b_result_int = b_result.IntVal.getSExtValue();
		QUARK_ASSERT(b_result_int == 667);

*/

void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name){
	const auto addr = ee.ee->getGlobalValueAddress(name);
	return  (void*)addr;
}

void* get_global_function(llvm_execution_engine_t& ee, const std::string& name){
	const auto addr = ee.ee->getFunctionAddress(name);
	return (void*)addr;
}





typedef int64_t (*FLOYD_RUNTIME_INIT)();

void print_module_contents(llvm::Module& module){
	const auto& functionList = module.getFunctionList();
	for(const auto& e: functionList){

/*
	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	module.print(stream2, nullptr);
	return dump;
*/
		e.print(llvm::errs());
	}

	const auto& globalList = module.getGlobalList();
	for(const auto& e: globalList){
		e.print(llvm::errs());
	}
}


//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
llvm_execution_engine_t make_engine_break_program_no_init(llvm_ir_program_t& program){
	QUARK_ASSERT(program.module);

	//	print_module_contents(*program.module);

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
	return { ee };
}

//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
llvm_execution_engine_t make_engine_break_program(llvm_ir_program_t& program){
	QUARK_ASSERT(program.module);

	llvm_execution_engine_t ee = make_engine_break_program_no_init(program);

	//	Call floyd_runtime_init().
	{
		auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_init"));
		int64_t a_result = (*a_func)();
		QUARK_ASSERT(a_result == 667);
	}

	return { ee };
}


//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_ir_program_t& program, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(program.module);

	auto ee = make_engine_break_program(program);
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
QUARK_UNIT_TEST("", "run_using_llvm()", "", ""){
	const auto r = floyd::run_using_llvm_helper("", "", {});
	QUARK_UT_VERIFY(r == 6765);
}
#endif

#if 0
QUARK_UNIT_TEST("Floyd test suite", "+", "", ""){
//	ut_verify_global_result(QUARK_POS, "let int result = 1 + 2 + 3", value_t::make_int(6));

	const auto pass3 = compile_to_sematic_ast__errors("let int result = 1 + 2 + 3", "myfile.floyd", floyd::compilation_unit_mode::k_no_core_lib);
	auto program = generate_llvm_ir(pass3, "myfile.floyd");
	auto ee = make_engine_break_program(*program);

	const auto result = *static_cast<uint64_t*>(floyd::get_global_ptr(ee, "result"));
	QUARK_ASSERT(result == 6);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}
#endif



const std::string test_1_json = R"ABCD(
{
	"function_defs": [
		["function-def", ["func", "^void", ["^**dyn**"], true], [{ "name": "dummy", "type": "^**dyn**" }], null, 1000, "^void"],
		[
			"function-def",
			["func", "^**dyn**", ["^**dyn**", "^**dyn**"], true],
			[{ "name": "dummy", "type": "^**dyn**" }, { "name": "dummy", "type": "^**dyn**" }],
			null,
			1011,
			"^**dyn**"
		]
	],
	"globals": {
		"statements": [
			[0, "expression-statement", ["call", ["@i", -1, 20, ["func", "^void", ["^**dyn**"], true]], [["k", 5, "^int"]], "^void"]]
		],
		"symbols": [
			[20, "print", "CONST", [["func", "^void", ["^**dyn**"], true], { "function_id": 20 }]],
			[37, "null", "CONST", ["^json_value", null]],
			[38, "**undef**", "LOCAL", { "type": "immutable_local", "value_type": "^**undef**" }],
			[39, "**dyn**", "CONST", ["^**dyn**", null]],
			[40, "void", "CONST", ["^void", null]],
			[41, "bool", "CONST", ["^typeid", "^bool"]],
			[42, "int", "CONST", ["^typeid", "^int"]],
			[43, "double", "CONST", ["^typeid", "^double"]],
			[44, "string", "CONST", ["^typeid", "^string"]],
			[45, "typeid", "CONST", ["^typeid", "^typeid"]],
			[46, "json_value", "CONST", ["^typeid", "^json_value"]],
			[47, "json_object", "CONST", ["^int", 1]],
			[48, "json_array", "CONST", ["^int", 2]],
			[49, "json_string", "CONST", ["^int", 3]],
			[50, "json_number", "CONST", ["^int", 4]],
			[51, "json_true", "CONST", ["^int", 5]],
			[52, "json_false", "CONST", ["^int", 6]],
			[53, "json_null", "CONST", ["^int", 7]]
		]
	}
}
")ABCD";

#include "text_parser.h"
#include "ast_json.h"

#if 0
floyd::semantic_ast_t json_to_semantic_ast(const json_t& j){


const std::vector<statement_t> astjson_to_statements(const ast_json_t& p){




	const floyd::pass2_ast_t pass3 = parse_tree_to_ast(floyd::ast_json_t::make(a.first));
//	const auto pass3 = compile_to_sematic_ast__errors("let int result = 1 + 2 + 3", "myfile.floyd", floyd::compilation_unit_mode::k_no_core_lib);

	return floyd::semantic_ast_t(pass3);
}



QUARK_UNIT_TEST_VIP("", "", "", ""){
	std::pair<json_t, seq_t> a = parse_json(seq_t(test_1_json));

	const auto pass3 = json_to_semantic_ast(a);
	auto program = generate_llvm_ir(pass3, "myfile.floyd");

	floyd::print_program(*program);

	auto ee = make_engine_break_program(*program);

	const auto result = *static_cast<uint64_t*>(floyd::get_global_ptr(ee, "result"));
	QUARK_ASSERT(result == 6);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}


#endif

