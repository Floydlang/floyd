//
//  floyd_llvm_values.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-18.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_values_hpp
#define floyd_llvm_values_hpp

#include "floyd_llvm_types.h"

#include <llvm/IR/IRBuilder.h>

#include "immer/vector.hpp"
#include "immer/map.hpp"

#include <atomic>
#include <mutex>

#include "quark.h"

struct json_t;

namespace floyd {

struct VECTOR_ARRAY_T;
struct VEC_HAMT_T;
struct DICT_T;
struct JSON_T;
struct STRUCT_T;
struct type_interner_t;
struct llvm_type_lookup;


////////////////////////////////		heap_t

/*
	Why: we need out own memory heap handling for:
	- Having a unified memory handler / RC / GC / arena.
	- Running separate heaps for separate threads / process
	- Tracking stats and heat maps
	- Finding problems.
	- Reducing the number of mallocs - by putting head and body into the same alloc.
	- Fitting data tighter than malloc()
	- Controlling alignment and letting us address allocations more effectively than 64 bit pointers.
	- Support never reusing the same allocation pointer/ID.

	NOTICE: Right now each alloc is made using malloc(). In the future we can switch to private heap / arena / pooling.
*/


struct heap_t;

static const uint64_t ALLOC_64_MAGIC = 0xa110a11c;

//	This header is followed by a number of uint64_t elements in the same heap block.
//	This header represents a sharepoint of many clients and holds an RC to count clients.
//	If you want to change the size of the allocation, allocate 0 following elements and make separate dynamic
//	allocation and stuff its pointer into data1.
//	Designed to be 64 bytes = 1 cacheline.
struct heap_alloc_64_t {
//	public: virtual ~heap_alloc_64_t(){};
	public: bool check_invariant() const;


	////////////////////////////////		STATE

	std::atomic<int32_t> rc;
	uint32_t magic;

	//	 data_*: 5 x 8 bytes.
	uint64_t data_a;
	uint64_t data_b;
	uint64_t data_c;
	uint64_t data_d;
	uint64_t data_e;

	heap_t* heap64;
	char debug_info[8];
};


struct heap_rec_t {
	heap_alloc_64_t* alloc_ptr;
//	bool in_use;
};



static const uint64_t HEAP_MAGIC = 0xf00d1234;

struct heap_t {
	heap_t() :
		magic(0xf00d1234)
	{
		alloc_records_mutex = std::make_shared<std::recursive_mutex>();
	}
	~heap_t();
	public: bool check_invariant() const;
	public: int count_used() const;


	////////////////////////////////		STATE
	uint64_t magic;
	std::shared_ptr<std::recursive_mutex> alloc_records_mutex;
	std::vector<heap_rec_t> alloc_records;
};

/*
	Allocates a block of data using malloc().

	It consists of two parts: the header and the dynamic elements.

	Header:
		64 byte header with reference counter and possibility to store custom data.
	Dynamic elements: N number of 8 byte elements.

	Pointer is always aligned to 8 or 16 bytes.
	Returned alloc has RC = 1
	The allocation is recorded into the heap_t.
	Only delete the block using release_ref(), never std::free() or c++ delete.

	DEBUG VERSION: We never actually free the heap blocks, we keep them around for debugging.
*/
heap_alloc_64_t* alloc_64(heap_t& heap, uint64_t allocation_word_count);

//	Returns pointer to the allocated words that sits after the
void* get_alloc_ptr(heap_alloc_64_t& alloc);
const void* get_alloc_ptr(const heap_alloc_64_t& alloc);
void add_ref(heap_alloc_64_t& alloc);
void release_ref(heap_alloc_64_t& alloc);

void trace_heap(const heap_t& heap);
void detect_leaks(const heap_t& heap);

uint64_t size_to_allocation_blocks(std::size_t size);

//	Returns updated RC, no need to atomically read it yourself.
//	If returned RC is 0, there is no way for any other client to bump it up again.
int32_t dec_rc(heap_alloc_64_t& alloc);
int32_t inc_rc(heap_alloc_64_t& alloc);

void dispose_alloc(heap_alloc_64_t& alloc);





////////////////////////////////	native_value_t


/*
	Native, runtime value, as used by x86 code when running optimized program. Executing.
	Usually this is a 64 bit value that holds either an integer / double etc OR a pointer to a separate allocation.

	Future: these can have different sizes. A vector can use SSO and embedd head + some body directly here.
*/

//	64 bits
union runtime_value_t {
	uint8_t bool_value;
	int64_t int_value;
	runtime_type_t typeid_itype;
	double double_value;

