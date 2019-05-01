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

//??? split host_functions.h into defintions vs inmplementation for bc-interpreter.
#include "host_functions.h"

#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "pass3.h"
#include "text_parser.h"
#include "ast_json.h"

#include "quark.h"

//http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html


typedef std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs_t;


/*
TODO:

- Make nicer mechanism to register all host functions, types and key-strings.
- Store explicit members like assert_f instead of search on string.
- Use _reg as suffix instead of _value.
- Need mechanism to map Floyd types vs machine-types.
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


struct llvm_code_generator_t {
	public: llvm_code_generator_t(llvm_instance_t& instance, llvm::Module* module, const type_interner_t& interner) :
		instance(&instance),
		module(module),
		builder(instance.context),
		interner(interner)
	{
		QUARK_ASSERT(instance.check_invariant());

		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();

		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
//		QUARK_ASSERT(scope_path.size() >= 1);
		QUARK_ASSERT(instance);
		QUARK_ASSERT(instance->check_invariant());
		QUARK_ASSERT(module);
		return true;
	}


	llvm_instance_t* instance;
	llvm::Module* module;
	llvm::IRBuilder<> builder;
	type_interner_t interner;
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




enum class gen_statement_mode {
	more,
	function_returning
};

static gen_statement_mode generate_statements(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements);
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


typeid_t unpack_itype(const llvm_code_generator_t& gen, int64_t itype){
	QUARK_ASSERT(gen.check_invariant());

	const itype_t t(static_cast<uint32_t>(itype));
	return lookup_type(gen.interner, t);
}

int64_t pack_itype(const llvm_code_generator_t& gen, const typeid_t& type){
	QUARK_ASSERT(gen.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return lookup_itype(gen.interner, type).itype;
}


/*
????????  static StructType *create(ArrayRef<Type *> Elements, StringRef Name, bool isPacked = false);
*/

static size_t get_struct_info____keep_for_copypaste(const llvm::DataLayout& data_layout, llvm::Type& type, int member_index){
	llvm::StructType* s2 = llvm::cast<llvm::StructType>(&type);

//	int size = data_layout.getTypeAllocSize(T);

	const auto member_count = s2->getNumElements();
	std::vector<llvm::Type*> member_types;
	for(int i = 0 ; i < member_count ; i++){
		auto t = s2->getElementType(i);
		member_types.push_back(t);
	}

	const llvm::StructLayout* layout = data_layout.getStructLayout(s2);
	const auto struct_bytes = layout->getSizeInBytes();

	const auto offset = layout->getElementOffset(member_index);
	return offset;
}

std::string compose_function_def_name(int function_id, const function_definition_t& def){
	const auto def_name = def._definition_name;
	const auto funcdef_name = def_name.empty() ? std::string() + "floyd_unnamed_function_" + std::to_string(function_id) : std::string("floyd_funcdef__") + def_name;
	return funcdef_name;
}



////////////////////////////////		CODE GENERATION PRIMITIVES

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//	Converts the LLVM value into a uint64_t for storing vector, pass as DYN value.
//	If the value is big, it's stored on the stack and a pointer returned => the returned value is not standalone and lifetime limited to emit function scope.
static llvm::Value* generate_encoded_value(llvm_code_generator_t& gen_acc, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	if(floyd_type.is_function()){
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, builder.getInt64Ty(), "function_as_arg");
	}
	else if(floyd_type.is_double()){
		return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, builder.getInt64Ty(), "double_as_arg");
	}
	else if(floyd_type.is_string()){
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, builder.getInt64Ty(), "string_as_arg");
	}
	else if(floyd_type.is_json_value()){
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, builder.getInt64Ty(), "json_as_arg");
	}
	else if(floyd_type.is_vector()){
		auto ptr = generate_vec_alloca(builder, &value);
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, ptr, builder.getInt64Ty(), "");
	}
	else if(floyd_type.is_dict()){
		auto ptr = generate_dict_alloca(builder, &value);
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, ptr, builder.getInt64Ty(), "");
	}
	else if(floyd_type.is_int()){
		return &value;
	}
	else if(floyd_type.is_typeid()){
		return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, builder.getInt64Ty(), "typeid_as_arg");
	}
	else if(floyd_type.is_bool()){
		return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, builder.getInt64Ty(), "bool_as_arg");
	}
	else if(floyd_type.is_struct()){
		return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, builder.getInt64Ty(), "");
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

static llvm::Value* generate_alloc_vec(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, uint64_t element_count, const std::string& debug){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto allocate_vector_func = find_function_def(gen_acc, "floyd_runtime__allocate_vector");

	const auto element_count_value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), element_count);

	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(emit_f),
		element_count_value
	};
	auto wide_return_reg = builder.CreateCall(allocate_vector_func.llvm_f, args2, "allocate_vector:" + debug);
	return generate__convert_wide_return_to_vec(builder, wide_return_reg);
}

static llvm::Value* generate_alloc_dict(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::string& debug){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto f = find_function_def(gen_acc, "floyd_runtime__allocate_dict");

	//	Local function, called once.
	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(emit_f)
	};
	auto wide_return_reg = builder.CreateCall(f.llvm_f, args2, "allocate_dict:" + debug);
	return generate__convert_wide_return_to_dict(builder, wide_return_reg);
}

static llvm::Value* generate_store_dict(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& dict_reg, llvm::Value& key_charptr_reg, llvm::Value& value_reg, llvm::Value& value_type_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto f = find_function_def(gen_acc, "floyd_runtime__store_dict");

//???			const auto packed_value = generate_encoded_value(gen_acc, *arg2, concrete_arg_type);

	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(emit_f),
		generate_dict_alloca(builder, &dict_reg),
		&key_charptr_reg,
		&value_reg,
		&value_type_reg
	};
	auto wide_return_reg = builder.CreateCall(f.llvm_f, args2, "store_dict:");
	return generate__convert_wide_return_to_dict(builder, wide_return_reg);
}

static llvm::Value* generate_lookup_dict(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& dict_reg, llvm::Value& key_charptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto f = find_function_def(gen_acc, "floyd_runtime__lookup_dict");

	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(emit_f),
		generate_dict_alloca(builder, &dict_reg),
		&key_charptr_reg
	};
	return builder.CreateCall(f.llvm_f, args2, "lookup_dict:");
}



static llvm::Value* generate_alloc_json(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& input_value_reg, const typeid_t& input_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto type_reg = llvm::ConstantInt::get(builder.getInt64Ty(), pack_itype(gen_acc, input_type));

	const auto allocate_vector_func = find_function_def(gen_acc, "floyd_runtime__allocate_json");
	std::vector<llvm::Value*> args2 = {
		get_callers_fcp(emit_f),
		generate_encoded_value(gen_acc, input_value_reg, input_type),
		type_reg
	};
	return builder.CreateCall(allocate_vector_func.llvm_f, args2, "allocate_json");
}

/*
			auto value_reg = generate_alloc_json(gen_acc, emit_f, input_value_type, element0_reg);
			if(input_value_type.is_string()){
				auto json = new json_t(
				return element0_reg;
	//			const auto arg = bcvalue_to_json(input_value);
	//			return bc_value_t::make_json_value(arg);
			}
			else{
				NOT_IMPLEMENTED_YET();
			}
		}
*/

