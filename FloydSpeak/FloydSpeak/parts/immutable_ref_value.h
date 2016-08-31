//
//  immutable_ref_value.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 31/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef immutable_ref_value_hpp
#define immutable_ref_value_hpp

#include <memory>

#include "quark.h"

/*
	This is like a smart pointer, but it is immutable and has value sematics and cannot be null. It has operator==().
	### Make it store the value in-place. At least as an option.

	operator==() checks if the two objects have the same values. It does NOT check if it's the same object.


	Use make_immutable_ref() to make instances:

		struct my_secret_stuff {
			my_secret_stuff(std::string title, int height){
			}
		};

		const auto a = make_immutable_ref<my_secret_stuff>("My title", 13);


	About T

	- This struct / class can have all its member data public.
	- It will never be copied
	- It will be deleted when the last immutable_ref_value_t is destructed.

	- T MUST have operator==() that compares its members' values. Creating two different T:s from scratch using the same input shall make operator==() return true.
		bool T::operator==(const T& rhs);

	### Require operator<() for sorting?
		bool T::operator==(const T& rhs);
*/

template <typename T> struct immutable_ref_value_t {
	public: immutable_ref_value_t(const T* will_own){
		QUARK_ASSERT(will_own != nullptr);

		_ptr.reset(will_own);

		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
		QUARK_ASSERT(_ptr);
		return true;
	};

	public: bool operator==(const immutable_ref_value_t& rhs) const{
		QUARK_ASSERT(check_invariant());

		//	Shortcut -- if we refer to the same T instance we know they are equal without checking all member values.
		if(_ptr == rhs._ptr){
			return true;
		}
		return *_ptr == *rhs._ptr;
	};

	public: bool operator!=(const immutable_ref_value_t& rhs) const{
		QUARK_ASSERT(check_invariant());

		return !(*this == rhs);
	};

	public:  const T& operator*() const{
		return *_ptr;
	}

	public:  const T* operator->() const{
		return _ptr.get();
	}


	/////////////////////////////////////		STATE
		private: std::shared_ptr<const T> _ptr;
};


/////////////////////////////////////		make_immutable_ref


//	c++11 Variadic templates
template <typename T, class... Args> immutable_ref_value_t<T> make_immutable_ref(Args&&... args){
	immutable_ref_value_t<T> r(new T(args...));
	return r;
}




#endif /* immutable_ref_value_hpp */
