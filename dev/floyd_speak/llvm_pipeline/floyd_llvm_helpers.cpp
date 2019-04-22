//
//  floyd_llvm_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_helpers.h"

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

#include "llvm/Bitcode/BitstreamWriter.h"


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
#include "text_parser.h"
#include "ast_json.h"

#include "quark.h"


namespace floyd {

void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}


/*
LLVM return struct byvalue:
http://lists.llvm.org/pipermail/llvm-dev/2009-June/023391.html
struct Big {
 int a, b, c;
int d;
};

struct Big foo() {
  struct Big result = {1, 2, 3, 4, 5};
  return result;
}


> "Note that the code generator does not yet fully support large return
> values. The specific sizes that are currently supported are dependent on the
> target. For integers, on 32-bit targets the limit is often 64 bits, and on
> 64-bit targets the limit is often 128 bits. For aggregate types, the current
> limits are dependent on the element types; for example targets are often
> limited to 2 total integer elements and 2 total floating-point elements."
>
> So, does this mean that I can't have a return type with more than two
> integers? Is there any other way to support longer return structure?

That's what it means (at least until someone like you contributes a
patch to support larger return structs). To work around it, imitate
what the C compiler does.

Try typing:

struct Big {
 int a, b, c;
};

struct Big foo() {
  struct Big result = {1, 2, 3};
  return result;
}
*/



/// Check a function for errors, useful for use when debugging a
/// pass.
///
/// If there are no errors, the function returns false. If an error is found,
/// a message describing the error is written to OS (if non-null) and true is
/// returned.
//bool verifyFunction(const Function &F, raw_ostream *OS = nullptr);

bool check_invariant__function(const llvm::Function* f){
	QUARK_ASSERT(f != nullptr);

	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	bool errors = llvm::verifyFunction(*f, &stream2);
	if(errors){
		f->print(stream2);

		QUARK_TRACE_SS("================================================================================");
		QUARK_TRACE_SS("\n" << dump);

//??? print("") and print(123) could be different functions.

		QUARK_ASSERT(false);
	}
	return !errors;
}

bool check_invariant__module(llvm::Module* module){
	QUARK_ASSERT(module != nullptr);

	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	bool module_errors_flag = llvm::verifyModule(*module, &stream2, nullptr);
	if(module_errors_flag){
		QUARK_TRACE_SS(dump);

		const auto& functions = module->getFunctionList();
		for(const auto& e: functions){
			QUARK_ASSERT(check_invariant__function(&e));
			llvm::verifyFunction(e);
		}

		QUARK_ASSERT(false);
		return false;
	}

	return true;
}

bool check_invariant__builder(llvm::IRBuilder<>* builder){
	QUARK_ASSERT(builder != nullptr);

	auto module = builder->GetInsertBlock()->getParent()->getParent();
	QUARK_ASSERT(check_invariant__module(module));
	return true;
}





