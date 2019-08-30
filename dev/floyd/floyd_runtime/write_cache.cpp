//
//  write_cache.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "write_cache.h"

//#include <experimental/simd>

#include "immer/vector.hpp"

#include "quark.h"


// ??? Faster to not use override_bits at all - just copy the entire page to/from cache when syncing.
//	Read one element from hamt brings in entire CL = use SIMD to load. memcpy() 
//	The write cache will map to multiples of 16 elements (page). Can extend past end of vector but not leave gap between end-of-vector and start of cache.
//	Has override_bits per element. 0 = access vec, 1 = access write_cache.buffer
//	When a write misses the cached page, the page will be committed to the vector and a new write_cache will be created for the new page.
//	- The same cache class can also work as a read cache.




/*

```
		BYTE CACHELINE
		|<.0.... 8....... 16...... 24...... 32...... 40...... 48...... 56....>|
		AAAAAAAA BBBBBBBB AAAAAAAA BBBBBBBB AAAAAAAA BBBBBBBB AAAAAAAA BBBBBBBB
```

*/

struct write_cache_t {
	bool check_invariant() const {
		return true;
	}

	//	New cache is always mapped to end of vector, for fast push_backs.
	write_cache_t(const immer::vector<uint64_t>& vec) :
		vec(vec),
		page_index(vec.size() / 16),
		override_bits(0b00000000'00000000),
		vec_size(vec.size())
	{
		for(int i = 0 ; i < 16 ; i++) { buffer[i] = 0xdeadbeef; }

		QUARK_ASSERT(check_invariant());
	}

	uint64_t size() const {
		QUARK_ASSERT(check_invariant());

		return vec_size;
	}

	const immer::vector<uint64_t> commit() const {
		QUARK_ASSERT(check_invariant());

		if(override_bits == 0x00){
			return vec;
		}
		else {
			//	??? This can just replace an entire node in the hamt = fast! Could even swap ptrs between hamt and cache.
			auto vec_acc = vec;
			for(int x = 0 ; x < 16 ; x++){
				bool bit = override_bits & static_cast<uint16_t>(1 << x);
				if(bit){
					const auto i = page_index * 16 + x;
					if(i == vec_acc.size()){
						vec_acc = vec_acc.push_back(buffer[x]);
					}
					else{
						QUARK_ASSERT(i < vec_acc.size());
						vec_acc = vec_acc.set(i, buffer[x]);
					}
				}
			}

			return vec_acc;
		}
	}

	uint64_t load_element(uint64_t index) const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(index < vec_size);

		const uint64_t page_index2 = index / 16;
		if(page_index2 == page_index){
			const uint64_t x = index - page_index2 * 16;
			bool bit = override_bits & static_cast<uint16_t>(1 << x);
			if(bit){
				return buffer[x];
			}
			else{
				return vec.operator[](index);
			}
		}
		else{
			return vec.operator[](index);
		}
	}

	write_cache_t push_back(uint64_t value) const{
		QUARK_ASSERT(check_invariant());

		uint64_t index = vec_size;
		const auto page_index2 = index / 16;
		if(page_index2 != page_index){
			const auto vec2 = commit();
			const auto a = write_cache_t(vec2);
			return a.push_back(value);
		}
		else{
			write_cache_t result = *this;
			const uint64_t x = index - page_index2 * 16;
			result.buffer[x] = value;
			result.override_bits = result.override_bits | static_cast<uint16_t>(1 << x);
			result.vec_size++;

			QUARK_ASSERT(result.check_invariant());
			return result;
		}
	}

	////////////////////////////////	STATE

	//---- CL 0 + 1
	uint64_t buffer[16];

	//---- CL 2
	immer::vector<uint64_t> vec;	//	32 bytes
	uint64_t page_index;
	uint16_t override_bits;
	uint64_t vec_size;
};



//#define QUARK_UNIT_TEST QUARK_UNIT_TEST_VIP


