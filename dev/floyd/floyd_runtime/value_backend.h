//
//  value_backend.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-18.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef value_backend_hpp
#define value_backend_hpp

#include "immer/vector.hpp"
#include "immer/map.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include "ast_value.h"

#include "quark.h"

struct json_t;

namespace floyd {

struct typeid_t;

struct VECTOR_CPPVECTOR_T;
struct VECTOR_HAMT_T;
struct DICT_CPPMAP_T;
struct JSON_T;
struct STRUCT_T;
struct type_interner_t;




enum vector_backend {
	cppvector,
	hamt
};

//	Temporary *global* constant that switches between array-based vector backened and HAMT-based vector.
//	The string always uses array-based vector.
//	There is still only one typeid_t/itype for vector.
//	Future: make this flag a per-vector setting.

#if 0
const vector_backend k_global_vector_type = vector_backend::cppvector;
#else
const vector_backend k_global_vector_type = vector_backend::hamt;
#endif


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

	mutable std::atomic<int32_t> rc;
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
int32_t dec_rc(const heap_alloc_64_t& alloc);
int32_t inc_rc(const heap_alloc_64_t& alloc);

void dispose_alloc(heap_alloc_64_t& alloc);





////////////////////////////////	runtime_type_t

/*
	An integer that specifies a unique type a type interner. Use this to specify types in running program.
	Avoid using floyd::typeid_t
*/

typedef int32_t runtime_type_t;

runtime_type_t make_runtime_type(int32_t itype);




////////////////////////////////	runtime_value_t


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
//	VECTOR_CPPVECTOR_T* string_cppvector_ptr;

	VECTOR_CPPVECTOR_T* vector_cppvector_ptr;
	VECTOR_HAMT_T* vector_hamt_ptr;
	DICT_CPPMAP_T* dict_cppmap_ptr;
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
runtime_value_t make_runtime_string(VECTOR_CPPVECTOR_T* string_cppvector_ptr);
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr);
runtime_value_t make_runtime_vector_cppvector(VECTOR_CPPVECTOR_T* vector_ptr);
runtime_value_t make_runtime_vector_hamt(VECTOR_HAMT_T* vector_hamt_ptr);
runtime_value_t make_runtime_dict_cppmap(DICT_CPPMAP_T* dict_cppmap_ptr);

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




////////////////////////////////		VECTOR_CPPVECTOR_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	data_a: element count
	data_b: allocation count
*/
struct VECTOR_CPPVECTOR_T {
	~VECTOR_CPPVECTOR_T();
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

	inline runtime_value_t load_element(const uint64_t index) const {
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

runtime_value_t alloc_vector_ccpvector2(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
void dispose_vector_cppvector(const runtime_value_t& value);




////////////////////////////////		VECTOR_HAMT_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64



	data_a: embeds immer::vector<runtime_value_t> in data_a, data_b, data_c.
	data_b
	data_c
	data_d: element_count
*/

struct VECTOR_HAMT_T {
	~VECTOR_HAMT_T();
	bool check_invariant() const;

	const immer::vector<runtime_value_t>& get_vecref() const {
		return *reinterpret_cast<const immer::vector<runtime_value_t>*>(&alloc.data_a);
	}
	immer::vector<runtime_value_t>& get_vecref_mut(){
		return *reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc.data_a);
	}

	inline uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.data_d;
	}

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.size();
	}


	inline immer::vector<runtime_value_t>::const_iterator begin() const {
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.begin();
	}
	inline immer::vector<runtime_value_t>::const_iterator end() const {
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.end();
	}

	inline runtime_value_t load_element(const uint64_t index) const {
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		const auto temp = vecref[index];
		return temp;
	}

	//	Mutates the VECTOR_HAMT_T implace -- only OK while constructing it when no other observers exists.
	inline void store_mutate(const uint64_t index, runtime_value_t value){
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(index < get_vecref().size());

		const auto& vecref = get_vecref();
		const auto v2 = vecref.set(index, value);
		get_vecref_mut() = v2;
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

runtime_value_t alloc_vector_hamt2(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
runtime_value_t alloc_vector_hamt2(heap_t& heap, const runtime_value_t elements[], uint64_t element_count);
void dispose_vector_hamt(const runtime_value_t& vec);

runtime_value_t store_immutable(const runtime_value_t& vec, const uint64_t index, runtime_value_t value);
runtime_value_t push_back_immutable(const runtime_value_t& vec0, runtime_value_t value);



////////////////////////////////		DICT_CPPMAP_T


/*
	A std::map<> is stored inplace into alloc.data_a / alloc.data_b / alloc.data-c.
*/

typedef std::map<std::string, runtime_value_t> STDMAP;

struct DICT_CPPMAP_T {
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

runtime_value_t alloc_dict_cppmap2(heap_t& heap);
void dispose_dict_cppmap(runtime_value_t& vec);



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












////////////////////////////////		struct_layout_t


struct struct_layout_t {
	std::vector<size_t> offsets;
	size_t size;
};



////////////////////////////////		value_mgr_t



struct value_mgr_t {
	value_mgr_t(
		const std::vector<std::pair<link_name_t, void*>>& native_func_lookup,
		const std::vector<std::pair<typeid_t, struct_layout_t>>& struct_layouts,
		const std::map<runtime_type_t, typeid_t>& itype_to_typeid
	) :
		heap(),
		itype_to_typeid(itype_to_typeid),
		native_func_lookup(native_func_lookup),
		struct_layouts(struct_layouts)
	{
	}

	bool check_invariant() const {
		QUARK_ASSERT(heap.check_invariant());
		return true;
	}


	////////////////////////////////		STATE

	heap_t heap;
	std::map<runtime_type_t, typeid_t> itype_to_typeid;
	std::vector<std::pair<link_name_t, void*>> native_func_lookup;
	std::vector<std::pair<typeid_t, struct_layout_t>> struct_layouts;
};


runtime_type_t lookup_runtime_type(const value_mgr_t& value_mgr, const typeid_t& type);
typeid_t lookup_type(const value_mgr_t& value_mgr, runtime_type_t itype);


runtime_value_t to_runtime_string2(value_mgr_t& value_mgr, const std::string& s);
std::string from_runtime_string2(const value_mgr_t& value_mgr, runtime_value_t encoded_value);


const std::pair<typeid_t, struct_layout_t>& find_struct_layout(const value_mgr_t& value_mgr, const typeid_t& type);


void retain_value(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type);
void release_deep(value_mgr_t& value_mgr, runtime_value_t value, const typeid_t& type);
void release_dict_deep(value_mgr_t& value_mgr, DICT_CPPMAP_T* dict, const typeid_t& type);
void release_vec_deep(value_mgr_t& value_mgr, runtime_value_t& vec, const typeid_t& type);
void release_struct_deep(value_mgr_t& value_mgr, STRUCT_T* s, const typeid_t& type);


/*
	vector_cppvector_pod
	vector_cppvector_rc

	vector_hamt_pod
	vector_hamt_rc
*/

inline bool is_vector_cppvector(const typeid_t& t){
	return t.is_vector() && k_global_vector_type == vector_backend::cppvector;
}
inline bool is_vector_hamt(const typeid_t& t){
	return t.is_vector() && k_global_vector_type == vector_backend::hamt;
}



}	// floyd

#endif /* value_backend_hpp */
