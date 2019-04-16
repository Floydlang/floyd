//
//  floyd_llvm_codegen.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen.h"

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

//http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html


typedef std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs_t;


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




struct llvmgen_t {
	public: llvmgen_t(llvm_instance_t& instance, llvm::Module* module) :
		instance(&instance),
		module(module),
		builder(instance.context)
	{
		QUARK_ASSERT(instance.check_invariant());

	//	llvm::IRBuilder<> builder(instance.context);
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();

		QUARK_ASSERT(check_invariant());
	}
	public: bool check_invariant() const {
//		QUARK_ASSERT(scope_path.size() >= 1);

		QUARK_ASSERT(instance);
		QUARK_ASSERT(instance->check_invariant());
		QUARK_ASSERT(module);

/*
		//	While emitting a function, the module is in an invalid state.
		if(emit_f != nullptr){
		}
		else{
			QUARK_ASSERT(check_invariant__module(module));
		}
*/
		return true;
	}


	llvm_instance_t* instance;
	llvm::Module* module;
	llvm::IRBuilder<> builder;
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

gen_statement_mode genllvm_statements(llvmgen_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements);
llvm::Value* genllvm_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e);








std::string print_resolved_symbols(const std::vector<resolved_symbol_t>& globals){
	std::stringstream out;

	out << "{" << std::endl;
	for(const auto& e: globals){
		out << "{ " << print_value(e.value_ptr) << " : " << e.debug_str << " }" << std::endl;
	}

	return out.str();
}


//	Print all symbols in scope_path
std::string print_gen(const llvmgen_t& gen){
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



resolved_symbol_t make_resolved_symbol(llvm::Value* value_ptr, std::string debug_str, resolved_symbol_t::esymtype t, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(value_ptr != nullptr);

	return { value_ptr, debug_str, t, symbol_name, symbol};
}




resolved_symbol_t find_symbol(llvmgen_t& gen_acc, const variable_address_t& reg){
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

std::string print_program(const llvm_ir_program_t& program){
	QUARK_ASSERT(program.check_invariant());

//	std::string dump;
//	llvm::raw_string_ostream stream2(dump);
//	program.module->print(stream2, nullptr);

	return print_module(*program.module);
}

//??? need mechanism to map Floyd types vs machine-types.
value_t llvm_global_to_value(const void* global_ptr, const typeid_t& type){
	QUARK_ASSERT(global_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	//??? more types.
	if(type.is_undefined()){
	}
	else if(type.is_bool()){
		//??? How is int1 encoded by LLVM?
		const auto temp = *static_cast<const uint8_t*>(global_ptr);
		return value_t::make_bool(temp == 0 ? false : true);
	}
	else if(type.is_int()){
		const auto temp = *static_cast<const uint64_t*>(global_ptr);
		return value_t::make_int(temp);
	}
	else if(type.is_double()){
		const auto temp = *static_cast<const double*>(global_ptr);
		return value_t::make_double(temp);
	}
	else if(type.is_string()){
		const char* s = *(const char**)(global_ptr);
		return value_t::make_string(s);
	}
	else if(type.is_json_value()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_typeid()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_struct()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_vector()){
		QUARK_ASSERT(type.get_vector_element_type().is_string());

		auto vec = *static_cast<const VEC_T*>(global_ptr);
		std::vector<value_t> vec2;
		for(int i = 0 ; i < vec.element_count ; i++){
			auto s = (const char*)vec.element_ptr[i];
			const auto a = std::string(s);
			vec2.push_back(value_t::make_string(a));
		}
		return value_t::make_vector_value(typeid_t::make_string(), vec2);
	}
/*
!!!
	else if(type.is_vector()){
		auto vec = *static_cast<const VEC_T*>(global_ptr);
		std::vector<value_t> vec2;
		if(type.get_vector_element_type().is_string()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = (const char*)vec.element_ptr[i];
				const auto a = std::string(s);
				vec2.push_back(value_t::make_string(a));
			}
			return value_t::make_vector_value(typeid_t::make_string(), vec2);
		}
		else if(type.get_vector_element_type().is_bool()){
			for(int i = 0 ; i < vec.element_count ; i++){
				auto s = vec.element_ptr[i / 64];
				bool masked = s & (1 << (i & 63));
				vec2.push_back(value_t::make_bool(masked == 0 ? false : true));
			}
			return value_t::make_vector_value(typeid_t::make_bool(), vec2);
		}
		else{
		NOT_IMPLEMENTED_YET();
		}
	}
*/
	else if(type.is_dict()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_function()){
		NOT_IMPLEMENTED_YET();
	}
	else{
	}
	NOT_IMPLEMENTED_YET();
	QUARK_ASSERT(false);
	throw std::exception();
}

value_t llvm_to_value(const uint64_t encoded_value, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? more types.
	if(type.is_undefined()){
	}
	else if(type.is_bool()){
		//??? How is int1 encoded by LLVM?
		const auto temp = static_cast<const uint8_t>(encoded_value);
		return value_t::make_bool(temp == 0 ? false : true);
	}
	else if(type.is_int()){
		const auto temp = static_cast<const uint64_t>(encoded_value);
		return value_t::make_int(temp);
	}
	else if(type.is_double()){
		const auto temp = *reinterpret_cast<const double*>(&encoded_value);
		return value_t::make_double(temp);
	}
	else if(type.is_string()){
		const char* s = (const char*)(encoded_value);
		return value_t::make_string(s);
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

std::pair<void*, typeid_t> bind_function(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto f = reinterpret_cast<FLOYD_RUNTIME_F*>(get_global_function(ee, name));
	if(f != nullptr){
		const function_def_t def = find_function_def2(ee.function_defs, std::string() + "floyd_funcdef__" + name);
		const auto function_type = def.floyd_fundef._function_type;
		return { f, function_type };
	}
	else{
		return { nullptr, typeid_t::make_undefined() };
	}
}

value_t call_function(const std::pair<void*, typeid_t>& f){
	QUARK_ASSERT(f.first != nullptr);
	QUARK_ASSERT(f.second.is_function());

	const auto function_ptr = reinterpret_cast<FLOYD_RUNTIME_F*>(f.first);
	int64_t return_encoded = (*function_ptr)(function_ptr, "?dummy arg to main()?");

	const auto return_type = f.second.get_function_return();
	return llvm_to_value(return_encoded, return_type);
}

std::pair<void*, typeid_t> bind_global(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto global_ptr = get_global_ptr(ee, name);
	if(global_ptr != nullptr){
		auto symbol = find_symbol(ee.global_symbols, name);
		QUARK_ASSERT(symbol != nullptr);

		return { global_ptr, symbol->get_type() };
	}
	else{
		return { nullptr, typeid_t::make_undefined() };
	}
}

value_t load_global(const std::pair<void*, typeid_t>& v){
	QUARK_ASSERT(v.first != nullptr);
	QUARK_ASSERT(v.second.is_undefined() == false);

	return llvm_global_to_value(v.first, v.second);
}




std::string get_function_def_name(int function_id, const function_definition_t& def){
	const auto def_name = def._definition_name;
	const auto funcdef_name = def_name.empty() ? std::string() + "floyd_unnamed_function_" + std::to_string(function_id) : std::string("floyd_funcdef__") + def_name;
	return funcdef_name;
}

llvm::Constant* make_constant(llvmgen_t& gen_acc, const value_t& value){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto& builder = gen_acc.builder;

	auto& module = *gen_acc.module;

	const auto type = value.get_type();
	const auto itype = intern_type(module, type);

	if(type.is_undefined()){
		return llvm::ConstantInt::get(itype, 17);
	}
	else if(type.is_internal_dynamic()){
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
		//	Stores trailing zero. ??? not pure string!
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
		return llvm::ConstantInt::get(itype, 7000);
	}
	else if(type.is_typeid()){
		return llvm::ConstantInt::get(itype, 6000);
	}
	else if(type.is_struct()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_vector()){
		NOT_IMPLEMENTED_YET();
	}
	else if(type.is_dict()){
		NOT_IMPLEMENTED_YET();
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

const function_def_t& find_function_def2(const std::vector<function_def_t>& function_defs, const std::string& function_name){
	auto it = std::find_if(function_defs.begin(), function_defs.end(), [&] (const function_def_t& e) { return e.def_name == function_name; } );
	QUARK_ASSERT(it != function_defs.end());

	QUARK_ASSERT(it->llvm_f != nullptr);
	return *it;
}

const function_def_t& find_function_def(llvmgen_t& gen_acc, const std::string& function_name){
	QUARK_ASSERT(gen_acc.check_invariant());

	return find_function_def2(gen_acc.function_defs, function_name);
}

std::vector<resolved_symbol_t> genllvm_make_function_def_symbols(llvmgen_t& gen_acc, llvm::Function& emit_f, const function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	const auto floyd_func = std::get<function_definition_t::floyd_func_t>(function_def._contents);

	const symbol_table_t& symbol_table = floyd_func._body->_symbol_table;

	const auto mapping0 = map_function_arguments(*gen_acc.module, function_def._function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	//	Make a resolved_symbol_t for each element in the symbol table. Some are local variables, some are arguments.
	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(*gen_acc.module, type);

		//	Figure out if this symbol is an argument in function definition or a local variable.
		//	Check if we can find an argument with this name => it's an argument.
		//	TODO: SAST could contain argument/local information to make this tighter.

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

		//	Reserve stack slot for each local. But not arguments, they already have stack slot.
		else{
			llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

			//	Init the slot if needed.
			if(e.second._init.is_undefined() == false){
				llvm::Value* c = make_constant(gen_acc, e.second._init);
				gen_acc.builder.CreateStore(c, dest);
			}
			const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
			result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
		}
	}
	QUARK_ASSERT(result.size() == symbol_table._symbols.size());
	return result;
}
std::vector<resolved_symbol_t> genllvm__alloc_symbols(llvmgen_t& gen_acc, llvm::Function& emit_f, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(symbol_table.check_invariant());

	std::vector<resolved_symbol_t> result;
	for(const auto& e: symbol_table._symbols){
		const auto type = e.second.get_type();
		const auto itype = intern_type(*gen_acc.module, type);

		//	Reserve stack slot for each local.
		llvm::Value* dest = gen_acc.builder.CreateAlloca(itype, nullptr, e.first);

		//	Init the slot if needed.
		if(e.second._init.is_undefined() == false){
			llvm::Value* c = make_constant(gen_acc, e.second._init);
			gen_acc.builder.CreateStore(c, dest);
		}
		const auto debug_str = "<LOCAL> name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, e.first, e.second));
	}
	return result;
}



llvm::Value* genllvm_literal_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto literal = e.get_literal();
	return make_constant(gen_acc, literal);
}

llvm::Value* llvmgen_lookup_element_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	auto parent_value = genllvm_expression(gen_acc, emit_f, *details.parent_address);
	auto key_value = genllvm_expression(gen_acc, emit_f, *details.lookup_key);

	const auto parent_type =  details.parent_address->get_output_type();
	if(parent_type.is_string()){
		auto element_index = key_value;
		const auto index_list = std::vector<llvm::Value*>{ element_index };
		llvm::Value* element_addr = builder.CreateGEP(llvm::Type::getIntNTy(context, 8), parent_value, index_list, "element_addr");

		llvm::Value* element_value_8bit = builder.CreateLoad(element_addr, "element_tmp");
		llvm::Type* output_type = llvm::Type::getIntNTy(context, 64);

		llvm::Value* element_value = gen_acc.builder.CreateCast(llvm::Instruction::CastOps::SExt, element_value_8bit, output_type, "char_to_int64");
		return element_value;
	}
	else if(parent_type.is_json_value()){
//		return bc_opcode::k_lookup_element_json_value;
	}
	else if(parent_type.is_vector()){
/*
		if(encode_as_vector_w_inplace_elements(parent_type)){
			return bc_opcode::k_lookup_element_vector_w_inplace_elements;
		}
		else{
			return bc_opcode::k_lookup_element_vector_w_external_elements;
		}
*/

	}
	else if(parent_type.is_dict()){
/*
		if(encode_as_dict_w_inplace_values(parent_type)){
			return bc_opcode::k_lookup_element_dict_w_inplace_values;
		}
		else{
			return bc_opcode::k_lookup_element_dict_w_external_values;
		}
*/

	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return nullptr;
}


llvm::Value* genllvm_arithmetic_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.lhs->get_output_type();

	auto lhs_temp = genllvm_expression(gen_acc, emit_f, *details.lhs);
	auto rhs_temp = genllvm_expression(gen_acc, emit_f, *details.rhs);

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
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add__2);

		const auto def = find_function_def(gen_acc, "floyd_runtime__append_strings");
		std::vector<llvm::Value*> args2;

		//	Insert floyd_runtime_ptr as first argument to called function.
		args2.push_back(get_callers_fcp(emit_f));
		args2.push_back(lhs_temp);
		args2.push_back(rhs_temp);
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "append_strings");
		return result;
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
		return conv_opcode.at(details.op);
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
			return conv_opcode.at(details.op);
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
			return conv_opcode.at(details.op);
		}
	}
*/

	UNSUPPORTED();

	return nullptr;
}


llvm::Value* llvmgen_comparison_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto lhs_temp = genllvm_expression(gen_acc, emit_f, *details.lhs);
	auto rhs_temp = genllvm_expression(gen_acc, emit_f, *details.rhs);

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
		std::vector<llvm::Value*> args2;
		llvm::Value* op_value = make_constant(gen_acc, value_t::make_int(static_cast<int64_t>(details.op)));
		args2.push_back(get_callers_fcp(emit_f));
		args2.push_back(op_value);
		args2.push_back(lhs_temp);
		args2.push_back(rhs_temp);
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "compare_strings");
		auto result2 = gen_acc.builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, result, llvm::ConstantInt::get(gen_acc.builder.getInt32Ty(), 0));

		QUARK_ASSERT(gen_acc.check_invariant());
		return result2;
	}
	else if(type.is_vector()){
		const auto def = find_function_def(gen_acc, "floyd_runtime__compare_vectors");
		llvm::Value* op_value = make_constant(gen_acc, value_t::make_int(static_cast<int64_t>(details.op)));
		auto lhs_vec_ptr = get_vec_ptr(gen_acc.builder, lhs_temp);
		auto rhs_vec_ptr = get_vec_ptr(gen_acc.builder, rhs_temp);

		std::vector<llvm::Value*> args2;
		args2.push_back(get_callers_fcp(emit_f));
		args2.push_back(op_value);
		args2.push_back(lhs_vec_ptr);
		args2.push_back(rhs_vec_ptr);
		auto result = gen_acc.builder.CreateCall(def.llvm_f, args2, "compare_vectors");
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


llvm::Value* genllvm_arithmetic_unary_minus_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	const auto type = details.expr->get_output_type();
	if(type.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return genllvm_expression(gen_acc, emit_f, e2);
	}
	else if(type.is_double()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract__2, expression_t::make_literal_double(0), *details.expr, e._output_type);
		return genllvm_expression(gen_acc, emit_f, e2);
	}
	else{
		UNSUPPORTED();
	}
}

