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
	- Element count: how many logical elements are stored? If an element is 1 byte big we can fit 8 elements in
		ONE allocation word


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
#include "floyd_ast.h"

#include "quark.h"

struct json_t;


namespace floyd {

////////////////////////////////	CONFIGURATION

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
struct bc_static_frame_t;
struct value_backend_t;


////////////////////////////////	runtime_type_t

/*
	An integer that specifies a unique type a type types. Use this to specify types in running program.
	Avoid using floyd::type_t
	It is 1:1 compatible with type_t. Use type_t except in binary situations.
*/

typedef uint32_t runtime_type_t;

runtime_type_t make_runtime_type(type_t type);


////////////////////////////////		heap_t


struct heap_rec_t {
	heap_alloc_64_t* alloc_ptr;
};

static const uint64_t k_alloc_start_id = 2000000;
static const uint64_t HEAP_MAGIC = 0xf00d1234;



static const uint64_t UNINITIALIZED_RUNTIME_VALUE = 0x4444deadbeef3333;


struct heap_t {
	heap_t(bool record_allocs_flag);
	~heap_t();
	public: bool check_invariant() const;
	public: int count_used() const;


	////////////////////////////////		STATE
	uint64_t magic;
	std::shared_ptr<std::recursive_mutex> alloc_records_mutex;
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
static const uint64_t ALLOC_64_MAGIC_DELETED = 0x11dead11;

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


//	Returns updated RC, no need to atomically read it yourself.
//	If returned RC is 0, there is no way for any other client to bump it up again.
int32_t dec_rc(const heap_alloc_64_t& alloc);
int32_t inc_rc(const heap_alloc_64_t& alloc);



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

	VECTOR_CARRAY_T* vector_carray_ptr;
	VECTOR_HAMT_T* vector_hamt_ptr;

	DICT_CPPMAP_T* dict_cppmap_ptr;
	DICT_HAMT_T* dict_hamt_ptr;

	JSON_T* json_ptr;
	STRUCT_T* struct_ptr;

	//	In the future a function should hold its context too = needs to alloc.
	int64_t function_link_id;

	void* frame_ptr;

	heap_alloc_64_t* gp_ptr;

	bool check_invariant() const {
		return true;
	}
};

runtime_value_t make_uninitialized_magic();

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



////////////////////////////////		VECTOR_CARRAY_T


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

	uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.data[0];
	}

	uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.allocation_word_count;
	}

	const runtime_value_t* begin() const {
		return static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
	}
	const runtime_value_t* end() const {
		return static_cast<const runtime_value_t*>(get_alloc_ptr(alloc)) + get_allocation_count();
	}


	const runtime_value_t* get_element_ptr() const{
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
		return p;
	}
	runtime_value_t* get_element_ptr(){
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<runtime_value_t*>(get_alloc_ptr(alloc));
		return p;
	}

	runtime_value_t load_element(const uint64_t index) const {
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const runtime_value_t*>(get_alloc_ptr(alloc));
		const auto temp = p[index];
		return temp;
	}

	void store(const uint64_t index, runtime_value_t value){
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

	uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.allocation_word_count;
	}

	uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.size();
	}


	immer::vector<runtime_value_t>::const_iterator begin() const {
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.begin();
	}
	immer::vector<runtime_value_t>::const_iterator end() const {
		QUARK_ASSERT(check_invariant());

		const auto& vecref = get_vecref();
		return vecref.end();
	}

	runtime_value_t load_element(const uint64_t index) const {
		QUARK_ASSERT(check_invariant());
		const auto& vecref = get_vecref();
		QUARK_ASSERT(index < vecref.size())

		const auto temp = vecref[index];
		return temp;
	}

	//	Mutates the VECTOR_HAMT_T implace -- only OK while constructing it when no other observers exists.
	void store_mutate(const uint64_t index, runtime_value_t value){
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

runtime_value_t store_immutable_hamt(const runtime_value_t& vec, const uint64_t index, runtime_value_t value);
runtime_value_t push_back_immutable_hamt(const runtime_value_t& vec0, runtime_value_t value);



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

runtime_value_t alloc_json(heap_t& heap, const json_t& init);


////////////////////////////////		STRUCT_T


struct STRUCT_T {
	~STRUCT_T();
	bool check_invariant() const;

	const uint8_t* get_data_ptr() const{
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<const uint8_t*>(get_alloc_ptr(alloc));
		return p;
	}
	uint8_t* get_data_ptr(){
		QUARK_ASSERT(check_invariant());

		auto p = static_cast<uint8_t*>(get_alloc_ptr(alloc));
		return p;
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

runtime_value_t alloc_struct(heap_t& heap, std::size_t size, type_t value_type);
runtime_value_t alloc_struct_copy(heap_t& heap, const uint64_t data[], std::size_t size, type_t value_type);


////////////////////////////////		HELPERS


runtime_value_t load_via_ptr2(const types_t& types, const void* value_ptr, const type_t& type);
void store_via_ptr2(const types_t& types, void* value_ptr, const type_t& type, const runtime_value_t& value);


//////////////////////////////////////		rt_value_t

/*
	Efficent representation of any value supported by the interpreter.
	It's immutable and uses value-semantics.
	Holds either an inplace value or an external value. Handles reference counting automatically when required.
*/

struct rt_value_t {
#if DEBUG
	public: bool check_invariant() const;
#endif
	public: rt_value_t();
	public: ~rt_value_t();
	public: rt_value_t(const rt_value_t& other);
	public: rt_value_t& operator=(const rt_value_t& other);
	public: void swap(rt_value_t& other);


	//	Bumps RC if needed.
	enum class rc_mode { adopt, bump };
	public: explicit rt_value_t(value_backend_t& backend, const type_t& type, const runtime_value_t& internals, rc_mode mode);

	//	Only works for simple values.
	public: explicit rt_value_t(const type_t& type, const runtime_value_t& internals);



	//////////////////////////////////////		internal-undefined type
	public: static rt_value_t make_undefined();


	//////////////////////////////////////		internal-dynamic type
	public: static rt_value_t make_any();


	//////////////////////////////////////		void
	public: static rt_value_t make_void();


	//////////////////////////////////////		bool
	public: static rt_value_t make_bool(bool v);
	public: bool get_bool_value() const;


	//////////////////////////////////////		int
	public: static rt_value_t make_int(int64_t v);
	public: int64_t get_int_value() const;


	//////////////////////////////////////		double
	public: static rt_value_t make_double(double v);
	public: double get_double_value() const;


	//////////////////////////////////////		string
	public: static rt_value_t make_string(value_backend_t& backend, const std::string& v);
	public: std::string get_string_value(const value_backend_t& backend) const;


	//////////////////////////////////////		json
	public: static rt_value_t make_json(value_backend_t& backend, const json_t& v);
	public: json_t get_json() const;


	//////////////////////////////////////		typeid
	public: static rt_value_t make_typeid_value(const type_t& type_id);
	public: type_t get_typeid_value() const;


	//////////////////////////////////////		struct
	public: static rt_value_t make_struct_value(
		value_backend_t& backend,
		const type_t& struct_type,
		const std::vector<rt_value_t>& values
	);
	public: const std::vector<rt_value_t> get_struct_value(value_backend_t& backend) const;


	//////////////////////////////////////		function
	public: static rt_value_t make_function_value(
		value_backend_t& backend,
		const type_t& function_type,
		const module_symbol_t& function_id
	);
	public: int64_t get_function_value() const;


	//////////////////////////////////////		bc_static_frame_t

	public: explicit rt_value_t(const bc_static_frame_t* frame_ptr);


	//////////////////////////////////////		INTERNALS

	private: explicit rt_value_t(bool value);
	private: explicit rt_value_t(int64_t value);
	private: explicit rt_value_t(double value);
	private: explicit rt_value_t(value_backend_t& backend, const std::string& value);
	private: explicit rt_value_t(value_backend_t& backend, const std::shared_ptr<json_t>& value);
	private: explicit rt_value_t(const type_t& type_id);
	private: explicit rt_value_t(
		value_backend_t& backend,
		const type_t& struct_type,
		const std::vector<rt_value_t>& values,
		bool struct_tag
	);
	private: explicit rt_value_t(value_backend_t& backend, const type_t& function_type, const module_symbol_t& function_id);


	//////////////////////////////////////		STATE

	public: value_backend_t* _backend;
	public: type_t _type;
	public: runtime_value_t _pod;
};


////////////////////////////////////////////			FREE


const immer::vector<rt_value_t> get_vector_elements(value_backend_t& backend, const rt_value_t& value);

//??? All functions like this should take type of *collection*, not its element / value.
rt_value_t make_vector_value(value_backend_t& backend, const type_t& element_type, const immer::vector<rt_value_t>& elements);

const immer::map<std::string, rt_value_t> get_dict_values(value_backend_t& backend, const rt_value_t& value);
rt_value_t make_dict_value(value_backend_t& backend, const type_t& value_type, const immer::map<std::string, rt_value_t>& entries);


json_t bcvalue_to_json(value_backend_t& backend, const rt_value_t& v);
json_t bcvalue_and_type_to_json(value_backend_t& backend, const rt_value_t& v);

int compare_value_true_deep(value_backend_t& backend, const rt_value_t& left, const rt_value_t& right, const type_t& type);

std::vector<rt_value_t> from_runtime_struct(value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);
runtime_value_t to_runtime_struct(value_backend_t& backend, const type_t& struct_type, const std::vector<rt_value_t>& values);


////////////////////////////////		struct_layout_t


struct member_info_t {
	size_t offset;
	type_t type;
};

struct struct_layout_t {
	std::vector<member_info_t> members;
	size_t size;
};


bool is_struct_pod(const types_t& types, const struct_type_desc_t& struct_def);


////////////////////////////////		func_entry_t


//	Every function has a func_entry_t. It may not yet be linked to a function.
struct func_link_t {
	enum class emachine { k_native, k_bytecode, k_native2 };

	func_link_t(
		const std::string& module,
		const module_symbol_t& module_symbol,
		const type_t& function_type_optional,

		func_link_t::emachine machine,

		void* f
	) :
		module(module),
		module_symbol(module_symbol),
		function_type_optional(function_type_optional),
		machine(machine),
		f(f)
	{
//		QUARK_ASSERT(module_symbol.s.empty() == false);
		QUARK_ASSERT(function_type_optional.is_function() || function_type_optional.is_undefined());
//		QUARK_ASSERT(f != nullptr);

		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const {
//		QUARK_ASSERT(module_symbol.s.empty() == false);
		QUARK_ASSERT(function_type_optional.is_function() || function_type_optional.is_undefined());
//		QUARK_ASSERT(dynamic_arg_count >= 0 && dynamic_arg_count < 1000);
//		QUARK_ASSERT(f != nullptr);
		return true;
	}


	////////////////////////////////		STATE

	//	"instrinsics", "corelib", "runtime", "user function" or whatever.
	std::string module;
	module_symbol_t module_symbol;
	type_t function_type_optional;

	emachine machine;

	//	Points to a native function or to a bc_static_frame_t. Nullptr: only a prototype, no implementation.
	void* f;
};

int count_dyn_args(const types_t& types, const type_t& function_type);
json_t func_link_to_json(const types_t& types, const func_link_t& def);
void trace_func_link(const types_t& types, const std::vector<func_link_t>& defs);



////////////////////////////////		value_backend_t



struct value_backend_t {
	value_backend_t(
		const std::vector<func_link_t>& func_link_lookup,
		const std::vector<std::pair<type_t, struct_layout_t>>& struct_layouts,
		const types_t& types,
		const config_t& config
	);

	~value_backend_t();

	bool check_invariant() const;


	////////////////////////////////		STATE

	heap_t heap;

	//	??? Also go from type -> struct_layout
	// 	??? also go from type -> collection element-type without using type_t.

	//	Should be const!
	types_t types;

	//	Should be const!
	std::vector<type_t> child_type;

	//	Index is stored inside function values.
	const std::vector<func_link_t> func_link_lookup;

	const std::vector<std::pair<type_t, struct_layout_t>> struct_layouts;

	//	Temporary *global* constant that switches between array-based vector backened and HAMT-based vector.
	//	The string always uses array-based vector.
	//	Future: make this flag a per-vector setting.
	const config_t config;
};

bool detect_leaks(const value_backend_t& backend);

bool check_invariant(const value_backend_t& backend, runtime_value_t value, const type_t& type);

void trace_value_backend(const value_backend_t& backend);

//	Traces only the non-const data of the backend.
void trace_value_backend_dynamic(const value_backend_t& backend);

const func_link_t* find_function_by_name2(const value_backend_t& backend, const module_symbol_t& s);

//	Returns index into func_link_t array of backend, or -1 of not found.
int64_t find_function_by_name0(const value_backend_t& backend, const module_symbol_t& s);


const func_link_t* lookup_func_link(const value_backend_t& backend, runtime_value_t value);
const func_link_t& lookup_func_link_required(const value_backend_t& backend, runtime_value_t value);
const func_link_t& lookup_func_link_from_id(const value_backend_t& backend, runtime_value_t value);
const func_link_t& lookup_func_link_from_native(const value_backend_t& backend, runtime_value_t value);

//??? kill this function
inline type_t lookup_type_ref(const value_backend_t& backend, runtime_type_t type){
	QUARK_ASSERT(backend.check_invariant());

	return type_t(type);
}

type_t lookup_vector_element_type(const value_backend_t& backend, type_t type);
type_t lookup_dict_value_type(const value_backend_t& backend, type_t type);

//??? Don't return pair, only struct_layout_t.
const std::pair<type_t, struct_layout_t>& find_struct_layout(const value_backend_t& backend, type_t type);

std::pair<runtime_value_t, type_t> load_struct_member(
	const value_backend_t& backend,
	uint8_t* data_ptr,
	const type_t& struct_type,
	int member_index
);



////////////////////////////////		REFERENCE COUNTING

//	Tells if this type uses reference counting for its values.
bool is_rc_value(const types_t& types, const type_t& type);


void retain_value(value_backend_t& backend, runtime_value_t value, type_t type);
void retain_vector_carray(value_backend_t& backend, runtime_value_t vec, type_t type);
void retain_vector_hamt(value_backend_t& backend, runtime_value_t vec, type_t type);
void retain_dict_cppmap(value_backend_t& backend, runtime_value_t dict, type_t type);
void retain_dict_hamt(value_backend_t& backend, runtime_value_t dict, type_t type);
void retain_struct(value_backend_t& backend, runtime_value_t s, type_t type);
void retain_json(value_backend_t& backend, runtime_value_t s);

void release_value(value_backend_t& backend, runtime_value_t value, type_t type);
void release_vector_carray_pod(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_vector_carray_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_vector_hamt_pod(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_vector_hamt_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_vec(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_dict_cppmap(value_backend_t& backend, runtime_value_t dict0, type_t type);
void release_dict_hamt(value_backend_t& backend, runtime_value_t dict0, type_t type);
void release_dict(value_backend_t& backend, runtime_value_t dict0, type_t type);
void release_vector_hamt_elements_internal(value_backend_t& backend, runtime_value_t vec, type_t type);
void release_struct(value_backend_t& backend, runtime_value_t s, type_t type);
void release_json(value_backend_t& backend, runtime_value_t s);



////////////////////////////////		DETECT TYPES

bool is_vector_carray(const types_t& types, const config_t& config, type_t t);
bool is_vector_hamt(const types_t& types, const config_t& config, type_t t);
bool is_dict_cppmap(const types_t& types, const config_t& config, type_t t);
bool is_dict_hamt(const types_t& types, const config_t& config, type_t t);


value_backend_t make_test_value_backend();


//////////////////////////////////////////		runtime_value_t


//	Warning: you need to release the returned value.
runtime_value_t alloc_carray_8bit(value_backend_t& backend, const uint8_t data[], std::size_t count);

//	Warning: you need to release the returned value.
runtime_value_t to_runtime_string2(value_backend_t& backend, const std::string& s);
std::string from_runtime_string2(const value_backend_t& backend, runtime_value_t encoded_value);

//	Warning: you need to release the returned value.
runtime_value_t to_runtime_value2(value_backend_t& backend, const value_t& value);
value_t from_runtime_value2(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);


runtime_value_t to_runtime_vector(value_backend_t& backend, const value_t& value);
value_t from_runtime_vector(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);

//	Warning: you need to release the returned value.
runtime_value_t to_runtime_dict(value_backend_t& backend, const dict_t& exact_type, const value_t& value);

value_t from_runtime_dict(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);


//	Warning: you need to release the returned value.
runtime_value_t to_runtime_struct(value_backend_t& backend, const struct_t& exact_type, const value_t& value);

value_t from_runtime_struct(const value_backend_t& backend, const runtime_value_t encoded_value, const type_t& type);


//////////////////////////////////////////		value_t vs rt_value_t


rt_value_t make_non_rc(const value_t& value);

value_t rt_to_value(const value_backend_t& backend, const rt_value_t& value);
rt_value_t value_to_rt(value_backend_t& backend, const value_t& value);

rt_value_t make_rt_value(value_backend_t& backend, runtime_value_t value, const type_t& type, rt_value_t::rc_mode mode);
runtime_value_t get_rt_value(value_backend_t& backend, const rt_value_t& value);




int compare_values(value_backend_t& backend, int64_t op, const runtime_type_t type, runtime_value_t lhs, runtime_value_t rhs);



runtime_value_t update__string(value_backend_t& backend, runtime_value_t s, runtime_value_t key_value, runtime_value_t value);
runtime_value_t update_element__vector_carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value);
runtime_value_t update_element__vector_hamt_pod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value, runtime_type_t value_type);
runtime_value_t update_element__vector_hamt_nonpod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value, runtime_type_t value_type);
runtime_value_t update_element__vector(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value);


const runtime_value_t update__dict_cppmap(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);
const runtime_value_t update__dict_hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);
runtime_value_t update_dict(value_backend_t& backend, runtime_value_t obj1, runtime_type_t coll_type, runtime_value_t key_value, runtime_value_t value);


runtime_value_t update_struct_member(value_backend_t& backend, runtime_value_t struct_value, const type_t& struct_type, int member_index, runtime_value_t value, const type_t& member_type);


runtime_value_t subset_vector_range(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end);
runtime_value_t subset_vector_range__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);
runtime_value_t subset_vector_range__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);
runtime_value_t subset_vector_range__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, uint64_t start, uint64_t end);


