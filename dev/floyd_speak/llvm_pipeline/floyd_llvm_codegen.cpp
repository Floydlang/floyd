//
//  floyd_llvm_codegen.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

const bool k_trace_input_output = false;
const bool k_trace_types = false;

#include "floyd_llvm_codegen.h"

#include "floyd_llvm_runtime.h"
#include "floyd_llvm_helpers.h"

#include "ast_value.h"

#include "bytecode_host_functions.h"

#include "floyd_parser.h"
#include "ast_json.h"
#include "pass3.h"

#include "quark.h"


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


typedef std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs_t;


/*
TODO ???:

- Pass llvm::Value* for the runtime-pointer instead of emit_f?
- Make nicer mechanism to register all host functions, types and key-strings.
- Store explicit members like assert_f instead of search on string.
*/

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
	public: llvm_code_generator_t(llvm_instance_t& instance, llvm::Module* module, const type_interner_t& interner) :
		instance(&instance),
		module(module),
		builder(instance.context),
		interner(instance.context, interner)
	{
		QUARK_ASSERT(instance.check_invariant());

		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();

		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
//		QUARK_ASSERT(scope_path.size() >= 1);
		QUARK_ASSERT(instance);
		QUARK_ASSERT(interner.check_invariant());
		QUARK_ASSERT(instance->check_invariant());
		QUARK_ASSERT(module);
		return true;
	}


	////////////////////////////////		STATE
	llvm_instance_t* instance;
	llvm::Module* module;
	llvm::IRBuilder<> builder;
	llvm_type_interner_t interner;
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
};




enum class function_return_mode {
	some_path_not_returned,
	return_executed
};

static function_return_mode generate_statements(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements);
static llvm::Value* generate_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e);



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

	auto& builder = gen_acc.builder;
	return generate_cast_to_runtime_value2(builder, value, floyd_type);
}

static llvm::Value* generate_cast_from_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& runtime_value_reg, const typeid_t& type){
	auto& builder = gen_acc.builder;
	return generate_cast_from_runtime_value2(builder, gen_acc.interner, runtime_value_reg, type);
}

const function_def_t& find_function_def(llvm_code_generator_t& gen_acc, const std::string& function_name){
	QUARK_ASSERT(gen_acc.check_invariant());

	return find_function_def2(gen_acc.function_defs, function_name);
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
	return lookup_type(gen_acc.interner.interner, t);
}

int64_t pack_itype(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return lookup_itype(gen_acc.interner.interner, type).itype;
}

llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto& context = gen_acc.instance->context;
	auto itype = pack_itype(gen_acc, type);
	auto t = make_runtime_type_type(context);
 	return llvm::ConstantInt::get(t, itype);
}

void generate_retain(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& value_reg, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(type.check_invariant());

	auto& builder = gen_acc.builder;
	if(is_rc_value(type)){
		if(type.is_string() || type.is_vector()){
			const auto f = find_function_def(gen_acc, "fr_retain_vec");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args, "");
		}
		else if(type.is_dict()){
			const auto f = find_function_def(gen_acc, "fr_retain_dict");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args, "");
		}
		else if(type.is_json_value()){
			const auto f = find_function_def(gen_acc, "fr_retain_json");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args, "");
		}
		else if(type.is_struct()){
			const auto f = find_function_def(gen_acc, "fr_retain_struct");

			auto generic_vec_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value_reg, get_generic_struct_type(gen_acc.interner)->getPointerTo(), "");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				generic_vec_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args, "");
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}
void generate_release(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& value_reg, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(type.check_invariant());

	auto& builder = gen_acc.builder;
	if(is_rc_value(type)){
		if(type.is_string() || type.is_vector()){
			const auto f = find_function_def(gen_acc, "fr_release_vec");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args);
		}
		else if(type.is_dict()){
			const auto f = find_function_def(gen_acc, "fr_release_dict");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args);
		}
		else if(type.is_json_value()){
			const auto f = find_function_def(gen_acc, "fr_release_json");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				&value_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args);
		}
		else if(type.is_struct()){
			const auto f = find_function_def(gen_acc, "fr_release_struct");
			auto generic_vec_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value_reg, get_generic_struct_type(gen_acc.interner)->getPointerTo(), "");
			std::vector<llvm::Value*> args = {
				get_callers_fcp(gen_acc.interner, emit_f),
				generic_vec_reg,
				generate_itype_constant(gen_acc, type)
			};
			builder.CreateCall(f.llvm_f, args);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
	}
}


std::string compose_function_def_name(function_id_t function_id, const function_definition_t& def){
	const auto def_name = def._definition_name;
	const auto funcdef_name = def_name.empty() ? std::string() + "floyd_unnamed_function_" + std::to_string(function_id) : std::string("floyd_funcdef__") + def_name;
	return funcdef_name;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////







/*
	Put the string characters in a LLVM constant,
	generate code that instantiates a VEC_T holding a *copy* of the characters.

	FUTURE: point directly to code segment chars, don't free() on VEC_t destruct.
	FUTURE: Here we construct a new VEC_T instance everytime. We could do it *once* instead and make a global const out of it.
*/
llvm::Value* generate_constant_string(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::string& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.builder;

	//	Make a global string constant.
	llvm::Constant* str_ptr = builder.CreateGlobalStringPtr(s);
	llvm::Constant* str_size = llvm::ConstantInt::get(builder.getInt64Ty(), s.size());

	const auto f = find_function_def(gen_acc, "fr_alloc_kstr");
	std::vector<llvm::Value*> args = {
		get_callers_fcp(gen_acc.interner, emit_f),
		str_ptr,
		str_size
	};
	auto string_vec_ptr_reg = builder.CreateCall(f.llvm_f, args, "string literal");
	return string_vec_ptr_reg;
};


//	Makes constant from a Floyd value. The constant may go in code segment or be computed at app init-time.
llvm::Value* generate_constant(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const value_t& value){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto& builder = gen_acc.builder;
	auto& context = gen_acc.module->getContext();

	const auto type = value.get_type();
	const auto itype = get_exact_llvm_type(gen_acc.interner, type);

	struct visitor_t {
		llvm_code_generator_t& gen_acc;
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		llvm::Type* itype;
		const value_t& value;
		llvm::Function& emit_f;

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
			return generate_constant_string(gen_acc, emit_f, value.get_string_value());
		}
		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			const auto& json_value0 = value.get_json_value();

			//	NOTICE: There is no clean way to embedd a json_value containing a json-null into the code segment.
			//	Here we use a nullptr instead of json_t*. This means we have to be prepared for json_t::null AND nullptr.
			if(json_value0.is_null()){
				auto json_type = get_exact_llvm_type(gen_acc.interner, typeid_t::make_json_value());
				llvm::PointerType* pointer_type = llvm::cast<llvm::PointerType>(json_type);
				return llvm::ConstantPointerNull::get(pointer_type);
			}
			else{
				UNSUPPORTED();
			}
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return generate_itype_constant(gen_acc, value.get_typeid_value());
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
			for(const auto& e: gen_acc.function_defs){
				if(e.floyd_function_id == function_id){
					return e.llvm_f;
				}
			}
			QUARK_ASSERT(false);
			throw std::exception();
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ gen_acc, builder, context, itype, value, emit_f }, type._contents);
}


