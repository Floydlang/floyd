//
//  floyd_llvm_codegen.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//


static const bool k_trace_pass_io = false;


#include "floyd_llvm_codegen.h"

#include "floyd_llvm_optimization.h"
#include "floyd_llvm_codegen_basics.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_runtime_functions.h"
#include "floyd_llvm_intrinsics.h"
#include "floyd_llvm_helpers.h"
#include "compiler_basics.h"
#include "utils.h"

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
#include <llvm/IR/PassManager.h>

#include "llvm/Support/TargetRegistry.h"


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"



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


struct llvm_code_generator_t;








////////////////////////////////		llvm_ir_program_t







llvm_ir_program_t::llvm_ir_program_t(llvm_instance_t* instance, std::unique_ptr<llvm::Module>& module2_swap, const llvm_type_lookup& type_lookup, const symbol_table_t& globals, const std::vector<function_link_entry_t>& function_link_map, const compiler_settings_t& settings) :
	instance(instance),
	type_lookup(type_lookup),
	debug_globals(globals),
	function_link_map(function_link_map),
	settings(settings)
{
	QUARK_ASSERT(settings.check_invariant());

	module.swap(module2_swap);
	QUARK_ASSERT(check_invariant());
}

bool llvm_ir_program_t::check_invariant() const {
	QUARK_ASSERT(instance != nullptr);
	QUARK_ASSERT(instance->check_invariant());
	QUARK_ASSERT(module);
	QUARK_ASSERT(check_invariant__module(module.get()));
	QUARK_ASSERT(settings.check_invariant());
	return true;
}







////////////////////////////////		BASICS





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

static std::string print_program(const llvm_ir_program_t& program){
	QUARK_ASSERT(program.check_invariant());

//	std::string dump;
//	llvm::raw_string_ostream stream2(dump);
//	program.module->print(stream2, nullptr);

	QUARK_SCOPED_TRACE("module");
	return print_module(*program.module);
}




////////////////////////////////		PRIMITIVES




resolved_symbol_t make_resolved_symbol(llvm::Value* value_ptr, std::string debug_str, resolved_symbol_t::esymtype t, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(value_ptr != nullptr);

	return { value_ptr, debug_str, t, symbol_name, symbol};
}

resolved_symbol_t find_symbol(llvm_code_generator_t& gen_acc, const symbol_pos_t& reg){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(gen_acc.scope_path.size() >= 1);
	QUARK_ASSERT(reg._parent_steps == symbol_pos_t::k_global_scope || (reg._parent_steps >= 0 && reg._parent_steps < gen_acc.scope_path.size()));

	if(reg._parent_steps == symbol_pos_t::k_global_scope){
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
	auto string_vec_ptr_reg = builder.CreateCall(gen_acc.gen.runtime_functions.floydrt_alloc_kstr.llvm_codegen_f, args, "string literal");
	return string_vec_ptr_reg;
};

//	Makes constant from a Floyd value. The constant may go in code segment or be computed at app init-time.
static llvm::Value* generate_constant(llvm_function_generator_t& gen_acc, const value_t& value){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(value.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	const auto type = value.get_type();
	const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, type);

	struct visitor_t {
		llvm_function_generator_t& gen_acc;
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		llvm::Type* itype;
		const value_t& value;


		llvm::Value* operator()(const undefined_t& e) const{
			UNSUPPORTED();
			return llvm::ConstantInt::get(itype, 17);
		}
		llvm::Value* operator()(const any_t& e) const{
			return llvm::ConstantInt::get(itype, 13);
		}

		llvm::Value* operator()(const void_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const bool_t& e) const{
			return llvm::ConstantInt::get(itype, value.get_bool_value() ? 1 : 0);
		}
		llvm::Value* operator()(const int_t& e) const{
			return llvm::ConstantInt::get(itype, value.get_int_value());
		}
		llvm::Value* operator()(const double_t& e) const{
			return llvm::ConstantFP::get(itype, value.get_double_value());
		}
		llvm::Value* operator()(const string_t& e) const{
			return generate_constant_string(gen_acc, value.get_string_value());
		}
		llvm::Value* operator()(const json_type_t& e) const{
			const auto& json0 = value.get_json();

			//	NOTICE: There is no clean way to embedd a json containing a json-null into the code segment.
			//	Here we use a nullptr instead of json_t*. This means we have to be prepared for json_t::null AND nullptr.
			if(json0.is_null()){
				auto json_type = get_llvm_type_as_arg(gen_acc.gen.type_lookup, type_t::make_json());
				llvm::PointerType* pointer_type = llvm::cast<llvm::PointerType>(json_type);
				return llvm::ConstantPointerNull::get(pointer_type);
			}
			else{
				UNSUPPORTED();
			}
		}
		llvm::Value* operator()(const typeid_type_t& e) const{
			return generate_itype_constant(gen_acc.gen, value.get_typeid_value());
		}

		llvm::Value* operator()(const struct_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const vector_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const dict_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const function_t& e2) const{
			const auto function_id = value.get_function_value();
			for(const auto& e: gen_acc.gen.link_map){
				const auto link_name = encode_floyd_func_link_name(function_id.name);
				if(e.link_name == link_name){
					return e.llvm_codegen_f;
				}
			}
			llvm::PointerType* ptr_type = llvm::cast<llvm::PointerType>(itype);
			return llvm::ConstantPointerNull::get(ptr_type);
		}
		llvm::Value* operator()(const symbol_ref_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
		llvm::Value* operator()(const named_type_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
	};
	return std::visit(visitor_t{ gen_acc, builder, context, itype, value }, get_type_variant(types, type));
}

//	Related: generate_global_symbol_slots()
//	Reserve stack slot for each local. But not arguments, they already have stack slot.
static std::vector<resolved_symbol_t> generate_symbol_slots(llvm_function_generator_t& gen_acc, const symbol_table_t& symbol_table, const llvm_function_def_t* mapping0){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

	auto& builder = gen_acc.gen.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	std::vector<resolved_symbol_t> result;
	for(const auto& symbol_kv: symbol_table._symbols){
		const auto& symbol = symbol_kv.second;
		const auto type = symbol.get_value_type();
		const auto type_peek = peek2(types, type);
		const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, type);

		const auto entry = [&]() -> resolved_symbol_t {
			//	Function arguments automatically get an alloca by LLVM. We just need to figure out its register (llvm::Value*).
			if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
				QUARK_ASSERT(mapping0 != nullptr);
				const auto mapping = *mapping0;

				const auto arg_it = std::find_if(mapping.args.begin(), mapping.args.end(), [&](const llvm_arg_mapping_t& arg) -> bool {
					return arg.floyd_name == symbol_kv.first;
				});

				auto f_args = gen_acc.emit_f.args();
				const auto f_args_size = f_args.end() - f_args.begin();

				QUARK_ASSERT(f_args_size >= 1);
				QUARK_ASSERT(f_args_size == mapping.args.size());

				const auto llvm_arg_index = arg_it - mapping.args.begin();
				QUARK_ASSERT(llvm_arg_index < f_args_size);

				llvm::Argument* dest = f_args.begin() + llvm_arg_index;

				const auto debug_str = "<ARGUMENT> name:" + symbol_kv.first + " symbol_t: " + symbol_to_string(types, symbol);
				return make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_function_argument, symbol_kv.first, symbol);
			}
			else if(symbol._symbol_type == symbol_t::symbol_type::named_type){
				const auto itype2 = get_llvm_type_as_arg(gen_acc.gen.type_lookup, type_desc_t::make_typeid());
				llvm::Value* dest = builder.CreateAlloca(itype2, nullptr, symbol_kv.first);
				auto c = generate_itype_constant(gen_acc.gen, type_desc_t::make_typeid());
				gen_acc.get_builder().CreateStore(c, dest);

				const auto debug_str = "<NAMED-TYPE> name:" + symbol_kv.first + " symbol_t: " + symbol_to_string(types, symbol);
				return make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, symbol_kv.first, symbol_kv.second);
			}
			else{
				//	Reserve stack slot for each local.
				llvm::Value* dest = builder.CreateAlloca(itype, nullptr, symbol_kv.first);

				//	Init the slot if needed.
				if(symbol._init.is_undefined() == false){
					llvm::Value* c = generate_constant(gen_acc, symbol._init);
					gen_acc.get_builder().CreateStore(c, dest);
				}

				if(symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
					QUARK_ASSERT(symbol._init.is_undefined());

					//	Make sure to null all RC values.
					if(is_rc_value(types, type)){
						auto c = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(itype));
						builder.CreateStore(c, dest);
					}
					else{
					}
				}
				else if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
					QUARK_ASSERT(false);
					throw std::exception();
				}
				else if(symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
					QUARK_ASSERT(symbol._init.is_undefined() == false);

					//	Make sure to null all RC values.
					if(is_rc_value(types, type)){
						auto c = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(itype));
						builder.CreateStore(c, dest);
					}
					else{
					}
				}
				else if(symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
					//	Make sure to null all RC values.
					if(is_rc_value(types, type)){
						auto c = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(itype));
						builder.CreateStore(c, dest);
					}
					else{
					}
				}
				else{
					QUARK_ASSERT(false);
					throw std::exception();
				}
				const auto debug_str = "<LOCAL> name:" + symbol_kv.first + " symbol_t: " + symbol_to_string(types, symbol);
				return make_resolved_symbol(dest, debug_str, resolved_symbol_t::esymtype::k_local, symbol_kv.first, symbol_kv.second);
			}
		}();

		result.push_back(entry);
	}
	return result;
}

