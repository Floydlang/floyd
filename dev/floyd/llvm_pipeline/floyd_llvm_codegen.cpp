//
//  floyd_llvm_codegen.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

const bool k_trace_input_output = false;
const bool k_trace_types = k_trace_input_output;


#include "floyd_llvm_codegen.h"

#include "floyd_llvm_runtime.h"
#include "floyd_llvm_helpers.h"

#include "ast_value.h"

#include "semantic_ast.h"

#include "quark.h"
#include "floyd_runtime.h"


//#include <llvm/ADT/APInt.h>
//#include <llvm/IR/Verifier.h>
//#include <llvm/ExecutionEngine/ExecutionEngine.h>
//#include <llvm/ExecutionEngine/GenericValue.h>
//#include <llvm/ExecutionEngine/MCJIT.h>
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

#include "llvm/Bitcode/BitstreamWriter.h"

#include <map>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>


//http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html




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

struct llvm_code_generator_t;




////////////////////////////////		llvm_code_generator_t


struct llvm_code_generator_t {
	public: llvm_code_generator_t(llvm_instance_t& instance, llvm::Module* module, const type_interner_t& interner, const llvm_type_lookup& type_lookup, const std::vector<function_def_t>& function_defs) :
		instance(&instance),
		module(module),
		builder(instance.context),
		type_lookup(type_lookup),
		function_defs(function_defs),

		floydrt_init(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("init"))),
		floydrt_deinit(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("deinit"))),


		floydrt_retain_vec(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("retain_vec"))),
//		floydrt_retain_hamt_vec(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("retain_hamt_vec"))),
		floydrt_retain_dict(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("retain_dict"))),
		floydrt_retain_json(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("retain_json"))),
		floydrt_retain_struct(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("retain_struct"))),

		floydrt_release_vec(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("release_vec"))),
//		floydrt_release_hamt_vec(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("release_hamt_vec"))),
		floydrt_release_dict(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("release_dict"))),
		floydrt_release_json(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("release_json"))),
		floydrt_release_struct(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("release_struct"))),

		floydrt_alloc_kstr(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("alloc_kstr"))),
		floydrt_update_struct_member(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("update_struct_member"))),
		floydrt_lookup_json(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("lookup_json"))),
		floydrt_lookup_dict(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("lookup_dict"))),
		floydrt_concatunate_vectors(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("concatunate_vectors"))),
//		floydrt_concatunate_hamt_vectors(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("concatunate_hamt_vectors"))),
		floydrt_compare_values(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("compare_values"))),
		floydrt_allocate_vector(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("allocate_vector"))),
//		floydrt_allocate_hamt_vector(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("allocate_hamt_vector"))),
		floydrt_allocate_dict(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("allocate_dict"))),
		floydrt_store_dict_mutable(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("store_dict_mutable"))),
		floydrt_allocate_struct(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("allocate_struct"))),
		floydrt_json_to_string(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("json_to_string"))),
		floydrt_allocate_json(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("allocate_json"))),
		floydrt_get_profile_time(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("get_profile_time"))),
		floydrt_analyse_benchmark_samples(find_function_def_from_link_name(function_defs, encode_runtime_func_link_name("analyse_benchmark_samples")))
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

	const function_def_t& floydrt_init;
	const function_def_t& floydrt_deinit;

	const function_def_t& floydrt_retain_vec;
//	const function_def_t& floydrt_retain_hamt_vec;
	const function_def_t& floydrt_retain_dict;
	const function_def_t& floydrt_retain_json;
	const function_def_t& floydrt_retain_struct;

	const function_def_t& floydrt_release_vec;
//	const function_def_t& floydrt_release_hamt_vec;
	const function_def_t& floydrt_release_dict;
	const function_def_t& floydrt_release_json;
	const function_def_t& floydrt_release_struct;

	const function_def_t& floydrt_alloc_kstr;
	const function_def_t& floydrt_update_struct_member;
	const function_def_t& floydrt_lookup_json;
	const function_def_t& floydrt_lookup_dict;
	const function_def_t& floydrt_concatunate_vectors;
//	const function_def_t& floydrt_concatunate_hamt_vectors;
	const function_def_t& floydrt_compare_values;
	const function_def_t& floydrt_allocate_vector;
//	const function_def_t& floydrt_allocate_hamt_vector;
	const function_def_t& floydrt_allocate_dict;
	const function_def_t& floydrt_store_dict_mutable;
	const function_def_t& floydrt_allocate_struct;
	const function_def_t& floydrt_json_to_string;
	const function_def_t& floydrt_allocate_json;
	const function_def_t& floydrt_get_profile_time;
	const function_def_t& floydrt_analyse_benchmark_samples;
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


enum class function_return_mode {
	some_path_not_returned,
	return_executed
};

static function_return_mode generate_statements(llvm_function_generator_t& gen_acc, const std::vector<statement_t>& statements);
static llvm::Value* generate_expression(llvm_function_generator_t& gen_acc, const expression_t& e);





////////////////////////////////		DEBUG




std::string print_resolved_symbols(const std::vector<resolved_symbol_t>& globals){
	std::stringstream out;

	out << "{" << std::endl;
	for(const auto& e: globals){
		out << "{ " << print_value(e.value_ptr) << " : " << e.debug_str << " }" << std::endl;
	}

	return out.str();
}

//	Print all symbols in scope_path
std::string print_gen(const llvm_code_generator_t& gen){
//	QUARK_ASSERT(gen.check_invariant());

	std::stringstream out;

	out << "--------------------------------------------------------------------------------" << std::endl;
	out << "module:"
		<< print_module(*gen.module)
		<< std::endl

	<< "scope_path" << std::endl;

	int index = 0;
	for(const auto& e: gen.scope_path){
		out << "--------------------------------------------------------------------------------" << std::endl;
		out << "scope #" << index << std::endl;
		out << print_resolved_symbols(e);
		index++;
	}


	return out.str();
}

std::string print_program(const llvm_ir_program_t& program){
	QUARK_ASSERT(program.check_invariant());

//	std::string dump;
//	llvm::raw_string_ostream stream2(dump);
//	program.module->print(stream2, nullptr);

	return print_module(*program.module);
}




////////////////////////////////		PRIMITIVES



static llvm::Value* generate_cast_to_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	return generate_cast_to_runtime_value2(builder, gen_acc.type_lookup, value, floyd_type);
}

static llvm::Value* generate_cast_from_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& runtime_value_reg, const typeid_t& type){
	auto& builder = gen_acc.get_builder();
	return generate_cast_from_runtime_value2(builder, gen_acc.type_lookup, runtime_value_reg, type);
}



resolved_symbol_t make_resolved_symbol(llvm::Value* value_ptr, std::string debug_str, resolved_symbol_t::esymtype t, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(value_ptr != nullptr);

	return { value_ptr, debug_str, t, symbol_name, symbol};
}

resolved_symbol_t find_symbol(llvm_code_generator_t& gen_acc, const variable_address_t& reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(gen_acc.scope_path.size() >= 1);
	QUARK_ASSERT(reg._parent_steps == -1 || (reg._parent_steps >= 0 && reg._parent_steps < gen_acc.scope_path.size()));

	if(reg._parent_steps == -1){
		QUARK_ASSERT(gen_acc.scope_path.size() >= 1);

		const auto& global_scope = gen_acc.scope_path.front();
		QUARK_ASSERT(reg._index >= 0 && reg._index < global_scope.size());

		return global_scope[reg._index];
	}
	else {
		//	0 == back().
		const auto scope_index = gen_acc.scope_path.size() - reg._parent_steps - 1;
		QUARK_ASSERT(scope_index >= 0 && scope_index < gen_acc.scope_path.size());

		const auto& scope = gen_acc.scope_path[scope_index];
		QUARK_ASSERT(reg._index >= 0 && reg._index < scope.size());

		return scope[reg._index];
	}
}


typeid_t unpack_itype(const llvm_code_generator_t& gen_acc, int64_t itype){
	QUARK_ASSERT(gen_acc.check_invariant());

	const itype_t t(static_cast<uint32_t>(itype));
	return lookup_type(gen_acc.type_lookup,  t);
}

int64_t pack_itype(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return lookup_itype(gen_acc.type_lookup, type).itype;
}

llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto itype = pack_itype(gen_acc, type);
	auto t = make_runtime_type_type(gen_acc.type_lookup);
 	return llvm::ConstantInt::get(t, itype);
}