llvm::Value* llvmgen_conditional_operator_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::conditional_t& conditional){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto result_type = e.get_output_type();
	const auto result_itype = intern_type(*gen_acc.module, result_type);

	llvm::Value* condition_value = genllvm_expression(gen_acc, emit_f, *conditional.condition);

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* join_bb = llvm::BasicBlock::Create(context, "cond_operator-join");

	builder.CreateCondBr(condition_value, then_bb, else_bb);


	// Emit then-value.
	builder.SetInsertPoint(then_bb);
	llvm::Value* then_value = genllvm_expression(gen_acc, emit_f, *conditional.a);
	builder.CreateBr(join_bb);
	// Codegen of 'Then' can change the current block, update then_bb.
	llvm::BasicBlock* then_bb2 = builder.GetInsertBlock();


	// Emit else block.
	parent_function->getBasicBlockList().push_back(else_bb);
	builder.SetInsertPoint(else_bb);
	llvm::Value* else_value = genllvm_expression(gen_acc, emit_f, *conditional.b);
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




//	Host functions, automatically called by the LLVM execution engine.
////////////////////////////////////////////////////////////////////////////////


struct host_func_t {
	std::string name_key;
	llvm::FunctionType* function_type;
	void* implementation_f;
};



/*
@variable = global i32 21
define i32 @main() {
%1 = load i32, i32* @variable ; load the global variable
%2 = mul i32 %1, 2
store i32 %2, i32* @variable ; store instruction to write to global variable
ret i32 %2
}
*/

llvm_execution_engine_t* get_floyd_runtime(void* floyd_runtime_ptr){
	QUARK_ASSERT(floyd_runtime_ptr != nullptr);

	auto ptr = reinterpret_cast<llvm_execution_engine_t*>(floyd_runtime_ptr);
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->debug_magic == k_debug_magic);
	return ptr;
}

void hook(const std::string& s, void* floyd_runtime_ptr, int64_t arg){
	std:: cout << s << arg << " arg: " << std::endl;
	auto r = get_floyd_runtime(floyd_runtime_ptr);
	throw std::runtime_error("HOST FUNCTION NOT IMPLEMENTED FOR LLVM");
}

//??? we assume all vector are [string] right now!!
VEC_T* as_vector_w_string(llvm_execution_engine_t* runtime, int64_t arg_value, int64_t arg_type){
	QUARK_ASSERT(arg_type == (int)base_type::k_vector);
	const auto vector_strings = (VEC_T*)arg_value;
	QUARK_ASSERT(vector_strings != nullptr);
	QUARK_ASSERT(vector_strings->magic == 0xDABBAD00);

	return vector_strings;
}



