//
//  floyd_llvm_codegen_basics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-31.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen_basics.h"


namespace floyd {




llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const type_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto itype = lookup_type(gen_acc.type_lookup, type).get_data();
	auto t = make_runtime_type_type(gen_acc.type_lookup);
 	return llvm::ConstantInt::get(t, itype);
}




llvm::Value* generate_cast_to_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& value, const type_t& floyd_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	return generate_cast_to_runtime_value2(builder, gen_acc.type_lookup, value, floyd_type);
}

llvm::Value* generate_cast_from_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& runtime_value_reg, const type_t& type){
	auto& builder = gen_acc.get_builder();
	return generate_cast_from_runtime_value2(builder, gen_acc.type_lookup, runtime_value_reg, type);
}



llvm::Value* generate_get_vec_element_ptr_needs_cast(llvm_function_generator_t& gen_acc, llvm::Value& vec_ptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto after_alloc64_ptr_reg = builder.CreateGEP(make_generic_vec_type_byvalue(gen_acc.gen.type_lookup), &vec_ptr_reg, gep, "");
	return after_alloc64_ptr_reg;
}

llvm::Value* generate_get_struct_base_ptr(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const type_t& final_type){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto type = get_generic_struct_type_byvalue(gen_acc.gen.type_lookup);
	auto ptr_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, &struct_ptr_reg, type->getPointerTo(), "");

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto ptr2_reg = builder.CreateGEP(type, ptr_reg, gep, "");
	auto final_type2 = get_exact_struct_type_byvalue(gen_acc.gen.type_lookup, final_type);
	auto ptr3_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, ptr2_reg, final_type2->getPointerTo(), "");
	return ptr3_reg;
}

llvm::Value* generate_floyd_call(llvm_function_generator_t& gen_acc, const type_t& callee_function_type, const type_t& resolved_function_type, llvm::Value& callee_reg, const std::vector<llvm::Value*> floyd_args){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(callee_function_type.check_invariant());
	QUARK_ASSERT(resolved_function_type.check_invariant());

	const auto& types = gen_acc.gen.type_lookup.state.types;

	QUARK_ASSERT(peek2(types, callee_function_type).get_function_args(types).size() == floyd_args.size());

	auto& builder = gen_acc.get_builder();

	//	Callee, as LLVM function signature. It gets the param #0 runtime pointer and each ANY is expanded to value / type pairs.
    const auto& callee_type_entry = gen_acc.gen.type_lookup.find_from_type(callee_function_type);
    const auto& callee_mapping = *callee_type_entry.optional_function_def;

	//	Generate code that evaluates all argument expressions.
	std::vector<llvm::Value*> arg_regs;
	std::vector<std::pair<llvm::Value*, type_t> > destroy;

	for(const auto& out_arg: callee_mapping.args){
		if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			auto f_args = gen_acc.emit_f.args();
			QUARK_ASSERT((f_args.end() - f_args.begin()) >= 1);
			auto floyd_context_arg_ptr = f_args.begin();
			arg_regs.push_back(floyd_context_arg_ptr);
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
			QUARK_ASSERT(out_arg.floyd_arg_index >= 0 && out_arg.floyd_arg_index < floyd_args.size());
			auto floyd_arg_reg = floyd_args[out_arg.floyd_arg_index];
			const auto arg_type = peek2(types, resolved_function_type).get_function_args(types)[out_arg.floyd_arg_index];

			arg_regs.push_back(floyd_arg_reg);
			destroy.push_back({ floyd_arg_reg, arg_type });
		}

		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
			QUARK_ASSERT(out_arg.floyd_arg_index >= 0 && out_arg.floyd_arg_index < floyd_args.size());
			auto floyd_arg_reg = floyd_args[out_arg.floyd_arg_index];
			const auto arg_type = peek2(types, resolved_function_type).get_function_args(types)[out_arg.floyd_arg_index];

			destroy.push_back({ floyd_arg_reg, arg_type });

			// We assume that the next arg in the callee_mapping is the dyn-type and store it too.
			const auto packed_reg = generate_cast_to_runtime_value(gen_acc.gen, *floyd_arg_reg, arg_type);
			arg_regs.push_back(packed_reg);
			arg_regs.push_back(generate_itype_constant(gen_acc.gen, arg_type));
		}
		else if(out_arg.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
		}
		else{
			QUARK_ASSERT(false);
		}
	}
	QUARK_ASSERT(arg_regs.size() == callee_mapping.args.size());

	llvm::PointerType* function_type0 = llvm::cast<llvm::PointerType>(callee_type_entry.llvm_type_specific);
	llvm::FunctionType* function_type = llvm::cast<llvm::FunctionType>(function_type0->getElementType());
	auto result0_reg = builder.CreateCall(function_type, &callee_reg, arg_regs, "");

	for(const auto& m: destroy){
		generate_release(gen_acc, *m.first, m.second);
	}

	//	??? Release callee?


	//	If the return type is dynamic, cast the returned runtime_value_t to the correct type.
	//	It must be retained already.
	llvm::Value* result_reg = result0_reg;
	if(peek2(types, peek2(types, callee_function_type).get_function_return(types)).is_any()){
		result_reg = generate_cast_from_runtime_value(gen_acc.gen, *result0_reg, peek2(types, resolved_function_type).get_function_return(types));
	}
	else{
	}

	QUARK_ASSERT(gen_acc.check_invariant());
	return result_reg;
}


}	//	floyd
