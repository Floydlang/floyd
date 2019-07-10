//
//  floyd_llvm_helpers.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_helpers_hpp
#define floyd_llvm_helpers_hpp

#include "ast_typeid.h"
#include "ast.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

#include <atomic>
#include "immer/vector.hpp"
#include "immer/map.hpp"

struct json_t;

namespace floyd {

struct VEC_T;
struct VEC_HAMT_T;
struct DICT_T;
struct JSON_T;
struct STRUCT_T;
struct type_interner_t;
struct llvm_type_interner_t;



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
//	If you want to change the size of the allocation, allocate 0 following elements and make separate dynamic allocation and stuff its pointer into data1.
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




////////////////////////////////	runtime_type_t

/*
	An integer that specifies a unique type a type interner. Use this to specify types in running program.
	Avoid using floyd::typeid_t
*/

typedef int64_t runtime_type_t;

llvm::Type* make_runtime_type_type(llvm::LLVMContext& context);

runtime_type_t make_runtime_type(int32_t itype);


base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type);

typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type);
runtime_type_t lookup_runtime_type(const type_interner_t& interner, const typeid_t& type);




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

	//	Strings are encoded as VEC_T:s
//	char* string_ptr;

	VEC_T* vector_ptr;
	VEC_HAMT_T* vector_hamt_ptr;
	DICT_T* dict_ptr;
	JSON_T* json_ptr;
	STRUCT_T* struct_ptr;
	void* function_ptr;

	bool check_invariant() const {
		return true;
	}
};


llvm::Type* make_runtime_value_type(llvm::LLVMContext& context);

runtime_value_t make_blank_runtime_value();

runtime_value_t make_runtime_bool(bool value);
runtime_value_t make_runtime_int(int64_t value);
runtime_value_t make_runtime_typeid(runtime_type_t type);
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr);


VEC_T* unpack_vec_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type);
DICT_T* unpack_dict_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type);










//	Must LLVMContext be kept while using the execution engine? Yes!
struct llvm_instance_t {
	bool check_invariant() const {
		return true;
	}

	llvm::LLVMContext context;
};



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


bool check_invariant__function(const llvm::Function* f);
bool check_invariant__module(llvm::Module* module);
bool check_invariant__builder(llvm::IRBuilder<>* builder);

std::string print_module(llvm::Module& module);
std::string print_type(llvm::Type* type);
std::string print_function(const llvm::Function* f);
std::string print_value(llvm::Value* value);



////////////////////////////////	floyd_runtime_ptr


/*
	The Floyd runtime doesn't use global variables at all. Not even for memory heaps etc.
	Instead it passes around an invisible argumen to all functions, called Floyd Runtime Ptr (FRP).
*/

struct floyd_runtime_t {
};

//	This pointer is passed as argument 0 to all compiled floyd functions and all runtime functions.

bool check_callers_fcp(const llvm_type_interner_t& interner, llvm::Function& emit_f);
bool check_emitting_function(const llvm_type_interner_t& interner, llvm::Function& emit_f);
llvm::Value* get_callers_fcp(const llvm_type_interner_t& interner, llvm::Function& emit_f);
llvm::Type* make_frp_type(const llvm_type_interner_t& interner);



////////////////////////////////		WIDE_RETURN_T


//	Used to return structs and bigger chunks of data from LLVM functions.
//	Can only be two members in LLVM struct, each a word wide.


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
inline WIDE_RETURN_T make_wide_return_1x64(runtime_value_t a){
	return make_wide_return_2x64(a, make_blank_runtime_value());
}
WIDE_RETURN_T make_wide_return_structptr(STRUCT_T* s);