void generate_retain(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& builder = gen_acc.get_builder();
	if(is_rc_value(type)){
		if(type.is_string() || type.is_vector()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_retain_vec.llvm_codegen_f, args, "");
		}
		else if(type.is_dict()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_retain_dict.llvm_codegen_f, args, "");
		}
		else if(type.is_json()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_retain_json.llvm_codegen_f, args, "");
		}
		else if(type.is_struct()){
			auto generic_vec_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value_reg, get_generic_struct_type(gen_acc.gen.type_lookup)->getPointerTo(), "");
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				generic_vec_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_retain_struct.llvm_codegen_f, args, "");
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}
void generate_release(llvm_function_generator_t& gen_acc, llvm::Value& value_reg, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& builder = gen_acc.get_builder();
	if(is_rc_value(type)){
		if(type.is_string() || type.is_vector()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_release_vec.llvm_codegen_f, args);
		}
		else if(type.is_dict()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_release_dict.llvm_codegen_f, args);
		}
		else if(type.is_json()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_release_json.llvm_codegen_f, args);
		}
		else if(type.is_struct()){
			std::vector<llvm::Value*> args = {
				gen_acc.get_callers_fcp(),
				&value_reg,
				generate_itype_constant(gen_acc.gen, type)
			};
			builder.CreateCall(gen_acc.gen.floydrt_release_struct.llvm_codegen_f, args);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////







/*
	Put the string characters in a LLVM constant,
	generate code that instantiates a VEC_T holding a *copy* of the characters.

	FUTURE: point directly to code segment chars, don't free() on VEC_t destruct.
	FUTURE: Here we construct a new VEC_T instance everytime. We could do it *once* instead and make a global const out of it.
*/
llvm::Value* generate_constant_string(llvm_function_generator_t& gen_acc, const std::string& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	//	Make a global string constant.
	llvm::Constant* str_ptr = builder.CreateGlobalStringPtr(s);
	llvm::Constant* str_size = llvm::ConstantInt::get(builder.getInt64Ty(), s.size());

	std::vector<llvm::Value*> args = {
		gen_acc.get_callers_fcp(),
		str_ptr,
		str_size
	};
	auto string_vec_ptr_reg = builder.CreateCall(gen_acc.gen.floydrt_alloc_kstr.llvm_codegen_f, args, "string literal");
	return string_vec_ptr_reg;
};


//	Makes constant from a Floyd value. The constant may go in code segment or be computed at app init-time.
static llvm::Value* generate_constant(llvm_function_generator_t& gen_acc, const value_t& value){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto type = value.get_type();
	const auto itype = get_exact_llvm_type(gen_acc.gen.type_lookup, type);

	struct visitor_t {
		llvm_function_generator_t& gen_acc;
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		llvm::Type* itype;
		const value_t& value;


		llvm::Value* operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
			return llvm::ConstantInt::get(itype, 17);
		}
		llvm::Value* operator()(const typeid_t::any_t& e) const{
			return llvm::ConstantInt::get(itype, 13);
		}

		llvm::Value* operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::bool_t& e) const{
			return llvm::ConstantInt::get(itype, value.get_bool_value() ? 1 : 0);
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return llvm::ConstantInt::get(itype, value.get_int_value());
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return llvm::ConstantFP::get(itype, value.get_double_value());
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return generate_constant_string(gen_acc, value.get_string_value());
		}
		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			const auto& json0 = value.get_json();

			//	NOTICE: There is no clean way to embedd a json containing a json-null into the code segment.
			//	Here we use a nullptr instead of json_t*. This means we have to be prepared for json_t::null AND nullptr.
			if(json0.is_null()){
				auto json_type = get_exact_llvm_type(gen_acc.gen.type_lookup, typeid_t::make_json());
				llvm::PointerType* pointer_type = llvm::cast<llvm::PointerType>(json_type);
				return llvm::ConstantPointerNull::get(pointer_type);
			}
			else{
				UNSUPPORTED();
			}
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return generate_itype_constant(gen_acc.gen, value.get_typeid_value());
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::function_t& e2) const{
			const auto function_id = value.get_function_value();
			for(const auto& e: gen_acc.gen.function_defs){
				if(e.floyd_fundef._definition_name == function_id.name){
					return e.llvm_codegen_f;
				}
			}
			llvm::PointerType* ptr_type = llvm::cast<llvm::PointerType>(itype);
			return llvm::ConstantPointerNull::get(ptr_type);
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ gen_acc, builder, context, itype, value }, type._contents);
}


static std::vector<resolved_symbol_t> generate_symbol_slots(llvm_function_generator_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = get_exact_llvm_type(gen_acc.gen.type_lookup, type);

		//	Reserve stack slot for each local.
		llvm::Value* dest = gen_acc.get_builder().CreateAlloca(itype, nullptr, e.first);

		//	Init the slot if needed.
		if(e.second._init.is_undefined() == false){
			llvm::Value* c = generate_constant(gen_acc, e.second._init);
			gen_acc.get_builder().CreateStore(c, dest);
		}
		const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
	}
	return result;
}

static void generate_destruct_scope_locals(llvm_function_generator_t& gen_acc, const std::vector<resolved_symbol_t>& symbols){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	for(const auto& e: symbols){
		if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
			const auto type = e.symbol.get_type();
			if(is_rc_value(type)){
				auto reg = builder.CreateLoad(e.value_ptr);
				generate_release(gen_acc, *reg, type);
			}
			else{
			}
		}
	}
}



static function_return_mode generate_body(llvm_function_generator_t& gen_acc, const std::vector<resolved_symbol_t>& resolved_symbols, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());

	gen_acc.gen.scope_path.push_back(resolved_symbols);
	const auto return_mode = generate_statements(gen_acc, statements);
	gen_acc.gen.scope_path.pop_back();

	//	Destruct body's locals. Unless return_executed.
	if(return_mode == function_return_mode::some_path_not_returned){
		generate_destruct_scope_locals(gen_acc, resolved_symbols);
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return return_mode;
}


static function_return_mode generate_block(llvm_function_generator_t& gen_acc, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto values = generate_symbol_slots(gen_acc, body._symbol_table);
	const auto return_mode = generate_body(gen_acc, values, body._statements);

	QUARK_ASSERT(gen_acc.check_invariant());
	return return_mode;
}

//	Returns pointer to first element of data after the alloc64. The returned pointer-type is struct { unit64_t x 8 }, so it needs to be cast to an element-ptr.
static llvm::Value* generate_get_vec_element_ptr_needs_cast(llvm_function_generator_t& gen_acc, llvm::Value& vec_ptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto after_alloc64_ptr_reg = builder.CreateGEP(make_generic_vec_type(gen_acc.gen.type_lookup), &vec_ptr_reg, gep, "");
	return after_alloc64_ptr_reg;
}

//	Returns pointer to the first byte of the first struct member.
static llvm::Value* generate_get_struct_base_ptr(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const typeid_t& final_type){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto type = get_generic_struct_type(gen_acc.gen.type_lookup);
	auto ptr_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, &struct_ptr_reg, type->getPointerTo(), "");

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto ptr2_reg = builder.CreateGEP(type, ptr_reg, gep, "");
	auto final_type2 = get_exact_struct_type(gen_acc.gen.type_lookup, final_type);
	auto ptr3_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, ptr2_reg, final_type2->getPointerTo(), "");
	return ptr3_reg;
}




////////////////////////////////		EXPRESSIONS



static llvm::Value* generate_literal_expression(llvm_function_generator_t& gen_acc, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto literal = e.get_literal();
	return generate_constant(gen_acc, literal);
}

static llvm::Value* generate_resolve_member_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::resolve_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto parent_struct_ptr_reg = generate_expression(gen_acc, *details.parent_address);

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_struct()){
		auto& struct_type_llvm = *get_exact_struct_type(gen_acc.gen.type_lookup, parent_type);

		const auto& struct_def = details.parent_address->get_output_type().get_struct();
		int member_index = find_struct_member_index(struct_def, details.member_name);
		QUARK_ASSERT(member_index != -1);

		const auto& member_type = struct_def._members[member_index]._type;

		auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, *parent_struct_ptr_reg, parent_type);

		const auto gep = std::vector<llvm::Value*>{
			//	Struct array index.
			builder.getInt32(0),

			//	Struct member index.
			builder.getInt32(member_index)
		};
		llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, base_ptr_reg, gep, "");
		auto member_value_reg = builder.CreateLoad(member_ptr_reg);
		generate_retain(gen_acc, *member_value_reg, member_type);
		generate_release(gen_acc, *parent_struct_ptr_reg, parent_type);

		return member_value_reg;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}

static llvm::Value* generate_update_member_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::update_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto parent_struct_ptr_reg = generate_expression(gen_acc, *details.parent_address);
	auto new_value_reg = generate_expression(gen_acc, *details.new_value);

	const auto struct_type = details.parent_address->get_output_type();
	auto member_index_reg = llvm::ConstantInt::get(builder.getInt64Ty(), details.member_index);
	const auto member_type = details.new_value->get_output_type();

	std::vector<llvm::Value*> args2 = {
		gen_acc.get_callers_fcp(),

		parent_struct_ptr_reg,
		generate_itype_constant(gen_acc.gen, struct_type),

		member_index_reg,

		generate_cast_to_runtime_value(gen_acc.gen, *new_value_reg, member_type),
		generate_itype_constant(gen_acc.gen, member_type)
	};
	auto struct2_ptr_reg = builder.CreateCall(gen_acc.gen.floydrt_update_struct_member.llvm_codegen_f, args2, "");

	generate_release(gen_acc, *new_value_reg, member_type);
	generate_release(gen_acc, *parent_struct_ptr_reg, struct_type);

	return struct2_ptr_reg;
}