static llvm::Value* generate_lookup_json(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& json_reg, llvm::Value& key_reg, const typeid_t& key_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& builder = gen_acc.builder;

	const auto f = find_function_def(gen_acc, "floyd_runtime__lookup_json");

	std::vector<llvm::Value*> args = {
		get_callers_fcp(emit_f),
		&json_reg,
		generate_encoded_value(gen_acc, key_reg, key_type),
		llvm::ConstantInt::get(builder.getInt64Ty(), pack_itype(gen_acc, key_type))
	};
	return builder.CreateCall(f.llvm_f, args, "lookup_json");
}


static llvm::Value* generate_json_to_string(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& json_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& builder = gen_acc.builder;

	const auto f = find_function_def(gen_acc, "floyd_runtime__json_to_string");

	std::vector<llvm::Value*> args = {
		get_callers_fcp(emit_f),
		&json_reg
	};
	return builder.CreateCall(f.llvm_f, args, "json_to_string");
}


static llvm::Value* generate_allocate_memory(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& bytes_reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	const auto def = find_function_def(gen_acc, "floyd_runtime__allocate_memory");

	std::vector<llvm::Value*> args = {
		get_callers_fcp(emit_f),
		&bytes_reg
	};
	return gen_acc.builder.CreateCall(def.llvm_f, args, "allocate_memory");
}

//??? Make this function take a builder, not gen_acc.
//	Makes constant from a Floyd value.

//	??? => generate_constant_reg()
llvm::Constant* generate_constant(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const value_t& value){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto& builder = gen_acc.builder;
	auto& context = gen_acc.module->getContext();

	const auto type = value.get_type();
	const auto itype = intern_type(context, type);

	if(type.is_undefined()){
		return llvm::ConstantInt::get(itype, 17);
	}
	else if(type.is_any()){
		return llvm::ConstantInt::get(itype, 13);
	}
	else if(type.is_void()){
		UNSUPPORTED();
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
//		llvm::Constant* array = llvm::ConstantDataArray::getString(context, value.get_string_value(), true);

		// The type of your string will be [n x i8], it needs to be i8*, so we cast here. We
		// explicitly use the type of printf's first arg to guarantee we are always right.

//		llvm::PointerType* int8Ptr_type = llvm::Type::getInt8PtrTy(context);

		llvm::Constant* c2 = builder.CreateGlobalStringPtr(value.get_string_value());
		//	, "cast [n x i8] to i8*"
//		llvm::Constant* c = gen_acc.builder.CreatePointerCast(array, int8Ptr_type);
		return c2;

//		return gen_acc.builder.CreateGlobalStringPtr(llvm::StringRef(value.get_string_value()));
	}
	else if(type.is_json_value()){
		const auto& json_value0 = value.get_json_value();
		if(json_value0.is_null()){
			return llvm::ConstantPointerNull::get(llvm::Type::getInt16PtrTy(context));
		}
		else{
			UNSUPPORTED();
		}
	}
	else if(type.is_typeid()){
		const auto t = pack_itype(gen_acc, value.get_typeid_value());
		return llvm::ConstantInt::get(itype, t);
	}
	else if(type.is_struct()){
		UNSUPPORTED();
	}
	else if(type.is_vector()){
		UNSUPPORTED();
	}
	else if(type.is_dict()){
		UNSUPPORTED();
	}
	else if(type.is_function()){
		const auto function_id = value.get_function_value();
		for(const auto& e: gen_acc.function_defs){
			if(e.floyd_function_id == function_id){
				return e.llvm_f;
			}
		}
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(type.is_unresolved_type_identifier()){
		UNSUPPORTED();
	}
	else{
		QUARK_ASSERT(false);
		return nullptr;
	}
}

//??? Known at compile time!
static llvm::Value* generate_type_size_calculation(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Type& type){
	auto& builder = gen_acc.builder;
/*
	Calc struct size for malloc:
	%Size = getelementptr %T* null, i32 1
	%SizeI = ptrtoint %T* %Size to i32
*/

	auto null_ptr = llvm::ConstantPointerNull::get(type.getPointerTo());
	auto element_index_reg = generate_constant(gen_acc, emit_f, value_t::make_int(1));

	const auto gep_index_list2 = std::vector<llvm::Value*>{ element_index_reg };
	llvm::Value* ptr_reg = builder.CreateGEP(&type, null_ptr, gep_index_list2, "");
	auto int_reg = builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, ptr_reg, builder.getInt64Ty(), "calcsize");
	return int_reg;
}

static llvm::Value* generate_allocate_instance(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Type& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& builder = gen_acc.builder;
	auto size_reg = generate_type_size_calculation(gen_acc, emit_f, type);
	auto memory_ptr_reg = generate_allocate_memory(gen_acc, emit_f, *size_reg);
	auto ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, memory_ptr_reg, type.getPointerTo(), "");
	return ptr_reg;
}

static std::vector<resolved_symbol_t> generate_symbol_slots(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(symbol_table.check_invariant());

	auto& context = gen_acc.module->getContext();

	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(context, type);

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


static gen_statement_mode generate_block(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto values = generate_symbol_slots(gen_acc, emit_f, body._symbol_table);

	gen_acc.scope_path.push_back(values);
	const auto more = generate_statements(gen_acc, emit_f, body._statements);
	gen_acc.scope_path.pop_back();

	QUARK_ASSERT(gen_acc.check_invariant());
	return more;
}


////////////////////////////////		EXPRESSIONS



static llvm::Value* generate_literal_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto literal = e.get_literal();
	return generate_constant(gen_acc, emit_f, literal);
}

static llvm::Value* generate_resolve_member_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::resolve_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto parent_struct_ptr_reg = generate_expression(gen_acc, emit_f, *details.parent_address);

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_struct()){
		auto& struct_type_llvm = *make_struct_type(context, parent_type);

		const auto& struct_def = details.parent_address->get_output_type().get_struct();
		int member_index = find_struct_member_index(struct_def, details.member_name);
		QUARK_ASSERT(member_index != -1);

		const auto gep = std::vector<llvm::Value*>{
			//	Struct array index.
			builder.getInt32(0),

			//	Struct member index.
			builder.getInt32(member_index)
		};
		llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, parent_struct_ptr_reg, gep, "");
		auto member_value_reg = builder.CreateLoad(member_ptr_reg);
		return member_value_reg;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}

