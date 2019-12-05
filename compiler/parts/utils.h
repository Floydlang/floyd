//
//  utils.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 11/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

/*
	Some basic C++ utilities for doing collections, smart pointers etc.
*/

#include <algorithm>
#include "quark.h"
#include <memory>
#include <vector>


//	Compare shared_ptr<>:s BY VALUE, not by-pointer.
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

//	Compares two vectors of shared_ptr<>:s BY VALUE, not by-pointer.
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


////////////////////////////////		Collections


//	Returns new collection.
template <typename T> std::vector<T> concat(const std::vector<T>& lhs, const std::vector<T>& rhs){
	std::vector<T> temp;
	temp.reserve(lhs.size() + rhs.size());
	temp.insert(temp.end(), lhs.begin(), lhs.end());
	temp.insert(temp.end(), rhs.begin(), rhs.end());
	return temp;
}

//	Returns new collection.
template <typename T> std::vector<T> concat(const std::vector<T>& lhs, const T& rhs){
	std::vector<T> temp;
	temp.reserve(lhs.size() + 1);
	temp.insert(temp.end(), lhs.begin(), lhs.end());
	temp.push_back(rhs);
	return temp;
}


////////////////////////////////		for_each_col()


// BETTER WAY: https://yongweiwu.wordpress.com/2014/12/07/study-notes-functional-programming-with-cplusplus/

template <typename COLLECTION, typename UNARY_OPERATION>
void for_each_col(COLLECTION col, UNARY_OPERATION op){
	std::for_each(col.begin(),col.end(),op);
}


////////////////////////////////		mapf()


//	Functional map()-function. Returns new collection.

template <typename DEST_ELEMENT_TYPE, typename COLLECTION, typename UNARY_OPERATION>
std::vector<DEST_ELEMENT_TYPE> mapf(const COLLECTION& col, const UNARY_OPERATION& op) {
	std::vector<DEST_ELEMENT_TYPE> result;
	result.reserve(col.size());

	//	Notice that transform() *writes* to output collection, it doesn't append.
	std::transform(col.begin(), col.end(), std::back_inserter(result), op);
	return result;
}


////////////////////////////////		reduce()

//	Functional reduce()-function. Returns new collection.

template <typename COLLECTION, typename VALUE, typename BINARY_OPERATION>
VALUE reduce(COLLECTION col, const VALUE& init, const BINARY_OPERATION op) {
	auto acc = init;
	for(const auto& e: col){
		acc = op(acc, e);
	}
	return acc;
}

////////////////////////////////		filterf()



template <typename DEST_ELEMENT_TYPE, typename COLLECTION, typename UNARY_OPERATION>
std::vector<DEST_ELEMENT_TYPE> filterf(const COLLECTION& col, const UNARY_OPERATION& op) {
	std::vector<DEST_ELEMENT_TYPE> result;
	result.reserve(col.size());

	for(const auto& e: col){
		if(op(e)){
			result.push_back(e);
		}
	}
	return result;
}


////////////////////////////////		HEX strings


std::string value_to_hex_string(uint64_t value, int hexchars);

std::string ptr_to_hexstring(const void* ptr);


////////////////////////////////		STRINGS

std::string concat_string(const std::vector<std::string>& vec, const std::string& divider);


////////////////////////////////		BINARY


struct byte4_t {
	uint8_t data[4];
};
inline bool operator==(const byte4_t& lhs, const byte4_t& rhs){ return std::memcmp(lhs.data, rhs.data, 4) == 0; }

uint32_t pack_32bit_little(const byte4_t& data);
uint32_t pack_32bit_little(const uint8_t data[]);
byte4_t unpack_32bit_little(uint32_t value);


#endif /* utils_hpp */