runtime_value_t replace_vector_range(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, int64_t start, int64_t end, runtime_value_t value, runtime_type_t replacement_type);
runtime_value_t replace_vector_range__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t value, runtime_type_t replacement_type);
runtime_value_t replace_vector_range__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t value, runtime_type_t replacement_type);
runtime_value_t replace_vector_range__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, size_t start, size_t end, runtime_value_t value, runtime_type_t replacement_type);


int64_t find_vector_element(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
int64_t find__string(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
int64_t find__carray(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);
int64_t find__hamt(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, const runtime_value_t value, runtime_type_t value_type);


runtime_value_t concatunate_vectors(value_backend_t& backend, const type_t& type, runtime_value_t lhs, runtime_value_t rhs);
runtime_value_t concat_strings(value_backend_t& backend, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_carray(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);
runtime_value_t concat_vector_hamt(value_backend_t& backend, const type_t& type, const runtime_value_t& lhs, const runtime_value_t& rhs);

uint64_t get_vector_size(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec);
runtime_value_t lookup_vector_element(value_backend_t& backend, const type_t& vector_type, runtime_value_t vec, uint64_t index);

runtime_value_t push_back_vector_element(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);
runtime_value_t push_back_vector_element__string(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);
runtime_value_t push_back_vector_element__carray_pod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);
runtime_value_t push_back_vector_element__carray_nonpod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);
runtime_value_t push_back_vector_element__hamt_pod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);
runtime_value_t push_back_vector_element__hamt_nonpod(value_backend_t& backend, runtime_value_t vec, const type_t& vec_type, runtime_value_t element);