static std::vector<resolved_symbol_t> generate_symbol_slots(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(symbol_table.check_invariant());

	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = get_exact_llvm_type(gen_acc.interner, type);

		//	Reserve stack slot for each local.
		llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

		//	Init the slot if needed.
		if(e.second._init.is_undefined() == false){
			llvm::Value* c = generate_constant(gen_acc, emit_f, e.second._init);
			gen_acc.builder.CreateStore(c, dest);
		}
		const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
	}
	return result;
}

static void generate_destruct_scope_locals(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<resolved_symbol_t>& symbols){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	for(const auto& e: symbols){
		if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
			const auto type = e.symbol.get_type();
			if(is_rc_value(type)){
				auto reg = builder.CreateLoad(e.value_ptr);
				generate_release(gen_acc, emit_f, *reg, type);
			}
			else{
			}
		}
	}
}



static function_return_mode generate_body(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<resolved_symbol_t>& resolved_symbols, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	gen_acc.scope_path.push_back(resolved_symbols);
	const auto return_mode = generate_statements(gen_acc, emit_f, statements);
	gen_acc.scope_path.pop_back();

	//	Destruct body's locals. Unless return_executed.
	if(return_mode == function_return_mode::some_path_not_returned){
		generate_destruct_scope_locals(gen_acc, emit_f, resolved_symbols);
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return return_mode;
}


static function_return_mode generate_block(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto values = generate_symbol_slots(gen_acc, emit_f, body._symbol_table);
	const auto return_mode = generate_body(gen_acc, emit_f, values, body._statements);

	QUARK_ASSERT(gen_acc.check_invariant());
	return return_mode;
}

static llvm::Value* generate_get_vec_element_ptr2(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& vec_ptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto after_alloc64_ptr_reg = builder.CreateGEP(make_generic_vec_type(gen_acc.interner), &vec_ptr_reg, gep, "");
	return after_alloc64_ptr_reg;
}

static llvm::Value* generate_get_vec_element_ptr(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& vec_ptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	auto ptr_reg = generate_get_vec_element_ptr2(gen_acc, emit_f, vec_ptr_reg);
	auto uint64_array_ptr_reg = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::BitCast, ptr_reg, builder.getInt64Ty()->getPointerTo(), "");
	return uint64_array_ptr_reg;
}

//	Returns pointer to the first byte of the first struct member.
static llvm::Value* generate_get_struct_base_ptr(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& struct_ptr_reg, const typeid_t& final_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	auto type = get_generic_struct_type(gen_acc.interner);
	auto ptr_reg = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::BitCast, &struct_ptr_reg, type->getPointerTo(), "");

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto ptr2_reg = builder.CreateGEP(type, ptr_reg, gep, "");
	auto final_type2 = get_exact_struct_type(gen_acc.interner, final_type);
	auto ptr3_reg = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::BitCast, ptr2_reg, final_type2->getPointerTo(), "");
	return ptr3_reg;
}




////////////////////////////////		EXPRESSIONS



static llvm::Value* generate_literal_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto literal = e.get_literal();
	return generate_constant(gen_acc, emit_f, literal);
}

static llvm::Value* generate_resolve_member_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::resolve_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.builder;

	auto parent_struct_ptr_reg = generate_expression(gen_acc, emit_f, *details.parent_address);

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_struct()){
		auto& struct_type_llvm = *get_exact_struct_type(gen_acc.interner, parent_type);

		const auto& struct_def = details.parent_address->get_output_type().get_struct();
		int member_index = find_struct_member_index(struct_def, details.member_name);
		QUARK_ASSERT(member_index != -1);

		const auto& member_type = struct_def._members[member_index]._type;

		auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, emit_f, *parent_struct_ptr_reg, parent_type);

		const auto gep = std::vector<llvm::Value*>{
			//	Struct array index.
			builder.getInt32(0),

			//	Struct member index.
			builder.getInt32(member_index)
		};
		llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, base_ptr_reg, gep, "");
		auto member_value_reg = builder.CreateLoad(member_ptr_reg);
		generate_retain(gen_acc, emit_f, *member_value_reg, member_type);
		generate_release(gen_acc, emit_f, *parent_struct_ptr_reg, parent_type);

		return member_value_reg;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}

static llvm::Value* generate_update_member_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::update_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.builder;

	auto parent_struct_ptr_reg = generate_expression(gen_acc, emit_f, *details.parent_address);
	auto new_value_reg = generate_expression(gen_acc, emit_f, *details.new_value);

	const auto struct_type = details.parent_address->get_output_type();
	auto member_index_reg = llvm::ConstantInt::get(builder.getInt64Ty(), details.member_index);
	const auto member_type = details.new_value->get_output_type();

	const auto f = find_function_def(gen_acc, "fr_update_struct_member");

	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(gen_acc.interner, emit_f),

		parent_struct_ptr_reg,
		generate_itype_constant(gen_acc, struct_type),

		member_index_reg,

		generate_cast_to_runtime_value(gen_acc, *new_value_reg, member_type),
		generate_itype_constant(gen_acc, member_type)
	};
	auto struct2_ptr_reg = builder.CreateCall(f.llvm_f, args2, "");

	generate_release(gen_acc, emit_f, *new_value_reg, member_type);
	generate_release(gen_acc, emit_f, *parent_struct_ptr_reg, struct_type);

	return struct2_ptr_reg;
}

