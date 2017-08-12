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
#include <vector>

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

	template <typename T> bool compare_shared_value_vectors(const std::vector<T>& vec_a, const std::vector<T>& vec_b){
		if(vec_a.size() != vec_b.size()){
			return false;
		}
		for(size_t i = 0 ; i < vec_a.size() ; i++){
			if(compare_shared_values(vec_a[i], vec_b[i]) == false){
				return false;
			}
		}
		return true;
	}


template <typename T> std::vector<T> operator+(const std::vector<T>& lhs, const std::vector<T>& rhs){
	std::vector<T> temp = lhs;
	temp.insert(temp.end(), rhs.begin(), rhs.end());
	return temp;
}

template <typename T> std::vector<T> operator+(const std::vector<T>& lhs, const T& rhs){
	std::vector<T> temp = lhs;
	temp.push_back(rhs);
	return temp;
}


#endif /* utils_hpp */