std::string print_module(llvm::Module& module){
	std::string dump;
	llvm::raw_string_ostream stream2(dump);

	stream2 << "\n" "MODULE" << "\n";
	module.print(stream2, nullptr);

/*
	Not needed, module.print() prints the exact list.
	stream2 << "\n" "FUNCTIONS" << "\n";
	const auto& functionList = module.getFunctionList();
	for(const auto& e: functionList){
		e.print(stream2);
	}
*/

	stream2 << "\n" "GLOBALS" << "\n";
	const auto& globalList = module.getGlobalList();
	int index = 0;
	for(const auto& e: globalList){
		stream2 << index << ": ";
		e.print(stream2);
		stream2 << "\n";
		index++;
	}

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

std::string print_function(const llvm::Function* f){
	if(f == nullptr){
		return "nullptr";
	}
	else{
		QUARK_ASSERT(check_invariant__function(f));

		std::string s;
		llvm::raw_string_ostream rso(s);
		f->print(rso);
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







////////////////////////////////	floyd_runtime_ptr


bool check_callers_fcp(llvm::Function& emit_f){
	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	QUARK_ASSERT(floyd_context_arg_ptr->getType()->isPointerTy());
	QUARK_ASSERT(floyd_context_arg_ptr->getType()->getPointerElementType()->isIntegerTy(32));
	return true;
}


bool check_emitting_function(llvm::Function& emit_f){
	QUARK_ASSERT(check_callers_fcp(emit_f));
	return true;
}

llvm::Value* get_callers_fcp(llvm::Function& emit_f){
	QUARK_ASSERT(check_callers_fcp(emit_f));

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();

	QUARK_ASSERT(floyd_context_arg_ptr->getType()->isPointerTy());
	QUARK_ASSERT(floyd_context_arg_ptr->getType()->getPointerElementType()->isIntegerTy(32));
	return floyd_context_arg_ptr;
}



llvm::Type* make_frp_type(llvm::LLVMContext& context){
	return llvm::Type::getInt32PtrTy(context);
}





////////////////////////////////		VEC_T



//	Makes a type for VEC_T.
llvm::Type* make_vec_type(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	element_ptr
		llvm::Type::getInt64Ty(context)->getPointerTo(),

		//	element_count
		llvm::Type::getInt32Ty(context),

		//	magic
		llvm::Type::getInt16Ty(context),

		//	element_bits
		llvm::Type::getInt16Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

llvm::Value* get_vec_ptr(llvm::IRBuilder<>& builder, llvm::Value* vec_byvalue){
	auto& context = builder.getContext();

	auto alloc_value = builder.CreateAlloca(make_vec_type(context));
	builder.CreateStore(vec_byvalue, alloc_value);
	return alloc_value;
}

bool check_invariant_vector(const VEC_T& v){
	QUARK_ASSERT(v.element_ptr != nullptr);
	QUARK_ASSERT(v.element_bits > 0 && v.element_bits < (8 * 128));
	QUARK_ASSERT(v.magic == 0xDABB);
	return true;
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(VEC_T);
	QUARK_UT_VERIFY(vec_struct_size == 16);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_UT_VERIFY(wr_struct_size == 16);
}

/*
VEC_T floyd_runtime__allocate_vector(void* floyd_runtime_ptr, uint16_t element_bits, uint32_t element_count){
	auto r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(element_bits <= 64);

	const auto alloc_bits = element_count * element_bits;
	const auto alloc_count = (alloc_bits / 64) + (alloc_bits & 64) ? 1 : 0;

	auto element_ptr = reinterpret_cast<uint64_t*>(std::calloc(alloc_count, sizeof(uint64_t)));
	if(element_ptr == nullptr){
		throw std::exception();
	}

	VEC_T result;
	result.element_ptr = element_ptr;
	result.magic = 0xDABB;
	result.element_bits = element_bits;
	result.element_count = element_count;
	return result;
}
*/

//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
VEC_T make_vec(uint32_t element_count){
	auto element_ptr = reinterpret_cast<uint64_t*>(std::calloc(element_count, sizeof(uint64_t)));
	if(element_ptr == nullptr){
		throw std::exception();
	}

	VEC_T result;
	result.element_ptr = element_ptr;
	result.element_count = element_count;
	result.magic = 0xDABB;
	result.element_bits = 123;

	QUARK_ASSERT(check_invariant_vector(result));
	return result;
}

void delete_vec(VEC_T& vec){
	QUARK_ASSERT(check_invariant_vector(vec));

	std::free(vec.element_ptr);
	vec.element_ptr = nullptr;
	vec.magic = 0xDEAD;
	vec.element_count = -vec.element_count;
}




////////////////////////////////		WIDE_RETURN_T



llvm::Type* make_wide_return_type(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	a
		llvm::Type::getInt64Ty(context),

		//	b
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}



WIDE_RETURN_T make_wide_return_2x64(uint64_t a, uint64_t b){
	return WIDE_RETURN_T{ a, b };
}


WIDE_RETURN_T make_wide_return_charptr(const char* s){
	return WIDE_RETURN_T{ reinterpret_cast<uint64_t>(s), 0 };
}

WIDE_RETURN_T make_wide_return_vec(const VEC_T& vec){
	return *reinterpret_cast<const WIDE_RETURN_T*>(&vec);
}

VEC_T wider_return_to_vec(const WIDE_RETURN_T& ret){
	return *reinterpret_cast<const VEC_T*>(&ret);
}






llvm::Value* generate__convert_wide_return_to_vec(llvm::IRBuilder<>& builder, llvm::Value* wide_return_reg){
	auto& context = builder.getContext();

	auto wide_return_ptr_reg = builder.CreateAlloca(make_wide_return_type(context), nullptr, "temp_vec");
	builder.CreateStore(wide_return_reg, wide_return_ptr_reg);
	auto vec_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, wide_return_ptr_reg, make_vec_type(context)->getPointerTo(), "");
	auto vec_reg = builder.CreateLoad(vec_ptr_reg, "final");
	return vec_reg;
}





////////////////////////////////		llvm_arg_mapping_t


llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args){
	if(args.empty()){
		QUARK_ASSERT(def.args.size() == 1);
		return def;
	}
	else{
		const auto floyd_arg_count = def.args.back().floyd_arg_index + 1;
		QUARK_ASSERT(floyd_arg_count == args.size());

		std::vector<llvm_arg_mapping_t> arg_results;

		//	Skip arg #0, which is "floyd_runtime_ptr".
		for(int out_index = 0 ; out_index < def.args.size() ; out_index++){
			auto arg_copy = def.args[out_index];
			if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_simple_value){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name;
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name + "-dynval";
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];

				arg_copy.floyd_name = floyd_arg._name + "-dyntype";
			}
			else{
				QUARK_ASSERT(false);
			}
			arg_results.push_back(arg_copy);
		}

		return llvm_function_def_t { def.return_type, arg_results, def.llvm_args };
	}
}