//	Related: generate_global_symbol_slots(), generate_function_symbol_slots(), generate_local_block_symbol_slots()
static std::vector<resolved_symbol_t> generate_local_block_symbol_slots(llvm_function_generator_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

	return generate_symbol_slots(gen_acc, symbol_table, nullptr);
}

//	Related: generate_floyd_runtime_deinit(), generate_destruct_scope_locals()
//	Used for function scope, local scopes. Not used for globals.
static void generate_destruct_scope_locals(llvm_function_generator_t& gen_acc, const std::vector<resolved_symbol_t>& symbols){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	for(const auto& e: symbols){
		if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
			if(e.symbol._symbol_type == symbol_t::symbol_type::named_type){
			}
			else{
				const auto type = e.symbol.get_value_type();
				if(is_rc_value(types, type)){
					auto reg = builder.CreateLoad(e.value_ptr);
					generate_release(gen_acc, *reg, type);
				}
				else{
				}
			}
		}
	}
}

static function_return_mode generate_body_and_destruct_locals_if_some_path_not_returned(llvm_function_generator_t& gen_acc, const std::vector<resolved_symbol_t>& resolved_symbols, const std::vector<statement_t>& statements){
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

	auto values = generate_local_block_symbol_slots(gen_acc, body._symbol_table);
	const auto return_mode = generate_body_and_destruct_locals_if_some_path_not_returned(gen_acc, values, body._statements);

	QUARK_ASSERT(gen_acc.check_invariant());
	return return_mode;
}

static type_t get_expr_output_type(const llvm_code_generator_t& gen_acc, const expression_t& e){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	return e.get_output_type();
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

//	auto& builder = gen_acc.get_builder();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	auto struct_ptr_reg = generate_expression(gen_acc, *details.parent_address);


	const auto parent_type = peek2(types, get_expr_output_type(gen_acc.gen, *details.parent_address));
	QUARK_ASSERT(parent_type.is_struct());

	const auto& struct_def = parent_type.get_struct(types);
	int member_index = find_struct_member_index(struct_def, details.member_name);
	QUARK_ASSERT(member_index != -1);

	const auto& member_type = peek2(types, struct_def._members[member_index]._type);

/*
	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, *struct_ptr_reg, parent_type);

	const auto gep = std::vector<llvm::Value*>{
		//	Struct array index.
		builder.getInt32(0),

		//	Struct member index.
		builder.getInt32(member_index)
	};
	llvm::Value* member_ptr_reg = builder.CreateGEP(&struct_type_llvm, base_ptr_reg, gep, "");
	auto member_value_reg = builder.CreateLoad(member_ptr_reg);
*/

	auto member_value_reg = generate_load_struct_member(gen_acc, *struct_ptr_reg, parent_type, member_index);
	generate_retain(gen_acc, *member_value_reg, member_type);
	generate_release(gen_acc, *struct_ptr_reg, parent_type);

	return member_value_reg;
}

static llvm::Value* generate_update_member_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::update_member_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto parent_struct_ptr_reg = generate_expression(gen_acc, *details.parent_address);
	auto new_value_reg = generate_expression(gen_acc, *details.new_value);

	const auto struct_type = get_expr_output_type(gen_acc.gen, *details.parent_address);
	const auto member_type = get_expr_output_type(gen_acc.gen, *details.new_value);
	auto struct2_ptr_reg = generate_update_struct_member(gen_acc, *parent_struct_ptr_reg, struct_type, details.member_index, *new_value_reg);

	generate_release(gen_acc, *new_value_reg, member_type);
	generate_release(gen_acc, *parent_struct_ptr_reg, struct_type);

	return struct2_ptr_reg;
}

