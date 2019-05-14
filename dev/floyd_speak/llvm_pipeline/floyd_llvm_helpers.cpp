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




////////////////////////////////	runtime_type_t


llvm::Type* make_runtime_type_type(llvm::LLVMContext& context){
	return llvm::Type::getInt32Ty(context);
/*
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt32Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
*/

}

runtime_type_t make_runtime_type(int32_t itype){
	return runtime_type_t{ itype };
}





////////////////////////////////	native_value_t


QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(runtime_value_t);
	QUARK_UT_VERIFY(s == 8);
}



QUARK_UNIT_TEST("", "", "", ""){
	auto y = make_runtime_int(8);
	auto a = make_runtime_int(1234);

	y = a;

	runtime_value_t c(y);
}


llvm::Type* make_runtime_value_type(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
}


runtime_value_t make_runtime_bool(bool value){
	return { .bool_value = value };
}

runtime_value_t make_runtime_int(int64_t value){
	return { .int_value = value };
}
runtime_value_t make_runtime_typeid(runtime_type_t type){
	return { .typeid_itype = type };
}
runtime_value_t make_runtime_struct(void* struct_ptr){
	return { .struct_ptr = struct_ptr };
}


VEC_T* unpack_vec_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(types, arg_type);
#endif
	QUARK_ASSERT(type.is_vector());
	QUARK_ASSERT(arg_value.vector_ptr != nullptr);
	QUARK_ASSERT(arg_value.vector_ptr->check_invariant());

	return arg_value.vector_ptr;
}

DICT_T* unpack_dict_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(types, arg_type);
#endif
	QUARK_ASSERT(type.is_dict());
	QUARK_ASSERT(arg_value.dict_ptr != nullptr);

	QUARK_ASSERT(arg_value.dict_ptr->check_invariant());

	return arg_value.dict_ptr;
}


base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto a = lookup_type(interner, type);
	const auto a_basetype = a.get_base_type();

	//??? We know ranges where type.itype maps to base_type -- no need to look up in type_interner.
	return a_basetype;
}


typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.first.itype == type; });
	if(it != interner.interned.end()){
		return it->second;
	}
	throw std::exception();
}

runtime_type_t lookup_runtime_type(const type_interner_t& interner, const typeid_t& type){
	const auto a = lookup_itype(interner, type);
	return make_runtime_type(a.itype);
}



////////////////////////////////	MISSING FEATURES




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




////////////////////////////////		WIDE_RETURN_T