llvm_function_def_t map_function_arguments(llvm::Module& module, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	auto& context = module.getContext();

	const auto ret = function_type.get_function_return();
	llvm::Type* return_type = ret.is_internal_dynamic() ? make_wide_return_type(context) : intern_type(module, ret);

	const auto args = function_type.get_function_args();
	std::vector<llvm_arg_mapping_t> arg_results;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation in Floyd function type.
	arg_results.push_back({ make_frp_type(context), "floyd_runtime_ptr", floyd::typeid_t::make_undefined(), -1, llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr });

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(arg.is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(arg.is_internal_dynamic()){
			arg_results.push_back({ llvm::Type::getInt64Ty(context), std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_dyn_value });
			arg_results.push_back({ llvm::Type::getInt64Ty(context), std::to_string(index), typeid_t::make_undefined(), index, llvm_arg_mapping_t::map_type::k_dyn_type });
		}
		else {
			auto arg_itype = intern_type(module, arg);
			arg_results.push_back({ arg_itype, std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_simple_value });
		}
	}

	std::vector<llvm::Type*> llvm_args;
	for(const auto& e: arg_results){
		llvm_args.push_back(e.llvm_type);
	}

	return llvm_function_def_t { return_type, arg_results, llvm_args };
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
	llvm_instance_t instance;
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	const auto r = map_function_arguments(*module, typeid_t::make_function(typeid_t::make_void(), {}, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
	llvm_instance_t instance;
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	const auto r = map_function_arguments(*module, typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isIntegerTy(64));

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
	llvm_instance_t instance;
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	const auto r = map_function_arguments(*module, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 2);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_simple_value);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int, DYN, bool)", ""){
	llvm_instance_t instance;
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	const auto r = map_function_arguments(*module, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int(), typeid_t::make_internal_dynamic(), typeid_t::make_bool() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 5);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_simple_value);

	QUARK_UT_VERIFY(r.args[2].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[2].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[2].floyd_type.is_internal_dynamic());
	QUARK_UT_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_UT_VERIFY(r.args[3].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[3].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[3].floyd_arg_index == 1);QUARK_UT_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_UT_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_UT_VERIFY(r.args[4].floyd_name == "2");
	QUARK_UT_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_UT_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_UT_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_simple_value);
}






////////////////////////////////		intern_type()