static llvm::Value* generate_lookup_element_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();
	const auto& types = gen_acc.gen.type_lookup.state.types;

	auto parent_reg = generate_expression(gen_acc, *details.parent_address);
	auto key_reg = generate_expression(gen_acc, *details.lookup_key);
	const auto key_type = get_expr_output_type(gen_acc.gen, *details.lookup_key);
	const auto key_type_peek = peek2(types, key_type);

	const auto parent_type =  get_expr_output_type(gen_acc.gen, *details.parent_address);
	const auto parent_type_peek = peek2(types, parent_type);
	if(parent_type_peek.is_string()){
		QUARK_ASSERT(key_type_peek.is_int());

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
	else if(parent_type_peek.is_json()){
		QUARK_ASSERT(key_type_peek.is_int() || key_type_peek.is_string());

		//	Notice that we only know at RUNTIME if the json can be looked up: it needs to be json-object or a
		//	json-array. The key is either a string or an integer.

		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),
			parent_reg,
			generate_cast_to_runtime_value(gen_acc.gen, *key_reg, key_type),
			generate_itype_constant(gen_acc.gen, key_type)
		};
		auto result = builder.CreateCall(gen_acc.gen.runtime_functions.floydrt_lookup_json.llvm_codegen_f, args, "");

		generate_release(gen_acc, *parent_reg, parent_type);
		generate_release(gen_acc, *key_reg, key_type);
		return result;
	}
	else if(is_vector_carray(types, gen_acc.gen.settings.config, parent_type)){
		QUARK_ASSERT(key_type_peek.is_int());

		const auto element_type0 = peek2(types, parent_type).get_vector_element_type(types);

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
	else if(is_vector_hamt(types, gen_acc.gen.settings.config, parent_type)){
		QUARK_ASSERT(key_type_peek.is_int());

		const auto element_type0 = peek2(types, parent_type).get_vector_element_type(types);
		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			parent_reg,
			generate_itype_constant(gen_acc.gen, parent_type),
			generate_cast_to_runtime_value(gen_acc.gen, *key_reg, key_type),
		};
		auto element_value_uint64_reg = builder.CreateCall(gen_acc.gen.runtime_functions.floydrt_load_vector_element_hamt.llvm_codegen_f, args2, "");
		auto result_reg = generate_cast_from_runtime_value(gen_acc.gen, *element_value_uint64_reg, element_type0);

		generate_retain(gen_acc, *result_reg, element_type0);
		generate_release(gen_acc, *parent_reg, parent_type);

		//?? not needed, key is always int
		generate_release(gen_acc, *key_reg, key_type);

		return result_reg;
	}
	else if(is_dict_cppmap(types, gen_acc.gen.settings.config, parent_type) || is_dict_hamt(types, gen_acc.gen.settings.config, parent_type)){
		QUARK_ASSERT(key_type_peek.is_string());

		const auto element_type0 = peek2(types, parent_type).get_dict_value_type(types);
		const auto dict_mode = is_dict_hamt(types, gen_acc.gen.settings.config, parent_type) ? dict_backend::hamt : dict_backend::cppmap;
		auto element_value_uint64_reg = generate_lookup_dict(gen_acc, *parent_reg, parent_type, *key_reg, dict_mode);
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

	const auto& types = gen_acc.gen.type_lookup.state.types;
	const auto type = get_expr_output_type(gen_acc.gen, *details.lhs);
	const auto type_peek = peek2(types, type);

	auto lhs_temp = generate_expression(gen_acc, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, *details.rhs);

	if(type_peek.is_bool()){
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
	else if(type_peek.is_int()){
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
	else if(type_peek.is_double()){
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
	else if(type_peek.is_string() || type_peek.is_vector()){
		//	Only add supported. Future: don't use arithmetic add to concat collections!
		QUARK_ASSERT(details.op == expression_type::k_arithmetic_add);

		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			generate_itype_constant(gen_acc.gen, type),
			lhs_temp,
			rhs_temp
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.runtime_functions.floydrt_concatunate_vectors.llvm_codegen_f, args2, "");
		generate_release(gen_acc, *lhs_temp, get_expr_output_type(gen_acc.gen, *details.lhs));
		generate_release(gen_acc, *rhs_temp, get_expr_output_type(gen_acc.gen, *details.rhs));
		return result;
	}
	else{
		//	No other types allowed.
		throw std::exception();
	}
	UNSUPPORTED();
}



static llvm::Value* generate_compare_values(llvm_function_generator_t& gen_acc, expression_type op, const type_t& type, llvm::Value& lhs_reg, llvm::Value& rhs_reg){
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
	auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.runtime_functions.floydrt_compare_values.llvm_codegen_f, args, "");

	QUARK_ASSERT(gen_acc.check_invariant());
	return result;
}


static llvm::Value* generate_comparison_expression(llvm_function_generator_t& gen_acc, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;

	auto lhs_temp = generate_expression(gen_acc, *details.lhs);
	auto rhs_temp = generate_expression(gen_acc, *details.rhs);

	//	Type is the data the opcode works on -- comparing two ints, comparing two strings etc.
	const auto type = get_expr_output_type(gen_acc.gen, *details.lhs);

	const auto type_peek = peek2(types, type);

	//	Output reg is always a bool.
	QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, e)).is_bool());

	if(type_peek.is_bool() || type_peek.is_int()){
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
	else if(type_peek.is_double()){
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
	else if(type_peek.is_string() || type_peek.is_vector()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, get_expr_output_type(gen_acc.gen, *details.lhs));
		generate_release(gen_acc, *rhs_temp, get_expr_output_type(gen_acc.gen, *details.rhs));
		return result_reg;
	}
	else if(type_peek.is_dict()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, get_expr_output_type(gen_acc.gen, *details.lhs));
		generate_release(gen_acc, *rhs_temp, get_expr_output_type(gen_acc.gen, *details.rhs));
		return result_reg;
	}
	else if(type_peek.is_struct()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, get_expr_output_type(gen_acc.gen, *details.lhs));
		generate_release(gen_acc, *rhs_temp, get_expr_output_type(gen_acc.gen, *details.rhs));
		return result_reg;
	}
	else if(type_peek.is_json()){
		auto result_reg = generate_compare_values(gen_acc, details.op, type, *lhs_temp, *rhs_temp);
		generate_release(gen_acc, *lhs_temp, get_expr_output_type(gen_acc.gen, *details.lhs));
		generate_release(gen_acc, *rhs_temp, get_expr_output_type(gen_acc.gen, *details.rhs));
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

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, e)).is_int());
	QUARK_ASSERT(operands.size() == 1 || operands.size() == 2);

	if(operands.size() == 1){
		QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, operands[0])).is_int());

		auto a = generate_expression(gen_acc, operands[0]);

		if(op == bitwize_operator::bw_not){
			return gen_acc.get_builder().CreateNot(a);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	else if(operands.size() == 2){
		QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, operands[0])).is_int());
		QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, operands[1])).is_int());

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

	const auto& types = gen_acc.gen.type_lookup.state.types;
	const auto type = get_expr_output_type(gen_acc.gen, *details.expr);
	const auto type_peek = peek2(types, type);
	if(type_peek.is_int()){
		const auto e2 = expression_t::make_arithmetic(expression_type::k_arithmetic_subtract, expression_t::make_literal_int(0), *details.expr, e._output_type);
		return generate_expression(gen_acc, e2);
	}
	else if(type_peek.is_double()){
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

	const auto result_type = get_expr_output_type(gen_acc.gen, e);
	const auto result_itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, result_type);

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
	generate_release(gen_acc, *condition_reg, type_t::make_bool());

	return phiNode;
}

//??? Why is this needed in backend, shouldn't semast have fixed this already? Hmm. I don't think semast stores the inferred type of the function in the AST.
//	Call functon type and callee function types are identical, except the callee function type can use ANY-types.
static type_t calc_resolved_function_type(const llvm_code_generator_t& gen, const expression_t& e, const type_t& callee_function_type, const std::vector<expression_t>& args){
	QUARK_ASSERT(gen.check_invariant());
	QUARK_ASSERT(callee_function_type.check_invariant());

	const auto& types = gen.type_lookup.state.types;

	//	Callee type can include ANY-arguments. Check the resolved call expression's types to know the types.
	const auto resolved_call_return_type = get_expr_output_type(gen, e);
	const auto resolved_call_arguments = mapf<type_t>(args, [&gen](auto& e){ return get_expr_output_type(gen, e); });


	if(false) trace_types(types);

	const auto resolved_call_function_type = make_function(
		types,
		resolved_call_return_type,
		resolved_call_arguments,
		peek2(gen.type_lookup.state.types, callee_function_type).get_function_pure(types)
	);

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(args.size() == peek2(gen.type_lookup.state.types, callee_function_type).get_function_args(types).size());

	if(false) trace_types(types);

	return resolved_call_function_type;
}