static llvm::Value* generate_lookup_element_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto parent_reg = generate_expression(gen_acc, emit_f, *details.parent_address);
	auto key_reg = generate_expression(gen_acc, emit_f, *details.lookup_key);
	const auto key_type = details.lookup_key->get_output_type();

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_string()){
		QUARK_ASSERT(key_type.is_int());

		auto element_index = key_reg;
		const auto index_list = std::vector<llvm::Value*>{ element_index };
		llvm::Value* element_addr = builder.CreateGEP(llvm::Type::getInt8Ty(context), parent_reg, index_list, "element_addr");
		llvm::Value* element_value_8bit = builder.CreateLoad(element_addr, "element_tmp");
		llvm::Type* output_type = llvm::Type::getInt64Ty(context);

		llvm::Value* element_value = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::SExt, element_value_8bit, output_type, "char_to_int64");
		return element_value;
	}
	else if(parent_type.is_json_value()){
		QUARK_ASSERT(key_type.is_int() || key_type.is_string());

		//	Notice that we only know at runtime if the json_value can be looked up: it needs to be json-object or a json-array. The key is either a string or an integer.
		return generate_lookup_json(gen_acc, emit_f, *parent_reg, *key_reg, key_type);
	}
	else if(parent_type.is_vector()){
		QUARK_ASSERT(key_type.is_int());

		//	parent_reg is a VEC_T byvalue.
		auto element_index = key_reg;

		auto uint64_array_ptr_reg = builder.CreateExtractValue(parent_reg, { (int)VEC_T_MEMBERS::element_ptr });

		const auto element_type0 = parent_type.get_vector_element_type();

		const auto gep = std::vector<llvm::Value*>{ element_index };
		llvm::Value* element_addr_reg = builder.CreateGEP(builder.getInt64Ty(), uint64_array_ptr_reg, gep, "element_addr");
		llvm::Value* element_value_uint64_reg = builder.CreateLoad(element_addr_reg, "element_tmp");

		//??? copied from vector equivalent. Use util function for all member-values: vector and dicts alike -- no testing here.

		if(element_type0.is_int()){
			return element_value_uint64_reg;
		}
		else if(element_type0.is_string()){
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, llvm::Type::getInt8PtrTy(context), "");
			return v;
		}
		else if(element_type0.is_json_value()){
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, llvm::Type::getInt16PtrTy(context), "");
			return v;
		}
		else if(element_type0.is_struct()){
			auto& struct_type = *make_struct_type(context, element_type0);
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, struct_type.getPointerTo(), "");
			return v;
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(parent_type.is_dict()){
		QUARK_ASSERT(key_type.is_string());
		const auto element_type0 = parent_type.get_dict_value_type();

		//??? copied from vector equivalent. Use util function for all member-values: vector and dicts alike -- no testing here.

		auto element_value_uint64_reg = generate_lookup_dict(gen_acc, emit_f, *parent_reg, *key_reg);
		if(element_type0.is_int()){
			return element_value_uint64_reg;
		}
		else if(element_type0.is_string()){
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, llvm::Type::getInt8PtrTy(context), "");
			return v;
		}
		else if(element_type0.is_json_value()){
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, llvm::Type::getInt16PtrTy(context), "");
			return v;
		}
		else if(element_type0.is_struct()){
			auto& struct_type = *make_struct_type(context, element_type0);
			llvm::Value* v = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, element_value_uint64_reg, struct_type.getPointerTo(), "");
			return v;
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}


static llvm::Value* generate_arithmetic_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.lhs->get_output_type();
	auto& builder = gen_acc.builder;

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
	else if(type.is_string()){
		//	Only add supported. Future: don't use arithmetic add to concat collections!
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add__2);

		const auto def = find_function_def(gen_acc, "floyd_runtime__concatunate_strings");
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			lhs_temp,
			rhs_temp
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "concatunate_strings");
		return result;
	}
	else if(type.is_vector()){
		//	Only add supported. Future: don't use arithmetic add to concat collections!
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add__2);

		const auto def = find_function_def(gen_acc, "floyd_runtime__concatunate_vectors");
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			generate_vec_alloca(builder, lhs_temp),
			generate_vec_alloca(builder, rhs_temp)
		};
		auto wide_return_reg = gen_acc.builder.CreateCall(def.llvm_f, args2, "concatunate_vectors");
		auto vec_reg = generate__convert_wide_return_to_vec(builder, wide_return_reg);
		return vec_reg;
	}
	else{
		//	No other types allowed.
		throw std::exception();
	}
	UNSUPPORTED();
}

static llvm::Value* generate_comparison_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto lhs_temp = generate_expression(gen_acc, emit_f, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, emit_f, *details.rhs);

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = details.lhs->get_output_type();

	//	Output reg always a bool.
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
	else if(type.is_string()){
		const auto def = find_function_def(gen_acc, "floyd_runtime__compare_strings");
		llvm::Value* op_value = generate_constant(gen_acc, emit_f, value_t::make_int(static_cast<int64_t>(details.op)));
		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			op_value,
			lhs_temp,
			rhs_temp,
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "compare_strings");

		//??? Return i1 directly, no need to compare again.
		auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

		QUARK_ASSERT(gen_acc.check_invariant());
		return result2;
	}
	else if(type.is_vector()){
		const auto def = find_function_def(gen_acc, "floyd_runtime__compare_values");
		llvm::Value* op_value = generate_constant(gen_acc, emit_f, value_t::make_int(static_cast<int64_t>(details.op)));
		llvm::Value* itype_reg = generate_constant(gen_acc, emit_f, value_t::make_int(pack_itype(gen_acc, type)));

		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			op_value,
			itype_reg,
			generate_encoded_value(gen_acc, *lhs_temp, type),
			generate_encoded_value(gen_acc, *rhs_temp, type)
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "floyd_runtime__compare_values");

		//??? Return i1 directly, no need to compare again.
		auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

		QUARK_ASSERT(gen_acc.check_invariant());
		return result2;
	}
	else if(type.is_dict()){
		const auto def = find_function_def(gen_acc, "floyd_runtime__compare_values");
		llvm::Value* op_value = generate_constant(gen_acc, emit_f, value_t::make_int(static_cast<int64_t>(details.op)));
		llvm::Value* itype_reg = generate_constant(gen_acc, emit_f, value_t::make_int(pack_itype(gen_acc, type)));

		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			op_value,
			itype_reg,
			generate_encoded_value(gen_acc, *lhs_temp, type),
			generate_encoded_value(gen_acc, *rhs_temp, type)
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "floyd_runtime__compare_values");

		//??? Return i1 directly, no need to compare again.
		auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

		QUARK_ASSERT(gen_acc.check_invariant());
		return result2;
	}
	else if(type.is_struct()){
		const auto def = find_function_def(gen_acc, "floyd_runtime__compare_values");
		llvm::Value* op_value = generate_constant(gen_acc, emit_f, value_t::make_int(static_cast<int64_t>(details.op)));
		llvm::Value* itype_reg = generate_constant(gen_acc, emit_f, value_t::make_int(pack_itype(gen_acc, type)));

		std::vector<llvm::Value*> args2 = {
			get_callers_fcp(emit_f),
			op_value,
			itype_reg,
			generate_encoded_value(gen_acc, *lhs_temp, type),
			generate_encoded_value(gen_acc, *rhs_temp, type)
		};
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "floyd_runtime__compare_values");

		//??? Return i1 directly, no need to compare again.
		auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

		QUARK_ASSERT(gen_acc.check_invariant());
		return result2;
	}
	else{
		NOT_IMPLEMENTED_YET();
#if 0
		//	Bool tells if to flip left / right.
		static const std::map<expression_type, std::pair<bool, bc_opcode>> conv_opcode = {
			{ expression_type::k_comparison_smaller_or_equal__2,			{ false, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_smaller__2,						{ false, bc_opcode::k_comparison_smaller } },
			{ expression_type::k_comparison_larger_or_equal__2,				{ true, bc_opcode::k_comparison_smaller_or_equal } },
			{ expression_type::k_comparison_larger__2,						{ true, bc_opcode::k_comparison_smaller } },

			{ expression_type::k_logical_equal__2,							{ false, bc_opcode::k_logical_equal } },
			{ expression_type::k_logical_nonequal__2,						{ false, bc_opcode::k_logical_nonequal } }
		};

		const auto result = conv_opcode.at(details.op);
		if(result.first == false){
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, left_expr._out, right_expr._out));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(result.second, target_reg2, right_expr._out, left_expr._out));
		}