llvm::StructType* make_wide_return_type(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	a
		llvm::Type::getInt64Ty(context),

		//	b
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

WIDE_RETURN_T make_wide_return_2x64(runtime_value_t a, runtime_value_t b){
	return WIDE_RETURN_T{ a, b };
}

WIDE_RETURN_T make_wide_return_charptr(char* s){
	return WIDE_RETURN_T{ { .string_ptr = s }, { .int_value = 0 } };
}

WIDE_RETURN_T make_wide_return_structptr(void* s){
	return WIDE_RETURN_T{ { .struct_ptr = s }, { .int_value = 0 } };
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



QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(VEC_T);
	QUARK_UT_VERIFY(vec_struct_size == 16);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_UT_VERIFY(wr_struct_size == 16);
}

VEC_T make_vec(uint32_t element_count){
	auto element_ptr = reinterpret_cast<runtime_value_t*>(std::calloc(element_count, sizeof(runtime_value_t)));
	if(element_ptr == nullptr){
		throw std::exception();
	}

	VEC_T result;
	result.element_ptr = element_ptr;
	result.element_count = element_count;
	result.magic = 0xDABB;
	result.element_bits = 123;

	QUARK_ASSERT(result.check_invariant());
	return result;
}

void delete_vec(VEC_T& vec){
	QUARK_ASSERT(vec.check_invariant());

	std::free(vec.element_ptr);
	vec.element_ptr = nullptr;
	vec.magic = 0xDEAD;
	vec.element_count = -vec.element_count;
}


llvm::StructType* make_vec_type(llvm::LLVMContext& context){
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

WIDE_RETURN_T make_wide_return_vec(VEC_T* vec){
	return make_wide_return_2x64(runtime_value_t{.vector_ptr = vec}, runtime_value_t{.int_value = 0});
}

VEC_T* wide_return_to_vec(const WIDE_RETURN_T& ret){
	return ret.a.vector_ptr;
}






////////////////////////////////		DICT_T





DICT_T make_dict(){
	DICT_BODY_T* body_ptr = new DICT_BODY_T();
	DICT_T result;
	result.body_ptr = body_ptr;

	QUARK_ASSERT(result.check_invariant());
	return result;
}

void delete_dict(DICT_T& v){
	QUARK_ASSERT(v.check_invariant());

	delete v.body_ptr;
	v.body_ptr = nullptr;
}





llvm::StructType* make_dict_type(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	body_otr
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

/*
llvm::Value* generate_dict_alloca(llvm::IRBuilder<>& builder, llvm::Value* dict_reg){
	auto& context = builder.getContext();

	auto alloc_value = builder.CreateAlloca(make_dict_type(context));
	builder.CreateStore(dict_reg, alloc_value);
	return alloc_value;
}

llvm::Value* generate__convert_wide_return_to_dict(llvm::IRBuilder<>& builder, llvm::Value* wide_return_reg){
	auto& context = builder.getContext();

	auto wide_return_ptr_reg = builder.CreateAlloca(make_wide_return_type(context), nullptr, "temp_dict");
	builder.CreateStore(wide_return_reg, wide_return_ptr_reg);
	auto dict_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, wide_return_ptr_reg, make_dict_type(context)->getPointerTo(), "");
	auto dict_reg = builder.CreateLoad(dict_ptr_reg, "final");
	return dict_reg;
}
*/

WIDE_RETURN_T make_wide_return_dict(DICT_T* dict){
	return make_wide_return_2x64({ .dict_ptr = dict }, { .int_value = 0 });
//	return *reinterpret_cast<const WIDE_RETURN_T*>(&dict);
}

DICT_T* wide_return_to_dict(const WIDE_RETURN_T& ret){
	return ret.a.dict_ptr;
//	return *reinterpret_cast<const DICT_T*>(&ret);
}






////////////////////////////////		HELPERS




void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg){
	QUARK_ASSERT(array_ptr_reg.getType()->isPointerTy());
	QUARK_ASSERT(array_ptr_reg.getType()->isPointerTy());

	auto element_type = array_ptr_reg.getType()->getPointerElementType();

	QUARK_ASSERT(element_type == element_reg.getType());

	auto element_index_reg = llvm::ConstantInt::get(builder.getInt64Ty(), element_index);
	const auto gep = std::vector<llvm::Value*>{
		element_index_reg
	};
	llvm::Value* element_n_ptr = builder.CreateGEP(element_type, &array_ptr_reg, gep, "");
	builder.CreateStore(&element_reg, element_n_ptr);
}



void generate_struct_member_store(llvm::IRBuilder<>& builder, llvm::StructType& struct_type, llvm::Value& struct_ptr_reg, int member_index, llvm::Value& value_reg){

	const auto gep = std::vector<llvm::Value*>{
		//	Struct array index.
		builder.getInt32(0),

		//	Struct member index.
		builder.getInt32(member_index)
	};
	llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type, &struct_ptr_reg, gep, "");
	builder.CreateStore(&value_reg, member_ptr_reg);
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

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto ret = function_type.get_function_return();
	llvm::Type* return_type = ret.is_any() ? make_wide_return_type(context) : intern_type(context, ret);

	const auto args = function_type.get_function_args();
	std::vector<llvm_arg_mapping_t> arg_results;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation in Floyd function type.
	arg_results.push_back({ make_frp_type(context), "floyd_runtime_ptr", floyd::typeid_t::make_undefined(), -1, llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr });

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(arg.is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(arg.is_any()){
			arg_results.push_back({ make_runtime_value_type(context), std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_dyn_value });
			arg_results.push_back({ make_runtime_type_type(context), std::to_string(index), typeid_t::make_undefined(), index, llvm_arg_mapping_t::map_type::k_dyn_type });
		}
		else {
			auto arg_itype = intern_type(context, arg);
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
	auto& context = module->getContext();
	const auto r = map_function_arguments(
		context,
		typeid_t::make_function(typeid_t::make_void(), {}, epure::pure)
	);

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
	auto& context = module->getContext();
	const auto r = map_function_arguments(context, typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));

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
	auto& context = module->getContext();
	const auto r = map_function_arguments(context, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int() }, epure::pure));

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
	auto& context = module->getContext();
	const auto r = map_function_arguments(context, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int(), typeid_t::make_any(), typeid_t::make_bool() }, epure::pure));

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
	QUARK_UT_VERIFY(r.args[2].floyd_type.is_any());
	QUARK_UT_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_UT_VERIFY(r.args[3].llvm_type->isIntegerTy(32));
	QUARK_UT_VERIFY(r.args[3].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[3].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_UT_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_UT_VERIFY(r.args[4].floyd_name == "2");
	QUARK_UT_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_UT_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_UT_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_simple_value);
}