static llvm::Value* generate_lookup_element_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto parent_reg = generate_expression(gen_acc, emit_f, *details.parent_address);
	auto key_reg = generate_expression(gen_acc, emit_f, *details.lookup_key);
	const auto key_type = details.lookup_key->get_output_type();

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_string()){
		QUARK_ASSERT(key_type.is_int());

		auto element_ptr_reg = generate_get_vec_element_ptr2(gen_acc, emit_f, *parent_reg);
		auto char_ptr_reg = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::BitCast, element_ptr_reg, builder.getInt8PtrTy(), "");

		const auto gep = std::vector<llvm::Value*>{ key_reg };
		llvm::Value* element_addr = builder.CreateGEP(llvm::Type::getInt8Ty(context), char_ptr_reg, gep, "element_addr");
		llvm::Value* element_value_8bit = builder.CreateLoad(element_addr, "element_tmp");
		llvm::Value* element_reg = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::SExt, element_value_8bit, builder.getInt64Ty(), "char_to_int64");

		generate_release(gen_acc, emit_f, *parent_reg, parent_type);

		//	No need to retain/release the element - it's an integer.

		return element_reg;
	}
	else if(parent_type.is_json_value()){
		QUARK_ASSERT(key_type.is_int() || key_type.is_string());

		//	Notice that we only know at RUNTIME if the json_value can be looked up: it needs to be json-object or a
		//	json-array. The key is either a string or an integer.

		const auto f = find_function_def(gen_acc, "floyd_runtime__lookup_json");

		std::vector<llvm::Value*> args = {
			get_callers_fcp(gen_acc.interner, emit_f),
			parent_reg,
			generate_cast_to_runtime_value(gen_acc, *key_reg, key_type),
			generate_itype_constant(gen_acc, key_type)
		};
		auto result = builder.CreateCall(f.llvm_f, args, "");

		generate_release(gen_acc, emit_f, *parent_reg, parent_type);
		generate_release(gen_acc, emit_f, *key_reg, key_type);
		return result;
	}
	else if(parent_type.is_vector()){
		QUARK_ASSERT(key_type.is_int());

		const auto element_type0 = parent_type.get_vector_element_type();
		auto uint64_array_ptr_reg = generate_get_vec_element_ptr(gen_acc, emit_f, *parent_reg);
		const auto gep = std::vector<llvm::Value*>{ key_reg };
		llvm::Value* element_addr_reg = builder.CreateGEP(builder.getInt64Ty(), uint64_array_ptr_reg, gep, "element_addr");
		llvm::Value* element_value_uint64_reg = builder.CreateLoad(element_addr_reg, "element_tmp");
		auto result_reg = generate_cast_from_runtime_value(gen_acc, *element_value_uint64_reg, element_type0);

		generate_retain(gen_acc, emit_f, *result_reg, element_type0);
		generate_release(gen_acc, emit_f, *parent_reg, parent_type);

		return result_reg;
	}
	else if(parent_type.is_dict()){
		QUARK_ASSERT(key_type.is_string());

		const auto element_type0 = parent_type.get_dict_value_type();

		const auto f = find_function_def(gen_acc, "floyd_runtime__lookup_dict");

		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(gen_acc.interner, emit_f),
			parent_reg,
			key_reg
		};
		auto element_value_uint64_reg = builder.CreateCall(f.llvm_f, args2, "");
		auto result_reg = generate_cast_from_runtime_value(gen_acc, *element_value_uint64_reg, element_type0);

		generate_retain(gen_acc, emit_f, *result_reg, element_type0);
		generate_release(gen_acc, emit_f, *parent_reg, parent_type);
		generate_release(gen_acc, emit_f, *key_reg, key_type);

		return result_reg;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}


