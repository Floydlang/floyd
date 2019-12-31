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









}	// floyd