uint64_t get_dict_size(value_backend_t& backend, const type_t& dict_type, runtime_value_t dict);
bool exists_dict_value(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t value, runtime_type_t value_type);
runtime_value_t erase_dict_value(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t key_value, runtime_type_t key_type);

runtime_value_t lookup_dict(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key);
runtime_value_t lookup_dict_cppmap(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key);
runtime_value_t lookup_dict_hamt(value_backend_t& backend, runtime_value_t dict, const type_t& dict_type, runtime_value_t key);


runtime_value_t get_keys(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type);
runtime_value_t get_keys__cppmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__cppmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__hamtmap_carray(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);
runtime_value_t get_keys__hamtmap_hamt(value_backend_t& backend, runtime_value_t dict_value, runtime_type_t dict_type);






/////////////////////////////////////////		INLINES






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


//??? optimize by copying alloc64_t directly, then overwrite 1 character.
inline runtime_value_t update__string(value_backend_t& backend, runtime_value_t s, runtime_value_t key_value, runtime_value_t value){
	QUARK_ASSERT(backend.check_invariant());

	const auto str = from_runtime_string2(backend, s);
	const auto i = key_value.int_value;
	const auto new_char = (char)value.int_value;

	const auto len = str.size();

	if(i < 0 || i >= len){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	auto result = str;
	result[i] = new_char;
	const auto result2 = to_runtime_string2(backend, result);
	return result2;
}

inline runtime_value_t update_element__vector_hamt_pod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());
	const auto coll_type2 = type_t(coll_type);
	QUARK_ASSERT(coll_type2.is_vector());
	const auto element_type = coll_type2.get_vector_element_type(backend.types);
	QUARK_ASSERT(is_rc_value(backend.types, element_type) == false);