static llvm::Value* generate_arithmetic_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.lhs->get_output_type();

	auto lhs_temp = generate_expression(gen_acc, emit_f, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, emit_f, *details.rhs);

	if(type.is_bool()){
		if(details.op == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract__2){
		}
		else if(details.op == expression_type::k_arithmetic_multiply__2){
		}
		else if(details.op == expression_type::k_arithmetic_divide__2){
		}
		else if(details.op == expression_type::k_arithmetic_remainder__2){
		}

		else if(details.op == expression_type::k_logical_and__2){
			return gen_acc.builder.CreateAnd(lhs_temp, rhs_temp, "logical_and_tmp");
		}
		else if(details.op == expression_type::k_logical_or__2){
			return gen_acc.builder.CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else{
			UNSUPPORTED();
		}
	}
	else if(type.is_int()){
		if(details.op == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract__2){
			return gen_acc.builder.CreateSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_multiply__2){
			return gen_acc.builder.CreateMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_divide__2){
			return gen_acc.builder.CreateSDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_remainder__2){
			return gen_acc.builder.CreateSRem(lhs_temp, rhs_temp, "reminder_tmp");
		}

		else if(details.op == expression_type::k_logical_and__2){
			return gen_acc.builder.CreateAnd(lhs_temp, rhs_temp, "logical_and_tmp");
		}
		else if(details.op == expression_type::k_logical_or__2){
			return gen_acc.builder.CreateOr(lhs_temp, rhs_temp, "logical_or_tmp");
		}
		else{
			UNSUPPORTED();
		}
	}
	else if(type.is_double()){
		if(details.op == expression_type::k_arithmetic_add__2){
			return gen_acc.builder.CreateFAdd(lhs_temp, rhs_temp, "add_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_subtract__2){
			return gen_acc.builder.CreateFSub(lhs_temp, rhs_temp, "subtract_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_multiply__2){
			return gen_acc.builder.CreateFMul(lhs_temp, rhs_temp, "mult_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_divide__2){
			return gen_acc.builder.CreateFDiv(lhs_temp, rhs_temp, "divide_tmp");
		}
		else if(details.op == expression_type::k_arithmetic_remainder__2){
			UNSUPPORTED();
		}
		else if(details.op == expression_type::k_logical_and__2){
			UNSUPPORTED();
		}
		else if(details.op == expression_type::k_logical_or__2){
			UNSUPPORTED();
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(type.is_string() || type.is_vector()){
		//	Only add supported. Future: don't use arithmetic add to concat collections!
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add__2);

		const auto def = find_function_def(gen_acc, "floyd_runtime__concatunate_vectors");
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(gen_acc.interner, emit_f),
			generate_itype_constant(gen_acc, type),
			lhs_temp,
			rhs_temp
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "");
		generate_release(gen_acc, emit_f, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, emit_f, *rhs_temp, *details.rhs->_output_type);
		return result;
	}
	else{
		//	No other types allowed.
		throw std::exception();
	}
	UNSUPPORTED();
}



static llvm::Value* generate_compare_values(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, expression_type op, const typeid_t& type, llvm::Value& lhs_reg, llvm::Value& rhs_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	const auto def = find_function_def(gen_acc, "floyd_runtime__compare_values");
	llvm::Value* op_reg = llvm::ConstantInt::get(builder.getInt64Ty(), static_cast<int64_t>(op));

	std::vector<llvm::Value*> args = {
		get_callers_fcp(gen_acc.interner, emit_f),
		op_reg,
		generate_itype_constant(gen_acc, type),
		generate_cast_to_runtime_value(gen_acc, lhs_reg, type),
		generate_cast_to_runtime_value(gen_acc, rhs_reg, type)
	};
	auto result = gen_acc.builder.CreateCall(def.llvm_f, args, "");

	//??? Return i1 directly, no need to compare again.
//	auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}


static llvm::Value* generate_comparison_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto lhs_temp = generate_expression(gen_acc, emit_f, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, emit_f, *details.rhs);

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
			{ expression_type::k_comparison_smaller_or_equal__2,			llvm::CmpInst::Predicate::ICMP_SLE },
			{ expression_type::k_comparison_smaller__2,						llvm::CmpInst::Predicate::ICMP_SLT },
			{ expression_type::k_comparison_larger_or_equal__2,				llvm::CmpInst::Predicate::ICMP_SGE },
			{ expression_type::k_comparison_larger__2,						llvm::CmpInst::Predicate::ICMP_SGT },

			{ expression_type::k_logical_equal__2,							llvm::CmpInst::Predicate::ICMP_EQ },
			{ expression_type::k_logical_nonequal__2,						llvm::CmpInst::Predicate::ICMP_NE }
		};
		const auto pred = conv_opcode_int.at(details.op);
		return gen_acc.builder.CreateICmp(pred, lhs_temp, rhs_temp);
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
			{ expression_type::k_comparison_smaller_or_equal__2,			llvm::CmpInst::Predicate::FCMP_OLE },
			{ expression_type::k_comparison_smaller__2,						llvm::CmpInst::Predicate::FCMP_OLT },
			{ expression_type::k_comparison_larger_or_equal__2,				llvm::CmpInst::Predicate::FCMP_OGE },
			{ expression_type::k_comparison_larger__2,						llvm::CmpInst::Predicate::FCMP_OGT },

			{ expression_type::k_logical_equal__2,							llvm::CmpInst::Predicate::FCMP_OEQ },
			{ expression_type::k_logical_nonequal__2,						llvm::CmpInst::Predicate::FCMP_ONE }
		};
		const auto pred = conv_opcode_int.at(details.op);
		return gen_acc.builder.CreateFCmp(pred, lhs_temp, rhs_temp);
	}
	else if(type.is_string() || type.is_vector()){
		auto result_reg = generate_compare_values(gen_acc, emit_f, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, emit_f, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, emit_f, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_dict()){
		auto result_reg = generate_compare_values(gen_acc, emit_f, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, emit_f, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, emit_f, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_struct()){
		auto result_reg = generate_compare_values(gen_acc, emit_f, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, emit_f, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, emit_f, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else if(type.is_json_value()){
		auto result_reg = generate_compare_values(gen_acc, emit_f, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, emit_f, *lhs_temp, *details.lhs->_output_type);
		generate_release(gen_acc, emit_f, *rhs_temp, *details.rhs->_output_type);
		return result_reg;
	}
	else{
		UNSUPPORTED();
	}
}

static llvm::Value* generate_arithmetic_unary_minus_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.expr->get_output_type();
	if(type.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, emit_f, e2);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_double(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, emit_f, e2);
	}
	else{
		UNSUPPORTED();
	}
}

static llvm::Value* generate_conditional_operator_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::conditional_t& conditional){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto result_type = e.get_output_type();
	const auto result_itype = get_exact_llvm_type(gen_acc.interner, result_type);

	llvm::Value* condition_reg = generate_expression(gen_acc, emit_f, *conditional.condition);

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* join_bb = llvm::BasicBlock::Create(context, "cond_operator-join");

	builder.CreateCondBr(condition_reg, then_bb, else_bb);


	// Emit then-value.
	builder.SetInsertPoint(then_bb);
	llvm::Value* then_reg = generate_expression(gen_acc, emit_f, *conditional.a);
	builder.CreateBr(join_bb);
	// Codegen of 'Then' can change the current block, update then_bb.
	llvm::BasicBlock* then_bb2 = builder.GetInsertBlock();


	// Emit else block.
	parent_function->getBasicBlockList().push_back(else_bb);
	builder.SetInsertPoint(else_bb);
	llvm::Value* else_reg = generate_expression(gen_acc, emit_f, *conditional.b);
	builder.CreateBr(join_bb);
	// Codegen of 'Else' can change the current block, update else_bb.
	llvm::BasicBlock* else_bb2 = builder.GetInsertBlock();


	// Emit join block.
	parent_function->getBasicBlockList().push_back(join_bb);
	builder.SetInsertPoint(join_bb);
	llvm::PHINode* phiNode = builder.CreatePHI(result_itype, 2, "cond_operator-result");
	phiNode->addIncoming(then_reg, then_bb2);
	phiNode->addIncoming(else_reg, else_bb2);

	//	Meaningless but shows that we handle condition_reg.
	generate_release(gen_acc, emit_f, *condition_reg, typeid_t::make_bool());

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
static llvm::Value* generate_call_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e0, const expression_t::call_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e0.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto callee_function_type = details.callee->get_output_type();
	const auto resolved_call_return_type = e0.get_output_type();
	//	IDEA: Build a complete resolved_function_type: argument types from the actual arguments and return = resolved_call_return_type.
	//	This lets us select specialisation of calls, like "string push_back(string, int)" vs [bool] push_back([bool], bool)

	const auto actual_call_arguments = mapf<typeid_t>(details.args, [](auto& e){ return e.get_output_type(); });

	const auto llvm_mapping = map_function_arguments(context, gen_acc.interner, callee_function_type);

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(details.args.size() == callee_function_type.get_function_args().size());

	llvm::Value* callee0_reg = generate_expression(gen_acc, emit_f, *details.callee);
	// Alternative: alter return type of callee0_reg to match resolved_call_return_type.

	//	Generate code that evaluates all argument expressions.
	std::vector<llvm::Value*> arg_regs;

	std::vector<std::pair<llvm::Value*, typeid_t> > destroy;

	for(const auto& out_arg: llvm_mapping.args){
		const auto& arg_details = details.args[out_arg.floyd_arg_index];

		if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			auto f_args = emit_f.args();
			QUARK_ASSERT((f_args.end() - f_args.begin()) >= 1);
			auto floyd_context_arg_ptr = f_args.begin();
			arg_regs.push_back(floyd_context_arg_ptr);
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
			llvm::Value* arg2 = generate_expression(gen_acc, emit_f, arg_details);
			arg_regs.push_back(arg2);
			destroy.push_back({ arg2, arg_details.get_output_type() });
		}

		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
			llvm::Value* arg2 = generate_expression(gen_acc, emit_f, arg_details);

			destroy.push_back({ arg2, arg_details.get_output_type() });

			//	Actual type of the argument, as specified inside the call expression. The concrete type for the DYN value for this call.
			const auto concrete_arg_type = arg_details.get_output_type();

			// We assume that the next arg in the llvm_mapping is the dyn-type and store it too.
			const auto packed_reg = generate_cast_to_runtime_value(gen_acc, *arg2, concrete_arg_type);
			arg_regs.push_back(packed_reg);
			arg_regs.push_back(generate_itype_constant(gen_acc, concrete_arg_type));
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
		generate_release(gen_acc, emit_f, *m.first, m.second);
	}
	//	Release callee?


	//	If the return type is dynamic, cast the returned int64 to the correct type.
	//	It must be retained already.
	llvm::Value* result = result0;
	if(callee_function_type.get_function_return().is_any()){
		auto wide_return_a_reg = builder.CreateExtractValue(result, { static_cast<int>(WIDE_RETURN_MEMBERS::a) });
		result = generate_cast_from_runtime_value(gen_acc, *wide_return_a_reg, resolved_call_return_type);
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}


//	Generates a call to the global function that implements the corecall.
static llvm::Value* generate_fallthrough_corecall(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::corecall_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	//	Find function
	const auto& symbols = gen_acc.scope_path[0];
    const auto it = std::find_if(symbols.begin(), symbols.end(), [&details](const resolved_symbol_t& e) { return (std::string("$") + e.symbol_name) == details.call_name; } );
    QUARK_ASSERT(it != symbols.end());
	const auto function_type = std::make_shared<typeid_t>(it->symbol.get_type());
	int global_index = static_cast<int>(it - symbols.begin());

	const auto call_details = expression_t::call_t {
		std::make_shared<expression_t>(expression_t::make_load2(variable_address_t::make_variable_address(-1, global_index), function_type)),
		details.args
	};

	const auto e2 = expression_t::make_call(*call_details.callee, call_details.args, std::make_shared<typeid_t>(e.get_output_type()));
	return generate_call_expression(gen_acc, emit_f, e2, call_details);
}


static llvm::Value* generate_corecall_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::corecall_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	if(details.call_name == get_opcode(make_assert_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_to_string_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_to_pretty_string_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_typeof_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_update_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_size_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_find_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_exists_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_erase_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_push_back_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_subset_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_replace_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_script_to_jsonvalue_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_jsonvalue_to_script_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_value_to_jsonvalue_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_jsonvalue_to_value_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_get_json_type_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_map_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_map_string_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_filter_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_reduce_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_supermap_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else if(details.call_name == get_opcode(make_print_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}
	else if(details.call_name == get_opcode(make_send_signature())){
		return generate_fallthrough_corecall(gen_acc, emit_f, e, details);
	}

	else{
		QUARK_ASSERT(false);
	}
}



//	Evaluate each element and store it directly into the array.
static void generate_fill_array(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& element_ptr_reg, llvm::Type& element_type, const std::vector<expression_t>& elements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& builder = gen_acc.builder;

	auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &element_ptr_reg, element_type.getPointerTo(), "");

	int element_index = 0;
	for(const auto& element_value: elements){

		//	Move ownwership from temp to member element, no need for retain-release.

		llvm::Value* element_value_reg = generate_expression(gen_acc, emit_f, element_value);
		generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
		element_index++;
	}
}


static llvm::Value* generate_construct_vector(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(details.value_type.is_vector());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto element_count = details.elements.size();
	const auto element_type0 = details.value_type.get_vector_element_type();
	auto& element_type1 = *get_exact_llvm_type(gen_acc.interner, element_type0);

	const auto f = find_function_def(gen_acc, "floyd_runtime__allocate_vector");
	const auto element_count_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), element_count);
	auto vec_ptr_reg = builder.CreateCall(f.llvm_f, { get_callers_fcp(gen_acc.interner, emit_f), element_count_reg }, "");
	auto ptr_reg = generate_get_vec_element_ptr2(gen_acc, emit_f, *vec_ptr_reg);

	if(element_type0.is_bool()){
		//	Each bool element is a uint64_t ???
		auto element_type = llvm::Type::getInt64Ty(context);
		auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, ptr_reg, element_type->getPointerTo(), "");

		//	Evaluate each element and store it directly into the the vector.
		int element_index = 0;
		for(const auto& arg: details.elements){
			llvm::Value* element0_reg = generate_expression(gen_acc, emit_f, arg);
			auto element_value_reg = builder.CreateCast(llvm::Instruction::CastOps::ZExt, element0_reg, make_runtime_value_type(context), "");
			generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
			element_index++;
		}
		return vec_ptr_reg;
	}
	else{
		generate_fill_array(gen_acc, emit_f, *ptr_reg, element_type1, details.elements);
		return vec_ptr_reg;
	}
}