static llvm::Value* generate_lookup_element_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	auto parent_reg = generate_expression(gen_acc, *details.parent_address);
	auto key_reg = generate_expression(gen_acc, *details.lookup_key);
	const auto key_type = details.lookup_key->get_output_type();

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_string()){
		QUARK_ASSERT(key_type.is_int());

		auto element_ptr_reg = generate_get_vec_element_ptr_needs_cast(gen_acc, *parent_reg);
		auto char_ptr_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, element_ptr_reg, builder.getInt8PtrTy(), "");

		const auto gep = std::vector<llvm::Value*>{ key_reg };
		llvm::Value* element_addr = builder.CreateGEP(llvm::Type::getInt8Ty(context), char_ptr_reg, gep, "element_addr");
		llvm::Value* value_8bit_reg = builder.CreateLoad(element_addr, "element_tmp");
		llvm::Value* element_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::SExt, value_8bit_reg, builder.getInt64Ty(), "char_to_int64");

		generate_release(gen_acc, *parent_reg, parent_type);

		//	No need to retain/release the element - it's an integer.

		return element_reg;
	}
	else if(parent_type.is_json()){
		QUARK_ASSERT(key_type.is_int() || key_type.is_string());

		//	Notice that we only know at RUNTIME if the json can be looked up: it needs to be json-object or a
		//	json-array. The key is either a string or an integer.

		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),
			parent_reg,
			generate_cast_to_runtime_value(gen_acc.gen, *key_reg, key_type),
			generate_itype_constant(gen_acc.gen, key_type)
		};
		auto result = builder.CreateCall(gen_acc.gen.floydrt_lookup_json.llvm_codegen_f, args, "");

		generate_release(gen_acc, *parent_reg, parent_type);
		generate_release(gen_acc, *key_reg, key_type);
		return result;
	}
	else if(parent_type.is_vector()){
		QUARK_ASSERT(key_type.is_int());

		const auto element_type0 = parent_type.get_vector_element_type();
		auto ptr_reg = generate_get_vec_element_ptr_needs_cast(gen_acc, *parent_reg);
		auto int64_ptr_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, ptr_reg, builder.getInt64Ty()->getPointerTo(), "");

		const auto gep = std::vector<llvm::Value*>{ key_reg };
		llvm::Value* element_addr_reg = builder.CreateGEP(builder.getInt64Ty(), int64_ptr_reg, gep, "element_addr");
		llvm::Value* element_value_uint64_reg = builder.CreateLoad(element_addr_reg, "element_tmp");
		auto result_reg = generate_cast_from_runtime_value(gen_acc.gen, *element_value_uint64_reg, element_type0);

		generate_retain(gen_acc, *result_reg, element_type0);
		generate_release(gen_acc, *parent_reg, parent_type);

		return result_reg;
	}
	else if(parent_type.is_dict()){
		QUARK_ASSERT(key_type.is_string());

		const auto element_type0 = parent_type.get_dict_value_type();

		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			parent_reg,
			key_reg
		};
		auto element_value_uint64_reg = builder.CreateCall(gen_acc.gen.floydrt_lookup_dict.llvm_codegen_f, args2, "");
		auto result_reg = generate_cast_from_runtime_value(gen_acc.gen, *element_value_uint64_reg, element_type0);

		generate_retain(gen_acc, *result_reg, element_type0);
		generate_release(gen_acc, *parent_reg, parent_type);
		generate_release(gen_acc, *key_reg, key_type);

		return result_reg;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}


static llvm::Value* generate_arithmetic_expression(llvm_function_generator_t& gen_acc, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.lhs->get_output_type();

	auto lhs_temp = generate_expression(gen_acc, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, *details.rhs);

	if(type.is_bool()){
		if(details.op == expression_type::k_arithmetic_add){
			return gen_acc.get_builder().CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract){
		}
		else if(details.op == expression_type::k_arithmetic_multiply){
		}
		else if(details.op == expression_type::k_arithmetic_divide){
		}
		else if(details.op == expression_type::k_arithmetic_remainder){
		}

		else if(details.op == expression_type::k_logical_and){
			return gen_acc.get_builder().CreateAnd(lhs_temp, rhs_temp, "logical_and_tmp");
		}
		else if(details.op == expression_type::k_logical_or){
			return gen_acc.get_builder().CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else{
			UNSUPPORTED();
		}
	}
	else if(type.is_int()){
		if(details.op == expression_type::k_arithmetic_add){
			return gen_acc.get_builder().CreateAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract){
			return gen_acc.get_builder().CreateSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_multiply){
			return gen_acc.get_builder().CreateMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_divide){
			return gen_acc.get_builder().CreateSDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_remainder){
			return gen_acc.get_builder().CreateSRem(lhs_temp, rhs_temp, "reminder_tmp");
		}

		else if(details.op == expression_type::k_logical_and){
			return gen_acc.get_builder().CreateAnd(lhs_temp, rhs_temp, "logical_and_tmp");
		}
		else if(details.op == expression_type::k_logical_or){
			return gen_acc.get_builder().CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else{
			UNSUPPORTED();
		}
	}
	else if(type.is_double()){
		if(details.op == expression_type::k_arithmetic_add){
			return gen_acc.get_builder().CreateFAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract){
			return gen_acc.get_builder().CreateFSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_multiply){
			return gen_acc.get_builder().CreateFMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_divide){
			return gen_acc.get_builder().CreateFDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_remainder){
			UNSUPPORTED();
		}
		else if(details.op == expression_type::k_logical_and){
			UNSUPPORTED();
		}
		else if(details.op == expression_type::k_logical_or){
			UNSUPPORTED();
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(type.is_string() || type.is_vector()){
		//	Only add supported. Future: don't use arithmetic add to concat collections!
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add);

		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			generate_itype_constant(gen_acc.gen, type),
			lhs_temp,
			rhs_temp
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.floydrt_concatunate_vectors.llvm_codegen_f, args2, "");
		generate_release(gen_acc, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, *rhs_temp, *details.rhs->_output_type);
		return result;
	}
	else{
		//	No other types allowed.
		throw std::exception();
	}
	UNSUPPORTED();
}



static llvm::Value* generate_compare_values(llvm_function_generator_t& gen_acc, expression_type op, const typeid_t& type, llvm::Value& lhs_reg, llvm::Value& rhs_reg){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	llvm::Value* op_reg = llvm::ConstantInt::get(builder.getInt64Ty(), static_cast<int64_t>(op));

	std::vector<llvm::Value*> args = {
		gen_acc.get_callers_fcp(),
		op_reg,
		generate_itype_constant(gen_acc.gen, type),
		generate_cast_to_runtime_value(gen_acc.gen, lhs_reg, type),
		generate_cast_to_runtime_value(gen_acc.gen, rhs_reg, type)
	};
	auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.floydrt_compare_values.llvm_codegen_f, args, "");

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}


static llvm::Value* generate_comparison_expression(llvm_function_generator_t& gen_acc, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto lhs_temp = generate_expression(gen_acc, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, *details.rhs);

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = details.lhs->get_output_type();

	//	Output reg is always a bool.
	QUARK_ASSERT(e.get_output_type().is_bool());

	if(type.is_bool() || type.is_int()){
		/*
			ICMP_EQ    = 32,  ///< equal
			ICMP_NE    = 33,  ///< not equal
			ICMP_UGT   = 34,  ///< unsigned greater than
			ICMP_UGE   = 35,  ///< unsigned greater or equal
			ICMP_ULT   = 36,  ///< unsigned less than
			ICMP_ULE   = 37,  ///< unsigned less or equal
			ICMP_SGT   = 38,  ///< signed greater than
			ICMP_SGE   = 39,  ///< signed greater or equal
			ICMP_SLT   = 40,  ///< signed less than
			ICMP_SLE   = 41,  ///< signed less or equal
		*/
		static const std::map<expression_type, llvm::CmpInst::Predicate> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal,			llvm::CmpInst::Predicate::ICMP_SLE },
			{ expression_type::k_comparison_smaller,						llvm::CmpInst::Predicate::ICMP_SLT },
			{ expression_type::k_comparison_larger_or_equal,				llvm::CmpInst::Predicate::ICMP_SGE },
			{ expression_type::k_comparison_larger,						llvm::CmpInst::Predicate::ICMP_SGT },

			{ expression_type::k_logical_equal,							llvm::CmpInst::Predicate::ICMP_EQ },
			{ expression_type::k_logical_nonequal,						llvm::CmpInst::Predicate::ICMP_NE }
		};
		const auto pred = conv_opcode_int.at(details.op);
		return gen_acc.get_builder().CreateICmp(pred, lhs_temp, rhs_temp);
	}
	else if(type.is_double()){
		//??? Use ordered or unordered?
		/*
			// Opcode              U L G E    Intuitive operation
			FCMP_FALSE =  0,  ///< 0 0 0 0    Always false (always folded)

			FCMP_OEQ   =  1,  ///< 0 0 0 1    True if ordered and equal
			FCMP_OGT   =  2,  ///< 0 0 1 0    True if ordered and greater than
			FCMP_OGE   =  3,  ///< 0 0 1 1    True if ordered and greater than or equal
			FCMP_OLT   =  4,  ///< 0 1 0 0    True if ordered and less than
			FCMP_OLE   =  5,  ///< 0 1 0 1    True if ordered and less than or equal
			FCMP_ONE   =  6,  ///< 0 1 1 0    True if ordered and operands are unequal
			FCMP_ORD   =  7,  ///< 0 1 1 1    True if ordered (no nans)
		*/
		static const std::map<expression_type, llvm::CmpInst::Predicate> conv_opcode_int = {
			{ expression_type::k_comparison_smaller_or_equal,			llvm::CmpInst::Predicate::FCMP_OLE },
			{ expression_type::k_comparison_smaller,						llvm::CmpInst::Predicate::FCMP_OLT },
			{ expression_type::k_comparison_larger_or_equal,				llvm::CmpInst::Predicate::FCMP_OGE },
			{ expression_type::k_comparison_larger,						llvm::CmpInst::Predicate::FCMP_OGT },

			{ expression_type::k_logical_equal,							llvm::CmpInst::Predicate::FCMP_OEQ },
			{ expression_type::k_logical_nonequal,						llvm::CmpInst::Predicate::FCMP_ONE }
		};
		const auto pred = conv_opcode_int.at(details.op);
		return gen_acc.get_builder().CreateFCmp(pred, lhs_temp, rhs_temp);
	}
	else if(type.is_string() || type.is_vector()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_dict()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_struct()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_json()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else{
		UNSUPPORTED();
	}
}