#endif

	const auto vec = coll_value.vector_hamt_ptr;
	const auto i = index.int_value;

	//??? Move runtime checkes like this to the client. Assert here.
	if(i < 0 || i >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}
	return store_immutable_hamt(coll_value, i, value);
}
inline runtime_value_t update_element__vector_hamt_nonpod(value_backend_t& backend, runtime_value_t coll_value, runtime_type_t coll_type, runtime_value_t index, runtime_value_t value){
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());
	const auto coll_type2 = type_t(coll_type);
	QUARK_ASSERT(coll_type2.is_vector());
	const auto element_type = coll_type2.get_vector_element_type(backend.types);
	QUARK_ASSERT(is_rc_value(backend.types, element_type) == true);
#endif

	const auto vec = coll_value.vector_hamt_ptr;
	const auto i = index.int_value;

	//??? Move runtime checkes like this to the client. Assert here.
	if(i < 0 || i >= vec->get_element_count()){
		quark::throw_runtime_error("Position argument to update() is outside collection span.");
	}

	//??? compile time. Provide as a constaint integer arg
	const auto element_itype = lookup_vector_element_type(backend, type_t(coll_type));

	const auto result = store_immutable_hamt(coll_value, i, value);

	for(int x = 0 ; x < result.vector_hamt_ptr->get_element_count() ; x++){
		auto v = result.vector_hamt_ptr->load_element(x);
		retain_value(backend, v, element_itype);
	}
	return result;
}





}	// floyd

#endif /* value_backend_hpp */