std::string gen_to_string(llvm_execution_engine_t* runtime, int64_t arg_value, int64_t arg_type){
	const auto type = (base_type)arg_type;
	if(type == base_type::k_int){
		const auto value = (int64_t)arg_value;
		return std::to_string(value);
	}
	else if(type == base_type::k_string){
		const auto value = (const char*)arg_value;
		return std::string(value);
	}
	else if(type == base_type::k_double){
		const auto d = sizeof(double);
		const auto i = sizeof(int64_t);
		QUARK_ASSERT(d == i);

		const auto value = *(double*)&arg_value;

		return double_to_string_always_decimals(value);
//		return std::to_string(value);
	}
	else if(type == base_type::k_vector){
		//??? we assume all vector are [string] right now!!
		const auto vs = as_vector_w_string(runtime, arg_value, arg_type);

		std::vector<value_t> elements;
		const int count = vs->element_count;
		for(int i = 0 ; i < count ; i++){
			const char* ss = (const char*)vs->element_ptr[i];
			QUARK_ASSERT(ss != nullptr);

			const auto e = std::string(ss);
			elements.push_back(value_t::make_string(e));
		}
		const auto val = value_t::make_vector_value(typeid_t::make_string(), elements);
		return to_compact_string2(val);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}






//??? Make nicer mechanism to register all host functions, types and key-strings. Store explicit members like assert_f instead of search on string.



//	The names of these are computed from the host-id in the symbol table, not the names of the functions/symbols.
//	They must use C calling convention so llvm JIT can find them.
//	Make sure they are not dead-stripped out of binary!

void floyd_runtime__unresolved_func(void* floyd_runtime_ptr){
	std:: cout << __FUNCTION__ << std::endl;
}


int32_t floyd_runtime__compare_strings(void* floyd_runtime_ptr, int64_t op, const char* lhs, const char* rhs){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	/*
		strcmp()
			Greater than zero ( >0 ): A value greater than zero is returned when the first not matching character in leftStr have the greater ASCII value than the corresponding character in rightStr or we can also say

			Less than Zero ( <0 ): A value less than zero is returned when the first not matching character in leftStr have lesser ASCII value than the corresponding character in rightStr.

	*/
	const auto result = std::strcmp(lhs, rhs);
	const auto op2 = static_cast<expression_type>(op);
	if(op2 == expression_type::k_comparison_smaller_or_equal__2){
		return result <= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_smaller__2){
		return result < 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger_or_equal__2){
		return result >= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger__2){
		return result > 0 ? 1 : 0;
	}

	else if(op2 == expression_type::k_logical_equal__2){
		return result == 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_logical_nonequal__2){
		return result != 0 ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

const char* floyd_runtime__append_strings(void* floyd_runtime_ptr, const char* lhs, const char* rhs){
	auto r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(lhs != nullptr);
	QUARK_ASSERT(rhs != nullptr);
	QUARK_TRACE_SS(__FUNCTION__ << " " << std::string(lhs) << " comp " <<std::string(rhs));

	std::size_t len = strlen(lhs) + strlen(rhs);
	char* s = reinterpret_cast<char*>(std::malloc(len + 1));
	strcpy(s, lhs);
	strcpy(s + strlen(lhs), rhs);
	return s;
}

const uint8_t* floyd_runtime__allocate_memory(void* floyd_runtime_ptr, int64_t bytes){
	auto s = reinterpret_cast<uint8_t*>(std::calloc(1, bytes));
	return s;
}


////////////////////////////////		allocate_vector()


//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
const VEC_T floyd_runtime__allocate_vector(void* floyd_runtime_ptr, uint32_t element_count){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	auto element_ptr = reinterpret_cast<uint64_t*>(std::calloc(element_count, sizeof(uint64_t)));
	if(element_ptr == nullptr){
		throw std::exception();
	}

	VEC_T result;
	result.magic = 0xDABBAD00;
	result.element_ptr = element_ptr;
	result.element_count = element_count;
	return result;
}
/*
!!!
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
host_func_t floyd_runtime__allocate_vector__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		make_vec_type(context),
		{
			make_frp_type(context),
			llvm::Type::getInt32Ty(context)
		},
		false
	);
	return { "floyd_runtime__allocate_vector", function_type, reinterpret_cast<void*>(floyd_runtime__allocate_vector) };
}


////////////////////////////////		delete_vector()


const void floyd_runtime__delete_vector(void* floyd_runtime_ptr, VEC_T* vec){
	auto r = get_floyd_runtime(floyd_runtime_ptr);
	QUARK_ASSERT(vec != nullptr);
	QUARK_ASSERT(vec->magic == 0xDABBAD00);

	std::free(vec->element_ptr);
	vec->element_ptr = nullptr;
	vec->magic = 0xDEADD0D0;
	vec->element_count = -vec->element_count;
}

host_func_t floyd_runtime__delete_vector__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getVoidTy(context),
		{
			make_frp_type(context),
			make_vec_type(context)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__delete_vector", function_type, reinterpret_cast<void*>(floyd_runtime__delete_vector) };
}


////////////////////////////////		compare_vectors()


int32_t floyd_runtime__compare_vectors(void* floyd_runtime_ptr, int64_t op, const VEC_T* lhs, const VEC_T* rhs){
	auto r = get_floyd_runtime(floyd_runtime_ptr);
//	const auto result = std::strcmp(lhs, rhs);
	const auto result = 0;

	const auto op2 = static_cast<expression_type>(op);
	if(op2 == expression_type::k_comparison_smaller_or_equal__2){
		return result <= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_smaller__2){
		return result < 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger_or_equal__2){
		return result >= 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_comparison_larger__2){
		return result > 0 ? 1 : 0;
	}

	else if(op2 == expression_type::k_logical_equal__2){
		return result == 0 ? 1 : 0;
	}
	else if(op2 == expression_type::k_logical_nonequal__2){
		return result != 0 ? 1 : 0;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
host_func_t floyd_runtime__compare_vectors__make(llvm::LLVMContext& context){
	llvm::FunctionType* function_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		{
			make_frp_type(context),
			llvm::Type::getInt64Ty(context),
			make_vec_type(context)->getPointerTo(),
			make_vec_type(context)->getPointerTo()
		},
		false
	);
	return { "floyd_runtime__compare_vectors", function_type, reinterpret_cast<void*>(floyd_runtime__compare_vectors) };
}




void floyd_funcdef__assert(void* floyd_runtime_ptr, int64_t arg){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	bool ok = arg == 0 ? false : true;
	if(!ok){
		r->_print_output.push_back("Assertion failed.");
		quark::throw_runtime_error("Floyd assertion failed.");
	}
}

void floyd_host_function_1001(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1002(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1003(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1004(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1005(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1006(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1007(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1008(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}



int64_t floyd_funcdef__find__string(llvm_execution_engine_t* floyd_runtime_ptr, const char s[], const char find[]){
	QUARK_ASSERT(s != nullptr);
	QUARK_ASSERT(find != nullptr);

	const auto str = std::string(s);
	const auto wanted2 = std::string(find);

	const auto r = str.find(wanted2);
	const auto result = r == std::string::npos ? -1 : static_cast<int64_t>(r);
	return result;
}

int64_t floyd_funcdef__find(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type, int64_t arg1_value, int64_t arg1_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = arg0_type;
	if(type == (int)base_type::k_string){
		if(arg1_type != (int)base_type::k_string){
			quark::throw_runtime_error("find(string) requires argument 2 to be a string.");
		}
		return floyd_funcdef__find__string(r, (const char*)arg0_value, (const char*)arg1_value);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}



void floyd_host_function_1010(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1011(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1012(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1013(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1014(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1015(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1016(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1017(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1018(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1019(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}




//	??? Make visitor to handle different types.
//	??? Extend dynamic system to support any type, not just base_type.
void floyd_funcdef__print(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);
	const auto s = gen_to_string(r, arg0_value, arg0_type);
	r->_print_output.push_back(s);
}



const DYN_RETURN_T floyd_funcdef__push_back(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type, int64_t arg1_value, int64_t arg1_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = (base_type)arg0_type;
	if(type == base_type::k_string){
		const auto value = (const char*)arg0_value;

		std::size_t len = strlen(value);
		char* s = reinterpret_cast<char*>(std::malloc(len + 1 + 1));
		strcpy(s, value);
		s[len + 0] = (char)arg1_value;
		s[len + 1] = 0x00;

		return make_dyn_return(reinterpret_cast<uint64_t>(s), (uint32_t)base_type::k_string);
	}
	else if(type == base_type::k_vector){
		//??? we assume all vector are [string] right now!!
		const auto vs = as_vector_w_string(r, arg0_value, arg0_type);

		QUARK_ASSERT(arg1_type == (int)base_type::k_string);
		const auto element = (const char*)arg1_value;

		VEC_T v2 = floyd_runtime__allocate_vector(floyd_runtime_ptr, vs->element_count + 1);
		for(int i = 0 ; i < vs->element_count ; i++){
			v2.element_ptr[i] = vs->element_ptr[i];
		}
		v2.element_ptr[vs->element_count] = reinterpret_cast<uint64_t>(element);
		return make_dyn_return(v2);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1022(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1023(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1024(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}


const char* floyd_funcdef__replace__string(llvm_execution_engine_t* floyd_runtime_ptr, const char s[], std::size_t start, std::size_t end, const char replace[]){
	auto s_len = std::strlen(s);
	auto replace_len = std::strlen(replace);
	if(start > end){
		quark::throw_runtime_error("replace() requires start <= end.");
	}

	auto end2 = std::min(end, s_len);
	auto start2 = std::min(start, end2);

	const std::size_t len2 = start2 + replace_len + (s_len - end2);
	auto s2 = reinterpret_cast<char*>(std::malloc(len2 + 1));
	std::memcpy(&s2[0], &s[0], start2);
	std::memcpy(&s2[start2], &replace[0], replace_len);
	std::memcpy(&s2[start2 + replace_len], &s[end2], s_len - end2);
	s2[start2 + replace_len + (s_len - end2)] = 0x00;
	return s2;
}
//??? Pass DYN as arguments too, skip int64_t arg0_value and int64_t arg0_type
const DYN_RETURN_T floyd_funcdef__replace(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type, int64_t start, int64_t end, int64_t arg3_value, int64_t arg3_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("replace() requires start and end to be non-negative.");
	}

	const auto type = arg0_type;
	if(type == (int)base_type::k_string){
		if(arg3_type != (int)base_type::k_string){
			quark::throw_runtime_error("replace(string) requires argument 4 to be a string.");
		}
		const auto ret = floyd_funcdef__replace__string(r, (const char*)arg0_value, (std::size_t)start, (std::size_t)end, (const char*)arg3_value);
		return make_dyn_return(ret);
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1026(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1027(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

int64_t floyd_funcdef__size(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = (base_type)arg0_type;
	if(type == base_type::k_string){
		const auto value = (const char*)arg0_value;
		//??? Strings are not clean.
		return std::strlen(value);
	}
	else if(type == base_type::k_vector){
		//??? we assume all vector are [string] right now!!
		const auto vs = as_vector_w_string(r, arg0_value, arg0_type);

		const int count = vs->element_count;
		return count;
	}

	else{
		NOT_IMPLEMENTED_YET();
	}
}

const char* floyd_funcdef__subset(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type, int64_t start, int64_t end){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	if(start < 0 || end < 0){
		quark::throw_runtime_error("subset() requires start and end to be non-negative.");
	}

	const auto type = (base_type)arg0_type;
	if(type == base_type::k_string){
		const auto value = (const char*)arg0_value;

		std::size_t len = strlen(value);
		const std::size_t end2 = std::min(static_cast<std::size_t>(end), len);
		const std::size_t start2 = std::min(static_cast<std::size_t>(start), end2);
		std::size_t len2 = end2 - start2;

		char* s = reinterpret_cast<char*>(std::malloc(len2 + 1));
		std::memcpy(&s[0], &value[start2], len2);
		s[len2] = 0x00;
		return s;
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}




void floyd_host_function_1030(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1031(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

const char* floyd_host__to_string(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	const auto s = gen_to_string(r, arg0_value, arg0_type);

	//??? leaks.
	return strdup(s.c_str());
}

void floyd_host__typeof(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
/*
bc_value_t host__typeof(interpreter_t& vm, const bc_value_t args[], int arg_count){
QUARK_ASSERT(vm.check_invariant());
QUARK_ASSERT(arg_count == 1);

const auto& value = args[0];
const auto type = value._type;
const auto result = value_t::make_typeid_value(type);
return value_to_bc(result);
}
*/
}

const char* floyd_funcdef__update(void* floyd_runtime_ptr, int64_t arg0_value, int64_t arg0_type, int64_t arg1_value, int64_t arg1_type, int64_t arg2_value, int64_t arg2_type){
	auto r = get_floyd_runtime(floyd_runtime_ptr);

	const auto type = (base_type)arg0_type;
	if(type == base_type::k_string){
		const auto str = (const char*)arg0_value;

		QUARK_ASSERT(arg1_type == (int)base_type::k_int);
		QUARK_ASSERT(arg2_type == (int)base_type::k_int);

		const auto index = arg1_value;
		const auto new_char = (char)arg2_value;


		const auto len = strlen(str);

		if(index < 0 || index >= len){
			throw std::runtime_error("Position argument to update() is outside collection span.");
		}

		auto result = strdup(str);
		result[index] = new_char;
		return result;

//			return DYN_RETURN_T{ reinterpret_cast<uint64_t>(value), (uint64_t)base_type::k_string };
	}
	else{
		NOT_IMPLEMENTED_YET();
	}
}

void floyd_host_function_1035(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1036(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1037(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1038(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}

void floyd_host_function_1039(void* floyd_runtime_ptr, int64_t arg){
	hook(__FUNCTION__, floyd_runtime_ptr, arg);
}




///////////////		TEST

void floyd_host_function_2002(void* floyd_runtime_ptr, int64_t arg){
	std::cout << __FUNCTION__ << arg << std::endl;
}

void floyd_host_function_2003(void* floyd_runtime_ptr, int64_t arg){
	std::cout << __FUNCTION__ << arg << std::endl;
}

/*
	NOTICES: The function signature for the callee can hold DYNs. The actual arguments will be explicit types, never DYNs.

	Function signature
	- In call's callee signature
	- In call's actual arguments types and output type.
	- In function def
*/
llvm::Value* llvmgen_call_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::call_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto callee_function_type = details.callee->get_output_type();
	const auto resolved_call_type = e.get_output_type();

	const auto actual_call_arguments = mapf<typeid_t>(details.args, [](auto& e){ return e.get_output_type(); });

	const auto llvm_mapping = map_function_arguments(*gen_acc.module, callee_function_type);

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(details.args.size() == callee_function_type.get_function_args().size());

	llvm::Value* callee0_value = genllvm_expression(gen_acc, emit_f, *details.callee);

	//??? alter return type of callee0_value to match resolved_call_type.

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
			llvm::Value* arg2 = genllvm_expression(gen_acc, emit_f, details.args[out_arg.floyd_arg_index]);
			arg_values.push_back(arg2);
		}

		//??? make separate function of this.
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
			llvm::Value* arg2 = genllvm_expression(gen_acc, emit_f, details.args[out_arg.floyd_arg_index]);

			//	Actual type of the argument, as specified inside the call expression. The concrete type for the DYN value for this call.
			const auto concrete_arg_type = details.args[out_arg.floyd_arg_index].get_output_type();

			if(concrete_arg_type.is_function()){
				llvm::Value* arg3 = builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, arg2, make_dyn_value_type(context), "function_as_arg");
				arg_values.push_back(arg3);
			}
			else if(concrete_arg_type.is_double()){
				llvm::Value* arg3 = builder.CreateCast(llvm::Instruction::CastOps::BitCast, arg2, make_dyn_value_type(context), "double_as_arg");
				arg_values.push_back(arg3);
			}
			else if(concrete_arg_type.is_string()){
				llvm::Value* arg3 = builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, arg2, make_dyn_value_type(context), "string_as_arg");
				arg_values.push_back(arg3);
			}
			else if(concrete_arg_type.is_vector()){
				QUARK_ASSERT(concrete_arg_type.get_vector_element_type().is_string());

				llvm::Value* arg3 = get_vec_as_dyn(gen_acc.builder, arg2);
				arg_values.push_back(arg3);
			}
			else if(concrete_arg_type.is_int()){
				arg_values.push_back(arg2);
			}
			else if(concrete_arg_type.is_bool()){
				arg_values.push_back(arg2);
			}
			else{
				NOT_IMPLEMENTED_YET();
			}

			// We assume that the next arg in the llvm_mapping is the dyn-type.
			//??? We only support the base-types, not composite types.
			const auto base_type_id = (int64_t)concrete_arg_type.get_base_type();
			llvm::Constant* gen_type = llvm::ConstantInt::get(make_dyn_type_type(context), base_type_id);

			arg_values.push_back(gen_type);
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
	if(callee_function_type.get_function_return().is_internal_dynamic()){
		if(resolved_call_type.is_string()){
			auto dyn_a = builder.CreateExtractValue(result, { static_cast<int>(DYN_RETURN_MEMBERS::a) });
			auto dyn_b = builder.CreateExtractValue(result, { static_cast<int>(DYN_RETURN_MEMBERS::b) });
			result = builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, dyn_a, llvm::Type::getInt8PtrTy(context), "encoded->string");
/*
!!!
			llvm::Value* vec_ptr = builder.CreateCast(llvm::Instruction::CastOps::BitCast, result0, make_dynreturn_type(context), "encoded->string");
			auto elements_ptr_value = builder.CreateExtractValue(vec_ptr, { static_cast<int>(DYN_RETURN_MEMBERS::a) });
			result = builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, elements_ptr_value, llvm::Type::getInt8PtrTy(context), "encoded->string");
*/

 }
		else if(resolved_call_type.is_vector()){
/*
			auto vec_elements_ptr_value = builder.CreateExtractValue(vec_ptr, { static_cast<int>(VEC_T_MEMBERS::element_ptr) });
			auto vec_magic_value = builder.CreateExtractValue(vec_ptr, { static_cast<int>(VEC_T_MEMBERS::magic) });
			auto vec_element_bits_value = builder.CreateExtractValue(vec_ptr, { static_cast<int>(VEC_T_MEMBERS::element_bits) });
			auto vec_element_count = builder.CreateExtractValue(vec_ptr, { static_cast<int>(VEC_T_MEMBERS::element_count) });

			auto element_ptr = builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, dyn_a, llvm::Type::getInt64PtrTy(context), "a->element_ptr");

			auto element_count_value = builder.CreateTrunc(dyn_b, llvm::Type::getInt32Ty(context));

			const auto vec_type = make_vec_type(context);
			auto vec_value = builder.CreateAlloca(vec_type, nullptr, "temp_vec");

			const auto gep_index_list = std::vector<llvm::Value*>{
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), static_cast<int>(VEC_T_MEMBERS::element_ptr)),
			};
			llvm::Value* e_addr = builder.CreateGEP(vec_type, vec_value, gep_index_list, "");

			const auto gep_index_list2 = std::vector<llvm::Value*>{
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
				llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), static_cast<int>(VEC_T_MEMBERS::element_count))
			};
			llvm::Value* f_addr = builder.CreateGEP(vec_type, vec_value, gep_index_list2, "");

			builder.CreateStore(element_ptr, e_addr);
			builder.CreateStore(element_count_value, f_addr);
			result = builder.CreateLoad(vec_value, "final");
*/

			//	Store the DYN to memory, then cast it to VEC and load it again.
			auto dyn_value = builder.CreateAlloca(make_dynreturn_type(context), nullptr, "temp_vec");
			builder.CreateStore(result0, dyn_value);
			auto x = builder.CreateCast(llvm::Instruction::CastOps::BitCast, dyn_value, make_vec_type(context)->getPointerTo(), "encoded-> [string]");
			result = builder.CreateLoad(x, "final");
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

llvm::Value* alloc_vec_int64(llvmgen_t& gen_acc, llvm::Function& emit_f, uint16_t element_bits, uint64_t element_count, const std::string& debug){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(element_bits > 0 && element_bits < (8 * 128));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto allocate_vector_func = find_function_def(gen_acc, "floyd_runtime__allocate_vector");
	const auto element_bits_value = llvm::ConstantInt::get(llvm::Type::getInt16Ty(context), element_bits);
	const auto element_count_value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), element_count);

	std::vector<llvm::Value*> args2;
	args2.push_back(get_callers_fcp(emit_f));
	args2.push_back(element_bits_value);
	args2.push_back(element_count_value);
	auto vec = builder.CreateCall(allocate_vector_func.llvm_f, args2, "allocate_vector()-" + debug);
	return vec;
}



llvm::Value* llvmgen_construct_value_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto target_type = e.get_output_type();
	const auto caller_arg_count = details.elements.size();

	if(target_type.is_vector()){
		const auto element_type0 = target_type.get_vector_element_type();

		const auto allocate_vector_func = find_function_def(gen_acc, "floyd_runtime__allocate_vector");

		if(element_type0.is_string()){

			//	Each element is a char*.
			auto element_type = llvm::Type::getInt8PtrTy(context);

			const auto vec_type = make_vec_type(context);

			const auto element_count = caller_arg_count;
			const auto element_count_value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), element_count);

			//	Local function, called once.
			const auto vec_value = [&](){
				std::vector<llvm::Value*> args2;
				args2.push_back(get_callers_fcp(emit_f));
				args2.push_back(element_count_value);
				auto result = builder.CreateCall(allocate_vector_func.llvm_f, args2, "allocate_vector()" + typeid_to_compact_string(target_type));
				return result;
			}();

			//	Get element_ptr, which is member0 of VEC_T.
/*
			const auto gep_index_list = std::vector<llvm::Value*>{ 0, 0 };
			llvm::Value* element_ptr_addr = builder.CreateGEP(vec_type, vec_value, gep_index_list, "element_ptr");
			llvm::Value* element_ptr = builder.CreateLoad(element_ptr_addr, "element_ptr");
*/
			auto uint64_element_ptr = builder.CreateExtractValue(vec_value, { (int)VEC_T_MEMBERS::element_ptr });
			auto element_ptr = builder.CreateCast(llvm::Instruction::CastOps::BitCast, uint64_element_ptr, element_type->getPointerTo(), "");

			//	Evaluate each element and store it directly into the the vector.
			int element_index = 0;
			for(const auto& arg: details.elements){
				llvm::Value* arg_value = genllvm_expression(gen_acc, emit_f, arg);
				auto element_index_value = make_constant(gen_acc, value_t::make_int(element_index));
				const auto gep_index_list2 = std::vector<llvm::Value*>{ element_index_value };
				llvm::Value* e_addr = builder.CreateGEP(element_type, element_ptr, gep_index_list2, "e_addr");
				builder.CreateStore(arg_value, e_addr);

				element_index++;
			}

			return vec_value;
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(target_type.is_dict()){
		NOT_IMPLEMENTED_YET();
/*
		if(encode_as_dict_w_inplace_values(target_type)){
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_inplace_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_external_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
*/

	}
	else if(target_type.is_struct()){
		NOT_IMPLEMENTED_YET();
/*
		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_struct,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
*/

	}
	else{
		NOT_IMPLEMENTED_YET();
		QUARK_ASSERT(caller_arg_count == 1);

/*
		const auto source_itype = arg_count == 0 ? -1 : intern_type(gen_acc, e._input_exprs[0].get_output_type());
		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_1,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(source_itype)
		));
*/

	}

/*
	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_popn, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));
*/
	return nullptr;
}