////////////////////////////////		VEC_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	Store element count in data_a.
*/
struct VEC_T {
	~VEC_T();
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

VEC_T* alloc_vec(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
void dispose_vec(VEC_T& vec);

WIDE_RETURN_T make_wide_return_vec(VEC_T* vec);




////////////////////////////////		VEC_HAMT_T


/*
	A fixed-size immutable vector with RC.

	- Mutation = copy entire vector every time.
	- Elements are always runtime_value_t. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64

	Embeds immer::vector<runtime_value_t> in data_a, data_b, data_c, data_d.
*/
typedef immer::vector<runtime_value_t> VEC_HAMT_IMPL_T;

struct VEC_HAMT_T {
	~VEC_HAMT_T();
	bool check_invariant() const;

	const VEC_HAMT_IMPL_T& get_vecref() const {
		return *reinterpret_cast<const VEC_HAMT_IMPL_T*>(&alloc.data_a);
	}
	VEC_HAMT_IMPL_T& get_vecref_mut(){
		return *reinterpret_cast<VEC_HAMT_IMPL_T*>(&alloc.data_a);
	}

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		return vecref.size();
	}

	inline VEC_HAMT_IMPL_T::const_iterator begin() const {
		QUARK_ASSERT(check_invariant());

		const auto vecref = get_vecref();
		return vecref.begin();
	}
	inline VEC_HAMT_IMPL_T::const_iterator end() const {
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

VEC_HAMT_T* alloc_vec_hamt(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
void dispose_vec_hamt(VEC_HAMT_T& vec);

WIDE_RETURN_T make_wide_return_vec_hamt(VEC_HAMT_T* vec);




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

WIDE_RETURN_T make_wide_return_dict(DICT_T* dict);
DICT_T* wide_return_to_dict(const WIDE_RETURN_T& ret);





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

WIDE_RETURN_T make_wide_return_struct(STRUCT_T* v);
STRUCT_T* wide_return_to_struct(const WIDE_RETURN_T& ret);



//llvm::StructType* make_exact_struct_type(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const typeid_t& type);




////////////////////////////////		HELPERS




void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg);

llvm::Type* deref_ptr(llvm::Type* type);



////////////////////////////////		llvm_arg_mapping_t


//	One element for each LLVM argument.
struct llvm_arg_mapping_t {
	llvm::Type* llvm_type;

	std::string floyd_name;
	typeid_t floyd_type;
	int floyd_arg_index;	//-1 is none. Several elements can specify the same Floyd arg index, since dynamic value use two.
	enum class map_type { k_floyd_runtime_ptr, k_known_value_type, k_dyn_value, k_dyn_type } map_type;
};

struct llvm_function_def_t {
	llvm::Type* return_type;
	std::vector<llvm_arg_mapping_t> args;
	std::vector<llvm::Type*> llvm_args;
};

llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args);

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const floyd::typeid_t& function_type);



llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr);


////////////////////////////////		get_exact_llvm_type()

/*
	Type interner: keeps a list of all types used statically in the program, their itype, their LLVM type and their Floyd type.

	Generic-type: vector (and string), dictionary, json and struct are passed around as 4 different types,
	not one for each vector type, struct type etc. These generic types are 64 bytes big, the same size as heap_alloc_64_t.
*/

struct llvm_type_interner_t {
	llvm_type_interner_t(llvm::LLVMContext& context, const type_interner_t& interner);
	bool check_invariant() const;


	////////////////////////////////		STATE
	//	Notice: we match indexes of the lookup vectors between interner.interned and exact_llvm_types.
	type_interner_t interner;
	std::vector<llvm::Type*> exact_llvm_types;

	llvm::StructType* generic_vec_type;
	llvm::StructType* generic_dict_type;
	llvm::StructType* json_type;
	llvm::StructType* generic_struct_type;
	llvm::StructType* wide_return_type;
	llvm::Type* runtime_ptr_type;
};

//	Returns the LLVM type used to pass this type of value around. It uses generic types for vector, dict and struct.
llvm::Type* get_exact_llvm_type(const llvm_type_interner_t& interner, const typeid_t& type);

//	Returns the exact LLVM struct layout that maps to the struct members, without any alloc-64 header. Not a pointer.
llvm::StructType* get_exact_struct_type(const llvm_type_interner_t& interner, const typeid_t& type);

llvm::StructType* make_wide_return_type(const llvm_type_interner_t& interner);

//	Returns generic types.
llvm::Type* make_generic_vec_type(const llvm_type_interner_t& interner);
llvm::Type* make_generic_dict_type(const llvm_type_interner_t& interner);
llvm::Type* make_json_type(const llvm_type_interner_t& interner);
llvm::Type* get_generic_struct_type(const llvm_type_interner_t& interner);


llvm::Type* make_function_type(const llvm_type_interner_t& interner, const typeid_t& function_type);


bool is_rc_value(const typeid_t& type);


/*
	floyd			C++			runtime_value_t			native func arg/return
	--------------------------------------------------------------------------------------------------------------------
	bool			bool		uint8					uint1
	int							int64_t					int64
	string			string		char*					char*
	vector[T]		vector<T>	VEC_T*					VEC_T*
	json_t			json_t		json_t*					int16*
*/



runtime_value_t load_via_ptr2(const void* value_ptr, const typeid_t& type);
void store_via_ptr2(void* value_ptr, const typeid_t& type, const runtime_value_t& value);

//	Converts the LLVM value into a uint64_t for storing vector, pass as DYN value.
llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, llvm::Value& value, const typeid_t& floyd_type);

//	Returns the specific LLVM type for the value, like VEC_T* etc.
llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_interner_t& interner, llvm::Value& runtime_value_reg, const typeid_t& type);


}	//	floyd

#endif /* floyd_llvm_helpers_hpp */