enum class bitwize_operator {
	bw_not,
	bw_and,
	bw_or,
	bw_xor,
	bw_shift_left,
	bw_shift_right,
	bw_shift_right_arithmetic
};

static llvm::Value* generate_bitwize_expression(llvm_function_generator_t& gen_acc, bitwize_operator op, const expression_t& e, const std::vector<expression_t>& operands){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(e.get_output_type().is_int());
	QUARK_ASSERT(operands.size() == 1 || operands.size() == 2);

	if(operands.size() == 1){
		QUARK_ASSERT(operands[0].get_output_type().is_int());

		auto a = generate_expression(gen_acc, operands[0]);

		if(op == bitwize_operator::bw_not){
			return gen_acc.get_builder().CreateNot(a);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(operands.size() == 2){
		QUARK_ASSERT(operands[0].get_output_type().is_int());
		QUARK_ASSERT(operands[1].get_output_type().is_int());

		auto a = generate_expression(gen_acc, operands[0]);
		auto b = generate_expression(gen_acc, operands[1]);

		if(op == bitwize_operator::bw_not){
			QUARK_ASSERT(false);
		}
		else if(op == bitwize_operator::bw_and){
			return gen_acc.get_builder().CreateAnd(a, b);
		}
		else if(op == bitwize_operator::bw_or){
			return gen_acc.get_builder().CreateOr(a, b);
		}
		else if(op == bitwize_operator::bw_xor){
			return gen_acc.get_builder().CreateXor(a, b);
		}

		else if(op == bitwize_operator::bw_shift_left){
			return gen_acc.get_builder().CreateShl(a, b);
		}
		else if(op == bitwize_operator::bw_shift_right){
			return gen_acc.get_builder().CreateLShr(a, b);
		}
		else if(op == bitwize_operator::bw_shift_right_arithmetic){
			return gen_acc.get_builder().CreateAShr(a, b);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

static llvm::Value* generate_arithmetic_unary_minus_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.expr->get_output_type();
	if(type.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, e2);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_double(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, e2);
	}
	else{
		UNSUPPORTED();
	}
}

static llvm::Value* generate_conditional_operator_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::conditional_t& conditional){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto result_type = e.get_output_type();
	const auto result_itype = get_exact_llvm_type(gen_acc.gen.type_lookup, result_type);

	llvm::Value* condition_reg = generate_expression(gen_acc, *conditional.condition);

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* join_bb = llvm::BasicBlock::Create(context, "cond_operator-join");

	builder.CreateCondBr(condition_reg, then_bb, else_bb);


	// Emit then-value.
	builder.SetInsertPoint(then_bb);
	llvm::Value* then_reg = generate_expression(gen_acc, *conditional.a);
	builder.CreateBr(join_bb);
	// Codegen of 'Then' can change the current block, update then_bb.
	llvm::BasicBlock* then_bb2 = builder.GetInsertBlock();


	// Emit else block.
	parent_function->getBasicBlockList().push_back(else_bb);
	builder.SetInsertPoint(else_bb);
	llvm::Value* else_reg = generate_expression(gen_acc, *conditional.b);
	builder.CreateBr(join_bb);
	// Codegen of 'Else' can change the current block, update else_bb.
	llvm::BasicBlock* else_bb2 = builder.GetInsertBlock();


	QUARK_ASSERT(result_itype == then_reg->getType());
	QUARK_ASSERT(result_itype == else_reg->getType());

	// Emit join block.
	parent_function->getBasicBlockList().push_back(join_bb);
	builder.SetInsertPoint(join_bb);
	llvm::PHINode* phiNode = builder.CreatePHI(result_itype, 2, "cond_operator-result");
	phiNode->addIncoming(then_reg, then_bb2);
	phiNode->addIncoming(else_reg, else_bb2);

	//	Meaningless but shows that we handle condition_reg.
	generate_release(gen_acc, *condition_reg, typeid_t::make_bool());

	return phiNode;
}

/*
	NOTICES: The function signature for the callee can hold DYNs.
	The actual arguments will be explicit types, never DYNs.

	Function signature
	- In call's callee signature
	- In call's actual arguments types and output type.
	- In function def
*/
static llvm::Value* generate_call_expression(llvm_function_generator_t& gen_acc, const expression_t& e0, const expression_t::call_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e0.check_invariant());

	auto& builder = gen_acc.get_builder();

	const auto callee_function_type = details.callee->get_output_type();
	const auto resolved_call_return_type = e0.get_output_type();
	//	IDEA: Build a complete resolved_function_type: argument types from the actual arguments and return = resolved_call_return_type.
	//	This lets us select specialisation of calls, like "string push_back(string, int)" vs [bool] push_back([bool], bool)

	const auto actual_call_arguments = mapf<typeid_t>(details.args, [](auto& e){ return e.get_output_type(); });

	const auto llvm_mapping = map_function_arguments(gen_acc.gen.type_lookup, callee_function_type);

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(details.args.size() == callee_function_type.get_function_args().size());

	llvm::Value* callee0_reg = generate_expression(gen_acc, *details.callee);
	// Alternative: alter return type of callee0_reg to match resolved_call_return_type.

	//	Generate code that evaluates all argument expressions.
	std::vector<llvm::Value*> arg_regs;

	std::vector<std::pair<llvm::Value*, typeid_t> > destroy;

	for(const auto& out_arg: llvm_mapping.args){
		const auto& arg_details = details.args[out_arg.floyd_arg_index];

		if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			auto f_args = gen_acc.emit_f.args();
			QUARK_ASSERT((f_args.end() - f_args.begin()) >= 1);
			auto floyd_context_arg_ptr = f_args.begin();
			arg_regs.push_back(floyd_context_arg_ptr);
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
			llvm::Value* arg2 = generate_expression(gen_acc, arg_details);
			arg_regs.push_back(arg2);
			destroy.push_back({ arg2, arg_details.get_output_type() });
		}

		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
			llvm::Value* arg2 = generate_expression(gen_acc, arg_details);

			destroy.push_back({ arg2, arg_details.get_output_type() });

			//	Actual type of the argument, as specified inside the call expression. The concrete type for the DYN value for this call.
			const auto concrete_arg_type = arg_details.get_output_type();

			// We assume that the next arg in the llvm_mapping is the dyn-type and store it too.
			const auto packed_reg = generate_cast_to_runtime_value(gen_acc.gen, *arg2, concrete_arg_type);
			arg_regs.push_back(packed_reg);
			arg_regs.push_back(generate_itype_constant(gen_acc.gen, concrete_arg_type));
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	QUARK_ASSERT(arg_regs.size() == llvm_mapping.args.size());
	auto result0 = builder.CreateCall(callee0_reg, arg_regs, callee_function_type.get_function_return().is_void() ? "" : "");

	for(const auto& m: destroy){
		generate_release(gen_acc, *m.first, m.second);
	}
	//	Release callee?


	//	If the return type is dynamic, cast the returned int64 to the correct type.
	//	It must be retained already.
	llvm::Value* result = result0;
	if(callee_function_type.get_function_return().is_any()){
		//??? Notice: we always resolve the return type in semantic analysis -- no need to use WIDE_RETURN and provide a dynamic type.
		auto wide_return_a_reg = builder.CreateExtractValue(result, { static_cast<int>(WIDE_RETURN_MEMBERS::a) });
		result = generate_cast_from_runtime_value(gen_acc.gen, *wide_return_a_reg, resolved_call_return_type);
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}


//	Generates a call to the global function that implements the intrinsic.
static llvm::Value* generate_fallthrough_intrinsic(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	//	Find function
	const auto& symbols = gen_acc.gen.scope_path[0];
    const auto it = std::find_if(symbols.begin(), symbols.end(), [&details](const resolved_symbol_t& e) { return (std::string("$") + e.symbol_name) == details.call_name; } );
    QUARK_ASSERT(it != symbols.end());
	const auto function_type = std::make_shared<typeid_t>(it->symbol.get_type());
	int global_index = static_cast<int>(it - symbols.begin());

	const auto call_details = expression_t::call_t {
		std::make_shared<expression_t>(expression_t::make_load2(variable_address_t::make_variable_address(-1, global_index), function_type)),
		details.args
	};

	const auto e2 = expression_t::make_call(*call_details.callee, call_details.args, std::make_shared<typeid_t>(e.get_output_type()));
	return generate_call_expression(gen_acc, e2, call_details);
}


static llvm::Value* generate_intrinsic_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(details.call_name == get_intrinsic_opcode(make_assert_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_to_string_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_to_pretty_string_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(make_typeof_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(make_update_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_size_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_find_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_exists_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_erase_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_get_keys_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_push_back_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_subset_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_replace_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(make_parse_json_script_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_generate_json_script_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_to_json_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_from_json_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(make_get_json_type_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}



	else if(details.call_name == get_intrinsic_opcode(make_map_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_map_string_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_map_dag_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_filter_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_reduce_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_stable_sort_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}



	else if(details.call_name == get_intrinsic_opcode(make_print_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(make_send_signature())){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}


	else if(details.call_name == get_intrinsic_opcode(make_bw_not_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_not, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_and_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_and, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_or_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_or, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_xor_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_xor, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_shift_left_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_left, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_shift_right_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_right, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(make_bw_shift_right_arithmetic_signature())){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_right_arithmetic, e, details.args);
	}


	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}



//	Evaluate each element and store it directly into the array.
static void generate_fill_array(llvm_function_generator_t& gen_acc, llvm::Value& element_ptr_reg, llvm::Type& element_type, const std::vector<expression_t>& elements){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &element_ptr_reg, element_type.getPointerTo(), "");

	int element_index = 0;
	for(const auto& element_value: elements){

		//	Move ownwership from temp to member element, no need for retain-release.

		llvm::Value* element_value_reg = generate_expression(gen_acc, element_value);
		generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
		element_index++;
	}
}


static llvm::Value* generate_construct_vector(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(details.value_type.is_vector());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto element_count = details.elements.size();
	const auto element_type0 = details.value_type.get_vector_element_type();
	auto& element_type1 = *get_exact_llvm_type(gen_acc.gen.type_lookup, element_type0);

	const auto element_count_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), element_count);
	auto vec_ptr_reg = builder.CreateCall(gen_acc.gen.floydrt_allocate_vector.llvm_codegen_f, { gen_acc.get_callers_fcp(), element_count_reg }, "");
	auto ptr_reg = generate_get_vec_element_ptr_needs_cast(gen_acc, *vec_ptr_reg);

	if(element_type0.is_bool()){
		//	Each bool element is a uint64_t ???
		auto element_type = llvm::Type::getInt64Ty(context);
		auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, ptr_reg, element_type->getPointerTo(), "");

		//	Evaluate each element and store it directly into the the vector.
		int element_index = 0;
		for(const auto& arg: details.elements){
			llvm::Value* element0_reg = generate_expression(gen_acc, arg);
			auto element_value_reg = builder.CreateCast(llvm::Instruction::CastOps::ZExt, element0_reg, make_runtime_value_type(gen_acc.gen.type_lookup), "");
			generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
			element_index++;
		}
		return vec_ptr_reg;
	}
	else{
		generate_fill_array(gen_acc, *ptr_reg, element_type1, details.elements);
		return vec_ptr_reg;
	}
}

static llvm::Value* generate_construct_dict(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(details.value_type.is_dict());

	auto& builder = gen_acc.get_builder();

	const auto element_type0 = details.value_type.get_dict_value_type();

	auto dict_acc_ptr_reg = builder.CreateCall(gen_acc.gen.floydrt_allocate_dict.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");

	//	Elements are stored as pairs.
	QUARK_ASSERT((details.elements.size() & 1) == 0);

	const auto count = details.elements.size() / 2;
	for(int element_index = 0 ; element_index < count ; element_index++){
		llvm::Value* key0_reg = generate_expression(gen_acc, details.elements[element_index * 2 + 0]);
		llvm::Value* element0_reg = generate_expression(gen_acc, details.elements[element_index * 2 + 1]);

		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			dict_acc_ptr_reg,
			key0_reg,
			generate_cast_to_runtime_value(gen_acc.gen, *element0_reg, element_type0),
			generate_itype_constant(gen_acc.gen, element_type0)
		};
		builder.CreateCall(gen_acc.gen.floydrt_store_dict_mutable.llvm_codegen_f, args2, "");

		generate_release(gen_acc, *key0_reg, typeid_t::make_string());
	}
	return dict_acc_ptr_reg;
}

static llvm::Value* generate_construct_struct(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(details.value_type.is_struct());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto target_type = details.value_type;
	const auto element_count = details.elements.size();

	const auto& struct_def = target_type.get_struct();
	auto& exact_struct_type = *get_exact_struct_type(gen_acc.gen.type_lookup, target_type);
	QUARK_ASSERT(struct_def._members.size() == element_count);


	const llvm::DataLayout& data_layout = gen_acc.gen.module->getDataLayout();
	const llvm::StructLayout* layout = data_layout.getStructLayout(&exact_struct_type);
	const auto struct_bytes = layout->getSizeInBytes();

	const auto size_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), struct_bytes);
	std::vector<llvm::Value*> args2 = {
		gen_acc.get_callers_fcp(),
		size_reg
	};
	//	Returns STRUCT_T*.
	auto generic_struct_ptr_reg = gen_acc.get_builder().CreateCall(gen_acc.gen.floydrt_allocate_struct.llvm_codegen_f, args2, "");


	//!!! We basically inline the entire constructor here -- bad idea? Maybe generate a construction function and call it.
	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, *generic_struct_ptr_reg, target_type);

	int member_index = 0;
	for(const auto& m: struct_def._members){
		(void)m;
		const auto& arg = details.elements[member_index];
		llvm::Value* member_value_reg = generate_expression(gen_acc, arg);

		const auto gep = std::vector<llvm::Value*>{
			//	Struct array index.
			builder.getInt32(0),

			//	Struct member index.
			builder.getInt32(member_index)
		};
		llvm::Value* member_ptr_reg = builder.CreateGEP(&exact_struct_type, base_ptr_reg, gep, "");
		builder.CreateStore(member_value_reg, member_ptr_reg);

		member_index++;
	}
	return generic_struct_ptr_reg;
}


static llvm::Value* generate_construct_primitive(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
//	QUARK_ASSERT(details.value_type.is_struct());

	const auto target_type = details.value_type;
	const auto element_count = details.elements.size();
	QUARK_ASSERT(element_count == 1);

	//	Construct a primitive, using int(113) or double(3.14)

	auto element0_reg = generate_expression(gen_acc, details.elements[0]);
	const auto input_value_type = details.elements[0].get_output_type();

	if(target_type.is_bool() || target_type.is_int() || target_type.is_double() || target_type.is_typeid()){
		return element0_reg;
	}

	//	NOTICE: string -> json needs to be handled at runtime.

	//	Automatically transform a json::string => string at runtime?
	else if(target_type.is_string() && input_value_type.is_json()){
		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),
			element0_reg
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.floydrt_json_to_string.llvm_codegen_f, args, "");

		generate_release(gen_acc, *element0_reg, input_value_type);
		return result;
	}
	else if(target_type.is_json()){
		//	Put a value_t into a json
		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			generate_cast_to_runtime_value(gen_acc.gen, *element0_reg, input_value_type),
			generate_itype_constant(gen_acc.gen, input_value_type)
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.floydrt_allocate_json.llvm_codegen_f, args2, "");

		generate_release(gen_acc, *element0_reg, input_value_type);
		return result;
	}
	else{
		return element0_reg;
	}
}