/*
	NOTICE: The function signature for the callee can hold DYNs.
	The actual arguments will be explicit types, never DYNs.

	Callee type can include ANY-arguments. Check the resolved call expression's types to know the types.

	Function signature
	- In call's callee signature
	- In call's actual arguments types and output type.
	- In function def

	IDEA: Use resolved_function_type + function name to select specialisation function. This lets
	us select specialisation of calls, like "string push_back(string, int)" vs [bool] push_back([bool], bool)
*/
static llvm::Value* generate_call_expression(llvm_function_generator_t& gen_acc, const expression_t& e0, const expression_t::call_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e0.check_invariant());

	const auto callee_function_type = get_expr_output_type(gen_acc.gen, *details.callee);
	const auto resolved_function_type = calc_resolved_function_type(gen_acc.gen, e0, get_expr_output_type(gen_acc.gen, *details.callee), details.args);
	auto callee_reg = generate_expression(gen_acc, *details.callee);

	std::vector<llvm::Value*> floyd_args;
	for(const auto& e: details.args){
		llvm::Value* arg_value = generate_expression(gen_acc, e);
		floyd_args.push_back(arg_value);
	}
	return generate_floyd_call(gen_acc, callee_function_type, resolved_function_type, *callee_reg, floyd_args);
}

//	Generates a call to the global function that implements the intrinsic.
llvm::Value* generate_fallthrough_intrinsic(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;

	if(false) trace_types(types);

	const auto& intrinsic_signatures = gen_acc.gen.intrinsic_signatures;

	//	Find function
    const auto it = std::find_if(intrinsic_signatures.vec.begin(), intrinsic_signatures.vec.end(), [&details](const intrinsic_signature_t& e) { return get_intrinsic_opcode(e) == details.call_name; } );
    QUARK_ASSERT(it != intrinsic_signatures.vec.end());
	const auto callee_function_type = it->_function_type;

	//	Verify that the actual argument expressions, their count and output types -- all match callee_function_type.
	QUARK_ASSERT(details.args.size() == peek2(types, callee_function_type).get_function_args(types).size());

	const auto resolved_call_function_type = calc_resolved_function_type(gen_acc.gen, e, callee_function_type, details.args);

	const auto name = it->name;
	const auto& def = find_function_def_from_link_name(gen_acc.gen.link_map, encode_intrinsic_link_name(name));
	auto callee_reg = def.llvm_codegen_f;
	QUARK_ASSERT(callee_reg != nullptr);

	std::vector<llvm::Value*> floyd_args;
	for(const auto& m: details.args){
		llvm::Value* arg_value = generate_expression(gen_acc, m);
		floyd_args.push_back(arg_value);
	}
	return generate_floyd_call(gen_acc, callee_function_type, resolved_call_function_type, *callee_reg, floyd_args);
}


static llvm::Value* generate_push_back_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, e)).is_vector() || peek2(types, get_expr_output_type(gen_acc.gen, e)).is_string());

	const auto it = std::find_if(gen_acc.gen.intrinsic_signatures.vec.begin(), gen_acc.gen.intrinsic_signatures.vec.end(), [&](const intrinsic_signature_t& s){ return s.name == "push_back"; } );

	const auto resolved_call_type = calc_resolved_function_type(gen_acc.gen, e, it->_function_type, details.args);
	const auto collection_type = get_expr_output_type(gen_acc.gen, details.args[0]);
	auto vector_reg = generate_expression(gen_acc, details.args[0]);
	auto element_reg = generate_expression(gen_acc, details.args[1]);
	return generate_instrinsic_push_back(gen_acc, resolved_call_type, *vector_reg, collection_type, *element_reg);
}

static llvm::Value* generate_size_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;

	const auto it = std::find_if(gen_acc.gen.intrinsic_signatures.vec.begin(), gen_acc.gen.intrinsic_signatures.vec.end(), [&](const intrinsic_signature_t& s){ return s.name == "size"; } );

	const auto collection_type = get_expr_output_type(gen_acc.gen, details.args[0]);
	QUARK_ASSERT(peek2(types, collection_type).is_vector() || peek2(types, collection_type).is_string() || peek2(types, collection_type).is_dict() || peek2(types, collection_type).is_json());

	const auto resolved_call_type = calc_resolved_function_type(gen_acc.gen, e, it->_function_type, details.args);
	auto collection_reg = generate_expression(gen_acc, details.args[0]);
	return generate_instrinsic_size(gen_acc, resolved_call_type, *collection_reg, collection_type);
}

static llvm::Value* generate_update_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, e)).is_vector() || peek2(types, get_expr_output_type(gen_acc.gen, e)).is_string() || peek2(types, get_expr_output_type(gen_acc.gen, e)).is_dict());

	const auto it = std::find_if(gen_acc.gen.intrinsic_signatures.vec.begin(), gen_acc.gen.intrinsic_signatures.vec.end(), [&](const intrinsic_signature_t& s){ return s.name == "update"; } );

	const auto resolved_call_type = calc_resolved_function_type(gen_acc.gen, e, it->_function_type, details.args);
	const auto collection_type = get_expr_output_type(gen_acc.gen, details.args[0]);
	auto vector_reg = generate_expression(gen_acc, details.args[0]);
	auto index_reg = generate_expression(gen_acc, details.args[1]);
	auto element_reg = generate_expression(gen_acc, details.args[2]);
	return generate_instrinsic_update(gen_acc, resolved_call_type, *vector_reg, collection_type, *index_reg, *element_reg);
}

static llvm::Value* generate_map_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;
	QUARK_ASSERT(peek2(types, get_expr_output_type(gen_acc.gen, e)).is_vector());

	const auto it = std::find_if(gen_acc.gen.intrinsic_signatures.vec.begin(), gen_acc.gen.intrinsic_signatures.vec.end(), [&](const intrinsic_signature_t& s){ return s.name == "map"; } );

	const auto resolved_call_type = calc_resolved_function_type(gen_acc.gen, e, it->_function_type, details.args);

	auto vector_reg = generate_expression(gen_acc, details.args[0]);
	const auto collection_type = get_expr_output_type(gen_acc.gen, details.args[0]);

	auto f_reg = generate_expression(gen_acc, details.args[1]);
	auto f_type = get_expr_output_type(gen_acc.gen, details.args[1]);

	auto context_reg = generate_expression(gen_acc, details.args[2]);
	auto context_type = get_expr_output_type(gen_acc.gen, details.args[2]);

	return generate_instrinsic_map(gen_acc, resolved_call_type, *vector_reg, collection_type, *f_reg, f_type, *context_reg, context_type);
}



