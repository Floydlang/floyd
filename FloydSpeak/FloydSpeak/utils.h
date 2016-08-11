//
//  utils.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 11/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

#include <memory>

	template <typename T> bool compare_shared_values(const T& ptr_a, const T& ptr_b){
		if(ptr_a && ptr_b){
			return *ptr_a == *ptr_b;
		}
		else if(!ptr_a && !ptr_b){
			return true;
		}
		else{
			return false;
		}
	}

#endif /* utils_hpp */