static llvm::Value* generate_construct_dict(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(details.value_type.is_dict());

	auto& builder = gen_acc.builder;

	const auto element_type0 = details.value_type.get_dict_value_type();

	const auto f = find_function_def(gen_acc, "floyd_runtime__allocate_dict");
	auto dict_acc_ptr_reg = builder.CreateCall(f.llvm_f, { get_callers_fcp(gen_acc.interner, emit_f) }, "");

	//	Elements are stored as pairs.
	QUARK_ASSERT((details.elements.size() & 1) == 0);

	const auto count = details.elements.size() / 2;
	for(int element_index = 0 ; element_index < count ; element_index++){
		llvm::Value* key0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 0]);
		llvm::Value* element0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 1]);

		const auto f2 = find_function_def(gen_acc, "floyd_runtime__store_dict_mutable");
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(gen_acc.interner, emit_f),
			dict_acc_ptr_reg,
			key0_reg,
			generate_cast_to_runtime_value(gen_acc, *element0_reg, element_type0),
			generate_itype_constant(gen_acc, element_type0)
		};
		builder.CreateCall(f2.llvm_f, args2, "");

		generate_release(gen_acc, emit_f, *key0_reg, typeid_t::make_string());
	}
	return dict_acc_ptr_reg;
}

static llvm::Value* generate_construct_struct(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(details.value_type.is_struct());

	auto& builder = gen_acc.builder;
	auto& context = gen_acc.module->getContext();

	const auto target_type = details.value_type;
	const auto element_count = details.elements.size();

	const auto& struct_def = target_type.get_struct();
	auto& exact_struct_type = *get_exact_struct_type(gen_acc.interner, target_type);
	QUARK_ASSERT(struct_def._members.size() == element_count);


	const llvm::DataLayout& data_layout = gen_acc.module->getDataLayout();
	const llvm::StructLayout* layout = data_layout.getStructLayout(&exact_struct_type);
	const auto struct_bytes = layout->getSizeInBytes();


	const auto f = find_function_def(gen_acc, "floyd_runtime__allocate_struct");
	const auto size_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), struct_bytes);
	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(gen_acc.interner, emit_f),
		size_reg
	};
	//	Returns STRUCT_T*.
	auto generic_struct_ptr_reg = gen_acc.builder.CreateCall(f.llvm_f, args2, "");


	//!!! We basically inline the entire constructor here -- bad idea? Maybe generate a construction function and call it.
	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, emit_f, *generic_struct_ptr_reg, target_type);

	int member_index = 0;
	for(const auto& m: struct_def._members){
		const auto& arg = details.elements[member_index];
		llvm::Value* member_value_reg = generate_expression(gen_acc, emit_f, arg);

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


static llvm::Value* generate_construct_primitive(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
//	QUARK_ASSERT(details.value_type.is_struct());

	const auto target_type = details.value_type;
	const auto element_count = details.elements.size();
	QUARK_ASSERT(element_count == 1);

	//	Construct a primitive, using int(113) or double(3.14)

	auto element0_reg = generate_expression(gen_acc, emit_f, details.elements[0]);
	const auto input_value_type = details.elements[0].get_output_type();

	if(target_type.is_bool() || target_type.is_int() || target_type.is_double() || target_type.is_typeid()){
		return element0_reg;
	}

	//	NOTICE: string -> json_value needs to be handled at runtime.

	//	Automatically transform a json_value::string => string at runtime?
	else if(target_type.is_string() && input_value_type.is_json_value()){
		const auto f = find_function_def(gen_acc, "floyd_runtime__json_to_string");
		std::vector<llvm::Value*> args = {
			get_callers_fcp(gen_acc.interner, emit_f),
			element0_reg
		};
		auto result = gen_acc.builder.CreateCall(f.llvm_f, args, "");

		generate_release(gen_acc, emit_f, *element0_reg, input_value_type);
		return result;
	}
	else if(target_type.is_json_value()){
		//	Put a value_t into a json
		const auto f = find_function_def(gen_acc, "floyd_runtime__allocate_json");
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(gen_acc.interner, emit_f),
			generate_cast_to_runtime_value(gen_acc, *element0_reg, input_value_type),
			generate_itype_constant(gen_acc, input_value_type)
		};
		auto result = gen_acc.builder.CreateCall(f.llvm_f, args2, "");

		generate_release(gen_acc, emit_f, *element0_reg, input_value_type);
		return result;
	}
	else{
		return element0_reg;
	}
}

