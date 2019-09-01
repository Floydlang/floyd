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




}	//	floyd