#if 0
!!!
llvm::Value* llvmgen_construct_value_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	const auto target_type = e.get_output_type();
	const auto caller_arg_count = details.elements.size();

	if(target_type.is_vector()){
		const auto element_type0 = target_type.get_vector_element_type();

		if(element_type0.is_string()){
			//	Each element is a char*.
			auto element_type = llvm::Type::getInt8PtrTy(context);
			auto element_count = caller_arg_count;
			auto vec_value = alloc_vec_int64(gen_acc, 64, element_count, "[string]");

			auto uint64_element_ptr = builder.CreateExtractValue(vec_value, { (int)VEC_T_MEMBERS::element_ptr });
			auto element_ptr = builder.CreateCast(llvm::Instruction::CastOps::BitCast, uint64_element_ptr, element_type->getPointerTo(), "");

			//	Evaluate each element and store it directly into the the vector.
			int element_index = 0;
			for(const auto& arg: details.elements){
				llvm::Value* arg_value = genllvm_expression(gen_acc, emit_f, arg);
				auto element_index_value = make_constant(gen_acc, value_t::make_int(element_index));
				const auto gep_index_list2 = std::vector<llvm::Value*>{ element_index_value };
				llvm::Value* e_addr = builder.CreateGEP(element_type, element_ptr, gep_index_list2, "e_addr");
				builder.CreateStore(arg_value, e_addr);

				element_index++;
			}
			return vec_value;
		}
		else if(element_type0.is_bool()){
			const auto input_element_count = caller_arg_count;

			//	Each element is a bit, packed into the 64bit elements of VEC_T.
			auto output_element_count = (input_element_count / 64) + ((input_element_count & 63) ? 1 : 0);
			auto vec_value = alloc_vec_int64(gen_acc, 1, output_element_count, "[bool]");

			auto uint64_element_ptr = builder.CreateExtractValue(vec_value, { (int)VEC_T_MEMBERS::element_ptr });

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
					auto arg_value = genllvm_expression(gen_acc, emit_f, details.elements[input_element_index]);
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

				auto output_element_index_value = make_constant(gen_acc, value_t::make_int(output_element_index));
				const auto gep_index_list2 = std::vector<llvm::Value*>{ output_element_index_value };
				llvm::Value* e_addr = builder.CreateGEP(gen_acc.builder.getInt64Ty(), uint64_element_ptr, gep_index_list2, "e_addr");
				builder.CreateStore(word_out, e_addr);

				output_element_index++;
			}
			return vec_value;
		}
		else{
			NOT_IMPLEMENTED_YET();
		}
	}
	else if(target_type.is_dict()){
		NOT_IMPLEMENTED_YET();
/*
		if(encode_as_dict_w_inplace_values(target_type)){
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_inplace_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
		else{
			body_acc._instrs.push_back(bcgen_instruction_t(
				bc_opcode::k_new_dict_w_external_values,
				target_reg2,
				make_imm_int(target_itype),
				make_imm_int(arg_count)
			));
		}
*/

	}
	else if(target_type.is_struct()){
		NOT_IMPLEMENTED_YET();
/*
		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_struct,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(arg_count)
		));
*/

	}
	else{
		NOT_IMPLEMENTED_YET();
		QUARK_ASSERT(caller_arg_count == 1);

/*
		const auto source_itype = arg_count == 0 ? -1 : intern_type(gen_acc, e._input_exprs[0].get_output_type());
		body_acc._instrs.push_back(bcgen_instruction_t(
			bc_opcode::k_new_1,
			target_reg2,
			make_imm_int(target_itype),
			make_imm_int(source_itype)
		));
*/

	}