	//	Strings are encoded as vector.
	VECTOR_ARRAY_T* string_ptr;

	VECTOR_ARRAY_T* vector_array_ptr;
	VEC_HAMT_T* hamt_vector_ptr;
	DICT_T* dict_ptr;
	JSON_T* json_ptr;
	STRUCT_T* struct_ptr;
	void* function_ptr;

	bool check_invariant() const {
		return true;
	}
};



runtime_value_t make_blank_runtime_value();

runtime_value_t make_runtime_bool(bool value);
runtime_value_t make_runtime_int(int64_t value);
runtime_value_t make_runtime_double(double value);
runtime_value_t make_runtime_typeid(runtime_type_t type);
runtime_value_t make_runtime_string(VECTOR_ARRAY_T* string_ptr);
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr);
runtime_value_t make_runtime_vector(VECTOR_ARRAY_T* vector_ptr);
runtime_value_t make_runtime_vector_hamt(VEC_HAMT_T* hamt_vector_ptr);
runtime_value_t make_runtime_dict(DICT_T* dict_ptr);


VECTOR_ARRAY_T* unpack_vec_arg(const llvm_type_lookup& type_lookup, runtime_value_t arg_value, runtime_type_t arg_type);
DICT_T* unpack_dict_arg(const llvm_type_lookup& type_lookup, runtime_value_t arg_value, runtime_type_t arg_type);


uint64_t get_vec_string_size(runtime_value_t str);

void copy_elements(runtime_value_t dest[], runtime_value_t source[], uint64_t count);



/*
First-class values

LLVM has a distinction between first class values and other types of values.
First class values can be returned by instructions, passed to functions,
loaded, stored, PHI'd etc.  Currently the first class value types are:

  1. Integer
  2. Floating point
  3. Pointer
  4. Vector
  5. Opaque (which is assumed to eventually resolve to 1-4)

The non-first-class types are:

  5. Array
  6. Structure/Packed Structure
  7. Function
  8. Void
  9. Label
*/




////////////////////////////////		WIDE_RETURN_T


//	LLVM has a limitation on return values. It can only be two members in LLVM struct, each a word wide.


//	### Also use for arguments, not only return.
struct WIDE_RETURN_T {
	runtime_value_t a;
	runtime_value_t b;
};

enum class WIDE_RETURN_MEMBERS {
	a = 0,
	b = 1
};

WIDE_RETURN_T make_wide_return_2x64(runtime_value_t a, runtime_value_t b);




////////////////////////////////		VECTOR_ARRAY_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	Store element count in data_a.
*/
struct VECTOR_ARRAY_T {
	~VECTOR_ARRAY_T();
	bool check_invariant() const;