static llvm::Value* generate_intrinsic_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::intrinsic_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.assert)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.to_string)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.to_pretty_string)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.typeof_sign)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.update)){
		return generate_update_expression(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.size)){
		return generate_size_expression(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.find)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.exists)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.erase)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.get_keys)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.push_back)){
		return generate_push_back_expression(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.subset)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.replace)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.parse_json_script)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.generate_json_script)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.to_json)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.from_json)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}

	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.get_json_type)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}



	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.map)){
		return generate_map_expression(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.map_dag)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.filter)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.reduce)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.stable_sort)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}



	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.print)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.send)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.exit)){
		return generate_fallthrough_intrinsic(gen_acc, e, details);
	}


	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_not)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_not, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_and)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_and, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_or)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_or, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_xor)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_xor, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_shift_left)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_left, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_shift_right)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_right, e, details.args);
	}
	else if(details.call_name == get_intrinsic_opcode(gen_acc.gen.intrinsic_signatures.bw_shift_right_arithmetic)){
		return generate_bitwize_expression(gen_acc, bitwize_operator::bw_shift_right_arithmetic, e, details.args);
	}


	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

static llvm::Value* generate_construct_vector(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& types = gen_acc.gen.type_lookup.state.types;

	const auto construct_type = details.value_type;

	QUARK_ASSERT(peek2(types, construct_type).is_vector());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto element_count = details.elements.size();
	const auto element_type0 = peek2(types, construct_type).get_vector_element_type(types);
	const auto& element_type1 = *get_llvm_type_as_arg(gen_acc.gen.type_lookup, element_type0);
	auto vec_type_reg = generate_itype_constant(gen_acc.gen, construct_type);

	if(is_vector_carray(types, gen_acc.gen.settings.config, construct_type)){
		auto vec_ptr_reg = generate_allocate_vector(gen_acc, construct_type, element_count, vector_backend::carray);

		auto ptr_reg = generate_get_vec_element_ptr_needs_cast(gen_acc, *vec_ptr_reg);

		if(peek2(types, element_type0).is_bool()){
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
			//	Evaluate each element and store it directly into the array.
			auto array_ptr_reg = builder.CreateCast(llvm::Instruction::CastOps::BitCast, ptr_reg, element_type1.getPointerTo(), "");

			int element_index = 0;
			for(const auto& element_value: details.elements){

				//	Move ownwership from temp to member element, no need for retain-release.

				llvm::Value* element_value_reg = generate_expression(gen_acc, element_value);
				generate_array_element_store(builder, *array_ptr_reg, element_index, *element_value_reg);
				element_index++;
			}
			return vec_ptr_reg;
		}
	}
	else if(is_vector_hamt(types, gen_acc.gen.settings.config, construct_type)){
		auto vec_ptr_reg = generate_allocate_vector(gen_acc, construct_type, element_count, vector_backend::hamt);
		int element_index = 0;
		for(const auto& element_value: details.elements){
			auto index_reg = generate_constant(gen_acc, value_t::make_int(element_index));
			auto element_value_reg = generate_expression(gen_acc, element_value);
			auto element_value2_reg = generate_cast_to_runtime_value(gen_acc.gen, *element_value_reg, element_type0);

			//	Move ownwership from temp to member element, no need for retain-release.
			builder.CreateCall(gen_acc.gen.runtime_functions.floydrt_store_vector_element_hamt_mutable.llvm_codegen_f, { gen_acc.get_callers_fcp(), vec_ptr_reg, vec_type_reg, index_reg, element_value2_reg }, "");
			element_index++;
		}
		return vec_ptr_reg;
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

static llvm::Value* generate_construct_dict(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& types = gen_acc.gen.type_lookup.state.types;
	const auto construct_type = details.value_type;
	QUARK_ASSERT(peek2(types, construct_type).is_dict());

	auto& builder = gen_acc.get_builder();

//	const auto element_type0 = peek2(types, construct_type).get_dict_value_type(types);
	auto dict_type_reg = generate_itype_constant(gen_acc.gen, construct_type);
	auto dict_acc_ptr_reg = builder.CreateCall(gen_acc.gen.runtime_functions.floydrt_allocate_dict.llvm_codegen_f, { gen_acc.get_callers_fcp(), dict_type_reg }, "");

	//	Elements are stored as pairs.
	QUARK_ASSERT((details.elements.size() & 1) == 0);

	const auto count = details.elements.size() / 2;
	for(int element_index = 0 ; element_index < count ; element_index++){
		llvm::Value* key0_reg = generate_expression(gen_acc, details.elements[element_index * 2 + 0]);
		llvm::Value* element0_reg = generate_expression(gen_acc, details.elements[element_index * 2 + 1]);
		generate_store_dict_mutable(gen_acc, *dict_acc_ptr_reg, construct_type, *key0_reg, *element0_reg, gen_acc.gen.settings.config.dict_backend_mode);
		generate_release(gen_acc, *key0_reg, type_t::make_string());
	}
	return dict_acc_ptr_reg;
}

static llvm::Value* generate_construct_struct(llvm_function_generator_t& gen_acc, const expression_t::value_constructor_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& types = gen_acc.gen.type_lookup.state.types;

	const auto construct_type = peek2(types, details.value_type);
	QUARK_ASSERT(construct_type.is_struct());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto element_count = details.elements.size();

	const auto& struct_def = construct_type.get_struct(types);
	auto& exact_struct_type = *get_exact_struct_type_byvalue(gen_acc.gen.type_lookup, construct_type);
	QUARK_ASSERT(struct_def._members.size() == element_count);


	const llvm::DataLayout& data_layout = gen_acc.gen.module->getDataLayout();
	const llvm::StructLayout* layout = data_layout.getStructLayout(&exact_struct_type);
	const auto struct_bytes = layout->getSizeInBytes();

	auto struct_type_reg = generate_itype_constant(gen_acc.gen, construct_type);
	const auto size_reg = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), struct_bytes);
	std::vector<llvm::Value*> args2 = {
		gen_acc.get_callers_fcp(),
		struct_type_reg,
		size_reg
	};
	//	Returns STRUCT_T*.
	auto generic_struct_ptr_reg = gen_acc.get_builder().CreateCall(gen_acc.gen.runtime_functions.floydrt_allocate_struct.llvm_codegen_f, args2, "");


	//!!! We basically inline the entire constructor here -- bad idea? Maybe generate a construction function and call it.
	auto base_ptr_reg = generate_get_struct_base_ptr(gen_acc, *generic_struct_ptr_reg, construct_type);

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

	auto& types = gen_acc.gen.type_lookup.state.types;
	const auto construct_type = details.value_type;

	const auto element_count = details.elements.size();
	QUARK_ASSERT(element_count == 1);

	//	Construct a primitive, using int(113) or double(3.14)

	auto element0_reg = generate_expression(gen_acc, details.elements[0]);
	const auto input_value_type = get_expr_output_type(gen_acc.gen, details.elements[0]);

	const auto construct_type_peek = peek2(types, construct_type);
	if(construct_type_peek.is_bool() || construct_type_peek.is_int() || construct_type_peek.is_double() || construct_type_peek.is_typeid()){
		return element0_reg;
	}

	//	NOTICE: string -> json needs to be handled at runtime.

	//	Automatically transform a json::string => string at runtime?
	else if(construct_type_peek.is_string() && peek2(types, input_value_type).is_json()){
		std::vector<llvm::Value*> args = {
			gen_acc.get_callers_fcp(),
			element0_reg
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.runtime_functions.floydrt_json_to_string.llvm_codegen_f, args, "");

		generate_release(gen_acc, *element0_reg, input_value_type);
		return result;
	}
	else if(construct_type_peek.is_json()){
		//	Put a value_t into a json
		std::vector<llvm::Value*> args2 = {
			gen_acc.get_callers_fcp(),
			generate_cast_to_runtime_value(gen_acc.gen, *element0_reg, input_value_type),
			generate_itype_constant(gen_acc.gen, input_value_type)
		};
		auto result = gen_acc.get_builder().CreateCall(gen_acc.gen.runtime_functions.floydrt_allocate_json.llvm_codegen_f, args2, "");

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

	auto& types = gen_acc.gen.type_lookup.state.types;
	const auto construct_type_peek = peek2(types, details.value_type);

	if(construct_type_peek.is_vector()){
		return generate_construct_vector(gen_acc, details);
	}
	else if(construct_type_peek.is_dict()){
		return generate_construct_dict(gen_acc, details);
	}
	else if(construct_type_peek.is_struct()){
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

//??? Could be generated in semantic analysis pass, by inserting statements around the generate_block(). Problem is implementing the mutating samples-array.

//??? Measured body needs access to all function's locals -- cannot easily put body into separate function.

//	Pointer to int64_t x 10000
/*
	; Foo bar[100]
	%bar = alloca %Foo, i32 100
	; bar[17].c = 0.0
	%2 = getelementptr %Foo, %Foo* %bar, i32 17, i32 2
	store double 0.0, double* %2
*/

static int64_t k_max_samples_count = 10000;
static int64_t k_max_run_time_ns = 3000000000;
static int64_t k_min_run_count = 2;

static llvm::Value* generate_benchmark_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::benchmark_expr_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();

	const auto& get_profile_time_f = gen_acc.gen.runtime_functions.floydrt_get_profile_time.llvm_codegen_f;
	const auto& analyse_benchmark_samples_f = gen_acc.gen.runtime_functions.floydrt_analyse_benchmark_samples.llvm_codegen_f;

	llvm::Function* parent_function = builder.GetInsertBlock()->getParent();

	auto while_cond1_bb = llvm::BasicBlock::Create(context, "while-cond1", parent_function);
	auto while_cond2_bb = llvm::BasicBlock::Create(context, "while-cond2", parent_function);
	auto while_loop_bb = llvm::BasicBlock::Create(context, "while-loop", parent_function);
	auto while_join_bb = llvm::BasicBlock::Create(context, "while-join", parent_function);


	////////	current BB

	auto max_sample_count_reg = generate_constant(gen_acc, value_t::make_int(k_max_samples_count));

	auto samples_ptr_reg = builder.CreateAlloca(llvm::Type::getInt64Ty(context), max_sample_count_reg, "samples array");
	QUARK_ASSERT(samples_ptr_reg->getType()->isPointerTy());
	QUARK_ASSERT(samples_ptr_reg->getType() == llvm::Type::getInt64Ty(context)->getPointerTo());

	auto index_ptr_reg = builder.CreateAlloca(llvm::Type::getInt64Ty(context));
	auto zero_int_reg = generate_constant(gen_acc, value_t::make_int(0));
	auto one_int_reg = generate_constant(gen_acc, value_t::make_int(1));
	builder.CreateStore(zero_int_reg, index_ptr_reg);
	auto min_count_reg = generate_constant(gen_acc, value_t::make_int(k_min_run_count));

	auto start_time_reg = builder.CreateCall(get_profile_time_f, { gen_acc.get_callers_fcp() }, "");

	auto length_ns_reg = generate_constant(gen_acc, value_t::make_int(k_max_run_time_ns));
	auto end_time_reg = builder.CreateAdd(start_time_reg, length_ns_reg);
	builder.CreateBr(while_cond1_bb);


	////////	while_cond1_bb: minimum 2 runs

	builder.SetInsertPoint(while_cond1_bb);
	auto index2_reg = builder.CreateLoad(index_ptr_reg);
	auto test2_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, index2_reg, min_count_reg);
	builder.CreateCondBr(test2_reg, while_loop_bb, while_cond2_bb);

	
	////////	while_cond2_bb: minimum run time
	builder.SetInsertPoint(while_cond2_bb);
	auto cur_time_reg = builder.CreateCall(get_profile_time_f, { gen_acc.get_callers_fcp() }, "");
	auto test3_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, cur_time_reg, end_time_reg);
	builder.CreateCondBr(test3_reg, while_loop_bb, while_join_bb);


	////////	while_loop_bb

	builder.SetInsertPoint(while_loop_bb);
	auto a_time_reg = builder.CreateCall(get_profile_time_f, { gen_acc.get_callers_fcp() }, "");

	//	BODY
	generate_block(gen_acc, *details.body);

	auto b_time_reg = builder.CreateCall(get_profile_time_f, { gen_acc.get_callers_fcp() }, "");

	auto duration_reg = builder.CreateSub(b_time_reg, a_time_reg, "calc dur");
	auto index_reg = builder.CreateLoad(index_ptr_reg);
	const auto gep = std::vector<llvm::Value*>{
		index_reg
	};
	auto dest_sample_reg = builder.CreateGEP(
		llvm::Type::getInt64Ty(context),
		samples_ptr_reg,
		std::vector<llvm::Value*>{ index_reg },
		""
	);
	builder.CreateStore(duration_reg, dest_sample_reg);

	//	Increment index.
	auto index4_reg = builder.CreateAdd(index_reg, one_int_reg);
	builder.CreateStore(index4_reg, index_ptr_reg);

	auto test4_reg = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, index4_reg, max_sample_count_reg);
	builder.CreateCondBr(test4_reg, while_cond1_bb, while_join_bb);


	////////	while_join_bb

	builder.SetInsertPoint(while_join_bb);
	auto index5_reg = builder.CreateLoad(index_ptr_reg);
	auto best_dur_reg = builder.CreateCall(
		analyse_benchmark_samples_f,
		{ gen_acc.get_callers_fcp(), samples_ptr_reg, index5_reg },
		""
	);

	return best_dur_reg;
}