static llvm::Value* generate_construct_value_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	if(details.value_type.is_vector()){
		return generate_construct_vector(gen_acc, emit_f, details);
	}
	else if(details.value_type.is_dict()){
		return generate_construct_dict(gen_acc, emit_f, details);
	}
	else if(details.value_type.is_struct()){
		return generate_construct_struct(gen_acc, emit_f, details);
	}

	//	Construct a primitive, using int(113) or double(3.14)
	else{
		return generate_construct_primitive(gen_acc, emit_f, details);
	}
}

static llvm::Value* generate_load2_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::load2_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_symbol(gen_acc, details.address);
	auto result = gen_acc.builder.CreateLoad(dest.value_ptr, "temp");
	generate_retain(gen_acc, emit_f, *result, e.get_output_type());
	return result;
}

static llvm::Value* generate_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		llvm_code_generator_t& gen_acc;
		const expression_t& e;
		llvm::Function& emit_f;


		llvm::Value* operator()(const expression_t::literal_exp_t& expr) const{
			return generate_literal_expression(gen_acc, emit_f, e);
		}
		llvm::Value* operator()(const expression_t::arithmetic_t& expr) const{
			return generate_arithmetic_expression(gen_acc, emit_f, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::comparison_t& expr) const{
			return generate_comparison_expression(gen_acc, emit_f, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::unary_minus_t& expr) const{
			return generate_arithmetic_unary_minus_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::conditional_t& expr) const{
			return generate_conditional_operator_expression(gen_acc, emit_f, e, expr);
		}

		llvm::Value* operator()(const expression_t::call_t& expr) const{
			return generate_call_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::corecall_t& expr) const{
			return generate_corecall_expression(gen_acc, emit_f, e, expr);
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
			const auto s = find_symbol(gen_acc, expr.address);
			if(s.symtype == resolved_symbol_t::esymtype::k_function_argument){
				auto result = s.value_ptr;

				//	No need to retain function arguments! We should track which expression-outputs that need release.
				generate_retain(gen_acc, emit_f, *result, e.get_output_type());
				return result;
			}
			else{
				return generate_load2_expression(gen_acc, emit_f, e, expr);
			}
		}

		llvm::Value* operator()(const expression_t::resolve_member_t& expr) const{
			return generate_resolve_member_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::update_member_t& expr) const{
			return generate_update_member_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::lookup_t& expr) const{
			return generate_lookup_element_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::value_constructor_t& expr) const{
			return generate_construct_value_expression(gen_acc, emit_f, e, expr);
		}
	};

	llvm::Value* result = std::visit(visitor_t{ gen_acc, e, emit_f }, e._expression_variant);
	return result;
}



	
////////////////////////////////		STATEMENTS





static void generate_assign2_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::assign2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	llvm::Value* value = generate_expression(gen_acc, emit_f, s._expression);

	auto dest = find_symbol(gen_acc, s._dest_variable);
	const auto type = dest.symbol.get_type();

	if(is_rc_value(type)){
		auto prev_value = gen_acc.builder.CreateLoad(dest.value_ptr);
		generate_release(gen_acc, emit_f, *prev_value, type);

		//	No need to retain new value. generate_expression() takes care of that.
		gen_acc.builder.CreateStore(value, dest.value_ptr);
	}
	else{
		gen_acc.builder.CreateStore(value, dest.value_ptr);
	}

	QUARK_ASSERT(gen_acc.check_invariant());
}

static void generate_init2_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::init2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	llvm::Value* value = generate_expression(gen_acc, emit_f, s._expression);

	auto dest = find_symbol(gen_acc, s._dest_variable);

	//	No need to retain new value. generate_expression() takes care of that.
	gen_acc.builder.CreateStore(value, dest.value_ptr);

	QUARK_ASSERT(gen_acc.check_invariant());
}

static function_return_mode generate_block_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::block_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	return generate_block(gen_acc, emit_f, s._body);
}

static function_return_mode generate_ifelse_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	//	Notice that generate_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* condition_reg = generate_expression(gen_acc, emit_f, statement._condition);
	auto start_bb = builder.GetInsertBlock();

	auto then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	auto else_bb = llvm::BasicBlock::Create(context, "else", parent_function);

	builder.SetInsertPoint(start_bb);
	builder.CreateCondBr(condition_reg, then_bb, else_bb);


	// Emit then-block.
	builder.SetInsertPoint(then_bb);

	//	Notice that generate_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto then_mode = generate_block(gen_acc, emit_f, statement._then_body);
	auto then_bb2 = builder.GetInsertBlock();


	// Emit else-block.
	builder.SetInsertPoint(else_bb);

	//	Notice that generate_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto else_mode = generate_block(gen_acc, emit_f, statement._else_body);
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
static function_return_mode generate_for_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(
		statement._range_type == statement_t::for_statement_t::k_closed_range
		|| statement._range_type ==statement_t::for_statement_t::k_open_range
	);

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto forloop_bb = llvm::BasicBlock::Create(context, "for-loop", parent_function);
	auto forend_bb = llvm::BasicBlock::Create(context, "for-end", parent_function);



	//	EMIT LOOP SETUP INTO CURRENT BB

	//	Notice that generate_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* start_reg = generate_expression(gen_acc, emit_f, statement._start_expression);
	llvm::Value* end_reg = generate_expression(gen_acc, emit_f, statement._end_expression);

	auto values = generate_symbol_slots(gen_acc, emit_f, statement._body._symbol_table);

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	llvm::Value* counter_reg = values[0].value_ptr;
	builder.CreateStore(start_reg, counter_reg);

	llvm::Value* add_reg = generate_constant(gen_acc, emit_f, value_t::make_int(1));

	llvm::CmpInst::Predicate pred = statement._range_type == statement_t::for_statement_t::k_closed_range
		? llvm::CmpInst::Predicate::ICMP_SLE
		: llvm::CmpInst::Predicate::ICMP_SLT;

	auto test_reg = builder.CreateICmp(pred, start_reg, end_reg);
	builder.CreateCondBr(test_reg, forloop_bb, forend_bb);



	//	EMIT LOOP BB

	builder.SetInsertPoint(forloop_bb);

	const auto return_mode = generate_body(gen_acc, emit_f, values, statement._body._statements);

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

static function_return_mode generate_while_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond_bb = llvm::BasicBlock::Create(context, "while-cond", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);


	////////	current BB

	builder.CreateBr(while_cond_bb);


	////////	while_cond_bb

	builder.SetInsertPoint(while_cond_bb);
	llvm::Value* condition = generate_expression(gen_acc, emit_f, statement._condition);
	builder.CreateCondBr(condition, while_loop_bb, while_join_bb);


	////////	while_loop_bb

	builder.SetInsertPoint(while_loop_bb);
	const auto mode = generate_block(gen_acc, emit_f, statement._body);
	builder.CreateBr(while_cond_bb);

	if(mode == function_return_mode::some_path_not_returned){
	}
	else{
	}


	////////	while_join_bb

	builder.SetInsertPoint(while_join_bb);
	return function_return_mode::some_path_not_returned;
}