/*
	const auto extbits = pack_bools(call_setup._exts);
	body_acc._instrs.push_back(bcgen_instruction_t(bc_opcode::k_popn, make_imm_int(call_setup._stack_count), make_imm_int(extbits), {} ));
*/
	return nullptr;
}

#endif

llvm::Value* llvmgen_load2_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e, const expression_t::load2_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_symbol(gen_acc, details.address);
	return gen_acc.builder.CreateLoad(dest.value_ptr, "temp");
}

llvm::Value* genllvm_expression(llvmgen_t& gen_acc, llvm::Function& emit_f, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		llvmgen_t& gen_acc;
		const expression_t& e;
		llvm::Function& emit_f;


		llvm::Value* operator()(const expression_t::literal_exp_t& expr) const{
			return genllvm_literal_expression(gen_acc, emit_f, e);
		}
		llvm::Value* operator()(const expression_t::arithmetic_t& expr) const{
			return genllvm_arithmetic_expression(gen_acc, emit_f, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::comparison_t& expr) const{
			return llvmgen_comparison_expression(gen_acc, emit_f, expr.op, e, expr);
		}
		llvm::Value* operator()(const expression_t::unary_minus_t& expr) const{
			return genllvm_arithmetic_unary_minus_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::conditional_t& expr) const{
			return llvmgen_conditional_operator_expression(gen_acc, emit_f, e, expr);
		}

		llvm::Value* operator()(const expression_t::call_t& expr) const{
			return llvmgen_call_expression(gen_acc, emit_f, e, expr);
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
				return llvmgen_load2_expression(gen_acc, emit_f, e, expr);
			}
		}

		llvm::Value* operator()(const expression_t::resolve_member_t& expr) const{
			NOT_IMPLEMENTED_YET();
	//		return bcgen_resolve_member_expression(gen_acc, emit_f, target_reg, e, body);
		}
		llvm::Value* operator()(const expression_t::lookup_t& expr) const{
			return llvmgen_lookup_element_expression(gen_acc, emit_f, e, expr);
		}
		llvm::Value* operator()(const expression_t::value_constructor_t& expr) const{
			return llvmgen_construct_value_expression(gen_acc, emit_f, e, expr);
		}
	};

	llvm::Value* result = std::visit(visitor_t{ gen_acc, e, emit_f }, e._contents);
	return result;
}