static llvm::Value* generate_load2_expression(llvm_function_generator_t& gen_acc, const expression_t& e, const expression_t::load2_t& details){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(e.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	auto dest = find_symbol(gen_acc.gen, details.address);
	auto result = gen_acc.get_builder().CreateLoad(dest.value_ptr, "temp");
	generate_retain(gen_acc, *result, get_expr_output_type(gen_acc.gen, e));
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
				generate_retain(gen_acc, *result, get_expr_output_type(gen_acc.gen, e));
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

	const auto& types = gen_acc.gen.type_lookup.state.types;
	llvm::Value* value = generate_expression(gen_acc, s._expression);

	auto dest = find_symbol(gen_acc.gen, s._dest_variable);
	const auto type = dest.symbol.get_value_type();

	if(is_rc_value(types, type)){
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

	auto values = generate_local_block_symbol_slots(gen_acc, statement._body._symbol_table);

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

	const auto return_mode = generate_body_and_destruct_locals_if_some_path_not_returned(gen_acc, values, statement._body._statements);

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
	generate_release(gen_acc, *result_reg, get_expr_output_type(gen_acc.gen, s._expression));

	QUARK_ASSERT(gen_acc.check_invariant());
}


static llvm::Value* generate_return_statement(llvm_function_generator_t& gen_acc, const statement_t::return_statement_t& s){
	QUARK_ASSERT(gen_acc.check_invariant());

	llvm::Value* value = generate_expression(gen_acc, s._expression);

	//	Destruct all local scopes before unwinding.
	auto path = gen_acc.gen.scope_path;
	QUARK_ASSERT(path.size() > 0);
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
		function_return_mode operator()(const statement_t::test_def_statement_t& s) const{
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
std::vector<resolved_symbol_t> generate_function_symbol_slots(llvm_function_generator_t& gen_acc, const function_definition_t& function_def){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());

	QUARK_ASSERT(function_def._optional_body);

	const symbol_table_t& symbol_table = function_def._optional_body->_symbol_table;
	const auto mapping0 = *gen_acc.gen.type_lookup.find_from_type(function_def._function_type).optional_function_def;
	const auto mapping = name_args(mapping0, function_def._named_args);
	return generate_symbol_slots(gen_acc, symbol_table, &mapping);
}

static llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr){
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

static llvm::Value* generate_global(llvm_function_generator_t& gen_acc, const std::string& symbol_name, const symbol_t& symbol){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_name.empty() == false);
	QUARK_ASSERT(symbol.check_invariant());

	auto& module = *gen_acc.gen.module;

	const auto symbol_value_type = symbol.get_value_type();

	if(symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
		QUARK_ASSERT(symbol._init.is_undefined());

		const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, symbol_value_type);
		return generate_global0(module, symbol_name, *itype, nullptr);
	}
	else if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
		QUARK_ASSERT(false);
		throw std::exception();
	}
	else if(symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
		QUARK_ASSERT(symbol._init.is_undefined() == false);

		const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, symbol_value_type);
		auto init_reg = generate_constant(gen_acc, symbol._init);
		if (llvm::Constant* CI = llvm::dyn_cast<llvm::Constant>(init_reg)){
			return generate_global0(module, symbol_name, *itype, CI);
		}
		else{
			return generate_global0(module, symbol_name, *itype, nullptr);
		}
	}
	else if(symbol._symbol_type == symbol_t::symbol_type::named_type){
		const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, type_desc_t::make_typeid());
		auto itype_reg = generate_itype_constant(gen_acc.gen, symbol_value_type);
		return generate_global0(module, symbol_name, *itype, itype_reg);
	}
	else if(symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
		const auto itype = get_llvm_type_as_arg(gen_acc.gen.type_lookup, symbol_value_type);
		return generate_global0(module, symbol_name, *itype, nullptr);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}

