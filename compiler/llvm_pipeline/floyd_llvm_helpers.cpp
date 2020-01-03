//
//  floyd_llvm_helpers.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_helpers.h"

#include <llvm/IR/Verifier.h>

#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <string>
#include <vector>

#include "floyd_llvm_types.h"

#include "quark.h"


namespace floyd {


bool target_t::check_invariant() const{
	QUARK_ASSERT(target_machine != nullptr);
	return true;
}

llvm_instance_t::llvm_instance_t(){
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	target = make_default_target();

	QUARK_ASSERT(check_invariant());
}

bool llvm_instance_t::check_invariant() const {
	QUARK_ASSERT(target.check_invariant());
	return true;
}

target_t make_default_target(){
	auto TargetTriple = llvm::sys::getDefaultTargetTriple();

	std::string Error;
	auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

	// This generally occurs if we've forgotten to initialise the
	// TargetRegistry or we have a bogus target triple.
	if (!Target) {
		throw std::exception();
	}

	auto CPU = "generic";
	auto Features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	//	Cannot copy DataLayout or put in shared_ptr.
//	auto data_layout = TargetMachine->createDataLayout();
	return target_t { TargetTriple, TargetMachine };
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
		QUARK_TRACE_SS("\n" << stream2.str());

		quark::throw_defective_request();
	}
	return !errors;
}

bool check_invariant__module(llvm::Module* module){
	QUARK_ASSERT(module != nullptr);

	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	bool module_errors_flag = llvm::verifyModule(*module, &stream2, nullptr);
	if(module_errors_flag){
		QUARK_TRACE_SS(stream2.str());

		const auto& functions = module->getFunctionList();
		for(const auto& e: functions){
			QUARK_ASSERT(check_invariant__function(&e));
			llvm::verifyFunction(e);
		}

		quark::throw_defective_request();
	}

	return true;
}

bool check_invariant__builder(llvm::IRBuilder<>* builder){
	QUARK_ASSERT(builder != nullptr);

	auto module = builder->GetInsertBlock()->getParent()->getParent();
	QUARK_ASSERT(check_invariant__module(module));
	return true;
}


static bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

//	We don't fix up the first occurence of declare since that would move it in contact with what's before it.
static std::string reformat_llvm_module_print(const std::string& s0){
	auto temp = s0;
	const auto from = std::string("\n\ndeclare ");
	const auto to = std::string("\ndeclare ");

	const auto first_pos = temp.find("\n\ndeclare", 0);

	//	+1 is to avoid replacing first occurance.
	size_t start_pos = first_pos + 1;

    while((start_pos = temp.find(from, start_pos)) != std::string::npos) {
        temp.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
	return temp;
}

std::string print_module(llvm::Module& module){
	std::string dump;
	llvm::raw_string_ostream stream2(dump);

//	stream2 << "\n" "MODULE" << "\n";
	module.print(stream2, nullptr);

	//	Printout of declare-lines have an extra newline, let's remove that extra newline for more compact printout.

	auto r = reformat_llvm_module_print(stream2.str());

/*
	Not needed, module.print() prints the exact list.
	stream2 << "\n" "FUNCTIONS" << "\n";
	const auto& functionList = module.getFunctionList();
	for(const auto& e: functionList){
		e.print(stream2);
	}
*/

/*
??? This is already part of module.print()

	stream2 << "\n" "GLOBALS" << "\n";
	const auto& globalList = module.getGlobalList();
	int index = 0;
	for(const auto& e: globalList){
		stream2 << index << ": ";
		e.print(stream2);
		stream2 << "\n";
		index++;
	}
*/

	return r;
}


std::string print_type(llvm::Type* type){
	if(type == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		type->print(rso);

		if(type->isStructTy()){
			std::string name = type->getStructName();
			rso << name;
		}

//		std::cout<<rso.str();
		return rso.str();
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

static std::string print_value0(llvm::Value* value){
	if(value == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		value->print(rso);
//		std::cout<<rso.str();
		return rso.str();
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



bool check_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(type_lookup.check_invariant());

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	QUARK_ASSERT(floyd_context_arg_ptr->getType() == make_frp_type(type_lookup));
	return true;
}

bool check_emitting_function(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(type_lookup.check_invariant());

	QUARK_ASSERT(check_callers_fcp(type_lookup, emit_f));
	return true;
}

llvm::Value* get_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(check_callers_fcp(type_lookup, emit_f));

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	return floyd_context_arg_ptr;
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



////////////////////////////////		VALUES



llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const type_t& floyd_type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		const llvm_type_lookup& type_lookup;
		llvm::Value& value;
		const type_t type;

		llvm::Value* operator()(const undefined_t& e) const{
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const any_t& e) const{
			quark::throw_defective_request();
		}

		llvm::Value* operator()(const void_t& e) const{
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const bool_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const int_t& e) const{
			return &value;
		}
		llvm::Value* operator()(const double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const symbol_ref_t& e) const {
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const named_type_t& e) const {
			return generate_cast_to_runtime_value2(builder, type_lookup, value, peek2(type_lookup.state.types, type));
		}
	};
	return std::visit(visitor_t{ builder, context, type_lookup, value, floyd_type }, get_type_variant(type_lookup.state.types, floyd_type));
}

llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const type_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		const llvm_type_lookup& type_lookup;
		llvm::LLVMContext& context;
		llvm::Value& runtime_value_reg;
		const type_t& type;

		llvm::Value* operator()(const undefined_t& e) const{
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const any_t& e) const{
			quark::throw_defective_request();
		}

		llvm::Value* operator()(const void_t& e) const{
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const bool_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt1Ty(context), "");
		}
		llvm::Value* operator()(const int_t& e) const{
			return &runtime_value_reg;
		}
		llvm::Value* operator()(const double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &runtime_value_reg, llvm::Type::getDoubleTy(context), "");
		}
		llvm::Value* operator()(const string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type_byvalue(type_lookup)->getPointerTo(), "");
		}

		llvm::Value* operator()(const json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_json_type_byvalue(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt32Ty(context), "");
		}

		llvm::Value* operator()(const struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_generic_struct_type_byvalue(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type_byvalue(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_dict_type_byvalue(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const function_t& e) const{
			// ??? Use get_llvm_function_type()
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_llvm_type_as_arg(type_lookup, type), "");
		}
		llvm::Value* operator()(const symbol_ref_t& e) const {
			quark::throw_defective_request();
		}
		llvm::Value* operator()(const named_type_t& e) const {
			return generate_cast_from_runtime_value2(builder, type_lookup, runtime_value_reg, peek2(type_lookup.state.types, type));
		}
	};
	return std::visit(visitor_t{ builder, type_lookup, context, runtime_value_reg, type }, get_type_variant(type_lookup.state.types, type) );
}

}	//	floyd