void genllvm_store2_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::store2_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	llvm::Value* value = genllvm_expression(gen_acc, emit_f, s._expression);

	auto dest = find_symbol(gen_acc, s._dest_variable);
	gen_acc.builder.CreateStore(value, dest.value_ptr);

	QUARK_ASSERT(gen_acc.check_invariant());
}

gen_statement_mode llvmgen_block(llvmgen_t& gen_acc, llvm::Function& emit_f, const body_t& body){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto values = genllvm__alloc_symbols(gen_acc, emit_f, body._symbol_table);

	gen_acc.scope_path.push_back(values);
	const auto more = genllvm_statements(gen_acc, emit_f, body._statements);
	gen_acc.scope_path.pop_back();

	QUARK_ASSERT(gen_acc.check_invariant());
	return more;
}


gen_statement_mode llvmgen_block_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::block_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	return llvmgen_block(gen_acc, emit_f, s._body);
}


//???	Needs short-circuit evaluation here!
gen_statement_mode llvmgen_ifelse_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	//	Notice that genllvm_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* condition_value = genllvm_expression(gen_acc, emit_f, statement._condition);
	auto start_bb = builder.GetInsertBlock();

	auto then_bb = llvm::BasicBlock::Create(context, "then", parent_function);
	auto else_bb = llvm::BasicBlock::Create(context, "else", parent_function);

	builder.SetInsertPoint(start_bb);
	builder.CreateCondBr(condition_value, then_bb, else_bb);


	// Emit then-block.
	builder.SetInsertPoint(then_bb);

	//	Notice that llvmgen_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto then_mode = llvmgen_block(gen_acc, emit_f, statement._then_body);
	auto then_bb2 = builder.GetInsertBlock();


	// Emit else-block.
	builder.SetInsertPoint(else_bb);

	//	Notice that llvmgen_block() may create its own BBs and a different BB than then_bb may current when it returns.
	const auto else_mode = llvmgen_block(gen_acc, emit_f, statement._else_body);
	auto else_bb2 = builder.GetInsertBlock();


//	auto module = builder->GetInsertBlock()->getParent()->getParent();


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
gen_statement_mode llvmgen_for_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::for_statement_t& statement){
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

	//	Notice that genllvm_expression() may create its own BBs and a different BB than then_bb may current when it returns.
	llvm::Value* start_value = genllvm_expression(gen_acc, emit_f, statement._start_expression);
	llvm::Value* end_value = genllvm_expression(gen_acc, emit_f, statement._end_expression);

	auto values = genllvm__alloc_symbols(gen_acc, emit_f, statement._body._symbol_table);

	//	IMPORTANT: Iterator register is the FIRST symbol of the loop body's symbol table.
	llvm::Value* counter_value = values[0].value_ptr;
	builder.CreateStore(start_value, counter_value);

	llvm::Value* add_value = make_constant(gen_acc, value_t::make_int(1));

	llvm::CmpInst::Predicate pred = statement._range_type == statement_t::for_statement_t::k_closed_range
		? llvm::CmpInst::Predicate::ICMP_SLE
		: llvm::CmpInst::Predicate::ICMP_SLT;

	auto test_value = builder.CreateICmp(pred, start_value, end_value);
	builder.CreateCondBr(test_value, forloop_bb, forend_bb);



	//	EMIT LOOP BB

	builder.SetInsertPoint(forloop_bb);

	gen_acc.scope_path.push_back(values);
	const auto more = genllvm_statements(gen_acc, emit_f, statement._body._statements);
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


gen_statement_mode llvmgen_while_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	auto& context = gen_acc.instance->context;
	auto& builder = gen_acc.builder;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond_bb = llvm::BasicBlock::Create(context, "while-cond", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);
//	auto true_int = make_constant(gen_acc, value_t::make_bool(true));


	////////	header


	builder.CreateBr(while_cond_bb);
	builder.SetInsertPoint(while_cond_bb);





	////////	while_cond_bb


	builder.SetInsertPoint(while_cond_bb);
	llvm::Value* condition = genllvm_expression(gen_acc, emit_f, statement._condition);
//	auto test_value2 = builder.CreateICmp(pred, llvm::CmpInst::Predicate::ICMP_EQ, true_int);
	builder.CreateCondBr(condition, while_loop_bb, while_join_bb);




	////////	while_loop_bb


	builder.SetInsertPoint(while_loop_bb);
	const auto mode = llvmgen_block(gen_acc, emit_f, statement._body);
	builder.CreateBr(while_cond_bb);

	if(mode == gen_statement_mode::more){
	}
	else{
	}



	////////	while_join_bb


	builder.SetInsertPoint(while_join_bb);
	return gen_statement_mode::more;
}




void genllvm_expression_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::expression_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	genllvm_expression(gen_acc, emit_f, s._expression);

	QUARK_ASSERT(gen_acc.check_invariant());
}

/*
	All Floyd's global statements becomes instructions in floyd_init()-function that is called by Floyd runtime before any other function is called.
*/


llvm::Value* llvmgen_return_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t::return_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	llvm::Value* value = genllvm_expression(gen_acc, emit_f, s._expression);
	return gen_acc.builder.CreateRet(value);
}


gen_statement_mode genllvm_statement(llvmgen_t& gen_acc, llvm::Function& emit_f, const statement_t& statement){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		llvmgen_t& acc0;
		llvm::Function& emit_f;

		gen_statement_mode operator()(const statement_t::return_statement_t& s) const{
			llvmgen_return_statement(acc0, emit_f, s);
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
			genllvm_store2_statement(acc0, emit_f, s);
			return gen_statement_mode::more;
		}
		gen_statement_mode operator()(const statement_t::block_statement_t& s) const{
			return llvmgen_block_statement(acc0, emit_f, s);
		}

		gen_statement_mode operator()(const statement_t::ifelse_statement_t& s) const{
			return llvmgen_ifelse_statement(acc0, emit_f, s);
		}
		gen_statement_mode operator()(const statement_t::for_statement_t& s) const{
			return llvmgen_for_statement(acc0, emit_f, s);
		}
		gen_statement_mode operator()(const statement_t::while_statement_t& s) const{
			return llvmgen_while_statement(acc0, emit_f, s);
		}


		gen_statement_mode operator()(const statement_t::expression_statement_t& s) const{
			genllvm_expression_statement(acc0, emit_f, s);
			return gen_statement_mode::more;
		}
		gen_statement_mode operator()(const statement_t::software_system_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
//					return body_acc;
		}
		gen_statement_mode operator()(const statement_t::container_def_statement_t& s) const{
			NOT_IMPLEMENTED_YET();
//					return body_acc;
		}
	};

	return std::visit(visitor_t{ gen_acc, emit_f }, statement._contents);
}

gen_statement_mode genllvm_statements(llvmgen_t& gen_acc, llvm::Function& emit_f, const std::vector<statement_t>& statements){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(check_emitting_function(emit_f));

	if(statements.empty() == false){
		for(const auto& statement: statements){
			QUARK_ASSERT(statement.check_invariant());
			const auto mode = genllvm_statement(gen_acc, emit_f, statement);
			if(mode == gen_statement_mode::function_returning){
				return gen_statement_mode::function_returning;
			}
		}
	}
	return gen_statement_mode::more;
}