static llvm::Value* generate_construct_value_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(details.value_type.is_vector()){
		return generate_construct_vector(gen_acc, details);
	}
	else if(details.value_type.is_dict()){
		return generate_construct_dict(gen_acc, details);
	}
	else if(details.value_type.is_struct()){
		return generate_construct_struct(gen_acc, details);
	}

	//	Construct a primitive, using int(113) or double(3.14)
	else{
		return generate_construct_primitive(gen_acc, details);
	}
}

/*
// do no allocate between execution of <body> -- that will pollute caches

start_bb:
	//	Max 10000 samples
	mutable int64_t samples[10000]
	mutabe index = 0
	int start = fr_get_profile_time()
	int end_time = start + 3.000.000
	br while_cond_bb

while_cond_bb
	//	Always record 2+ iterations
	if(index < 2) goto loop_bb
	if(index == 10000) goto end_bb
	if(b > end_time) goto end_bb
	goto loop_bb

loop_bb:
	const int a = fr_get_profile_time()
	<body>
	const int b = fr_get_profile_time()

	const int dur = b - a
	samples[index] = dur
	index++

end_bb:
	// First tests are cold tests = slow = ignore.
	//	Measure time() - time() to get overhead, then subtract from all results.
	// Only keep fastest
	int best_dur = find_smallest_int(samples, index, overhead)
*/

//??? Could be performed in semantic analysis pass, by inserting statements around the generate_block(). Problem is mutating samples-array.
//??? Measured body needs access to all function's locals -- cannot easily put body into separate function.