#endif
		return nullptr;
	}
}

static llvm::Value* generate_arithmetic_unary_minus_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
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
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto result_type = e.get_output_type();
	const auto result_itype = intern_type(context, result_type);

	llvm::Value* condition_value = generate_expression(gen_acc, emit_f, *conditional.condition);

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* join_bb = llvm::BasicBlock::Create(context, "cond_operator-join");

	builder.CreateCondBr(condition_value, then_bb, else_bb);


	// Emit then-value.
	builder.SetInsertPoint(then_bb);
	llvm::Value* then_value = generate_expression(gen_acc, emit_f, *conditional.a);
	builder.CreateBr(join_bb);
	// Codegen of 'Then' can change the current block, update then_bb.
	llvm::BasicBlock* then_bb2 = builder.GetInsertBlock();


	// Emit else block.
	parent_function->getBasicBlockList().push_back(else_bb);
	builder.SetInsertPoint(else_bb);
	llvm::Value* else_value = generate_expression(gen_acc, emit_f, *conditional.b);
	builder.CreateBr(join_bb);
	// Codegen of 'Else' can change the current block, update else_bb.
	llvm::BasicBlock* else_bb2 = builder.GetInsertBlock();


	// Emit join block.
	parent_function->getBasicBlockList().push_back(join_bb);
	builder.SetInsertPoint(join_bb);
	llvm::PHINode* phiNode = builder.CreatePHI(result_itype, 2, "cond_operator-result");
	phiNode->addIncoming(then_value, then_bb2);
	phiNode->addIncoming(else_value, else_bb2);

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
static llvm::Value* generate_call_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::call_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto callee_function_type = details.callee->get_output_type();
	const auto resolved_call_return_type = e.get_output_type();
	//	IDEA: Build a complete resolved_function_type: argument types from the actual arguments and return = resolved_call_return_type.
	//	This lets us select specialisation of calls, like "string push_back(string, int)" vs [bool] push_back([bool], bool)

	const auto actual_call_arguments = mapf<typeid_t>(details.args, [](auto& e){ return e.get_output_type(); });

	const auto llvm_mapping = map_function_arguments(context, callee_function_type);

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(details.args.size() == callee_function_type.get_function_args().size());

	llvm::Value* callee0_value = generate_expression(gen_acc, emit_f, *details.callee);
	// Alternative: alter return type of callee0_value to match resolved_call_return_type.

	//	Generate code that evaluates all argument expressions.
	std::vector<llvm::Value*> arg_values;
	for(const auto& out_arg: llvm_mapping.args){
		if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			auto f_args = emit_f.args();
			QUARK_ASSERT((f_args.end() - f_args.begin()) >= 1);
			auto floyd_context_arg_ptr = f_args.begin();
			arg_values.push_back(floyd_context_arg_ptr);
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_simple_value){
			llvm::Value* arg2 = generate_expression(gen_acc, emit_f, details.args[out_arg.floyd_arg_index]);
			arg_values.push_back(arg2);
		}

		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
			llvm::Value* arg2 = generate_expression(gen_acc, emit_f, details.args[out_arg.floyd_arg_index]);

			//	Actual type of the argument, as specified inside the call expression. The concrete type for the DYN value for this call.
			const auto concrete_arg_type = details.args[out_arg.floyd_arg_index].get_output_type();

			// We assume that the next arg in the llvm_mapping is the dyn-type and store it too.
			const auto itype = pack_itype(gen_acc, concrete_arg_type);
			const auto packed_value = generate_encoded_value(gen_acc, *arg2, concrete_arg_type);
			arg_values.push_back(packed_value);
			arg_values.push_back(llvm::ConstantInt::get(builder.getInt64Ty(), itype));
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	QUARK_ASSERT(arg_values.size() == llvm_mapping.args.size());
	auto result0 = builder.CreateCall(callee0_value, arg_values, callee_function_type.get_function_return().is_void() ? "" : "call_result");

	//	If the return type is dynamic, cast the returned int64 to the correct type.
	llvm::Value* result = result0;
	if(callee_function_type.get_function_return().is_any()){
		if(resolved_call_return_type.is_string()){
			auto wide_return_a_reg = builder.CreateExtractValue(result, { static_cast<int>(WIDE_RETURN_MEMBERS::a) });
//			auto dyn_b = builder.CreateExtractValue(result, { static_cast<int>(WIDE_RETURN_MEMBERS::b) });
			result = builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, wide_return_a_reg, llvm::Type::getInt8PtrTy(context), "encoded->string");
		}
		else if(resolved_call_return_type.is_vector()){
			return generate__convert_wide_return_to_vec(builder, result0);
		}
		else if(resolved_call_return_type.is_dict()){
			return generate__convert_wide_return_to_dict(builder, result0);
		}
		else if(resolved_call_return_type.is_struct()){
			auto struct_type = make_struct_type(context, resolved_call_return_type);
			auto wide_return_a_reg = builder.CreateExtractValue(result, { static_cast<int>(WIDE_RETURN_MEMBERS::a) });
			result = builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, wide_return_a_reg, struct_type->getPointerTo(), "encoded->structptr");
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}

//	Evaluate each element and store it directly into the array.
static void generate_fill_array(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, llvm::Value& uint64_array_ptr_reg, llvm::Type& element_type, const std::vector<expression_t>& elements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, &uint64_array_ptr_reg, element_type.getPointerTo(), "");

	int element_index = 0;
	for(const auto& element_value: elements){
		llvm::Value* element_value_reg = generate_expression(gen_acc, emit_f, element_value);
		generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
		element_index++;
	}
}

/*
		%struct.MYS_T = type { i64, i32, double }

		; Function Attrs: noinline nounwind optnone
		define void @test(%struct.MYS_T* noalias sret) #0 {
		  %2 = alloca %struct.MYS_T, align 8
		  %3 = getelementptr inbounds %struct.MYS_T, %struct.MYS_T* %2, i32 0, i32 0
		  store i64 1311953686388150836, i64* %3, align 8
		  %4 = getelementptr inbounds %struct.MYS_T, %struct.MYS_T* %2, i32 0, i32 1
		  store i32 -559038737, i32* %4, align 8
		  %5 = getelementptr inbounds %struct.MYS_T, %struct.MYS_T* %2, i32 0, i32 2
		  store double 3.141500e+00, double* %5, align 8
		  %6 = bitcast %struct.MYS_T* %0 to i8*
		  %7 = bitcast %struct.MYS_T* %2 to i8*
		  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %6, i8* %7, i64 24, i32 8, i1 false)
		  ret void
		}
*/