static void generate_expression_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::expression_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto result_reg = generate_expression(gen_acc, emit_f, s._expression);
	generate_release(gen_acc, emit_f, *result_reg, s._expression.get_output_type());

	QUARK_ASSERT(gen_acc.check_invariant());
}


static llvm::Value* generate_return_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::return_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	llvm::Value* value = generate_expression(gen_acc, emit_f, s._expression);

	//	Destruct all locals before unwinding.
	auto path = gen_acc.scope_path;
	while(path.size() > 1){
		generate_destruct_scope_locals(gen_acc, emit_f, path.back());
		path.pop_back();
	}

	return gen_acc.builder.CreateRet(value);
}

static function_return_mode generate_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		llvm_code_generator_t& acc0;
		llvm::Function& emit_f;

		function_return_mode operator()(const statement_t::return_statement_t& s) const{
			generate_return_statement(acc0, emit_f, s);
			return function_return_mode::return_executed;
		}
		function_return_mode operator()(const statement_t::define_struct_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}
		function_return_mode operator()(const statement_t::define_function_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}

		function_return_mode operator()(const statement_t::bind_local_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::assign_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::assign2_t& s) const{
			generate_assign2_statement(acc0, emit_f, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::init2_t& s) const{
			generate_init2_statement(acc0, emit_f, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::block_statement_t& s) const{
			return generate_block_statement(acc0, emit_f, s);
		}

		function_return_mode operator()(const statement_t::ifelse_statement_t& s) const{
			return generate_ifelse_statement(acc0, emit_f, s);
		}
		function_return_mode operator()(const statement_t::for_statement_t& s) const{
			return generate_for_statement(acc0, emit_f, s);
		}
		function_return_mode operator()(const statement_t::while_statement_t& s) const{
			return generate_while_statement(acc0, emit_f, s);
		}


		function_return_mode operator()(const statement_t::expression_statement_t& s) const{
			generate_expression_statement(acc0, emit_f, s);
			return function_return_mode::some_path_not_returned;
		}
		function_return_mode operator()(const statement_t::software_system_statement_t& s) const{
			UNSUPPORTED();
		}
		function_return_mode operator()(const statement_t::container_def_statement_t& s) const{
			UNSUPPORTED();
		}
	};

	return std::visit(visitor_t{ gen_acc, emit_f }, statement._contents);
}

static function_return_mode generate_statements(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());
			const auto mode = generate_statement(gen_acc, emit_f, statement);
			if(mode == function_return_mode::return_executed){
				return function_return_mode::return_executed;
			}
		}
	}
	return function_return_mode::some_path_not_returned;
}




//	Generates local symbols for arguments and local variables. Only toplevel of function, not nested scopes.
std::vector<resolved_symbol_t> generate_function_local_symbols(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(check_emitting_function(gen_acc.interner, emit_f));

	auto& context = gen_acc.module->getContext();

	const auto floyd_func = std::get<function_definition_t::floyd_func_t>(function_def._contents);

	const symbol_table_t& symbol_table = floyd_func._body->_symbol_table;

	const auto mapping0 = map_function_arguments(context, gen_acc.interner, function_def._function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	//	Make a resolved_symbol_t for each element in the symbol table. Some are local variables, some are arguments.
	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = get_exact_llvm_type(gen_acc.interner, type);

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
			auto f_args = emit_f.args();
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
			llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

			//	Init the slot if needed.
			if(e.second._init.is_undefined() == false){
				llvm::Value* c = generate_constant(gen_acc, emit_f, e.second._init);
				gen_acc.builder.CreateStore(c, dest);
			}
			const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
			result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
		}
	}
	QUARK_ASSERT(result.size() == symbol_table._symbols.size());
	return result;
}