#if 1
static llvm::Value* generate_benchmark_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::benchmark_expr_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto& get_profile_time_def = gen_acc.gen.floydrt_get_profile_time;
	const auto& analyse_benchmark_samples_def = gen_acc.gen.floydrt_analyse_benchmark_samples;


	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond1_bb = llvm::BasicBlock::Create(context, "while-cond1", parent_function);
	auto while_cond2_bb = llvm::BasicBlock::Create(context, "while-cond2", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);


	////////	current BB

	auto samples_array_type = llvm::Type::getInt64Ty(context);
	auto sample_count_reg = generate_constant(gen_acc, value_t::make_int(10000));

	//	Pointer to int64_t x 10000
	/*
		; Foo bar[100]
		%bar = alloca %Foo, i32 100
		; bar[17].c = 0.0
		%2 = getelementptr %Foo, %Foo* %bar, i32 17, i32 2
		store double 0.0, double* %2
	*/
	auto samples_ptr_reg = builder.CreateAlloca(samples_array_type, sample_count_reg, "samples array");
	QUARK_ASSERT(samples_ptr_reg->getType()->isPointerTy());
	QUARK_ASSERT(samples_ptr_reg->getType() == llvm::Type::getInt64Ty(context)->getPointerTo());



	auto index_ptr_reg = builder.CreateAlloca(llvm::Type::getInt64Ty(context));
	auto zero_int_reg = generate_constant(gen_acc, value_t::make_int(0));
	auto one_int_reg = generate_constant(gen_acc, value_t::make_int(1));
	builder.CreateStore(zero_int_reg, index_ptr_reg);
	auto min_count_reg = generate_constant(gen_acc, value_t::make_int(2));

	auto start_time_reg = builder.CreateCall(get_profile_time_def.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");

	auto length_ns_reg = generate_constant(gen_acc, value_t::make_int(1000000000));
	auto end_time_reg = builder.CreateAdd(start_time_reg, length_ns_reg);
	builder.CreateBr(while_cond1_bb);




	////////	while_cond1_bb: minimum 2 runs

	builder.SetInsertPoint(while_cond1_bb);
	auto index2_reg = builder.CreateLoad(index_ptr_reg);
	auto test2_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, index2_reg, min_count_reg);
	builder.CreateCondBr(test2_reg, while_loop_bb, while_cond2_bb);

	
	////////	while_cond2_bb: minimum 1 second = 1000000000 ns
	builder.SetInsertPoint(while_cond2_bb);
	auto cur_time_reg = builder.CreateCall(get_profile_time_def.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");
	auto test3_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, cur_time_reg, end_time_reg);
	builder.CreateCondBr(test3_reg, while_loop_bb, while_join_bb);





	////////	while_loop_bb

	builder.SetInsertPoint(while_loop_bb);
	auto a_time_reg = builder.CreateCall(get_profile_time_def.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");
	generate_block(gen_acc, *details.body);

	auto b_time_reg = builder.CreateCall(get_profile_time_def.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");
	auto duration_reg = builder.CreateSub(b_time_reg, a_time_reg, "calc dur");

	auto index_reg = builder.CreateLoad(index_ptr_reg);

	const auto gep = std::vector<llvm::Value*>{
		index_reg
	};
	auto dest_sample_reg = builder.CreateGEP(llvm::Type::getInt64Ty(context), samples_ptr_reg, std::vector<llvm::Value*>{ index_reg }, "");
	builder.CreateStore(duration_reg, dest_sample_reg);

	//	Increment index.
	auto index3_reg = builder.CreateLoad(index_ptr_reg);
	auto index4_reg = builder.CreateAdd(index3_reg, one_int_reg);
	builder.CreateStore(index4_reg, index_ptr_reg);

//	builder.CreateBr(while_cond1_bb);
	auto test4_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, index4_reg, sample_count_reg);
	builder.CreateCondBr(test4_reg, while_cond1_bb, while_join_bb);


	////////	while_join_bb

	builder.SetInsertPoint(while_join_bb);
	auto index5_reg = builder.CreateLoad(index_ptr_reg);
	auto best_dur_reg = builder.CreateCall(analyse_benchmark_samples_def.llvm_codegen_f, { gen_acc.get_callers_fcp(), samples_ptr_reg, index5_reg }, "");

	return best_dur_reg;
}
#endif
#if 0
static llvm::Value* generate_benchmark_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::benchmark_expr_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto get_profile_time_f = gen_acc.gen.floydrt_get_profile_time.llvm_codegen_f

	auto& builder = gen_acc.get_builder();
	auto start_time_reg = builder.CreateCall(get_profile_time_f.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");

	generate_block(gen_acc, *details.body);

	auto end_time_reg = builder.CreateCall(get_profile_time_f.llvm_codegen_f, { gen_acc.get_callers_fcp() }, "");
	auto duration_reg = gen_acc.get_builder().CreateSub(end_time_reg, start_time_reg, "calc dur");
	return duration_reg;
}
#endif

static llvm::Value* generate_load2_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::load2_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_symbol(gen_acc.gen, details.address);
	auto result = gen_acc.get_builder().CreateLoad(dest.value_ptr, "temp");
	generate_retain(gen_acc, *result, e.get_output_type());
	return result;
}