static llvm::Value* generate_construct_value_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto target_type = e.get_output_type();
	const auto element_count = details.elements.size();

	if(target_type.is_vector()){
		//??? Try storing vectors in vectors

		const auto element_type0 = target_type.get_vector_element_type();

		auto vec_reg = generate_alloc_vec(gen_acc, emit_f, element_count, typeid_to_compact_string(target_type));
		auto uint64_array_ptr_reg = builder.CreateExtractValue(vec_reg, { (int)VEC_T_MEMBERS::element_ptr });

		if(element_type0.is_bool()){
			//	Each element is a uint64_t ???
			auto element_type = llvm::Type::getInt64Ty(context);
			auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, uint64_array_ptr_reg, element_type->getPointerTo(), "");

			//	Evaluate each element and store it directly into the the vector.
			int element_index = 0;
			for(const auto& arg: details.elements){
				llvm::Value* element0_reg = generate_expression(gen_acc, emit_f, arg);
				auto element_value_reg = builder.CreateCast(llvm::Instruction::CastOps::ZExt, element0_reg, builder.getInt64Ty(), "");
				generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
				element_index++;
			}
			return vec_reg;

/*
			//	Store 64 bits per uint64_t.
			const auto input_element_count = caller_arg_count;

			//	Each element is a bit, packed into the 64bit elements of VEC_T.
			auto output_element_count = (input_element_count / 64) + ((input_element_count & 63) ? 1 : 0);
			auto vec_value = generate_alloc_vec(gen_acc, 1, output_element_count, "[bool]");

			auto uint64_array_ptr_reg = builder.CreateExtractValue(vec_value, { (int)VEC_T_MEMBERS::array_ptr_reg });

			//	Evaluate each element and store it directly into the the vector.
			auto buffer_word = builder.CreateAlloca(gen_acc.builder.getInt64Ty());
			auto zero = llvm::ConstantInt::get(gen_acc.builder.getInt64Ty(), 0);
			auto one = llvm::ConstantInt::get(gen_acc.builder.getInt64Ty(), 1);

			int input_element_index = 0;
			int output_element_index = 0;
			while(input_element_index < input_element_count){
				//	Clear 64-bit word.
				builder.CreateStore(zero, buffer_word);

				const auto batch_size = std::min(input_element_count, (size_t)64);
				for(int i = 0 ; i < batch_size ; i++){
					auto arg_value = generate_expression(gen_acc, emit_f, details.elements[input_element_index]);
					auto arg32_value = builder.CreateZExt(arg_value, gen_acc.builder.getInt64Ty());

					//	Store the bool into the buffer_word 64bit word at the correct bit position.
					auto bit_value = builder.CreateShl(arg32_value, one);
					auto word1 = builder.CreateLoad(buffer_word);
					auto word2 = builder.CreateOr(word1, bit_value);
					builder.CreateStore(word2, buffer_word);

					input_element_index++;
				}

				//	Store the 64-bit word to the VEC_T.
				auto word_out = builder.CreateLoad(buffer_word);

				auto output_element_index_value = generate_constant(gen_acc, value_t::make_int(output_element_index));
				const auto gep_index_list2 = std::vector<llvm::Value*>{ output_element_index_value };
				llvm::Value* e_addr = builder.CreateGEP(gen_acc.builder.getInt64Ty(), uint64_array_ptr_reg, gep_index_list2, "");
				builder.CreateStore(word_out, e_addr);

				output_element_index++;
			}
			return vec_value;
		}
*/

		}
		else if(element_type0.is_string()){
			//	Each string element is a char*.
			generate_fill_array(gen_acc, emit_f, *uint64_array_ptr_reg, *llvm::Type::getInt8PtrTy(context), details.elements);
			return vec_reg;
		}
		else if(element_type0.is_json_value()){
			generate_fill_array(gen_acc, emit_f, *uint64_array_ptr_reg, *llvm::Type::getInt16PtrTy(context), details.elements);
			return vec_reg;
		}
		else if(element_type0.is_struct()){
			//	Structs are stored as pointer-to-struct in the vector elements.
			generate_fill_array(gen_acc, emit_f, *uint64_array_ptr_reg, *make_struct_type(context, element_type0)->getPointerTo(), details.elements);
			return vec_reg;
		}
		else if(element_type0.is_int()){
			generate_fill_array(gen_acc, emit_f, *uint64_array_ptr_reg, *llvm::Type::getInt64Ty(context), details.elements);
			return vec_reg;
		}
		else if(element_type0.is_double()){
			generate_fill_array(gen_acc, emit_f, *uint64_array_ptr_reg, *builder.getDoubleTy(), details.elements);
			return vec_reg;
		}
		//??? All types of elements must be possible in a vector!
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(target_type.is_dict()){
		const auto element_type0 = target_type.get_dict_value_type();

		auto dict_acc_reg = generate_alloc_dict(gen_acc, emit_f, typeid_to_compact_string(target_type));
//		auto body_ptr_reg = builder.CreateExtractValue(dict_reg, { (int)DICT_T_MEMBERS::body_ptr });

		auto value_itype = lookup_itype(gen_acc.interner, element_type0);

/*
			const auto itype = pack_itype(gen_acc, concrete_arg_type);
			const auto packed_value = generate_encoded_value(gen_acc, *arg2, concrete_arg_type);
*/

		auto value_type_reg = llvm::ConstantInt::get(builder.getInt64Ty(), value_itype.itype);

		if(element_type0.is_int()){
			//	Elements are stored as pairs.
			QUARK_ASSERT((details.elements.size() & 1) == 0);
			const auto count = details.elements.size() / 2;
			for(int element_index = 0 ; element_index < count ; element_index++){
				llvm::Value* key0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 0]);
				llvm::Value* element0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 1]);
				auto dict2_reg = generate_store_dict(gen_acc, emit_f, *dict_acc_reg, *key0_reg, *element0_reg, *value_type_reg);

				dict_acc_reg = dict2_reg;
			}
			return dict_acc_reg;
		}
		else if(element_type0.is_json_value()){
			//	Elements are stored as pairs.
			QUARK_ASSERT((details.elements.size() & 1) == 0);
			const auto count = details.elements.size() / 2;
			for(int element_index = 0 ; element_index < count ; element_index++){
				llvm::Value* key0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 0]);
				llvm::Value* element0_reg = generate_expression(gen_acc, emit_f, details.elements[element_index * 2 + 1]);
				auto dict2_reg = generate_store_dict(gen_acc, emit_f, *dict_acc_reg, *key0_reg, *element0_reg, *value_type_reg);

				dict_acc_reg = dict2_reg;
			}
			return dict_acc_reg;
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(target_type.is_struct()){
		const auto& struct_def = target_type.get_struct();
		auto& struct_type_llvm = *make_struct_type(context, target_type);
		QUARK_ASSERT(struct_def._members.size() == element_count);

		auto struct_ptr_reg = generate_allocate_instance(gen_acc, emit_f, struct_type_llvm);

		int member_index = 0;
		for(const auto& m: struct_def._members){
			const auto& arg = details.elements[member_index];
			llvm::Value* member_value_reg = generate_expression(gen_acc, emit_f, arg);
			generate_struct_member_store(builder, struct_type_llvm, *struct_ptr_reg, member_index, *member_value_reg);
			member_index++;
		}
		return struct_ptr_reg;
	}
	else{
		QUARK_ASSERT(element_count == 1);

		auto element0_reg = generate_expression(gen_acc, emit_f, details.elements[0]);
		const auto input_value_type = details.elements[0].get_output_type();

		if(target_type.is_bool() || target_type.is_int() || target_type.is_double() || target_type.is_typeid()){
			return element0_reg;
		}

		//	NOTICE: string -> json_value needs to be handled at runtime.

		//	Automatically transform a json_value::string => string at runtime?
		else if(target_type.is_string() && input_value_type.is_json_value()){
			return generate_json_to_string(gen_acc, emit_f, *element0_reg);
		}
		else if(target_type.is_json_value()){
			return generate_alloc_json(gen_acc, emit_f, *element0_reg, input_value_type);
		}
		else{
			return element0_reg;
		}
	}
}

