//
//  utils.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 11/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

#include "quark.h"
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


// BETTER WAY: https://yongweiwu.wordpress.com/2014/12/07/study-notes-functional-programming-with-cplusplus/

template <typename Collection,typename unop>
void for_each(Collection col, unop op){
	std::for_each(col.begin(),col.end(),op);
}

template <typename Collection,typename unop>
Collection mapf(Collection col, unop op) {
	Collection result;
	std::transform(col.begin(),col.end(),result.begin(),op);
	return result;
}

template <typename Collection,typename Predicate>
Collection filterNot(Collection col,Predicate predicate ) {
	auto returnIterator = std::remove_if(col.begin(),col.end(),predicate);
	col.erase(returnIterator,std::end(col));
	return col;
}

template <typename Collection,typename Predicate>
Collection filter(Collection col,Predicate predicate) {
	//capture the predicate in order to be used inside function
	auto fnCol = filterNot(col,[predicate](typename Collection::value_type i) { return !predicate(i);});
	return fnCol;
}


//std::string float_to_string_no_trailing_zeros(float v);


#endif /* utils_hpp */
