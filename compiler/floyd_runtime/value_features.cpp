//
//  value_features.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "value_features.h"

#include "expression.h"


namespace floyd {



int64_t analyse_samples(const int64_t* samples, int64_t count){
	const auto use_ptr = &samples[1];
	const auto use_count = count - 1;

	auto smallest_acc = use_ptr[0];
	auto largest_acc = use_ptr[0];
	for(int64_t i = 0 ; i < use_count ; i++){
		const auto value = use_ptr[i];
		if(value < smallest_acc){
			smallest_acc = value;
		}
		if(value > largest_acc){
			largest_acc = value;
		}
	}
//	std::cout << "smallest: " << smallest_acc << std::endl;
//	std::cout << "largest: " << largest_acc << std::endl;
	return smallest_acc;
}






std::string gen_to_string(value_backend_t& backend, rt_pod_t arg_value, type_t arg_type){
	QUARK_ASSERT(backend.check_invariant());

	const auto& types = backend.types;

	if(peek2(types, arg_type).is_typeid()){
		const auto value = from_runtime_value2(backend, arg_value, arg_type);
		const auto type2 = value.get_typeid_value();
		const auto type3 = peek2(types, type2);
		const auto a = type_to_compact_string(types, type3);
		return a;
	}
	else{
		const auto value = from_runtime_value2(backend, arg_value, arg_type);
		const auto a = to_compact_string2(backend.types, value);
		return a;
	}
}





}	// floyd