//	Related: generate_global_symbol_slots(), generate_function_symbol_slots(), generate_local_block_symbol_slots()
//	Make LLVM globals for every global in the AST.
//	Inits the globals when possible.
//	Other globals are uninitialised and global init2-statements will store to them from floyd_runtime_init().
static std::vector<resolved_symbol_t> generate_global_symbol_slots(llvm_function_generator_t& gen_acc, const symbol_table_t& symbol_table){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(symbol_table.check_invariant());

//	QUARK_TRACE_SS("result = " << floyd::print_program(gen_acc.program_acc));

	std::vector<resolved_symbol_t> result;
	const auto& types = gen_acc.gen.type_lookup.state.types;

	for(const auto& symbol_kv: symbol_table._symbols){
		const auto debug_str = "name:" + symbol_kv.first + " symbol_t: " + symbol_to_string(types, symbol_kv.second);
		llvm::Value* value = generate_global(gen_acc, symbol_kv.first, symbol_kv.second);
		const auto resolved_symbol = make_resolved_symbol(value, debug_str, resolved_symbol_t::esymtype::k_global, symbol_kv.first, symbol_kv.second);
		result.push_back(resolved_symbol);
	}

	if(false){
		QUARK_TRACE_SS(print_module(*gen_acc.gen.module));
	}
	return result;
}