////////////////////////////////		intern_type()



//	Function-types are always returned as pointer-to-function types.
llvm::Type* make_function_type(llvm::LLVMContext& context, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments(context, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}


llvm::StructType* make_struct_type(llvm::LLVMContext& context, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto m2 = intern_type(context, m._type);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

//	Returns the LLVM type we chose to use to encode each Floyd type.
llvm::Type* intern_type(llvm::LLVMContext& context, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		llvm::LLVMContext& context;
		const typeid_t& type;

		llvm::Type* operator()(const typeid_t::undefined_t& e) const{
			return llvm::Type::getInt16Ty(context);
		}
		llvm::Type* operator()(const typeid_t::any_t& e) const{
			return make_runtime_value_type(context);
		}

		llvm::Type* operator()(const typeid_t::void_t& e) const{
			return llvm::Type::getVoidTy(context);
		}
		llvm::Type* operator()(const typeid_t::bool_t& e) const{
			return llvm::Type::getInt1Ty(context);
		}
		llvm::Type* operator()(const typeid_t::int_t& e) const{
			return llvm::Type::getInt64Ty(context);
		}
		llvm::Type* operator()(const typeid_t::double_t& e) const{
			return llvm::Type::getDoubleTy(context);
		}
		llvm::Type* operator()(const typeid_t::string_t& e) const{
			return llvm::Type::getInt8PtrTy(context);
		}

		llvm::Type* operator()(const typeid_t::json_type_t& e) const{
			return llvm::Type::getInt16PtrTy(context);
		}
		llvm::Type* operator()(const typeid_t::typeid_type_t& e) const{
			return make_runtime_type_type(context);
		}

		llvm::Type* operator()(const typeid_t::struct_t& e) const{
			return make_struct_type(context, type)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::vector_t& e) const{
			return make_vec_type(context)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::dict_t& e) const{
			return make_dict_type(context)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::function_t& e) const{
			return make_function_type(context, type);
		}
		llvm::Type* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
			return llvm::Type::getInt16Ty(context);
		}
	};
	return std::visit(visitor_t{ context, type }, type._contents);
}

// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
runtime_value_t load_via_ptr2(const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const void* value_ptr;

		runtime_value_t operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		runtime_value_t operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::bool_t& e) const{
			const auto temp = *static_cast<const uint8_t*>(value_ptr);
			return runtime_value_t{ .bool_value = temp };
		}
		runtime_value_t operator()(const typeid_t::int_t& e) const{
			const auto temp = *static_cast<const uint64_t*>(value_ptr);
			return make_runtime_int(temp);
		}
		runtime_value_t operator()(const typeid_t::double_t& e) const{
			const auto temp = *static_cast<const double*>(value_ptr);
			return runtime_value_t{ .double_value = temp };
		}
		runtime_value_t operator()(const typeid_t::string_t& e) const{
			char* s = *(char**)(value_ptr);
			return runtime_value_t{ .string_ptr = s };
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
			json_t* json_ptr = *(json_t**)(value_ptr);
			return runtime_value_t{ .json_ptr = json_ptr };
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto value = *static_cast<const int32_t*>(value_ptr);
			return runtime_value_t{ .typeid_itype = value };
		}

		runtime_value_t operator()(const typeid_t::struct_t& e) const{
			void* struct_ptr = *reinterpret_cast<void* const *>(value_ptr);
			return make_runtime_struct(struct_ptr);
		}
		runtime_value_t operator()(const typeid_t::vector_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::dict_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::function_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ value_ptr }, type._contents);
}

// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
void store_via_ptr2(void* value_ptr, const typeid_t& type, const runtime_value_t& value){
	struct visitor_t {
		void* value_ptr;
		const runtime_value_t& value;

		void operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		void operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const typeid_t::bool_t& e) const{
			*static_cast<uint8_t*>(value_ptr) = value.bool_value;
		}
		void operator()(const typeid_t::int_t& e) const{
			*(int64_t*)value_ptr = value.int_value;
		}
		void operator()(const typeid_t::double_t& e) const{
			*static_cast<double*>(value_ptr) = value.double_value;
		}
		void operator()(const typeid_t::string_t& e) const{
			*(char**)(value_ptr) = value.string_ptr;
		}

		void operator()(const typeid_t::json_type_t& e) const{
			*(json_t**)(value_ptr) = value.json_ptr;
		}
		void operator()(const typeid_t::typeid_type_t& e) const{
			*static_cast<int32_t*>(value_ptr) = value.typeid_itype;
		}

		void operator()(const typeid_t::struct_t& e) const{
			*reinterpret_cast<void**>(value_ptr) = value.struct_ptr;
		}
		void operator()(const typeid_t::vector_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::dict_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::function_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	std::visit(visitor_t{ value_ptr, value }, type._contents);
}

llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::LLVMContext& context;
		llvm::IRBuilder<>& builder;
		llvm::Value& value;

		llvm::Value* operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		llvm::Value* operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::bool_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(context), "bool_as_arg");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &value;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, make_runtime_value_type(context), "double_as_arg");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "string_as_arg");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "json_as_arg");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(context), "typeid_as_arg");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "function_as_arg");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ context, builder, value }, floyd_type._contents);
}

//??? use visit()!
llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, llvm::Value& runtime_value_reg, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = builder.getContext();

	//??? This function is tightly coupled to intern_type().
	if(type.is_int()){
		return &runtime_value_reg;
	}
	else if(type.is_bool()){
		return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt1Ty(context), "");
	}
	else if(type.is_double()){
		return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &runtime_value_reg, llvm::Type::getDoubleTy(context), "");
	}
	else if(type.is_typeid()){
		return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt32Ty(context), "");
	}
	else if(type.is_string()){
		return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, llvm::Type::getInt8PtrTy(context), "");
	}
	else if(type.is_json_value()){
		return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, llvm::Type::getInt16PtrTy(context), "");
	}
	else if(type.is_struct()){
		auto& struct_type = *make_struct_type(context, type);
		return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, struct_type.getPointerTo(), "");
	}
	else if(type.is_vector()){
		return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_vec_type(context)->getPointerTo(), "");
	}
	else if(type.is_dict()){
		return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_dict_type(context)->getPointerTo(), "");
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}


}	//	floyd