static llvm::Value* generate_global(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_name.empty() == false);
	QUARK_ASSERT(symbol.check_invariant());

	auto& module = *gen_acc.module;

	const auto type0 = symbol.get_type();
	const auto itype = get_exact_llvm_type(gen_acc.interner, type0);

	if(symbol._init.is_undefined()){
		return generate_global0(module, symbol_name, *itype, nullptr);
	}
	else{
		auto init_reg = generate_constant(gen_acc, emit_f, symbol._init);
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
static std::vector<resolved_symbol_t> generate_globals_from_ast(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const semantic_ast_t& ast, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	std::vector<resolved_symbol_t> result;

	for(const auto& e: symbol_table._symbols){
		llvm::Value* value = generate_global(gen_acc, emit_f, e.first, e.second);
		const auto debug_str = "name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(value, debug_str, resolved_symbol_t::esymtype::k_global, e.first, e.second));

//		QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));
	}
	return result;
}

//	NOTICE: Fills-in the body of an existing LLVM function prototype.
static void generate_floyd_function_body(llvm_code_generator_t& gen_acc, function_id_t function_id, const floyd::function_definition_t& function_def, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto funcdef_name = compose_function_def_name(function_id, function_def);

	auto f = gen_acc.module->getFunction(funcdef_name);
	QUARK_ASSERT(check_invariant__function(f));
	llvm::Function& emit_f = *f;

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.instance->context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	auto symbol_table_values = generate_function_local_symbols(gen_acc, emit_f, function_def);

	const auto return_mode = generate_body(gen_acc, emit_f, symbol_table_values, body._statements);

	//	Not all paths returns a value!
	if(return_mode == function_return_mode::some_path_not_returned && function_def._function_type.get_function_return().is_void() == false){
		throw std::exception();
	}

	if(function_def._function_type.get_function_return().is_void()){
		gen_acc.builder.CreateRetVoid();
	}

	QUARK_ASSERT(check_invariant__function(f));
}

static void generate_all_floyd_function_bodies(llvm_code_generator_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	We have already generate the LLVM function-prototypes for the global functions in generate_module().
	for(function_id_t function_id = 0 ; function_id < semantic_ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = *semantic_ast._tree._function_defs[function_id];

		struct visitor_t {
			llvm_code_generator_t& gen_acc;
			const function_id_t function_id;
			const function_definition_t& function_def;

			void operator()(const function_definition_t::empty_t& e) const{
				QUARK_ASSERT(false);
			}
			void operator()(const function_definition_t::floyd_func_t& e) const{
				generate_floyd_function_body(gen_acc, function_id, function_def, *e._body);
			}
			void operator()(const function_definition_t::host_func_t& e) const{
			}
		};
		std::visit(visitor_t{ gen_acc, function_id, function_def }, function_def._contents);
	}
}


//	The AST contains statements that initializes the global variables, including global functions.

//	Function prototype must NOT EXIST already.
static llvm::Function* generate_function_prototype(llvm::Module& module, const llvm_type_interner_t& interner, const function_definition_t& function_def, function_id_t function_id){
	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(function_id >= 0);

	auto& context = module.getContext();

	const auto function_type = function_def._function_type;
	const auto funcdef_name = compose_function_def_name(function_id, function_def);

	auto existing_f = module.getFunction(funcdef_name);
	QUARK_ASSERT(existing_f == nullptr);


	const auto mapping0 = map_function_arguments(context, interner, function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	llvm::Type* function_ptr_type = get_exact_llvm_type(interner, function_type);
	const auto function_byvalue_type = deref_ptr(function_ptr_type);

	//	IMPORTANT: Must cast to (llvm::FunctionType*) to get correct overload of getOrInsertFunction() to be called!
	auto f3 = module.getOrInsertFunction(funcdef_name, (llvm::FunctionType*)function_byvalue_type);
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



//??? have better mechanism to register these.
static function_definition_t make_dummy_function_definition(){
	return function_definition_t::make_empty();
}

//	Create llvm function prototypes for each function.
static function_def_t generate_c_prototype(llvm::Module& module, const host_func_t& host_function){
	QUARK_ASSERT(check_invariant__module(&module));
//	QUARK_TRACE_SS(print_type(host_function.function_type));

//	QUARK_TRACE_SS(print_module(module));
	auto f = module.getOrInsertFunction(host_function.name_key, host_function.function_type);

//	QUARK_TRACE_SS(print_module(module));

	const auto result = function_def_t{ host_function.name_key, llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition() };

//	QUARK_TRACE_SS(print_module(module));

	QUARK_ASSERT(check_invariant__module(&module));

	return result;
}

//	Make LLVM functions -- runtime, floyd host functions, floyd functions.
static std::vector<function_def_t> make_all_function_prototypes(llvm::Module& module, const llvm_type_interner_t& interner, const function_defs_t& defs){
	QUARK_ASSERT(interner.check_invariant());

	std::vector<function_def_t> result;

	auto& context = module.getContext();

	const auto runtime_functions = get_runtime_functions(context, interner);
	for(const auto& e: runtime_functions){
		result.push_back(generate_c_prototype(module, e));
	}


	//	Register all function defs as LLVM function prototypes.
	//	host functions are later linked by LLVM execution engine, by matching the function names.
	//	Floyd functions are later filled with instructions.

	//	floyd_runtime_init()
	{
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(interner)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime_init", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}

	//	floyd_runtime_deinit()
	{
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(context),
			{
				make_frp_type(interner)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime_deinit", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}

	{
		for(function_id_t function_id = 0 ; function_id < defs.size() ; function_id++){
			const auto& function_def = *defs[function_id];
			auto f = generate_function_prototype(module, interner, function_def, function_id);
			result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), function_id, function_def});
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

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto def = find_function_def(gen_acc, "floyd_runtime_init");
	llvm::Function* f = def.llvm_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);
	llvm::BasicBlock* destructBB = llvm::BasicBlock::Create(context, "destruct", f);

	llvm::Function& emit_f = *f;

	//	entryBB
	{
		builder.SetInsertPoint(entryBB);

		//	Global statements, using the global symbol scope.
		//	This includes init2-statements to initialise global variables.
		generate_statements(gen_acc, emit_f, globals._statements);

		builder.CreateBr(destructBB);
	}

	//	destructBB
	{
		builder.SetInsertPoint(destructBB);

		llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 667);
		builder.CreateRet(dummy_result);
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

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto def = find_function_def(gen_acc, "floyd_runtime_deinit");
	llvm::Function* f = def.llvm_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);

	llvm::Function& emit_f = *f;

	//	entryBB
	{
		builder.SetInsertPoint(entryBB);

		//	Destruct global variables.
		for(const auto& e: gen_acc.scope_path.front()){
			if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
				const auto type = e.symbol.get_type();
				if(is_rc_value(type)){
					auto reg = builder.CreateLoad(e.value_ptr);
					generate_release(gen_acc, emit_f, *reg, type);
				}
				else{
				}
			}
		}

		llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 668);
		builder.CreateRet(dummy_result);
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

	llvm_code_generator_t gen_acc(instance, module.get(), semantic_ast._tree._interned_types);

	//	Generate all LLVM nodes: functions and globals.
	//	This lets all other code reference them, even if they're not filled up with code yet.
	{
		const auto funcs = make_all_function_prototypes(*module, gen_acc.interner, semantic_ast._tree._function_defs);
		gen_acc.function_defs = funcs;

		//	Global variables.
		{
			const auto def = find_function_def(gen_acc, "floyd_runtime_init");
			llvm::Function* f = def.llvm_f;

			std::vector<resolved_symbol_t> globals = generate_globals_from_ast(
				gen_acc,
				*f,
				semantic_ast,
				semantic_ast._tree._globals._symbol_table
			);
			gen_acc.scope_path = { globals };
		}
	}

/*
	{
		QUARK_SCOPED_TRACE("prototypes");
		QUARK_TRACE_SS(floyd::print_gen(gen_acc));
	}
*/

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());

	generate_all_floyd_function_bodies(gen_acc, semantic_ast);

	generate_floyd_runtime_init(gen_acc, semantic_ast._tree._globals);
	generate_floyd_runtime_deinit(gen_acc, semantic_ast._tree._globals);

	return { std::move(module), gen_acc.function_defs };
}



std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());
//	QUARK_ASSERT(module_name.empty() == false);

//	type_interner_t types = collect_used_types(ast0._tree);
//	QUARK_ASSERT(types.interned.size() == ast0._tree._interned_types.interned.size());

	if(k_trace_input_output){
		QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(semantic_ast_to_json(ast0)._value));
	}
	if(k_trace_types){
		{
			QUARK_SCOPED_TRACE("ast0 types");
			for(const auto& e: ast0._tree._interned_types.interned){
				QUARK_TRACE_SS(e.first.itype << ": " << typeid_to_compact_string(e.second));
			}
		}
/*
		{
			QUARK_SCOPED_TRACE("collected types");
			for(const auto& e: types.interned){
				QUARK_TRACE_SS(e.first.itype << ": " << typeid_to_compact_string(e.second));
			}
		}
*/

	}

	auto ast = ast0;

	auto result0 = generate_module(instance, module_name, ast);
	auto module = std::move(result0.first);
	auto funcs = result0.second;

	const auto interner = llvm_type_interner_t(instance.context, ast0._tree._interned_types);

	auto result = std::make_unique<llvm_ir_program_t>(&instance, module, interner, ast._tree._globals._symbol_table, funcs);

	result->container_def = ast0._tree._container_def;
	result->software_system = ast0._tree._software_system;

	if(k_trace_input_output){
		QUARK_TRACE_SS("result = " << floyd::print_program(*result));
	}
	return result;
}


//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_instance_t& instance, llvm_ir_program_t& program_breaks, const std::vector<std::string>& main_args){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

//???main_args

	auto ee = make_engine_run_init(instance, program_breaks);
	call_floyd_runtime_deinit(ee);
	return 0;
}



}	//	floyd
