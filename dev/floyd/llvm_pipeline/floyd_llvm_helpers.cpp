//
//  floyd_llvm_helpers.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_helpers.h"

#include <llvm/IR/Verifier.h>

#include <string>
#include <vector>

#include "floyd_llvm_types.h"
#include "floyd_llvm_value_thunking.h"

#include "quark.h"


namespace floyd {


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
		QUARK_TRACE_SS(stream2.str());

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

	return stream2.str();
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

std::string print_value0(llvm::Value* value){
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





llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr){
//	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(symbol_name.empty() == false);

	llvm::GlobalVariable* gv = new llvm::GlobalVariable(
		module,
		&itype,
		false,	//	isConstant
		llvm::GlobalValue::ExternalLinkage,
		init_or_nullptr ? init_or_nullptr : llvm::Constant::getNullValue(&itype),
		symbol_name
	);

//	QUARK_ASSERT(check_invariant__module(&module));

	return gv;
}



static const std::string k_floyd_func_link_prefix = "floydf_";


//	"hello" => "floyd_f_hello"
link_name_t encode_floyd_func_link_name(const std::string& name){
	return link_name_t { k_floyd_func_link_prefix + name };
}
std::string decode_floyd_func_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.s.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}


static const std::string k_runtime_func_link_prefix = "floydrt_";

//	"hello" => "floyd_rt_hello"
link_name_t encode_runtime_func_link_name(const std::string& name){
	return link_name_t { k_runtime_func_link_prefix + name };
}

std::string decode_runtime_func_link_name(const link_name_t& name){
	const auto left = name.s.substr(0, k_runtime_func_link_prefix.size());
	const auto right = name.s.substr(k_runtime_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_runtime_func_link_prefix);
	return right;
}








////////////////////////////////		VALUES




VECTOR_CPPVECTOR_T* unpack_vector_cppvector_arg(const value_mgr_t& value_mgr, runtime_value_t arg_value, runtime_type_t arg_type){
	QUARK_ASSERT(value_mgr.check_invariant());
#if DEBUG
	const auto type = lookup_type(value_mgr, arg_type);
#endif
	QUARK_ASSERT(type.is_vector());
	QUARK_ASSERT(arg_value.vector_cppvector_ptr != nullptr);
	QUARK_ASSERT(arg_value.vector_cppvector_ptr->check_invariant());

	return arg_value.vector_cppvector_ptr;
}

DICT_CPPMAP_T* unpack_dict_cppmap_arg(const value_mgr_t& value_mgr, runtime_value_t arg_value, runtime_type_t arg_type){
	QUARK_ASSERT(value_mgr.check_invariant());
#if DEBUG
	const auto type = lookup_type(value_mgr, arg_type);
#endif
	QUARK_ASSERT(type.is_dict());
	QUARK_ASSERT(arg_value.dict_cppmap_ptr != nullptr);

	QUARK_ASSERT(arg_value.dict_cppmap_ptr->check_invariant());

	return arg_value.dict_cppmap_ptr;
}
llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		const llvm_type_lookup& type_lookup;
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
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &value;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ builder, context, type_lookup, value }, floyd_type._contents);
}

llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		const llvm_type_lookup& type_lookup;
		llvm::LLVMContext& context;
		llvm::Value& runtime_value_reg;
		const typeid_t& type;

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
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt1Ty(context), "");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &runtime_value_reg;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &runtime_value_reg, llvm::Type::getDoubleTy(context), "");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type(type_lookup)->getPointerTo(), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_json_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt32Ty(context), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_generic_struct_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_dict_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_exact_llvm_type(type_lookup, type), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ builder, type_lookup, context, runtime_value_reg, type }, type._contents);
}

}	//	floyd

