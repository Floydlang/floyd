//
//  immutable_ref_value.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 31/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef immutable_ref_value_hpp
#define immutable_ref_value_hpp

/*
	Wrap any type and make it immutable, even if original type is mutable.
*/

#include <memory>
#include "quark.h"



/////////////////////////////////////		immutable_ref_value_t

//??? add operator=, copy-constructor and SWAP().
/*
	This makes a new *value object*, based on T.
	Internally a pointer to T is stored, like a smart pointer.

	The new type is is immutable and has value semantics.
	It cannot be null.
	It has an operator==() that checks if the two objects have the same value. It does NOT check if it's the same object.

	It can cheaply be copied around, stored in collections etc.

	Usage:

		struct my_secret_stuff {
			my_secret_stuff(std::string title, int height){
			}
		};

		const auto a = make_immutable_ref<my_secret_stuff>("My title", 13);


	ABOUT T

	- This struct / class can have all its member data public. Clients can read but not modify them.
	- It will never be copied
	- It will be deleted when the last immutable_ref_value_t is destructed.

	- T MUST have operator==() that compares its members' values. Creating two different T:s from scratch using the same input shall make operator==() return true.
		bool T::operator==(const T& rhs);


	FUTURE

	### Require operator<() for sorting?
		bool T::operator==(const T& rhs);

//	### This can be compiled-out of code in release mode!
*/
template <typename T> struct immutable_ref_value_t {
	public: immutable_ref_value_t(){
		_ptr.reset(new T());

		QUARK_ASSERT(check_invariant());
	}

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


/////////////////////////////////////		make_immutable_ref()


//	c++11 Variadic templates
template <typename T, class... Args> immutable_ref_value_t<T> make_immutable_ref(Args&&... args){
	return immutable_ref_value_t<T>(new T(args...));
}



/////////////////////////////////////		immutable_value_t




/*
	This class wraps another struct/class and makes it immutable/const. The new type is a read-only value object.

	It has an operator==() that checks if the two objects have the same value. It does NOT check if it's the same object.

	Use make_immutable_value() to make instances:

		struct my_secret_stuff {
			my_secret_stuff(std::string title, int height){
			}
		};

		const auto a = make_immutable_value<my_secret_stuff>("My title", 13);


	ABOUT T

	- This struct / class can have all its member data public. Clients can read but not modify them.
	- T is embedded inside the immutable_value_t and be copied along with it, needs propery copy constructor.

	- T MUST have operator==() that compares its members' values. Creating two different T:s from scratch using the same input shall make operator==() return true.
		bool T::operator==(const T& rhs);


	FUTURE

	### Require operator<() for sorting?
		bool T::operator==(const T& rhs);
	### Use move operator to avoid copying value.
	### This can be compiled-out of code in release mode!
*/
template <typename T> struct immutable_value_t {
	public: immutable_value_t(const T& value) :
		_value(value)
	{
		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const {
		return true;
	};

	public: bool operator==(const immutable_value_t& rhs) const{
		QUARK_ASSERT(check_invariant());

		return _value == rhs._value;
	};

	public: bool operator!=(const immutable_value_t& rhs) const{
		QUARK_ASSERT(check_invariant());

		return !(*this == rhs);
	};

	public:  const T& operator*() const{
		return _value;
	}

	public:  const T* operator->() const{
		return &_value;
	}


	/////////////////////////////////////		STATE
		private: T _value;
};


/////////////////////////////////////		make_immutable_value()


template <typename T, class... Args> immutable_value_t<T> make_immutable_value(Args&&... args){
	return immutable_value_t<T>(T(args...));
}


#endif /* immutable_ref_value_hpp */