static llvm::Value* generate_expression(llvm_function_generator_t& gen_acc, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		llvm_function_generator_t& gen_acc;
		const expression_t& e;


		llvm::Value* operator()(const expression_t::literal_exp_t& expr) const{
			return generate_literal_expression(gen_acc, e);
		}
		llvm::Value* operator()(const expression_t::arithmetic_t& expr) const{
			return generate_arithmetic_expression(gen_acc, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::comparison_t& expr) const{
			return generate_comparison_expression(gen_acc, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::unary_minus_t& expr) const{
			return generate_arithmetic_unary_minus_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::conditional_t& expr) const{
			return generate_conditional_operator_expression(gen_acc, e, expr);
		}

		llvm::Value* operator()(const expression_t::call_t& expr) const{
			return generate_call_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::intrinsic_t& expr) const{
			return generate_intrinsic_expression(gen_acc, e, expr);
		}


		llvm::Value* operator()(const expression_t::struct_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		llvm::Value* operator()(const expression_t::function_definition_expr_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		llvm::Value* operator()(const expression_t::load_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
		llvm::Value* operator()(const expression_t::load2_t& expr) const{
			//	No need / must not load function arguments.
			const auto s = find_symbol(gen_acc.gen, expr.address);
			if(s.symtype == resolved_symbol_t::esymtype::k_function_argument){
				auto result = s.value_ptr;

				//	No need to retain function arguments! We should track which expression-outputs that need release.
				generate_retain(gen_acc, *result, e.get_output_type());
				return result;
			}
			else{
				return generate_load2_expression(gen_acc, e, expr);
			}
		}

		llvm::Value* operator()(const expression_t::resolve_member_t& expr) const{
			return generate_resolve_member_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::update_member_t& expr) const{
			return generate_update_member_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::lookup_t& expr) const{
			return generate_lookup_element_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::value_constructor_t& expr) const{
			return generate_construct_value_expression(gen_acc, e, expr);
		}
		llvm::Value* operator()(const expression_t::benchmark_expr_t& expr) const{
			return generate_benchmark_expression(gen_acc, e, expr);
		}
	};

	llvm::Value* result = std::visit(visitor_t{ gen_acc, e }, e._expression_variant);
	return result;
}



	
////////////////////////////////		STATEMENTS





static void generate_assign2_statement(llvm_function_generator_t& gen_acc, const statement_t::assign2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = generate_expression(gen_acc, s._expression);

	auto dest = find_symbol(gen_acc.gen, s._dest_variable);
	const auto type = dest.symbol.get_type();

	if(is_rc_value(type)){
		auto prev_value = gen_acc.get_builder().CreateLoad(dest.value_ptr);
		generate_release(gen_acc, *prev_value, type);

		//	No need to retain new value. generate_expression() takes care of that.
		gen_acc.get_builder().CreateStore(value, dest.value_ptr);
	}
	else{
		gen_acc.get_builder().CreateStore(value, dest.value_ptr);
	}

	QUARK_ASSERT(gen_acc.check_invariant());
}

static void generate_init2_statement(llvm_function_generator_t& gen_acc, const statement_t::init2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = generate_expression(gen_acc, s._expression);

	auto dest = find_symbol(gen_acc.gen, s._dest_variable);

	//	No need to retain new value. generate_expression() takes care of that.
	gen_acc.get_builder().CreateStore(value, dest.value_ptr);

	QUARK_ASSERT(gen_acc.check_invariant());
}

static function_return_mode generate_block_statement(llvm_function_generator_t& gen_acc, const statement_t::block_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	return generate_block(gen_acc, s._body);
}

static function_return_mode generate_ifelse_statement(llvm_function_generator_t& gen_acc, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	//	Notice that generate_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* condition_reg = generate_expression(gen_acc, statement._condition);
	auto start_bb = builder.GetInsertBlock();

	auto then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	auto else_bb = llvm::BasicBlock::Create(context, "else", parent_function);

	builder.SetInsertPoint(start_bb);
	builder.CreateCondBr(condition_reg, then_bb, else_bb);


	// Emit then-block.
	builder.SetInsertPoint(then_bb);

	//	Notice that generate_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto then_mode = generate_block(gen_acc, statement._then_body);
	auto then_bb2 = builder.GetInsertBlock();


	// Emit else-block.
	builder.SetInsertPoint(else_bb);

	//	Notice that generate_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto else_mode = generate_block(gen_acc, statement._else_body);
	auto else_bb2 = builder.GetInsertBlock();


	//	Scenario A: both then-block and else-block always returns = no need for join BB.
	if(then_mode == function_return_mode::return_executed && else_mode == function_return_mode::return_executed){
		return function_return_mode::return_executed;
	}

	//	Scenario B: eith then-block or else-block continues. We need a join-block where it can jump.
	else{
		auto join_bb = llvm::BasicBlock::Create(context, "join-then-else", parent_function);

		if(then_mode == function_return_mode::some_path_not_returned){
			builder.SetInsertPoint(then_bb2);
			builder.CreateBr(join_bb);
		}
		if(else_mode == function_return_mode::some_path_not_returned){
			builder.SetInsertPoint(else_bb2);
			builder.CreateBr(join_bb);
		}

		builder.SetInsertPoint(join_bb);
		return function_return_mode::some_path_not_returned;
	}
}

/*
	start_bb
		...
		...


		start_value = start_expression
		...
		end_value = end_expression
		...
		store start_value in ITERATOR
		if start_value < end_value { goto for-loop } else { goto for-end }

	for-loop:
		loop-body-statement
		loop-body-statement
		loop-body-statement

		count2 = load counter
		count3 = counter2 + 1
		store count3 => counter
		if start_value < end_value { goto for-loop } else { goto for-end }

	for-end:
		<CURRENT POS AT RETURN>
*/
static function_return_mode generate_for_statement(llvm_function_generator_t& gen_acc, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto forloop_bb = llvm::BasicBlock::Create(context, "for-loop", parent_function);
	auto forend_bb = llvm::BasicBlock::Create(context, "for-end", parent_function);



	//	EMIT LOOP SETUP INTO CURRENT BB

	//	Notice that generate_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* start_reg = generate_expression(gen_acc, statement._start_expression);
	llvm::Value* end_reg = generate_expression(gen_acc, statement._end_expression);

	auto values = generate_symbol_slots(gen_acc, statement._body._symbol_table);

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	llvm::Value* counter_reg = values[0].value_ptr;
	builder.CreateStore(start_reg, counter_reg);

	llvm::Value* add_reg = generate_constant(gen_acc, value_t::make_int(1));

	llvm::CmpInst::Predicate pred = statement._range_type == statement_t::for_statement_t::k_closed_range
		? llvm::CmpInst::Predicate::ICMP_SLE
		: llvm::CmpInst::Predicate::ICMP_SLT;

	auto test_reg = builder.CreateICmp(pred, start_reg, end_reg);
	builder.CreateCondBr(test_reg, forloop_bb, forend_bb);



	//	EMIT LOOP BB

	builder.SetInsertPoint(forloop_bb);

	const auto return_mode = generate_body(gen_acc, values, statement._body._statements);

	if(return_mode == function_return_mode::some_path_not_returned){
		llvm::Value* counter2 = builder.CreateLoad(counter_reg);
		llvm::Value* counter3 = builder.CreateAdd(counter2, add_reg, "inc_for_counter");
		builder.CreateStore(counter3, counter_reg);

		auto test_reg2 = builder.CreateICmp(pred, counter3, end_reg);
		builder.CreateCondBr(test_reg2, forloop_bb, forend_bb);
	}
	else{
	}


	//	EMIT LOOP END BB

	builder.SetInsertPoint(forend_bb);
	return function_return_mode::some_path_not_returned;
}

/*
	current_bb
		...

		br while-cond

	while_cond_bb:
		cond = generate_expression()
 		CreateCondBr(condition, while_loop_bb, while_join_bb)

	while_loop_bb:
		generate_body()
		br while_cond_bb
 
	while_join_bb:
		...
*/

static function_return_mode generate_while_statement(llvm_function_generator_t& gen_acc, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond_bb = llvm::BasicBlock::Create(context, "while-cond", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);


	////////	current BB

	builder.CreateBr(while_cond_bb);


	////////	while_cond_bb

	builder.SetInsertPoint(while_cond_bb);
	llvm::Value* condition = generate_expression(gen_acc, statement._condition);
	builder.CreateCondBr(condition, while_loop_bb, while_join_bb);


	////////	while_loop_bb

	builder.SetInsertPoint(while_loop_bb);
	const auto mode = generate_block(gen_acc, statement._body);
	builder.CreateBr(while_cond_bb);

	if(mode == function_return_mode::some_path_not_returned){
	}
	else{
	}


	////////	while_join_bb

	builder.SetInsertPoint(while_join_bb);
	return function_return_mode::some_path_not_returned;
}

static void generate_expression_statement(llvm_function_generator_t& gen_acc, const statement_t::expression_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto result_reg = generate_expression(gen_acc, s._expression);
	generate_release(gen_acc, *result_reg, s._expression.get_output_type());

	QUARK_ASSERT(gen_acc.check_invariant());
}


static llvm::Value* generate_return_statement(llvm_function_generator_t& gen_acc, const statement_t::return_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = generate_expression(gen_acc, s._expression);

	//	Destruct all locals before unwinding.
	auto path = gen_acc.gen.scope_path;
	while(path.size() > 1){
		generate_destruct_scope_locals(gen_acc, path.back());
		path.pop_back();
	}

	return gen_acc.get_builder().CreateRet(value);
}

static function_return_mode generate_statement(llvm_function_generator_t& gen_acc, const statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		llvm_function_generator_t& acc0;

		function_return_mode operator()(const statement_t::return_statement_t& s) const{
			generate_return_statement(acc0, s);
			return function_return_mode::return_executed;
		}

		function_return_mode operator()(const statement_t::bind_local_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::assign_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::assign2_t& s) const{
			generate_assign2_statement(acc0, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::init2_t& s) const{
			generate_init2_statement(acc0, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::block_statement_t& s) const{
			return generate_block_statement(acc0, s);
		}

		function_return_mode operator()(const statement_t::ifelse_statement_t& s) const{
			return generate_ifelse_statement(acc0, s);
		}
		function_return_mode operator()(const statement_t::for_statement_t& s) const{
			return generate_for_statement(acc0, s);
		}
		function_return_mode operator()(const statement_t::while_statement_t& s) const{
			return generate_while_statement(acc0, s);
		}


		function_return_mode operator()(const statement_t::expression_statement_t& s) const{
			generate_expression_statement(acc0, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::software_system_statement_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::container_def_statement_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::benchmark_def_statement_t& s) const{
			UNSUPPORTED();
		}
	};

	return std::visit(visitor_t{ gen_acc }, statement._contents);
}

static function_return_mode generate_statements(llvm_function_generator_t& gen_acc, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());
			const auto mode = generate_statement(gen_acc, statement);
			if(mode == function_return_mode::return_executed){
				return function_return_mode::return_executed;
			}
		}
	}
	return function_return_mode::some_path_not_returned;
}




//	Generates local symbols for arguments and local variables. Only toplevel of function, not nested scopes.
std::vector<resolved_symbol_t> generate_function_local_symbols(llvm_function_generator_t& gen_acc, const function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	QUARK_ASSERT(function_def._optional_body);
	const symbol_table_t& symbol_table = function_def._optional_body->_symbol_table;

	const auto mapping0 = map_function_arguments(gen_acc.gen.type_lookup, function_def._function_type);
	const auto mapping = name_args(mapping0, function_def._named_args);

	//	Make a resolved_symbol_t for each element in the symbol table. Some are local variables, some are arguments.
	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = get_exact_llvm_type(gen_acc.gen.type_lookup, type);

		//	Figure out if this symbol is an argument or a local variable.
		//	Check if we can find an argument with this name => it's an argument.
		//	TODO: SAST could contain argument/local information to make this tighter.
		//	Reserve stack slot for each local. But not arguments, they already have stack slot.
		const auto arg_it = std::find_if(mapping.args.begin(), mapping.args.end(), [&](const llvm_arg_mapping_t& arg) -> bool {
//			QUARK_TRACE_SS(arg.floyd_name);
			return arg.floyd_name == e.first;
		});
		bool is_arg = arg_it != mapping.args.end();

		if(is_arg){
			//	Find Value* for the argument by matching the argument index. Remember that we always add a floyd_runtime_ptr to all LLVM functions.
			auto f_args = gen_acc.emit_f.args();
			const auto f_args_size = f_args.end() - f_args.begin();

			QUARK_ASSERT(f_args_size >= 1);
			QUARK_ASSERT(f_args_size == mapping.args.size());

			const auto llvm_arg_index = arg_it - mapping.args.begin();
			QUARK_ASSERT(llvm_arg_index < f_args_size);

			llvm::Argument* f_arg_ptr = f_args.begin() + llvm_arg_index;

			//	The argument is used as the llvm::Value*.
			llvm::Value* dest = f_arg_ptr;

			const auto debug_str = "<ARGUMENT> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);

			result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_function_argument, e.first, e.second));
		}
		else{
			llvm::Value* dest = gen_acc.get_builder().CreateAlloca(itype, nullptr, e.first);

			//	Init the slot if needed.
			if(e.second._init.is_undefined() == false){
				llvm::Value* c = generate_constant(gen_acc, e.second._init);
				gen_acc.get_builder().CreateStore(c, dest);
			}
			const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
			result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
		}
	}
	QUARK_ASSERT(result.size() == symbol_table._symbols.size());
	return result;
}


static llvm::Value* generate_global(llvm_function_generator_t& gen_acc, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_name.empty() == false);
	QUARK_ASSERT(symbol.check_invariant());

	auto& module = *gen_acc.gen.module;

	const auto type0 = symbol.get_type();
	const auto itype = get_exact_llvm_type(gen_acc.gen.type_lookup, type0);

	if(symbol._init.is_undefined()){
		return generate_global0(module, symbol_name, *itype, nullptr);
	}
	else{
		auto init_reg = generate_constant(gen_acc, symbol._init);
		if (llvm::Constant* CI = llvm::dyn_cast<llvm::Constant>(init_reg)){

			//	dest->setInitializer(constant_reg);
			return generate_global0(module, symbol_name, *itype, CI);

/*
			if (CI->getBitWidth() <= 32) {
				constIntValue = CI->getSExtValue();
			}
*/
		}
		else{
			return generate_global0(module, symbol_name, *itype, nullptr);
		}
	}
}

//	Make LLVM globals for every global in the AST.
//	Inits the globals when possible.
//	Other globals are uninitialised and global init2-statements will store to them from floyd_runtime_init().
static std::vector<resolved_symbol_t> generate_globals_from_ast(llvm_function_generator_t& gen_acc, const semantic_ast_t& ast, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	std::vector<resolved_symbol_t> result;

	for(const auto& e: symbol_table._symbols){
		llvm::Value* value = generate_global(gen_acc, e.first, e.second);
		const auto debug_str = "name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(value, debug_str, resolved_symbol_t::esymtype::k_global, e.first, e.second));

//		QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));
	}
	return result;
}