//	Function-types are always returned as pointer-to-function types.
llvm::Type* make_function_type(llvm::Module& module, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments(module, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}


llvm::Type* make_struct_type(llvm::Module& module, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	auto& context = module.getContext();

#if 0
		std::vector<llvm::Type*> members;
		for(const auto& m: type.get_struct_ref()->_members){
			const auto m2 = intern_type(*gen_acc.module, m._type, encode);
			members.push_back(m2);
		}

  		llvm::StructType* s = llvm::StructType::get(context, members, false);

//		return llvm::StructType::get(context);
//		return llvm::Type::getInt32Ty(context);
		return s;
#endif
	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto m2 = intern_type(module, m._type);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

//	Returns the LLVM type we chose to use to encode each Floyd type.
llvm::Type* intern_type(llvm::Module& module, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = module.getContext();

	if(type.is_void()){
		return llvm::Type::getVoidTy(context);
	}
	else if(type.is_int()){
		return llvm::Type::getInt64Ty(context);
	}
	else if(type.is_bool()){
		return llvm::Type::getInt1Ty(context);
	}

	else if(type.is_string()){
		return llvm::Type::getInt8PtrTy(context);
	}
	else if(type.is_json_value()){
		return llvm::Type::getInt16Ty(context);
	}
	else if(type.is_vector()){
/*
		const auto element_type = type.get_vector_element_type();
		const auto element_type2 = intern_type(module, element_type);
		return element_type2->getPointerTo();
*/
		return make_vec_type(context);
	}
	else if(type.is_typeid()){
		return llvm::Type::getInt32Ty(context);
	}
	else if(type.is_undefined()){
		return llvm::Type::getInt16Ty(context);
	}
	else if(type.is_unresolved_type_identifier()){
		NOT_IMPLEMENTED_YET();
		return llvm::Type::getInt16Ty(context);
	}
	else if(type.is_double()){
		return llvm::Type::getDoubleTy(context);
	}
	else if(type.is_struct()){
		return make_struct_type(module, type)->getPointerTo();
	}

	else if(type.is_internal_dynamic()){
		//??? should return DYN
//		QUARK_ASSERT(false);
		return llvm::Type::getInt64Ty(context);
	}
	else if(type.is_function()){
		return make_function_type(module, type);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}




/*
??? TEMP HACK TO STORE VECTOR & DICT IN ITYPE
100 +  base_type => vector[base_type]
200 +  base_type => dict[string: base_type]

enum class base_type {
	//	k_internal_undefined is never exposed in code, only used internally in compiler.
	k_internal_undefined,

	//	Used by host functions arguments / returns to tell this is a dynamic value, not static type.
	k_internal_dynamic,

	//	Means no value. Used as return type for print() etc.
	k_void,

	k_bool,
	k_int,
	k_double,
	k_string,
	k_json_value,

	//	This is a type that specifies any other type at runtime.
	k_typeid,

	k_struct,
	k_vector,
	k_dict,
	k_function,

	//	We have an identifier, like "pixel" or "print" but haven't resolved it to an actual type yet.
	//	Keep the identifier so it can be resolved later
	k_internal_unresolved_type_identifier
};
*/
int64_t pack_itype(const typeid_t& type){
	if(type.is_internal_dynamic()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_void()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_bool()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_int()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_double()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_string()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_json_value()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_typeid()){
		return static_cast<int64_t>(type.get_base_type());
	}
	else if(type.is_struct()){
		if(type.get_struct()._members == std::vector<member_t>{ member_t{ typeid_t::make_int(), "red" }, member_t{ typeid_t::make_int(), "green" }, member_t{ typeid_t::make_int(), "blue" } }){
			return 101;
		}
		else{
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(type.is_vector()){
		return 200 + pack_itype(type.get_vector_element_type());
	}
	else if(type.is_dict()){
		return 300 + pack_itype(type.get_dict_value_type());
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

typeid_t unpack_itype(int64_t type){
	switch(type){
		case (int)base_type::k_internal_undefined:
			return typeid_t::make_undefined();

		case (int)base_type::k_internal_dynamic:
			return typeid_t::make_internal_dynamic();

		case (int)base_type::k_void:
			return typeid_t::make_void();

		case (int)base_type::k_bool:
			return typeid_t::make_bool();

		case (int)base_type::k_int:
			return typeid_t::make_int();

		case (int)base_type::k_double:
			return typeid_t::make_double();

		case (int)base_type::k_string:
			return typeid_t::make_string();

		case (int)base_type::k_json_value:
			return typeid_t::make_json_value();

		case (int)base_type::k_typeid:
			return typeid_t::make_typeid();

		case (int)base_type::k_struct:
			QUARK_ASSERT(false);
			throw std::exception();

		case (int)base_type::k_vector:
			QUARK_ASSERT(false);
			throw std::exception();

		case (int)base_type::k_dict:
			QUARK_ASSERT(false);
			throw std::exception();

		case (int)base_type::k_function:
			QUARK_ASSERT(false);
			throw std::exception();

		case (int)base_type::k_internal_unresolved_type_identifier:
			QUARK_ASSERT(false);
			throw std::exception();

		default:
			if(type >= 100 && type < 200){
				if(type == 101){
					const auto members = std::vector<member_t>{
						member_t{ typeid_t::make_int(), "red" }, member_t{ typeid_t::make_int(), "green" }, member_t{ typeid_t::make_int(), "blue" }
					};
					return typeid_t::make_struct2(members);
				}
				else{
					NOT_IMPLEMENTED_YET();
				}
			}
			else if(type >= 200 && type < 300){
				return typeid_t::make_vector(unpack_itype(type - 200));
			}
			else if(type >= 300 && type < 400){
				return typeid_t::make_dict(unpack_itype(type - 300));
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
	}
}




}	//	floyd

