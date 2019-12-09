//
//  value_backend.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-18.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

/*
	TERMS

	- Alloc: one memory allocation. It contains a 64 byte header, then optional a number of 8 byte allocations
	- Allocation word: an 8 byte section of data.
	- Allocation word count: how many allocation words
	- Element count: how many logical elements are stored? If an element is 1 byte big we can fit 8 elements in ONE allocation word


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

#ifndef value_backend_hpp
#define value_backend_hpp

#include "immer/vector.hpp"
#include "immer/map.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include "ast_value.h"
#include "types.h"
#include "ast.h"

#include "quark.h"

struct json_t;


namespace floyd {





////////////////////////////////	CONFIGURATION


#define HEAP_MUTEX 0
#define ATOMIC_RC 1




////////////////////////////////	FORWARD DECL


struct type_t;
struct heap_alloc_64_t;


struct VECTOR_CARRAY_T;
struct VECTOR_HAMT_T;
struct DICT_CPPMAP_T;
struct DICT_HAMT_T;
struct JSON_T;
struct STRUCT_T;




////////////////////////////////	runtime_type_t

/*
	An integer that specifies a unique type a type types. Use this to specify types in running program.
	Avoid using floyd::type_t
	It is 1:1 compatible with type_t. Use type_t except in binary situations.
*/

typedef int32_t runtime_type_t;

runtime_type_t make_runtime_type(type_t type);




////////////////////////////////		heap_t



struct heap_rec_t {
	heap_alloc_64_t* alloc_ptr;
//	bool in_use;
};



static const uint64_t HEAP_MAGIC = 0xf00d1234;

struct heap_t {
	heap_t(bool record_allocs_flag) :
		magic(0xf00d1234),
		allocation_id_generator(1000000),
		record_allocs_flag(record_allocs_flag)
	{
#if HEAP_MUTEX
		alloc_records_mutex = std::make_shared<std::recursive_mutex>();
#endif
	}
	~heap_t();
	public: bool check_invariant() const;
	public: int count_used() const;


	////////////////////////////////		STATE
	uint64_t magic;
#if HEAP_MUTEX
	std::shared_ptr<std::recursive_mutex> alloc_records_mutex;
#endif
	std::vector<heap_rec_t> alloc_records;

	uint64_t allocation_id_generator;
	bool record_allocs_flag;
};




////////////////////////////////		heap_alloc_64_t



/*
64 bytes = 8 x int64_t

[ atomic RC] [ magic: 0xa110a11c	]
[ data #0							]
[ data #1							]
[ data #2							]
[ data #3							]
[ elements							]
[ heap								]
[ debug_info char x 8				]
[ allocation word 0 (optional)		]
[ allocation word 1 (optional)		]
[ allocation word 2 (optional)		]
[ ...								]
*/


struct heap_t;

static const uint64_t ALLOC_64_MAGIC = 0xa110a11c;

//	This header is followed by a number of uint64_t elements in the same heap block.
//	This header represents a sharepoint of many clients and holds an RC to count clients.
//	If you want to change the size of the allocation, allocate 0 following elements and make separate dynamic
//	allocation and stuff its pointer into data1.
//	Designed to be 64 bytes = 1 cacheline.
struct heap_alloc_64_t {
	static const int k_data_elements = 4;
	static const size_t k_data_bytes = sizeof(uint64_t) * k_data_elements;


	heap_alloc_64_t(heap_t* heap0, uint64_t allocation_word_count, type_t debug_value_type, const char debug_string[]) :
		rc(1),
		magic(ALLOC_64_MAGIC),
		allocation_word_count(allocation_word_count),
		heap(heap0)
#if DEBUG
		,
		debug_value_type(debug_value_type)
#endif
		,alloc_id(heap->allocation_id_generator++)
	{
		QUARK_ASSERT(heap0 != nullptr);
		assert(heap0 != nullptr);
		QUARK_ASSERT(heap0->check_invariant());
		QUARK_ASSERT(debug_string != nullptr && strlen(debug_string) < sizeof(debug_info));

		data[0] = 0x00000000'00000000;
		data[1] = 0x00000000'00000000;
		data[2] = 0x00000000'00000000;
		data[3] = 0x00000000'00000000;

#if DEBUG
		debug_info = std::string(debug_string);
#endif

		QUARK_ASSERT(check_invariant());
	}

	public: bool check_invariant() const;


	////////////////////////////////		STATE

#if ATOMIC_RC
	mutable std::atomic<int32_t> rc;
#else
	mutable int32_t rc;
#endif
	uint32_t magic;

	//	 data_*: 4 x 8 bytes.
	uint64_t data[4];