llvm::Value* make_global(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr){
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

llvm::Type* deref_ptr(llvm::Type* type){
	if(type->isPointerTy()){
		llvm::PointerType* type2 = llvm::cast<llvm::PointerType>(type);
  		llvm::Type* element_type = type2->getElementType();
  		return element_type;
	}
	else{
		return type;
	}
}


llvm::Value* genllvm_make_global(llvmgen_t& gen_acc, const semantic_ast_t& ast, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(symbol_name.empty() == false);
	QUARK_ASSERT(symbol.check_invariant());

	auto& module = *gen_acc.module;
	const auto type0 = symbol.get_type();
	const auto itype = intern_type(module, type0);

	if(symbol._init.is_undefined()){
		return make_global(module, symbol_name, *itype, nullptr);
	}
	else{
		llvm::Constant* init = make_constant(gen_acc, symbol._init);
		//	dest->setInitializer(constant_value);
		return make_global(module, symbol_name, *itype, init);
	}
}


//	Make LLVM globals for every global in the AST.
//	Inits the globals when possible.
//	Other globals are uninitialised and global statements will store to them from floyd_runtime_init().
std::vector<resolved_symbol_t> genllvm_make_globals(llvmgen_t& gen_acc, const semantic_ast_t& ast, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(ast.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	std::vector<resolved_symbol_t> result;

	for(const auto& e: symbol_table._symbols){
		llvm::Value* value = genllvm_make_global(gen_acc, ast, e.first, e.second);
		const auto debug_str = "name:" + e.first + " symbol_t: " + symbol_to_string(e.second);
		result.push_back(make_resolved_symbol(value, debug_str, resolved_symbol_t::esymtype::k_global, e.first, e.second));

//		QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));
	}
	return result;
}


//	NOTICE: Fills-in the body of an existing LLVM function prototype.
void genllvm_fill_floyd_function_def(llvmgen_t& gen_acc, int function_id, const floyd::function_definition_t& function_def, const function_definition_t::floyd_func_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	const auto funcdef_name = get_function_def_name(function_id, function_def);

	auto f = gen_acc.module->getFunction(funcdef_name);
	QUARK_ASSERT(check_invariant__function(f));
	llvm::Function& emit_f = *f;


	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.instance->context, "entry", f);
	gen_acc.builder.SetInsertPoint(entryBB);

	auto symbol_table_values = genllvm_make_function_def_symbols(gen_acc, emit_f, function_def);
	gen_acc.scope_path.push_back(symbol_table_values);
	const auto more = genllvm_statements(gen_acc, *f, details._body->_statements);
	gen_acc.scope_path.pop_back();

	QUARK_ASSERT(check_invariant__function(f));
}

void genllvm_fill_functions_with_instructions(llvmgen_t& gen_acc, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	We already generate the LLVM function-prototypes for the global functions in genllvm_all().
	for(int function_id = 0 ; function_id < semantic_ast._tree._function_defs.size() ; function_id++){
		const auto& function_def = *semantic_ast._tree._function_defs[function_id];

		struct visitor_t {
			llvmgen_t& gen_acc;
			const int function_id;
			const function_definition_t& function_def;

			void operator()(const function_definition_t::empty_t& e) const{
				QUARK_ASSERT(false);
			}
			void operator()(const function_definition_t::floyd_func_t& e) const{
				genllvm_fill_floyd_function_def(gen_acc, function_id, function_def, e);
			}
			void operator()(const function_definition_t::host_func_t& e) const{
			}
		};
		std::visit(visitor_t{ gen_acc, function_id, function_def }, function_def._contents);
	}
}


//	Generate floyd_runtime_init() that runs all global statements, before main() is run.
//	The AST contains statements that initializes the global variables, including global functions.

//	Function prototype must NOT EXIST already.
llvm::Function* make_function_prototype2(llvm::Module& module, const function_definition_t& function_def, int function_id){
	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(function_id >= 0);

	const auto function_type = function_def._function_type;
	const auto funcdef_name = get_function_def_name(function_id, function_def);

	auto existing_f = module.getFunction(funcdef_name);
	QUARK_ASSERT(existing_f == nullptr);


	const auto mapping0 = map_function_arguments(module, function_type);
	const auto mapping = name_args(mapping0, function_def._args);

	llvm::Type* function_ptr_type = intern_type(module, function_type);
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
function_definition_t make_dummy_function_definition(){
	return function_definition_t::make_empty();
}

function_def_t make_host_proto(llvm::Module& module, const host_func_t& host_function){
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
std::vector<function_def_t> make_all_function_prototypes(llvm::Module& module, const function_defs_t& defs){
	std::vector<function_def_t> result;

	auto& context = module.getContext();



	//	floyd_runtime__compare_strings()
	{
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt32Ty(context),
			{
				make_frp_type(context),
				llvm::Type::getInt64Ty(context),
				llvm::Type::getInt8PtrTy(context),
				llvm::Type::getInt8PtrTy(context)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime__compare_strings", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}

	//	floyd_runtime__append_strings()
	{
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt8PtrTy(context),
			{
				make_frp_type(context),
				llvm::Type::getInt8PtrTy(context),
				llvm::Type::getInt8PtrTy(context)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime__append_strings", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}

	//	floyd_runtime__allocate_memory()
	{
		llvm::FunctionType* function_type = llvm::FunctionType::get(
			llvm::Type::getInt8PtrTy(context),
			{
				make_frp_type(context),
				llvm::Type::getInt64Ty(context)
			},
			false
		);
		auto f = module.getOrInsertFunction("floyd_runtime__allocate_memory", function_type);
		result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), -1, make_dummy_function_definition()});
	}


	result.push_back(make_host_proto(module, floyd_runtime__allocate_vector__make(context)));
	result.push_back(make_host_proto(module, floyd_runtime__delete_vector__make(context)));
	result.push_back(make_host_proto(module, floyd_runtime__compare_vectors__make(context)));


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

	//	Register all function defs as LLVM function prototypes.
	//	host functions are later linked by LLVM execution engine, by matching the function names.
	//	Floyd functions are later filled with instructions.
	{
		for(int function_id = 0 ; function_id < defs.size() ; function_id++){
			const auto& function_def = *defs[function_id];
			auto f = make_function_prototype2(module, function_def, function_id);
			result.push_back({ f->getName(), llvm::cast<llvm::Function>(f), function_id, function_def});
		}
	}
	return result;
}



//	Convert all use of dynamic host functions into explicit functions - a compile time transform like C++ templates.
//	Currently this is a runtime-thing, using dynamic-type, which is like std::any<>.
std::pair<statement_t, function_defs_t> expand_generics(const statement_t& statement, const function_defs_t& functions){
	QUARK_ASSERT(statement.check_invariant());

	struct visitor_t {
		const statement_t& in_statement;
		const function_defs_t& in_functions;

		std::pair<statement_t, function_defs_t> operator()(const statement_t::return_statement_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::define_struct_statement_t& s) const{
			UNSUPPORTED();
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::define_function_statement_t& s) const{
			UNSUPPORTED();
		}

		std::pair<statement_t, function_defs_t> operator()(const statement_t::bind_local_t& s) const{
			UNSUPPORTED();
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::store_t& s) const{
			UNSUPPORTED();
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::store2_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::block_statement_t& s) const{
			return { in_statement, in_functions };
		}

		std::pair<statement_t, function_defs_t> operator()(const statement_t::ifelse_statement_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::for_statement_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::while_statement_t& s) const{
			return { in_statement, in_functions };
		}


		std::pair<statement_t, function_defs_t> operator()(const statement_t::expression_statement_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::software_system_statement_t& s) const{
			return { in_statement, in_functions };
		}
		std::pair<statement_t, function_defs_t> operator()(const statement_t::container_def_statement_t& s) const{
			return { in_statement, in_functions };
		}
	};

	return std::visit(visitor_t{ statement, functions }, statement._contents);
}

//			[20, "print", { "init": { "function_id": 20 }, "symbol_type": "immutable_local", "value_type": ["func", "^void", ["^**dyn**"], true] }],
std::pair<body_t, function_defs_t>  expand_generics(const body_t& body, const function_defs_t& functions){
	auto functions2 = functions;

	//??? How to expand symbols, like a function-value in the symbol table?

	std::vector<statement_t> statements2;
	symbol_table_t symbol_table2;
	for(const auto& e: body._statements){
		const auto r = expand_generics(e, functions2);
		statements2.push_back(r.first);
		functions2 = r.second;
	}
	return { body_t(statements2, body._symbol_table), functions2 };
}


//	Find all calls of functions with dynamic arguments-types. This lets use generate-out variants of the function for those types.
//		print("test") print(123) => print$string(string v), => print$int(int v)

//	Alternative 2: transform funcdefs / calls of print(DYN) to print(int v_itype, uint64 v_packed)
//	Replace all use of DYN arguments with explicit function-defs and calls.
semantic_ast_t expand_generics(const semantic_ast_t& semantic_ast){
	//	Pass 1: find all *use* of generic functions and make those explicit with their types. Also collect all used variants of generic functions.
	//	Pass 2: replace generic functions with 0...many explicitly typed versions.

	//??? NOTICE: This can introduce semantic type-errors.
	//??? NOTICE: Implementation of a generic function can use typeof(v) to find out exact type.

//	auto globals2 = expand_generics(semantic_ast._tree._globals, semantic_ast._tree._function_defs);
	return semantic_ast;
}


std::pair<std::unique_ptr<llvm::Module>, std::vector<function_def_t>> genllvm_all(llvm_instance_t& instance, const std::string& module_name, const semantic_ast_t& semantic_ast){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(module_name.empty() == false);
	QUARK_ASSERT(semantic_ast.check_invariant());

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	auto module = std::make_unique<llvm::Module>(module_name.c_str(), instance.context);

	llvmgen_t gen_acc(instance, module.get());

	auto& context = instance.context;
	auto& builder = gen_acc.builder;

	//	Generate all LLVM nodes: functions and globals.
	//	This lets all other code reference them, even if they're not filled up with code yet.
	{
		const auto funcs = make_all_function_prototypes(*module, semantic_ast._tree._function_defs);

		gen_acc.function_defs = funcs;

		//	Global variables.
		{
			std::vector<resolved_symbol_t> globals = genllvm_make_globals(
				gen_acc,
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

	//	GENERATE CODE for floyd_runtime_init()
	//	Global instructions are packed into the "floyd_runtime_init"-function. LLVM doesn't have global instructions.
	{
		const auto def = find_function_def(gen_acc, "floyd_runtime_init");
		llvm::Function* f = def.llvm_f;
		llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);
		builder.SetInsertPoint(entryBB);
		llvm::Function& emit_f = *f;
		{
			//	Verify we've got a valid floyd_runtime_ptr as argument #0.
			llvm::Value* magic_index_value = llvm::ConstantInt::get(builder.getInt64Ty(), 0);
			const auto index_list = std::vector<llvm::Value*>{ magic_index_value };

			auto args = f->args();
			QUARK_ASSERT((args.end() - args.begin()) >= 1);
			auto floyd_context_arg_ptr = args.begin();

			llvm::Value* uint64ptr_value = builder.CreateCast(llvm::Instruction::CastOps::BitCast, floyd_context_arg_ptr, llvm::Type::getInt64Ty(context)->getPointerTo(), "");

			llvm::Value* magic_addr = builder.CreateGEP(llvm::Type::getIntNTy(context, 64), uint64ptr_value, index_list, "magic_addr");
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
			const auto more = genllvm_statements(gen_acc, emit_f, semantic_ast._tree._globals._statements);

			llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 667);
			builder.CreateRet(dummy_result);
		}
		QUARK_ASSERT(check_invariant__function(f));
	}

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());

	genllvm_fill_functions_with_instructions(gen_acc, semantic_ast);

	return { std::move(module), gen_acc.function_defs };
}

std::unique_ptr<llvm_ir_program_t> generate_llvm_ir(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());
	QUARK_ASSERT(module_name.empty() == false);
//	QUARK_TRACE_SS("INPUT:  " << json_to_pretty_string(semantic_ast_to_json(ast0)._value));

	auto ast = expand_generics(ast0);

	auto result0 = genllvm_all(instance, module_name, ast);
	auto module = std::move(result0.first);
	auto funcs = result0.second;

	auto result = std::make_unique<llvm_ir_program_t>(&instance, module, ast._tree._globals._symbol_table, funcs);
//	QUARK_TRACE_SS("result = " << floyd::print_program(*result));
	return result;
}


void* get_global_ptr(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getGlobalValueAddress(name);
	return  (void*)addr;
}

void* get_global_function(llvm_execution_engine_t& ee, const std::string& name){
	QUARK_ASSERT(ee.check_invariant());
	QUARK_ASSERT(name.empty() == false);

	const auto addr = ee.ee->getFunctionAddress(name);
	return (void*)addr;
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


std::pair<std::string, void*> make_host_function_mapping(const host_func_t& host_function){
	return {
		host_function.name_key,
		host_function.implementation_f
	};
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
	auto ee2 = llvm_execution_engine_t{ k_debug_magic, ee1, program_breaks.debug_globals, program_breaks.function_defs, {} };
	QUARK_ASSERT(ee2.check_invariant());

	const std::map<std::string, void*> function_map = {
		{ "floyd_runtime__compare_strings", reinterpret_cast<void *>(&floyd_runtime__compare_strings) },
		{ "floyd_runtime__append_strings", reinterpret_cast<void *>(&floyd_runtime__append_strings) },
		{ "floyd_runtime__allocate_memory", reinterpret_cast<void *>(&floyd_runtime__allocate_memory) },
		make_host_function_mapping(floyd_runtime__allocate_vector__make(instance.context)),
		make_host_function_mapping(floyd_runtime__delete_vector__make(instance.context)),
		make_host_function_mapping(floyd_runtime__compare_vectors__make(instance.context)),



		{ "floyd_funcdef__assert", reinterpret_cast<void *>(&floyd_funcdef__assert) },
		{ "floyd_funcdef__calc_binary_sha1", reinterpret_cast<void *>(&floyd_host_function_1001) },
		{ "floyd_funcdef__calc_string_sha1", reinterpret_cast<void *>(&floyd_host_function_1002) },
		{ "floyd_funcdef__create_directory_branch", reinterpret_cast<void *>(&floyd_host_function_1003) },
		{ "floyd_funcdef__delete_fsentry_deep", reinterpret_cast<void *>(&floyd_host_function_1004) },
		{ "floyd_funcdef__does_fsentry_exist", reinterpret_cast<void *>(&floyd_host_function_1005) },
		{ "floyd_funcdef__erase", reinterpret_cast<void *>(&floyd_host_function_1006) },
		{ "floyd_funcdef__exists", reinterpret_cast<void *>(&floyd_host_function_1007) },
		{ "floyd_funcdef__filter", reinterpret_cast<void *>(&floyd_host_function_1008) },
		{ "floyd_funcdef__find", reinterpret_cast<void *>(&floyd_funcdef__find) },

		{ "floyd_funcdef__get_fs_environment", reinterpret_cast<void *>(&floyd_host_function_1010) },
		{ "floyd_funcdef__get_fsentries_deep", reinterpret_cast<void *>(&floyd_host_function_1011) },
		{ "floyd_funcdef__get_fsentries_shallow", reinterpret_cast<void *>(&floyd_host_function_1012) },
		{ "floyd_funcdef__get_fsentry_info", reinterpret_cast<void *>(&floyd_host_function_1013) },
		{ "floyd_funcdef__get_json_type", reinterpret_cast<void *>(&floyd_host_function_1014) },
		{ "floyd_funcdef__get_time_of_day", reinterpret_cast<void *>(&floyd_host_function_1015) },
		{ "floyd_funcdef__jsonvalue_to_script", reinterpret_cast<void *>(&floyd_host_function_1016) },
		{ "floyd_funcdef__jsonvalue_to_value", reinterpret_cast<void *>(&floyd_host_function_1017) },
		{ "floyd_funcdef__map", reinterpret_cast<void *>(&floyd_host_function_1018) },
		{ "floyd_funcdef__map_string", reinterpret_cast<void *>(&floyd_host_function_1019) },

		{ "floyd_funcdef__print", reinterpret_cast<void *>(&floyd_funcdef__print) },
		{ "floyd_funcdef__push_back", reinterpret_cast<void *>(&floyd_funcdef__push_back) },
		{ "floyd_funcdef__read_text_file", reinterpret_cast<void *>(&floyd_host_function_1022) },
		{ "floyd_funcdef__reduce", reinterpret_cast<void *>(&floyd_host_function_1023) },
		{ "floyd_funcdef__rename_fsentry", reinterpret_cast<void *>(&floyd_host_function_1024) },
		{ "floyd_funcdef__replace", reinterpret_cast<void *>(&floyd_funcdef__replace) },
		{ "floyd_funcdef__script_to_jsonvalue", reinterpret_cast<void *>(&floyd_host_function_1026) },
		{ "floyd_funcdef__send", reinterpret_cast<void *>(&floyd_host_function_1027) },
		{ "floyd_funcdef__size", reinterpret_cast<void *>(&floyd_funcdef__size) },
		{ "floyd_funcdef__subset", reinterpret_cast<void *>(&floyd_funcdef__subset) },

		{ "floyd_funcdef__supermap", reinterpret_cast<void *>(&floyd_host_function_1030) },
		{ "floyd_funcdef__to_pretty_string", reinterpret_cast<void *>(&floyd_host_function_1031) },
		{ "floyd_funcdef__to_string", reinterpret_cast<void *>(&floyd_host__to_string) },
		{ "floyd_funcdef__typeof", reinterpret_cast<void *>(&floyd_host__typeof) },
		{ "floyd_funcdef__update", reinterpret_cast<void *>(&floyd_funcdef__update) },
		{ "floyd_funcdef__value_to_jsonvalue", reinterpret_cast<void *>(&floyd_host_function_1035) },
		{ "floyd_funcdef__write_text_file", reinterpret_cast<void *>(&floyd_host_function_1036) }
	};

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

uint64_t call_floyd_runtime_init(llvm_execution_engine_t& ee){
	QUARK_ASSERT(ee.check_invariant());

	auto a_func = reinterpret_cast<FLOYD_RUNTIME_INIT>(get_global_function(ee, "floyd_runtime_init"));
	QUARK_ASSERT(a_func != nullptr);

	int64_t a_result = (*a_func)((void*)&ee);
	QUARK_ASSERT(a_result == 667);
	return a_result;
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
		if(print_f){
			QUARK_ASSERT(print_f != nullptr);

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


//	Destroys program, can only run it once!
int64_t run_llvm_program(llvm_instance_t& instance, llvm_ir_program_t& program_breaks, const std::vector<floyd::value_t>& args){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(program_breaks.check_invariant());

	auto ee = make_engine_run_init(instance, program_breaks);
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



std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const std::string& program, const std::string& file){
	QUARK_ASSERT(instance.check_invariant());

	const auto cu = floyd::make_compilation_unit_nolib(program, file);
	const auto pass3 = compile_to_sematic_ast__errors(cu);
	auto bc = generate_llvm_ir(instance, pass3, file);
	return bc;
}


int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<floyd::value_t>& args){
	const auto cu = floyd::make_compilation_unit_nolib(program_source, file);
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir(instance, pass3, file);
	const auto result = run_llvm_program(instance, *program, args);
	QUARK_TRACE_SS("Fib = " << result);
	return result;
}


}	//	namespace floyd




////////////////////////////////		TESTS



QUARK_UNIT_TEST("", "From source: Check that floyd_runtime_init() runs and sets 'result' global", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("let int result = 1 + 2 + 3", "myfile.floyd");
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir(instance, pass3, "myfile.floyd");
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
	auto program = generate_llvm_ir(instance, pass3, "myfile.floyd");
	auto ee = make_engine_run_init(instance, *program);
	QUARK_ASSERT(ee._print_output == std::vector<std::string>{"5"});
}