//	NOTICE: Fills-in the body of an existing LLVM function prototype.
static void generate_floyd_function_body(llvm_code_generator_t& gen_acc0, const floyd::function_definition_t& function_def, const body_t& body){
	QUARK_ASSERT(gen_acc0.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto link_name = encode_floyd_func_link_name(function_def._definition_name);

	auto f = gen_acc0.module->getFunction(link_name.s);
	QUARK_ASSERT(check_invariant__function(f));

	{
		llvm_function_generator_t gen_acc(gen_acc0, *f);

		llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.gen.instance->context, "entry", f);
		gen_acc.get_builder().SetInsertPoint(entryBB);

		auto symbol_table_values = generate_function_local_symbols(gen_acc, function_def);

		const auto return_mode = generate_body(gen_acc, symbol_table_values, body._statements);

		//	Not all paths returns a value!
		if(return_mode == function_return_mode::some_path_not_returned && function_def._function_type.get_function_return().is_void() == false){
			throw std::runtime_error("Not all function paths returns a value!");
		}

		if(function_def._function_type.get_function_return().is_void()){
			gen_acc.get_builder().CreateRetVoid();
		}
	}
	QUARK_ASSERT(check_invariant__function(f));
}

static void generate_all_floyd_function_bodies(llvm_code_generator_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	We have already generate the LLVM function-prototypes for the global functions in generate_module().
	for(const auto& function_def: semantic_ast._tree._function_defs){
		if(function_def._optional_body){
			generate_floyd_function_body(gen_acc, function_def, *function_def._optional_body);
		}
	}
}


//	The AST contains statements that initializes the global variables, including global functions.

//	Function prototype must NOT EXIST already.
static llvm::Function* generate_function_prototype(llvm::Module& module, const llvm_type_lookup& type_lookup, const function_definition_t& function_def){
	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	const auto function_type = function_def._function_type;
	const auto link_name = encode_floyd_func_link_name(function_def._definition_name);

	auto existing_f = module.getFunction(link_name.s);
	QUARK_ASSERT(existing_f == nullptr);


	const auto mapping0 = map_function_arguments(type_lookup, function_type);
	const auto mapping = name_args(mapping0, function_def._named_args);

	llvm::Type* function_ptr_type = get_exact_llvm_type(type_lookup, function_type);
	const auto function_byvalue_type = deref_ptr(function_ptr_type);

	//	IMPORTANT: Must cast to (llvm::FunctionType*) to get correct overload of getOrInsertFunction() to be called!
	auto f3 = module.getOrInsertFunction(link_name.s, (llvm::FunctionType*)function_byvalue_type);
	llvm::Function* f = llvm::cast<llvm::Function>(f3);

	//	Set names for all arguments.
	auto f_args = f->args();
	const auto f_arg_count = f_args.end() - f_args.begin();
	QUARK_ASSERT(f_arg_count == mapping.args.size());

	int index = 0;
	for(auto& e: f_args){
		const auto& m = mapping.args[index];
		const auto name = m.floyd_name;
		e.setName(name);
		index++;
	}

	QUARK_ASSERT(check_invariant__function(f));
	QUARK_ASSERT(check_invariant__module(&module));
	return f;
}



//??? Generate all llvm function prototypes from Floyd prototypes using unified system: including init() and deinit().
//	Make LLVM functions -- runtime, floyd host functions, floyd functions.
//	host functions are later linked by LLVM execution engine, by matching the function names.
static std::vector<function_def_t> make_all_function_prototypes(llvm::Module& module, const llvm_type_lookup& type_lookup, const std::vector<floyd::function_definition_t>& defs){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<function_def_t> result;

	auto& context = module.getContext();

	//	Make prototypes for all runtime functions, like floydrt_retain_vec().
	const auto runtime_functions = get_runtime_functions(context, type_lookup);
	for(const auto& e: runtime_functions){
		const auto link_name = encode_runtime_func_link_name(e.name);
		auto f = module.getOrInsertFunction(link_name.s, e.function_type);
		QUARK_ASSERT(check_invariant__module(&module));

		const auto def0 = function_definition_t::make_func(k_no_location, e.name, typeid_t::make_void(), {}, {});
		const auto def = function_def_t{ link_name, llvm::cast<llvm::Function>(f), def0 };
		result.push_back(def);
	}

	//	init()
	{
		const auto name = "init";
		const auto link_name = encode_runtime_func_link_name(name);
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		auto f = module.getOrInsertFunction(link_name.s, function_type);
		const auto def0 = function_definition_t::make_func(k_no_location, name, typeid_t::make_void(), {}, {});
		const auto def = function_def_t{ link_name, llvm::cast<llvm::Function>(f), def0 };
		result.push_back(def);
	}

	//	deinit()
	{
		const auto name = "deinit";
		const auto link_name = encode_runtime_func_link_name(name);
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(type_lookup)
			},
			false
		);
		auto f = module.getOrInsertFunction(link_name.s, function_type);
		const auto def0 = function_definition_t::make_func(k_no_location, name, typeid_t::make_void(), {}, {});
		const auto def = function_def_t{ link_name, llvm::cast<llvm::Function>(f), def0 };
		result.push_back(def);
	}

	//	Make function prototypes for all floyd functions.
	{
		for(const auto& function_def: defs){
			auto f = generate_function_prototype(module, type_lookup, function_def);
			const auto def = function_def_t{ encode_floyd_func_link_name(function_def._definition_name), llvm::cast<llvm::Function>(f), function_def };
			result.push_back(def);
		}
	}
	return result;
}

/*
	GENERATE CODE for floyd_runtime_init()
	Floyd's global instructions are packed into the "floyd_runtime_init"-function. LLVM doesn't have global instructions.
	All Floyd's global statements becomes instructions in floyd_init()-function that is called by Floyd runtime before any other function is called.
*/
static void generate_floyd_runtime_init(llvm_code_generator_t& gen_acc, const body_t& globals){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	llvm::Function* f = gen_acc.floydrt_init.llvm_codegen_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);
	llvm::BasicBlock* destructBB = llvm::BasicBlock::Create(context, "destruct", f);

	{
		llvm_function_generator_t function_gen_acc(gen_acc, *f);

		//	entryBB
		{
			builder.SetInsertPoint(entryBB);

			//	Global statements, using the global symbol scope.
			//	This includes init2-statements to initialise global variables.
			generate_statements(function_gen_acc, globals._statements);

			builder.CreateBr(destructBB);
		}

		//	destructBB
		{
			builder.SetInsertPoint(destructBB);

			llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 667);
			builder.CreateRet(dummy_result);
		}
	}

	if(k_trace_input_output){
		QUARK_TRACE_SS(print_module(*gen_acc.module));
	}
	QUARK_ASSERT(check_invariant__function(f));

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());
}

static void generate_floyd_runtime_deinit(llvm_code_generator_t& gen_acc, const body_t& globals){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	llvm::Function* f = gen_acc.floydrt_deinit.llvm_codegen_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);

	{
		llvm_function_generator_t function_gen_acc(gen_acc, *f);

		//	entryBB
		{
			builder.SetInsertPoint(entryBB);

			//	Destruct global variables.
			for(const auto& e: function_gen_acc.gen.scope_path.front()){
				if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
					const auto type = e.symbol.get_type();
					if(is_rc_value(type)){
						auto reg = builder.CreateLoad(e.value_ptr);
						generate_release(function_gen_acc, *reg, type);
					}
					else{
					}
				}
			}

			llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 668);
			builder.CreateRet(dummy_result);
		}
	}

	if(k_trace_input_output){
		QUARK_TRACE_SS(print_module(*gen_acc.module));
	}
	QUARK_ASSERT(check_invariant__function(f));

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());
}



static std::pair<std::unique_ptr<llvm::Module>, std::vector<function_def_t>> generate_module(llvm_instance_t& instance, const std::string& module_name, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	auto module = std::make_unique<llvm::Module>(module_name.c_str(), instance.context);

	llvm_type_lookup type_lookup(instance.context, semantic_ast._tree._interned_types);

	//	Generate all LLVM nodes: functions (without implementation) and globals.
	//	This lets all other code reference them, even if they're not filled up with code yet.

	const auto funcs = make_all_function_prototypes(*module, type_lookup, semantic_ast._tree._function_defs);

	llvm_code_generator_t gen_acc(instance, module.get(), semantic_ast._tree._interned_types, type_lookup, funcs);

	//	Global variables.
	{
		llvm_function_generator_t function_gen_acc(gen_acc, *gen_acc.floydrt_init.llvm_codegen_f);

		std::vector<resolved_symbol_t> globals = generate_globals_from_ast(
			function_gen_acc,
			semantic_ast,
			semantic_ast._tree._globals._symbol_table
		);
		gen_acc.scope_path = { globals };
	}

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());


	//	Generate bodies of functions.
	{
		generate_all_floyd_function_bodies(gen_acc, semantic_ast);
		generate_floyd_runtime_init(gen_acc, semantic_ast._tree._globals);
		generate_floyd_runtime_deinit(gen_acc, semantic_ast._tree._globals);
	}

	return { std::move(module), gen_acc.function_defs };
}



std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());

	if(k_trace_input_output){
		QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(semantic_ast_to_json(ast0)));
	}
	if(k_trace_types){
		{
			QUARK_SCOPED_TRACE("ast0 types");			for(const auto& e: ast0._tree._interned_types.interned){
				QUARK_TRACE_SS(e.first.itype << ": " << typeid_to_compact_string(e.second));
			}
		}
	}

	auto ast = ast0;

	auto result0 = generate_module(instance, module_name, ast);
	auto module = std::move(result0.first);
	auto funcs = result0.second;

	const auto type_lookup = llvm_type_lookup(instance.context, ast0._tree._interned_types);

	auto result = std::make_unique<llvm_ir_program_t>(&instance, module, type_lookup, ast._tree._globals._symbol_table, funcs);

	result->container_def = ast0._tree._container_def;
	result->software_system = ast0._tree._software_system;

	if(k_trace_input_output){
		QUARK_TRACE_SS("result = " << floyd::print_program(*result));
	}
	return result;
}

}	//	floyd