	uint64_t allocation_word_count;

	heap_t* heap;

#if DEBUG
	std::string debug_info;
	type_t debug_value_type;
//	std::string debug_value_type_str;
#endif
	int64_t alloc_id;

};

std::string get_debug_info(const heap_alloc_64_t& alloc);



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
heap_alloc_64_t* alloc_64(heap_t& heap, uint64_t allocation_word_count, type_t value_type, const char debug_string[]);

//	Returns pointer to the allocated words that sits after the
void* get_alloc_ptr(heap_alloc_64_t& alloc);
const void* get_alloc_ptr(const heap_alloc_64_t& alloc);

void trace_heap(const heap_t& heap);
void detect_leaks(const heap_t& heap);

inline uint64_t size_to_allocation_blocks(std::size_t size);

//	Returns updated RC, no need to atomically read it yourself.
//	If returned RC is 0, there is no way for any other client to bump it up again.
inline int32_t dec_rc(const heap_alloc_64_t& alloc);
inline int32_t inc_rc(const heap_alloc_64_t& alloc);

void dispose_alloc(heap_alloc_64_t& alloc);




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
//	VECTOR_CARRAY_T* string_carray_ptr;

	VECTOR_CARRAY_T* vector_carray_ptr;
	VECTOR_HAMT_T* vector_hamt_ptr;

	DICT_CPPMAP_T* dict_cppmap_ptr;
	DICT_HAMT_T* dict_hamt_ptr;

	JSON_T* json_ptr;
	STRUCT_T* struct_ptr;
	void* function_ptr;
	char* function_id_str;
	void* frame_ptr;

	bool check_invariant() const {
		return true;
	}
};


runtime_value_t make_blank_runtime_value();

runtime_value_t make_runtime_bool(bool value);
runtime_value_t make_runtime_int(int64_t value);
runtime_value_t make_runtime_double(double value);
runtime_value_t make_runtime_typeid(type_t type);
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr);
runtime_value_t make_runtime_vector_carray(VECTOR_CARRAY_T* vector_ptr);
runtime_value_t make_runtime_vector_hamt(VECTOR_HAMT_T* vector_hamt_ptr);
runtime_value_t make_runtime_dict_cppmap(DICT_CPPMAP_T* dict_cppmap_ptr);
runtime_value_t make_runtime_dict_hamt(DICT_HAMT_T* dict_hamt_ptr);

uint64_t get_vec_string_size(runtime_value_t str);

void copy_elements(runtime_value_t dest[], runtime_value_t source[], uint64_t count);

inline function_id_t get_function_id(const runtime_value_t& value){
	QUARK_ASSERT(value.function_id_str != nullptr);
	const auto s = std::string(value.function_id_str);
	return function_id_t { s };
}


////////////////////////////////		WIDE_RETURN_T


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



////////////////////////////////		VECTOR_CARRAY_T

//?? rename to "C ARRAY"
/*
	A fixed-size immutable vector with RC. Deep copy everytime = expensive to mutate.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	data[0]: element count
*/
struct VECTOR_CARRAY_T {
	~VECTOR_CARRAY_T();
	bool check_invariant() const;

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.data[0];
	}

	inline uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.allocation_word_count;
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

runtime_value_t alloc_vector_carray(heap_t& heap, uint64_t allocation_count, uint64_t element_count, type_t value_type);
void dispose_vector_carray(const runtime_value_t& value);



////////////////////////////////		VECTOR_HAMT_T


/*
	A fixed-size immutable vector with RC. Use HAMT.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	data: embeds immer::vector<runtime_value_t>
*/

struct VECTOR_HAMT_T {
	~VECTOR_HAMT_T();
	bool check_invariant() const;

	const immer::vector<runtime_value_t>& get_vecref() const {
		return *reinterpret_cast<const immer::vector<runtime_value_t>*>(&alloc.data[0]);
	}
	immer::vector<runtime_value_t>& get_vecref_mut(){
		return *reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc.data[0]);
	}

	inline uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.allocation_word_count;
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
		QUARK_ASSERT(index < vecref.size())

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

runtime_value_t alloc_vector_hamt(heap_t& heap, uint64_t allocation_count, uint64_t element_count, type_t value_type);
runtime_value_t alloc_vector_hamt(heap_t& heap, const runtime_value_t elements[], uint64_t element_count, type_t value_type);
void dispose_vector_hamt(const runtime_value_t& vec);

runtime_value_t store_immutable(const runtime_value_t& vec, const uint64_t index, runtime_value_t value);
runtime_value_t push_back_immutable(const runtime_value_t& vec0, runtime_value_t value);