static llvm::Value* generate_load2_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::load2_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_symbol(gen_acc, details.address);
	return gen_acc.builder.CreateLoad(dest.value_ptr, "temp");
}

static llvm::Value* generate_expression(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
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
				return s.value_ptr;
			}
			else{
				return generate_load2_expression(gen_acc, emit_f, e, expr);
			}
		}

		llvm::Value* operator()(const expression_t::resolve_member_t& expr) const{
			return generate_resolve_member_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::lookup_t& expr) const{
			return generate_lookup_element_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::value_constructor_t& expr) const{
			return generate_construct_value_expression(gen_acc, emit_f, e, expr);
		}
	};

	llvm::Value* result = std::visit(visitor_t{ gen_acc, e, emit_f }, e._contents);
	return result;
}




////////////////////////////////		STATEMENTS





static void generate_store2_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::store2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	llvm::Value* value = generate_expression(gen_acc, emit_f, s._expression);

	auto dest = find_symbol(gen_acc, s._dest_variable);
	gen_acc.builder.CreateStore(value, dest.value_ptr);

	QUARK_ASSERT(gen_acc.check_invariant());
}

static gen_statement_mode generate_block_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::block_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	return generate_block(gen_acc, emit_f, s._body);
}

static gen_statement_mode generate_ifelse_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	//	Notice that generate_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* condition_value = generate_expression(gen_acc, emit_f, statement._condition);
	auto start_bb = builder.GetInsertBlock();

	auto then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	auto else_bb = llvm::BasicBlock::Create(context, "else", parent_function);

	builder.SetInsertPoint(start_bb);
	builder.CreateCondBr(condition_value, then_bb, else_bb);


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
	if(then_mode == gen_statement_mode::function_returning && else_mode == gen_statement_mode::function_returning){
		return gen_statement_mode::function_returning;
	}

	//	Scenario B: eith then-block or else-block continues. We need a join-block where it can jump.
	else{
		auto join_bb = llvm::BasicBlock::Create(context, "join-then-else", parent_function);

		if(then_mode == gen_statement_mode::more){
			builder.SetInsertPoint(then_bb2);
			builder.CreateBr(join_bb);
		}
		if(else_mode == gen_statement_mode::more){
			builder.SetInsertPoint(else_bb2);
			builder.CreateBr(join_bb);
		}

		builder.SetInsertPoint(join_bb);
		return gen_statement_mode::more;
	}
}

