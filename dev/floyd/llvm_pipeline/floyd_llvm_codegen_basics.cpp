//
//  floyd_llvm_codegen_basics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-31.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen_basics.h"


namespace floyd {




llvm::Constant* generate_itype_constant(const llvm_code_generator_t& gen_acc, const typeid_t& type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto itype = lookup_itype(gen_acc.type_lookup, type).get_data();
	auto t = make_runtime_type_type(gen_acc.type_lookup);
 	return llvm::ConstantInt::get(t, itype);
}




llvm::Value* generate_cast_to_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(gen_acc.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& builder = gen_acc.get_builder();
	return generate_cast_to_runtime_value2(builder, gen_acc.type_lookup, value, floyd_type);
}

llvm::Value* generate_cast_from_runtime_value(llvm_code_generator_t& gen_acc, llvm::Value& runtime_value_reg, const typeid_t& type){
	auto& builder = gen_acc.get_builder();
	return generate_cast_from_runtime_value2(builder, gen_acc.type_lookup, runtime_value_reg, type);
}



llvm::Value* generate_get_vec_element_ptr_needs_cast(llvm_function_generator_t& gen_acc, llvm::Value& vec_ptr_reg){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto after_alloc64_ptr_reg = builder.CreateGEP(make_generic_vec_type(gen_acc.gen.type_lookup), &vec_ptr_reg, gep, "");
	return after_alloc64_ptr_reg;
}

llvm::Value* generate_get_struct_base_ptr(llvm_function_generator_t& gen_acc, llvm::Value& struct_ptr_reg, const typeid_t& final_type){
	QUARK_ASSERT(gen_acc.check_invariant());

	auto& builder = gen_acc.get_builder();

	auto type = get_generic_struct_type(gen_acc.gen.type_lookup);
	auto ptr_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, &struct_ptr_reg, type->getPointerTo(), "");

	const auto gep = std::vector<llvm::Value*>{
		builder.getInt32(1)
	};
	auto ptr2_reg = builder.CreateGEP(type, ptr_reg, gep, "");
	auto final_type2 = get_exact_struct_type_noptr(gen_acc.gen.type_lookup, final_type);
	auto ptr3_reg = gen_acc.get_builder().CreateCast(llvm::Instruction::CastOps::BitCast, ptr2_reg, final_type2->getPointerTo(), "");
	return ptr3_reg;
}


}	//	floyd