////////////////////////////////		DICT_CPPMAP_T


/*
	A std::map<> is stored inplace:
	data: embeds std::map<std::string, runtime_value_t>
*/

typedef std::map<std::string, runtime_value_t> CPPMAP;

struct DICT_CPPMAP_T {
	bool check_invariant() const;
	uint64_t size() const;

	const CPPMAP& get_map() const {
		return *reinterpret_cast<const CPPMAP*>(&alloc.data[0]);
	}
	CPPMAP& get_map_mut(){
		return *reinterpret_cast<CPPMAP*>(&alloc.data[0]);
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

runtime_value_t alloc_dict_cppmap(heap_t& heap, type_t value_type);
void dispose_dict_cppmap(runtime_value_t& vec);



////////////////////////////////		DICT_HAMT_T


/*
	A std::map<> is stored inplace:
	data: embeds std::map<std::string, runtime_value_t>
*/
typedef immer::map<std::string, runtime_value_t> HAMT_MAP;

struct DICT_HAMT_T {
	bool check_invariant() const;
	uint64_t size() const;

	const HAMT_MAP& get_map() const {
		return *reinterpret_cast<const HAMT_MAP*>(&alloc.data[0]);
	}
	HAMT_MAP& get_map_mut(){
		return *reinterpret_cast<HAMT_MAP*>(&alloc.data[0]);
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

runtime_value_t alloc_dict_hamt(heap_t& heap, type_t value_type);
void dispose_dict_hamt(runtime_value_t& vec);



////////////////////////////////		JSON_T


/*
	Store a json_t* in data[0]. It need to be new/deletes via C++.
*/

typedef std::map<std::string, runtime_value_t> CPPMAP;

struct JSON_T {
	bool check_invariant() const;

	const json_t& get_json() const {
		return *reinterpret_cast<const json_t*>(alloc.data[0]);
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

STRUCT_T* alloc_struct(heap_t& heap, std::size_t size, type_t value_type);
STRUCT_T* alloc_struct_copy(heap_t& heap, const uint64_t data[], std::size_t size, type_t value_type);
void dispose_struct(STRUCT_T& v);






////////////////////////////////		HELPERS



runtime_value_t load_via_ptr2(const types_t& types, const void* value_ptr, const type_t& type);
void store_via_ptr2(const types_t& types, void* value_ptr, const type_t& type, const runtime_value_t& value);







struct bc_static_frame_t;
struct value_backend_t;


//////////////////////////////////////		bc_value_t

/*
	Efficent representation of any value supported by the interpreter.
	It's immutable and uses value-semantics.
	Holds either an inplace value or an external value. Handles reference counting automatically when required.
	??? replace with variant<>
*/

struct bc_value_t {
#if DEBUG
	public: bool check_invariant() const;
#endif
	public: bc_value_t();
	public: ~bc_value_t();
	public: bc_value_t(const bc_value_t& other);
	public: bc_value_t& operator=(const bc_value_t& other);
	public: void swap(bc_value_t& other);
	public: explicit bc_value_t(const bc_static_frame_t* frame_ptr);

	enum class mode {
		k_unwritten_ext_value
	};
	public: explicit bc_value_t(const type_t& type, mode mode);


	//////////////////////////////////////		internal-undefined type
	public: static bc_value_t make_undefined();


	//////////////////////////////////////		internal-dynamic type
	public: static bc_value_t make_any();


	//////////////////////////////////////		void
	public: static bc_value_t make_void();


	//////////////////////////////////////		bool
	public: static bc_value_t make_bool(bool v);
	public: bool get_bool_value() const;
	private: explicit bc_value_t(bool value);


	//////////////////////////////////////		int
	public: static bc_value_t make_int(int64_t v);
	public: int64_t get_int_value() const;
	private: explicit bc_value_t(int64_t value);


	//////////////////////////////////////		double
	public: static bc_value_t make_double(double v);
	public: double get_double_value() const;
	private: explicit bc_value_t(double value);


	//////////////////////////////////////		string
	public: static bc_value_t make_string(value_backend_t& backend, const std::string& v);
	public: std::string get_string_value(const value_backend_t& backend) const;
	private: explicit bc_value_t(value_backend_t& backend, const std::string& value);


	//////////////////////////////////////		json
	public: static bc_value_t make_json(value_backend_t& backend, const json_t& v);
	public: json_t get_json() const;
	private: explicit bc_value_t(value_backend_t& backend, const std::shared_ptr<json_t>& value);


	//////////////////////////////////////		typeid
	public: static bc_value_t make_typeid_value(const type_t& type_id);
	public: type_t get_typeid_value() const;
	private: explicit bc_value_t(const type_t& type_id);


	//////////////////////////////////////		struct
	public: static bc_value_t make_struct_value(value_backend_t& backend, const type_t& struct_type, const std::vector<bc_value_t>& values);
	public: const std::vector<bc_value_t> get_struct_value(value_backend_t& backend) const;
	private: explicit bc_value_t(value_backend_t& backend, const type_t& struct_type, const std::vector<bc_value_t>& values, bool struct_tag);


	//////////////////////////////////////		function
	public: static bc_value_t make_function_value(const type_t& function_type, const function_id_t& function_id);
	public: function_id_t get_function_value() const;
	private: explicit bc_value_t(const type_t& function_type, const function_id_t& function_id, bool dummy);


	//	Bumps RC if needed.
	public: explicit bc_value_t(value_backend_t& backend, const type_t& type, const runtime_value_t& internals);

	//	Only works for simple values.
	public: explicit bc_value_t(const type_t& type, const runtime_value_t& internals);



	//////////////////////////////////////		STATE

	//	IDEA: have two types: bc_value_t and bc_pod_t. bc_pod_t has type + value but does no RC.
	public: value_backend_t* _backend;
	public: type_t _type;
	public: runtime_value_t _pod;
};


////////////////////////////////////////////			FREE


const immer::vector<bc_value_t> get_vector(value_backend_t& backend, const bc_value_t& value);
bc_value_t make_vector(value_backend_t& backend, const type_t& element_type, const immer::vector<bc_value_t>& elements);

const immer::map<std::string, bc_value_t> get_dict_values(value_backend_t& backend, const bc_value_t& value);
bc_value_t make_dict(value_backend_t& backend, const type_t& value_type, const immer::map<std::string, bc_value_t>& entries);


json_t bcvalue_to_json(value_backend_t& backend, const bc_value_t& v);
json_t bcvalue_and_type_to_json(value_backend_t& backend, const bc_value_t& v);

int bc_compare_value_true_deep(value_backend_t& backend, const bc_value_t& left, const bc_value_t& right, const type_t& type);




////////////////////////////////		bc_value_t JSON





////////////////////////////////		struct_layout_t


struct member_info_t {
	size_t offset;
	type_t type;
};

struct struct_layout_t {
	std::vector<member_info_t> members;
	size_t size;
};



////////////////////////////////		value_backend_t



struct value_backend_t {
	value_backend_t(
		const std::vector<std::pair<link_name_t, void*>>& native_func_lookup,
		const std::vector<std::pair<type_t, struct_layout_t>>& struct_layouts,
		const types_t& types,
		const config_t& config
	);

	bool check_invariant() const {
		QUARK_ASSERT(heap.check_invariant());
		QUARK_ASSERT(child_type.size() == types.nodes.size());
		QUARK_ASSERT(config.check_invariant());
		return true;
	}


	////////////////////////////////		STATE

	heap_t heap;

	//	??? Also go from type -> struct_layout
	// 	??? also go from type -> collection element-type without using type_t.

	types_t types;
	std::vector<type_t> child_type;


	std::vector<std::pair<link_name_t, void*>> native_func_lookup;
	std::vector<std::pair<type_t, struct_layout_t>> struct_layouts;

	//	Temporary *global* constant that switches between array-based vector backened and HAMT-based vector.
	//	The string always uses array-based vector.
	//	Future: make this flag a per-vector setting.
	config_t config;
};


type_t lookup_type_ref(const value_backend_t& backend, runtime_type_t type);

inline type_t lookup_vector_element_type(const value_backend_t& backend, type_t type);
inline type_t lookup_dict_value_type(const value_backend_t& backend, type_t type);

//??? Don't return pair, only struct_layout_t.
const std::pair<type_t, struct_layout_t>& find_struct_layout(const value_backend_t& backend, type_t type);

runtime_value_t load_struct_member(const value_backend_t& backend, uint8_t* data_ptr, const type_t& struct_type, int member_index);



////////////////////////////////		REFERENCE COUNTING

//	Tells if this type uses reference counting for its values.
bool is_rc_value(const types_t& types, const type_t& type);



void retain_value(value_backend_t& backend, runtime_value_t value, type_t type);

void retain_vector_carray(value_backend_t& backend, runtime_value_t vec, type_t type);
inline void retain_vector_hamt(value_backend_t& backend, runtime_value_t vec, type_t type);

void retain_dict_cppmap(value_backend_t& backend, runtime_value_t dict, type_t type);
void retain_dict_hamt(value_backend_t& backend, runtime_value_t dict, type_t type);

void retain_struct(value_backend_t& backend, runtime_value_t s, type_t type);



void release_value(value_backend_t& backend, runtime_value_t value, type_t type);

void release_vector_carray_pod(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_vector_carray_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type);

inline void release_vector_hamt_pod(value_backend_t& backend, runtime_value_t vec, type_t type);
inline void release_vector_hamt_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type);

void release_vec(value_backend_t& backend, runtime_value_t vec, type_t type);


void release_dict_cppmap(value_backend_t& backend, runtime_value_t dict0, type_t type);
void release_dict_hamt(value_backend_t& backend, runtime_value_t dict0, type_t type);
void release_dict(value_backend_t& backend, runtime_value_t dict0, type_t type);


void release_vector_hamt_elements_internal(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_struct(value_backend_t& backend, runtime_value_t s, type_t type);



////////////////////////////////		DETECT TYPES



inline bool is_vector_carray(const types_t& types, const config_t& config, type_t t){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(config.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	return peek2(types, t).is_vector() && config.vector_backend_mode == vector_backend::carray;
}
inline bool is_vector_hamt(const types_t& types, const config_t& config, type_t t){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(config.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	return peek2(types, t).is_vector() && config.vector_backend_mode == vector_backend::hamt;
}

inline bool is_dict_cppmap(const types_t& types, const config_t& config, type_t t){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(config.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	return peek2(types, t).is_dict() && config.dict_backend_mode == dict_backend::cppmap;
}
inline bool is_dict_hamt(const types_t& types, const config_t& config, type_t t){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(config.check_invariant());
	QUARK_ASSERT(t.check_invariant());

	return peek2(types, t).is_dict() && config.dict_backend_mode == dict_backend::hamt;
}






value_backend_t make_test_value_backend();





//	Inlines
////////////////////////////////////////////////////////////////////////////////////////////////////


inline int32_t dec_rc(const heap_alloc_64_t& alloc){
	assert(alloc.heap != nullptr);
	QUARK_ASSERT(alloc.check_invariant());

#if ATOMIC_RC
	const auto prev_rc = std::atomic_fetch_sub_explicit(&alloc.rc, 1, std::memory_order_relaxed);
#else
	const auto prev_rc = alloc.rc;
	alloc.rc--;
#endif
	const auto rc2 = prev_rc - 1;

	if(rc2 < 0){
		QUARK_ASSERT(false);
		throw std::exception();
	}

	return rc2;
}

inline int32_t inc_rc(const heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

#if ATOMIC_RC
	const auto prev_rc = std::atomic_fetch_add_explicit(&alloc.rc, 1, std::memory_order_relaxed);
#else
	const auto prev_rc = alloc.rc;
	alloc.rc++;
#endif
	const auto rc2 = prev_rc + 1;
	return rc2;
}



inline void retain_vector_hamt(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(backend.types, type));
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));

	inc_rc(vec.vector_hamt_ptr->alloc);

	QUARK_ASSERT(backend.check_invariant());
}

inline void release_vector_hamt_pod(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(backend.types, lookup_vector_element_type(backend, type)) == false);

	if(dec_rc(vec.vector_hamt_ptr->alloc) == 0){
		dispose_vector_hamt(vec);
	}

	QUARK_ASSERT(backend.check_invariant());
}

inline void release_vector_hamt_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(backend.types, lookup_vector_element_type(backend, type)) == true);

	if(dec_rc(vec.vector_hamt_ptr->alloc) == 0){
		release_vector_hamt_elements_internal(backend, vec, type);
		dispose_vector_hamt(vec);
	}

	QUARK_ASSERT(backend.check_invariant());
}



inline uint64_t size_to_allocation_blocks(std::size_t size){
	const auto r = (size >> 3) + ((size & 7) > 0 ? 1 : 0);

	QUARK_ASSERT((r * sizeof(uint64_t) - size) >= 0);
	QUARK_ASSERT((r * sizeof(uint64_t) - size) < sizeof(uint64_t));

	return r;
}




inline type_t lookup_vector_element_type(const value_backend_t& backend, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(backend.types, type);
	QUARK_ASSERT(peek.is_vector());

	QUARK_ASSERT(backend.check_invariant());

	return backend.child_type[peek.get_lookup_index()];
}

inline type_t lookup_dict_value_type(const value_backend_t& backend, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	const auto& peek = peek2(backend.types, type);
	QUARK_ASSERT(peek.is_dict());

	QUARK_ASSERT(backend.check_invariant());
	return backend.child_type[type.get_lookup_index()];
}

}	// floyd

#endif /* value_backend_hpp */