/*
	start_bb
		...
		...


		start_value = start_expression
		???
		end_value = end_expression
		???
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
static gen_statement_mode generate_for_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
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
	llvm::Value* start_value = generate_expression(gen_acc, emit_f, statement._start_expression);
	llvm::Value* end_value = generate_expression(gen_acc, emit_f, statement._end_expression);

	auto values = generate_symbol_slots(gen_acc, emit_f, statement._body._symbol_table);

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	llvm::Value* counter_value = values[0].value_ptr;
	builder.CreateStore(start_value, counter_value);

	llvm::Value* add_value = generate_constant(gen_acc, emit_f, value_t::make_int(1));

	llvm::CmpInst::Predicate pred = statement._range_type == statement_t::for_statement_t::k_closed_range
		? llvm::CmpInst::Predicate::ICMP_SLE
		: llvm::CmpInst::Predicate::ICMP_SLT;

	auto test_value = builder.CreateICmp(pred, start_value, end_value);
	builder.CreateCondBr(test_value, forloop_bb, forend_bb);



	//	EMIT LOOP BB

	builder.SetInsertPoint(forloop_bb);

	gen_acc.scope_path.push_back(values);
	const auto more = generate_statements(gen_acc, emit_f, statement._body._statements);
	gen_acc.scope_path.pop_back();

	if(more == gen_statement_mode::more){
		llvm::Value* counter2 = builder.CreateLoad(counter_value);
		llvm::Value* counter3 = builder.CreateAdd(counter2, add_value, "inc_for_counter");
		builder.CreateStore(counter3, counter_value);

		auto test_value2 = builder.CreateICmp(pred, counter3, end_value);
		builder.CreateCondBr(test_value2, forloop_bb, forend_bb);
	}
	else{
	}


	//	EMIT LOOP END BB

	builder.SetInsertPoint(forend_bb);
	return gen_statement_mode::more;
}

static gen_statement_mode generate_while_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond_bb = llvm::BasicBlock::Create(context, "while-cond", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);


	////////	header

	builder.CreateBr(while_cond_bb);
	builder.SetInsertPoint(while_cond_bb);


	////////	while_cond_bb

	builder.SetInsertPoint(while_cond_bb);
	llvm::Value* condition = generate_expression(gen_acc, emit_f, statement._condition);
	builder.CreateCondBr(condition, while_loop_bb, while_join_bb);


	////////	while_loop_bb

	builder.SetInsertPoint(while_loop_bb);
	const auto mode = generate_block(gen_acc, emit_f, statement._body);
	builder.CreateBr(while_cond_bb);

	if(mode == gen_statement_mode::more){
	}
	else{
	}


	////////	while_join_bb

	builder.SetInsertPoint(while_join_bb);
	return gen_statement_mode::more;
}

static void generate_expression_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::expression_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	generate_expression(gen_acc, emit_f, s._expression);

	QUARK_ASSERT(gen_acc.check_invariant());
}


static llvm::Value* generate_return_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t::return_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	llvm::Value* value = generate_expression(gen_acc, emit_f, s._expression);
	return gen_acc.builder.CreateRet(value);
}

static gen_statement_mode generate_statement(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		llvm_code_generator_t& acc0;
		llvm::Function& emit_f;

		gen_statement_mode operator()(const statement_t::return_statement_t& s) const{
			generate_return_statement(acc0, emit_f, s);
			return gen_statement_mode::function_returning;
		}
		gen_statement_mode operator()(const statement_t::define_struct_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}
		gen_statement_mode operator()(const statement_t::define_function_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}

		gen_statement_mode operator()(const statement_t::bind_local_t& s) const{
			UNSUPPORTED();
		}
		gen_statement_mode operator()(const statement_t::store_t& s) const{
			UNSUPPORTED();
		}
		gen_statement_mode operator()(const statement_t::store2_t& s) const{
			generate_store2_statement(acc0, emit_f, s);
			return gen_statement_mode::more;
		}
		gen_statement_mode operator()(const statement_t::block_statement_t& s) const{
			return generate_block_statement(acc0, emit_f, s);
		}

		gen_statement_mode operator()(const statement_t::ifelse_statement_t& s) const{
			return generate_ifelse_statement(acc0, emit_f, s);
		}
		gen_statement_mode operator()(const statement_t::for_statement_t& s) const{
			return generate_for_statement(acc0, emit_f, s);
		}
		gen_statement_mode operator()(const statement_t::while_statement_t& s) const{
			return generate_while_statement(acc0, emit_f, s);
		}


		gen_statement_mode operator()(const statement_t::expression_statement_t& s) const{
			generate_expression_statement(acc0, emit_f, s);
			return gen_statement_mode::more;
		}
		gen_statement_mode operator()(const statement_t::software_system_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}
		gen_statement_mode operator()(const statement_t::container_def_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
		}
	};

	return std::visit(visitor_t{ gen_acc, emit_f }, statement._contents);
}

static gen_statement_mode generate_statements(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());
			const auto mode = generate_statement(gen_acc, emit_f, statement);
			if(mode == gen_statement_mode::function_returning){
				return gen_statement_mode::function_returning;
			}
		}
	}
	return gen_statement_mode::more;
}






////////////////////////////////		GLOBALS




//	Generates local symbols for arguments and local variables.
std::vector<resolved_symbol_t> generate_function_symbols(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.module->getContext();

	const auto floyd_func = std::get<function_definition_t::floyd_func_t>(function_def._contents);

	const symbol_table_t& symbol_table = floyd_func._body->_symbol_table;

	const auto mapping0 = map_function_arguments(context, function_def._function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	//	Make a resolved_symbol_t for each element in the symbol table. Some are local variables, some are arguments.
	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(context, type);

		//	Figure out if this symbol is an argument in function definition or a local variable.
		//	Check if we can find an argument with this name => it's an argument.
		//	TODO: SAST could contain argument/local information to make this tighter.
		//	Reserve stack slot for each local. But not arguments, they already have stack slot.
		const auto arg_it = std::find_if(mapping.args.begin(), mapping.args.end(), [&](const llvm_arg_mapping_t& arg) -> bool {
			QUARK_TRACE_SS(arg.floyd_name);
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

static llvm::Value* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr){
	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(symbol_name.empty() == false);

	llvm::Value* gv = new llvm::GlobalVariable(
		module,
		&itype,
		false,	//	isConstant
		llvm::GlobalValue::ExternalLinkage,
		init_or_nullptr ? init_or_nullptr : llvm::Constant::getNullValue(&itype),
		symbol_name
	);
	return gv;
}

static llvm::Value* generate_global(llvm_code_generator_t& gen_acc, llvm::Function& emit_f, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_name.empty() == false);
	QUARK_ASSERT(symbol.check_invariant());

	auto& module = *gen_acc.module;
	auto& context = gen_acc.module->getContext();

	const auto type0 = symbol.get_type();
	const auto itype = intern_type(context, type0);

	if(symbol._init.is_undefined()){
		return generate_global0(module, symbol_name, *itype, nullptr);
	}
	else{
		llvm::Constant* init = generate_constant(gen_acc, emit_f, symbol._init);
		//	dest->setInitializer(constant_value);
		return generate_global0(module, symbol_name, *itype, init);
	}
}

//	Make LLVM globals for every global in the AST.
//	Inits the globals when possible.
//	Other globals are uninitialised and global statements will store to them from floyd_runtime_init().
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
static void generate_floyd_function_body(llvm_code_generator_t& gen_acc, int function_id, const floyd::function_definition_t& function_def, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	const auto funcdef_name = compose_function_def_name(function_id, function_def);

	auto f = gen_acc.module->getFunction(funcdef_name);
	QUARK_ASSERT(check_invariant__function(f));
	llvm::Function& emit_f = *f;

	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.instance->context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	auto symbol_table_values = generate_function_symbols(gen_acc, emit_f, function_def);
	gen_acc.scope_path.push_back(symbol_table_values);
	generate_statements(gen_acc, *f, body._statements);
	gen_acc.scope_path.pop_back();

	QUARK_ASSERT(check_invariant__function(f));
}

static void generate_all_floyd_function_bodies(llvm_code_generator_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	We have already generate the LLVM function-prototypes for the global functions in generate_module().
	for(int function_id = 0 ; function_id < semantic_ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = *semantic_ast._tree._function_defs[function_id];

		struct visitor_t {
			llvm_code_generator_t& gen_acc;
			const int function_id;
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

static llvm::Type* deref_ptr(llvm::Type* type){
	if(type->isPointerTy()){
		llvm::PointerType* type2 = llvm::cast<llvm::PointerType>(type);
  		llvm::Type* element_type = type2->getElementType();
  		return element_type;
	}
	else{
		return type;
	}
}

//	The AST contains statements that initializes the global variables, including global functions.

//	Function prototype must NOT EXIST already.
static llvm::Function* generate_function_prototype(llvm::Module& module, const function_definition_t& function_def, int function_id){
	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(function_id >= 0);

	auto& context = module.getContext();

	const auto function_type = function_def._function_type;
	const auto funcdef_name = compose_function_def_name(function_id, function_def);

	auto existing_f = module.getFunction(funcdef_name);
	QUARK_ASSERT(existing_f == nullptr);


	const auto mapping0 = map_function_arguments(context, function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	llvm::Type* function_ptr_type = intern_type(context, function_type);
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
static std::vector<function_def_t> make_all_function_prototypes(llvm::Module& module, const function_defs_t& defs){
	std::vector<function_def_t> result;

	auto& context = module.getContext();

	const auto runtime_functions = get_runtime_functions(context);
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
				make_frp_type(context)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime_init", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}

	{
		for(int function_id = 0 ; function_id < defs.size() ; function_id++){
			const auto& function_def = *defs[function_id];
			auto f = generate_function_prototype(module, function_def, function_id);
			result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), function_id, function_def});
		}
	}
	return result;
}

static std::map<std::string, void*> register_c_functions(llvm::LLVMContext& context){
	const auto runtime_functions = get_runtime_functions(context);

	std::map<std::string, void*> runtime_functions_map;
	for(const auto& e: runtime_functions){
		const auto e2 = std::pair<std::string, void*>(e.name_key, e.implementation_f);
		runtime_functions_map.insert(e2);
	}
	const auto host_functions_map = get_host_functions_map2();

	std::map<std::string, void*> function_map = runtime_functions_map;
	function_map.insert(host_functions_map.begin(), host_functions_map.end());

	return function_map;
}

/*
//	GENERATE CODE for floyd_runtime_init()
	Floyd's global instructions are packed into the "floyd_runtime_init"-function. LLVM doesn't have global instructions.
	All Floyd's global statements becomes instructions in floyd_init()-function that is called by Floyd runtime before any other function is called.
*/
static void generate_floyd_runtime_init(llvm_code_generator_t& gen_acc, const std::vector<statement_t>& global_statements){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto def = find_function_def(gen_acc, "floyd_runtime_init");
	llvm::Function* f = def.llvm_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);
	builder.SetInsertPoint(entryBB);
	llvm::Function& emit_f = *f;

	{
		//	Global statements, using the global symbol scope.
		generate_statements(gen_acc, emit_f, global_statements);

		llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 667);
		builder.CreateRet(dummy_result);
	}