QUARK_UNIT_TEST("", "", "", ""){
	const immer::vector<uint64_t> vec;

	const write_cache_t cache(vec);
	QUARK_UT_VERIFY(cache.size() == 0);
}

QUARK_UNIT_TEST("", "", "", ""){
	const immer::vector<uint64_t> vec;

	const write_cache_t cache(vec);
	const auto b = cache.push_back(1234);
	QUARK_UT_VERIFY(vec.size() == 0);
	QUARK_UT_VERIFY(cache.size() == 0);
	QUARK_UT_VERIFY(b.size() == 1);
	QUARK_UT_VERIFY(b.load_element(0) == 1234);
}

QUARK_UNIT_TEST("", "", "", ""){
	const immer::vector<uint64_t> vec;

	const write_cache_t cache(vec);
	auto acc = cache;
	for(int i = 0 ; i < 16 ; i++){
		acc = acc.push_back(i);
	}
	QUARK_UT_VERIFY(vec.size() == 0);
	QUARK_UT_VERIFY(cache.size() == 0);
	QUARK_UT_VERIFY(acc.size() == 16);
	QUARK_UT_VERIFY(acc.load_element(0) == 0);
	QUARK_UT_VERIFY(acc.load_element(1) == 1);
	QUARK_UT_VERIFY(acc.load_element(2) == 2);
	QUARK_UT_VERIFY(acc.load_element(3) == 3);

	QUARK_UT_VERIFY(acc.load_element(4) == 4);
	QUARK_UT_VERIFY(acc.load_element(5) == 5);
	QUARK_UT_VERIFY(acc.load_element(6) == 6);
	QUARK_UT_VERIFY(acc.load_element(7) == 7);

	QUARK_UT_VERIFY(acc.load_element(8) == 8);
	QUARK_UT_VERIFY(acc.load_element(9) == 9);
	QUARK_UT_VERIFY(acc.load_element(10) == 10);
	QUARK_UT_VERIFY(acc.load_element(11) == 11);

	QUARK_UT_VERIFY(acc.load_element(12) == 12);
	QUARK_UT_VERIFY(acc.load_element(13) == 13);
	QUARK_UT_VERIFY(acc.load_element(14) == 14);
	QUARK_UT_VERIFY(acc.load_element(15) == 15);
}

QUARK_UNIT_TEST("", "", "", ""){
	const immer::vector<uint64_t> vec;

	const write_cache_t cache(vec);
	auto acc = cache;
	for(int i = 0 ; i < 17 ; i++){
		acc = acc.push_back(i);
	}
	QUARK_UT_VERIFY(vec.size() == 0);
	QUARK_UT_VERIFY(cache.size() == 0);
	QUARK_UT_VERIFY(acc.size() == 17);
	QUARK_UT_VERIFY(acc.load_element(0) == 0);
	QUARK_UT_VERIFY(acc.load_element(1) == 1);
	QUARK_UT_VERIFY(acc.load_element(2) == 2);
	QUARK_UT_VERIFY(acc.load_element(3) == 3);

	QUARK_UT_VERIFY(acc.load_element(4) == 4);
	QUARK_UT_VERIFY(acc.load_element(5) == 5);
	QUARK_UT_VERIFY(acc.load_element(6) == 6);
	QUARK_UT_VERIFY(acc.load_element(7) == 7);

	QUARK_UT_VERIFY(acc.load_element(8) == 8);
	QUARK_UT_VERIFY(acc.load_element(9) == 9);
	QUARK_UT_VERIFY(acc.load_element(10) == 10);
	QUARK_UT_VERIFY(acc.load_element(11) == 11);

	QUARK_UT_VERIFY(acc.load_element(12) == 12);
	QUARK_UT_VERIFY(acc.load_element(13) == 13);
	QUARK_UT_VERIFY(acc.load_element(14) == 14);
	QUARK_UT_VERIFY(acc.load_element(15) == 15);

	QUARK_UT_VERIFY(acc.load_element(16) == 16);
}