//	NOTICE: Fills-in the body of an existing LLVM function prototype.
static void generate_floyd_function_body(llvm_code_generator_t& gen_acc0, const floyd::function_definition_t& function_def, const body_t& body){
	QUARK_ASSERT(gen_acc0.check_invariant());
	QUARK_ASSERT(function_def.check_invariant());
	QUARK_ASSERT(body.check_invariant());

	auto& types = gen_acc0.type_lookup.state.types;
	const auto link_name = encode_floyd_func_link_name(function_def._definition_name);

	auto f = gen_acc0.module->getFunction(link_name.s);
	QUARK_ASSERT(check_invariant__function(f));

	{
		llvm_function_generator_t gen_acc(gen_acc0, *f);

		llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(gen_acc.gen.instance->context, "entry", f);
		gen_acc.get_builder().SetInsertPoint(entryBB);

		auto symbol_table_values = generate_function_symbol_slots(gen_acc, function_def);

		const auto return_mode = generate_body_and_destruct_locals_if_some_path_not_returned(gen_acc, symbol_table_values, body._statements);

		//	Not all paths returns a value!
		if(return_mode == function_return_mode::some_path_not_returned && peek2(types, peek2(types, function_def._function_type).get_function_return(types)).is_void() == false){
			throw std::runtime_error("Not all function paths returns a value!");
		}

		if(peek2(types, peek2(types, function_def._function_type).get_function_return(types)).is_void()){
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

//	Generate LLVM function nodes and merge them into link map.
static std::vector<function_link_entry_t> generate_function_nodes(llvm::Module& module, const llvm_type_lookup& type_lookup, const std::vector<function_link_entry_t>& link_map1){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<function_link_entry_t> result;
	for(const auto& e: link_map1){
		auto existing_f = module.getFunction(e.link_name.s);
		QUARK_ASSERT(existing_f == nullptr);

		auto f0 = module.getOrInsertFunction(e.link_name.s, e.llvm_function_type);
		auto f = llvm::cast<llvm::Function>(f0);

		QUARK_ASSERT(check_invariant__function(f));
		QUARK_ASSERT(check_invariant__module(&module));

		//	Set names for all function defintion's arguments - makes IR easier to read.
		if(e.function_type_or_undef.is_undefined() == false && e.arg_names_or_empty.empty() == false){
			const auto unnamed_mapping_ptr = type_lookup.find_from_type(e.function_type_or_undef).optional_function_def;
			if(unnamed_mapping_ptr != nullptr){
				const auto named_mapping = name_args(*unnamed_mapping_ptr, e.arg_names_or_empty);

				auto f_args = f->args();
				const auto f_arg_count = f_args.end() - f_args.begin();
				QUARK_ASSERT(f_arg_count == named_mapping.args.size());

				int index = 0;
				for(auto& a: f_args){
					const auto& m = named_mapping.args[index];
					const auto name = m.floyd_name;
					a.setName(name);
					index++;
				}
			}
		}

		QUARK_ASSERT(check_invariant__function(f));
		QUARK_ASSERT(check_invariant__module(&module));

		result.push_back(function_link_entry_t{ e.module, e.link_name, e.llvm_function_type, f, e.function_type_or_undef, e.arg_names_or_empty, e.native_f });
	}
	if(false){
		trace_function_link_map(type_lookup.state.types, result);
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

	llvm::Function* f = gen_acc.runtime_functions.floydrt_init.llvm_codegen_f;
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

	if(false){
		QUARK_TRACE_SS(print_module(*gen_acc.module));
	}
	QUARK_ASSERT(check_invariant__function(f));

	QUARK_ASSERT(gen_acc.check_invariant());
}


//	Related: generate_floyd_runtime_deinit(), generate_destruct_scope_locals()
static void generate_floyd_runtime_deinit(llvm_code_generator_t& gen_acc, const body_t& globals){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();
	auto& context = builder.getContext();
	const auto& types = gen_acc.type_lookup.state.types;

	llvm::Function* f = gen_acc.runtime_functions.floydrt_deinit.llvm_codegen_f;
	llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(context, "entry", f);

	{
		llvm_function_generator_t function_gen_acc(gen_acc, *f);

		//	entryBB
		{
			builder.SetInsertPoint(entryBB);

			//	Destruct global variables.
			for(const auto& e: function_gen_acc.gen.scope_path.front()){
/*
				if(e.symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
				}
				else if(e.symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
				}
				else if(e.symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
				}
				else if(e.symbol._symbol_type == symbol_t::symbol_type::named_type){
				}
				else if(e.symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
				}
				else{
					QUARK_ASSERT(false);
					throw std::exception();
				}
*/
				if(e.symtype == resolved_symbol_t::esymtype::k_global || e.symtype == resolved_symbol_t::esymtype::k_local){
					bool needs_destruct = e.symbol._symbol_type != symbol_t::symbol_type::named_type;
					if(needs_destruct){
						const auto type = e.symbol.get_value_type();
						if(is_rc_value(types, type)){
							auto reg = builder.CreateLoad(e.value_ptr);
							generate_release(function_gen_acc, *reg, type);
						}
						else{
						}
					}

				}
			}

			llvm::Value* dummy_result = llvm::ConstantInt::get(builder.getInt64Ty(), 668);
			builder.CreateRet(dummy_result);
		}
	}

	if(false){
		QUARK_TRACE_SS(print_module(*gen_acc.module));
	}
	QUARK_ASSERT(check_invariant__function(f));

//	QUARK_TRACE_SS("result = " << floyd::print_gen(gen_acc));
	QUARK_ASSERT(gen_acc.check_invariant());
}



struct module_output_t {
	std::unique_ptr<llvm::Module> module;
	std::vector<function_link_entry_t> link_map;
};

static module_output_t generate_module(llvm_instance_t& instance, const std::string& module_name, const semantic_ast_t& semantic_ast, const compiler_settings_t& settings){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(semantic_ast.check_invariant());
	QUARK_ASSERT(settings.check_invariant());

	//	Module must sit in a unique_ptr<> because llvm::EngineBuilder needs that.
	auto module = std::make_unique<llvm::Module>(module_name.c_str(), instance.context);
	auto data_layout = instance.target.target_machine->createDataLayout();
	module->setTargetTriple(instance.target.target_triple);
	module->setDataLayout(data_layout);


	llvm_type_lookup type_lookup(instance.context, semantic_ast._tree._types);

	//	Generate all LLVM function nodes: functions (without implementation) and globals.
	//	This lets all other code reference them, even if they're not filled up with code yet.
	const auto link_map1 = make_function_link_map1(module->getContext(), type_lookup, semantic_ast._tree._function_defs, semantic_ast.intrinsic_signatures);
	if(false){
		trace_function_link_map(type_lookup.state.types, link_map1);
	}
	const auto link_map2 = generate_function_nodes(*module, type_lookup, link_map1);

	auto gen_acc = llvm_code_generator_t(instance, module.get(), semantic_ast._tree._types, type_lookup, link_map2, settings, semantic_ast.intrinsic_signatures);

	//	Globals.
	{
		llvm_function_generator_t function_gen_acc(gen_acc, *gen_acc.runtime_functions.floydrt_init.llvm_codegen_f);

		std::vector<resolved_symbol_t> globals = generate_global_symbol_slots(
			function_gen_acc,
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

	return module_output_t{ std::move(module), gen_acc.link_map };
}


static std::vector<uint8_t> write_object_file(llvm::Module& module, const target_t& target, llvm::TargetMachine::CodeGenFileType type){
	QUARK_ASSERT(target.check_invariant());

	//	??? Migrate from legacy.
	llvm::legacy::PassManager pass;

	llvm::SmallVector<char, 0> stream_vec;
	llvm::raw_svector_ostream s(stream_vec);

	if (target.target_machine->addPassesToEmitFile(pass, s, nullptr, type)) {
		llvm::errs() << "TargetMachine can't emit a file of this type";
		throw std::exception();
	}

	pass.run(module);
	return std::vector<uint8_t>(stream_vec.begin(), stream_vec.end());
}

std::vector<uint8_t> write_object_file(llvm_ir_program_t& program, const target_t& target){
	QUARK_ASSERT(target.check_invariant());

	return write_object_file(*program.module, target, llvm::TargetMachine::CGFT_ObjectFile);
}
std::string write_ir_file(llvm_ir_program_t& program, const target_t& target){
	QUARK_ASSERT(target.check_invariant());

	const auto a = write_object_file(*program.module, target, llvm::TargetMachine::CGFT_AssemblyFile);
	return std::string(a.begin(), a.end());
}

static std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program_internal(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name, const compiler_settings_t& settings){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());
	QUARK_ASSERT(settings.check_invariant());

	auto ast = ast0;

	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	auto result0 = generate_module(instance, module_name, ast, settings);
	auto module = std::move(result0.module);

	if(settings.optimization_level == eoptimization_level::g_no_optimizations_enable_debugging){
	}
	else{
		optimize_module_mutating(instance, module, settings);
	}
//	write_object_file(module, *result0.target_machine);

	//???	Don't make a new llvm_type_lookup, generate_module() already created one.
	const auto type_lookup = llvm_type_lookup(instance.context, ast0._tree._types);

	auto result = std::make_unique<llvm_ir_program_t>(&instance, module, type_lookup, ast._tree._globals._symbol_table, result0.link_map, settings);

	result->container_def = ast0._tree._container_def;
	result->software_system = ast0._tree._software_system;
	return result;
}


std::unique_ptr<llvm_ir_program_t> generate_llvm_ir_program(llvm_instance_t& instance, const semantic_ast_t& ast0, const std::string& module_name, const compiler_settings_t& settings){
	QUARK_ASSERT(instance.check_invariant());
	QUARK_ASSERT(ast0.check_invariant());
	QUARK_ASSERT(settings.check_invariant());

	if(k_trace_pass_io){
		QUARK_SCOPED_TRACE("LLVM CODE GENERATION");

		{
			QUARK_SCOPED_TRACE("LLVM CODE GENERATION -- INPUT AST");
			QUARK_TRACE_SS(json_to_pretty_string(semantic_ast_to_json(ast0)));
		}

		auto result = generate_llvm_ir_program_internal(instance, ast0, module_name, settings);

		{
			QUARK_SCOPED_TRACE("LLVM CODE GENERATION -- OUTPUT LLVM PROGRAM");
			QUARK_TRACE(print_llvm_ir_program(*result));
		}
		return result;
	}
	else{
		return generate_llvm_ir_program_internal(instance, ast0, module_name, settings);
	}
}

std::string print_llvm_ir_program(const llvm_ir_program_t& program){
	QUARK_ASSERT(program.check_invariant());

	const auto a = print_module(*program.module);
	const auto b = print_function_link_map(program.type_lookup.state.types, program.function_link_map);
	return a + b;
}




}	//	floyd