#if 0
	{
		//???	Remove floyd_runtime_ptr-check in release version.
		//	Verify we've got a valid floyd_runtime_ptr as argument #0.
		llvm::Value* magic_index_value = llvm::ConstantInt::get(builder.getInt64Ty(), 0);
		const auto index_list = std::vector<llvm::Value*>{ magic_index_value };

		auto args = f->args();
		QUARK_ASSERT((args.end() - args.begin()) >= 1);
		auto floyd_context_arg_ptr = args.begin();

		llvm::Value* uint64ptr_value = builder.CreateCast(llvm::Instruction::CastOps::BitCast, floyd_context_arg_ptr, llvm::Type::getInt64Ty(context)->getPointerTo(), "");

		llvm::Value* magic_addr = builder.CreateGEP(llvm::Type::getInt64Ty(context), uint64ptr_value, index_list, "magic_addr");
		auto magic_value = builder.CreateLoad(magic_addr);
		llvm::Value* correct_magic_value = llvm::ConstantInt::get(builder.getInt64Ty(), k_debug_magic);
		auto cmp_result = builder.CreateICmpEQ(magic_value, correct_magic_value);

		llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "contBB", f);
		llvm::BasicBlock* failBB = llvm::BasicBlock::Create(context, "failBB", f);
		builder.CreateCondBr(cmp_result, contBB, failBB);

		builder.SetInsertPoint(failBB);
		llvm::Value* dummy_result2 = llvm::ConstantInt::get(builder.getInt64Ty(), 666);
		builder.CreateRet(dummy_result2);


		builder.SetInsertPoint(contBB);

		//	Global statements, using the global symbol scope.
		const auto more = generate_statements(gen_acc, emit_f, global_statements);

		llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 667);
		builder.CreateRet(dummy_result);
	}
#endif
	QUARK_ASSERT(check_invariant__function(f));

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());
}

static std::pair<std::unique_ptr<llvm::Module>, std::vector<function_def_t>> generate_module(llvm_instance_t& instance, const std::string& module_name, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(module_name.empty() == false);
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	auto module = std::make_unique<llvm::Module>(module_name.c_str(), instance.context);

	llvm_code_generator_t gen_acc(instance, module.get(), semantic_ast._tree._interned_types);

	//	Generate all LLVM nodes: functions and globals.
	//	This lets all other code reference them, even if they're not filled up with code yet.
	{
		const auto funcs = make_all_function_prototypes(*module, semantic_ast._tree._function_defs);

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

	generate_floyd_runtime_init(gen_acc, semantic_ast._tree._globals._statements);

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());

	generate_all_floyd_function_bodies(gen_acc, semantic_ast);

	return { std::move(module), gen_acc.function_defs };
}





std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());
	QUARK_ASSERT(module_name.empty() == false);

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

	auto result = std::make_unique<llvm_ir_program_t>(&instance, module, ast0._tree._interned_types, ast._tree._globals._symbol_table, funcs);

	if(k_trace_input_output){
		QUARK_TRACE_SS("result = " << floyd::print_program(*result));
	}
	return result;
}


#if DEBUG && 1
//	Verify that all global functions can be accessed. If *one* is unresolved, then all return NULL!?
void check_nulls(llvm_execution_engine_t& ee2, const llvm_ir_program_t& p){
	int index = 0;
	for(const auto& e: p.debug_globals._symbols){
		if(e.second.get_type().is_function()){
			const auto global_var = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee2, e.first);
			QUARK_ASSERT(global_var != nullptr);

			const auto f = *global_var;
//				QUARK_ASSERT(f != nullptr);

			const std::string suffix = f == nullptr ? " NULL POINTER" : "";
			const uint64_t addr = reinterpret_cast<uint64_t>(f);
//			QUARK_TRACE_SS(index << " " << e.first << " " << addr << suffix);
		}
		else{
		}
		index++;
	}
}
#endif


//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_instance_t& instance, llvm_ir_program_t& program_breaks, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	auto ee = make_engine_run_init(instance, program_breaks);
	return 0;
}



////////////////////////////////		HELPERS



std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const std::string& program, const std::string& file){
	QUARK_ASSERT(instance.check_invariant());

	const auto cu = floyd::make_compilation_unit_nolib(program, file);
	const auto pass3 = compile_to_sematic_ast__errors(cu);
	auto bc = generate_llvm_ir_program(instance, pass3, file);
	return bc;
}


//	Destroys program, can only run it once!
llvm_execution_engine_t make_engine_no_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	std::string collectedErrors;

	//	WARNING: Destroys p -- uses std::move().
	llvm::ExecutionEngine* exeEng = llvm::EngineBuilder(std::move(program_breaks.module))
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
	QUARK_ASSERT(collectedErrors.empty());

	auto ee1 = std::shared_ptr<llvm::ExecutionEngine>(exeEng);
	auto ee2 = llvm_execution_engine_t{ k_debug_magic, &instance, ee1, program_breaks.type_interner, program_breaks.debug_globals, program_breaks.function_defs, {} };
	QUARK_ASSERT(ee2.check_invariant());

	auto function_map = register_c_functions(instance.context);

	//	Resolve all unresolved functions.
	{
		//	https://stackoverflow.com/questions/33328562/add-mapping-to-c-lambda-from-llvm
		auto lambda = [&](const std::string& s) -> void* {
			QUARK_ASSERT(s.empty() == false);
			QUARK_ASSERT(s[0] == '_');
			const auto s2 = s.substr(1);

			const auto it = function_map.find(s2);
			if(it != function_map.end()){
				return it->second;
			}
			else{
			}

			return nullptr;
		};
		std::function<void*(const std::string&)> on_lazy_function_creator2 = lambda;

		//	NOTICE! Patch during finalizeObject() only, then restore!
		ee2.ee->InstallLazyFunctionCreator(on_lazy_function_creator2);
		ee2.ee->finalizeObject();
		ee2.ee->InstallLazyFunctionCreator(nullptr);

	//	ee2.ee->DisableGVCompilation(false);
	//	ee2.ee->DisableSymbolSearching(false);
	}

	check_nulls(ee2, program_breaks);

//	llvm::WriteBitcodeToFile(exeEng->getVerifyModules(), raw_ostream &Out);
	return ee2;
}

//	Destroys program, can only run it once!
//	Automatically runs floyd_runtime_init() to execute Floyd's global functions and initialize global constants.
llvm_execution_engine_t make_engine_run_init(llvm_instance_t& instance, llvm_ir_program_t& program_breaks){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	llvm_execution_engine_t ee = make_engine_no_init(instance, program_breaks);

#if DEBUG
	{
		const auto print_global_ptr = (FLOYD_RUNTIME_HOST_FUNCTION*)floyd::get_global_ptr(ee, "print");
		QUARK_ASSERT(print_global_ptr != nullptr);

		const auto print_f = *print_global_ptr;
		QUARK_ASSERT(print_f != nullptr);
		if(print_f){

//			(*print_f)(&ee, 109);
		}
	}

	{
		auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_init"));
		QUARK_ASSERT(a_func != nullptr);
	}
#endif

	const auto init_result = call_floyd_runtime_init(ee);
	QUARK_ASSERT(init_result == 667);

	check_nulls(ee, program_breaks);

	return ee;
}


}	//	namespace floyd


////////////////////////////////		TESTS



QUARK_UNIT_TEST("", "From source: Check that floyd_runtime_init() runs and sets 'result' global", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("let int result = 1 + 2 + 3", "myfile.floyd");
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, pass3, "myfile.floyd");
	auto ee = make_engine_run_init(instance, *program);

	const auto result = *static_cast<uint64_t*>(floyd::get_global_ptr(ee, "result"));
	QUARK_ASSERT(result == 6);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}

//	BROKEN!
QUARK_UNIT_TEST("", "From JSON: Simple function call, call print() from floyd_runtime_init()", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("print(5)", "myfile.floyd");
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, pass3, "myfile.floyd");
	auto ee = make_engine_run_init(instance, *program);
	QUARK_ASSERT(ee._print_output == std::vector<std::string>{"5"});
}