	inline uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.data_b;
	}

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.data_a;
	}

	inline const runtime_value_t* begin() const {
		return static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
	}
	inline const runtime_value_t* end() const {
		return static_cast<const runtime_value_t*>(get_alloc_ptr(alloc)) + get_allocation_count();
	}


	inline const runtime_value_t* get_element_ptr() const{
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
		return p;
	}
	inline runtime_value_t* get_element_ptr(){
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<runtime_value_t*>(get_alloc_ptr(alloc));
		return p;
	}

	inline runtime_value_t operator[](const uint64_t index) const {
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
		const auto temp = p[index];
		return temp;
	}

	inline void store(const uint64_t index, runtime_value_t value){
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<runtime_value_t*>(get_alloc_ptr(alloc));
		p[index] = value;
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

VECTOR_ARRAY_T* alloc_vec(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
void dispose_vec(VECTOR_ARRAY_T& vec);





////////////////////////////////		VEC_HAMT_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	Embeds immer::vector<runtime_value_t> in data_a, data_b, data_c, data_d.
*/

struct VEC_HAMT_T {
	~VEC_HAMT_T();
	bool check_invariant() const;

	const immer::vector<runtime_value_t>& get_vecref() const {
		return *reinterpret_cast<const immer::vector<runtime_value_t>*>(&alloc.data_a);
	}
	immer::vector<runtime_value_t>& get_vecref_mut(){
		return *reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc.data_a);
	}

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		return vecref.size();
	}

	inline immer::vector<runtime_value_t>::const_iterator begin() const {
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		return vecref.begin();
	}
	inline immer::vector<runtime_value_t>::const_iterator end() const {
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		return vecref.end();
	}

	inline runtime_value_t operator[](const uint64_t index) const {
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		const auto temp = vecref[index];
		return temp;
	}

	//	Mutates the VEC_HAMT_T implace -- only OK while constructing it when no other observers exists.
	inline void store(const uint64_t index, runtime_value_t value){
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		const auto v2 = vecref.set(index, value);
		get_vecref_mut() = v2;
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

VEC_HAMT_T* alloc_vec_hamt(heap_t& heap, const runtime_value_t elements[], uint64_t element_count);
void dispose_vec_hamt(VEC_HAMT_T& vec);





////////////////////////////////		DICT_T



/*
	A std::map<> is stored inplace into alloc.data_a / alloc.data_b / alloc.data-c.
*/

typedef std::map<std::string, runtime_value_t> STDMAP;

struct DICT_T {
	bool check_invariant() const;
	uint64_t size() const;

	const STDMAP& get_map() const {
		return *reinterpret_cast<const STDMAP*>(&alloc.data_a);
	}
	STDMAP& get_map_mut(){
		return *reinterpret_cast<STDMAP*>(&alloc.data_a);
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

DICT_T* alloc_dict(heap_t& heap);
void dispose_dict(DICT_T& vec);




////////////////////////////////		JSON_T


/*
	Store a json_t* in data_a. It need to be new/deletes via C++.
*/

typedef std::map<std::string, runtime_value_t> STDMAP;

struct JSON_T {
	bool check_invariant() const;

	const json_t& get_json() const {
		return *reinterpret_cast<const json_t*>(alloc.data_a);
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

JSON_T* alloc_json(heap_t& heap, const json_t& init);
void dispose_json(JSON_T& vec);




////////////////////////////////		STRUCT_T


struct STRUCT_T {
	~STRUCT_T();
	bool check_invariant() const;

	inline const uint8_t* get_data_ptr() const{
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const uint8_t*>(get_alloc_ptr(alloc));
		return p;
	}
	inline uint8_t* get_data_ptr(){
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<uint8_t*>(get_alloc_ptr(alloc));
		return p;
	}



	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

STRUCT_T* alloc_struct(heap_t& heap, std::size_t size);
void dispose_struct(STRUCT_T& v);




////////////////////////////////		HELPERS



bool is_rc_value(const typeid_t& type);

runtime_value_t load_via_ptr2(const void* value_ptr, const typeid_t& type);
void store_via_ptr2(void* value_ptr, const typeid_t& type, const runtime_value_t& value);

//	Converts the LLVM value into a uint64_t for storing vector, pass as DYN value.
llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const typeid_t& floyd_type);

//	Returns the specific LLVM type for the value, like VECTOR_ARRAY_T* etc.
llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const typeid_t& type);






}	// floyd

#endif /* floyd_llvm_values_hpp */
