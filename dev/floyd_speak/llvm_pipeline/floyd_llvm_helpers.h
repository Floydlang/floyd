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

struct json_t;

namespace floyd {

struct VEC_T;
struct DICT_T;
struct type_interner_t;







////////////////////////////////		heap_t



struct heap_t;

static const uint64_t ALLOC_64_MAGIC = 0x0a110c64;

//	This header is followed by a number of uint64_t elements in the same heap block.
//	This header represents a sharepoint of many clients and holds an RC to count clients.
//	If you want to change the size of the allocation, allocate 0 following elements and make separate dynamic allocation and stuff its pointer into data1.
struct heap_alloc_64_t {
//	public: virtual ~heap_alloc_64_t(){};
	public: bool check_invariant() const;


	////////////////////////////////		STATE
	uint64_t allocation_word_count;
	uint64_t element_count;
	uint32_t rc;
	uint32_t data0;
	uint64_t data1;
	uint64_t magic;
	heap_t* heap64;
	char debug_info[16];
};

struct heap_rec_t {
	heap_alloc_64_t* alloc_ptr;
	bool in_use;
};

static const uint64_t HEAP_MAGIC = 0xf00d1234;

struct heap_t {
	heap_t() :
		magic(0xf00d1234)
	{
	}
	~heap_t();
	public: bool check_invariant() const;
	public: int count_used() const;
	public: int count_leaks() const;


	////////////////////////////////		STATE
	uint64_t magic;
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





////////////////////////////////	runtime_type_t


typedef int32_t runtime_type_t;

llvm::Type* make_runtime_type_type(llvm::LLVMContext& context);

runtime_type_t make_runtime_type(int32_t itype);


base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type);

typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type);
runtime_type_t lookup_runtime_type(const type_interner_t& interner, const typeid_t& type);




////////////////////////////////	native_value_t



//	Native, runtime value, as used by x86 code when running optimized program. Executing.

//	64 bits
union runtime_value_t {
	uint8_t bool_value;
	int64_t int_value;
	int32_t typeid_itype;
	double double_value;

	//	Strings are encoded as VEC_T:s
//	char* string_ptr;

	VEC_T* vector_ptr;
	DICT_T* dict_ptr;
	json_t* json_ptr;
	void* struct_ptr;
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
runtime_value_t make_runtime_struct(void* struct_ptr);

char* get_vec_chars(runtime_value_t str);
uint64_t get_vec_string_size(runtime_value_t str);



VEC_T* unpack_vec_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type);
DICT_T* unpack_dict_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type);





////////////////////////////////	MISSING FEATURES



void NOT_IMPLEMENTED_YET() __dead2;
void UNSUPPORTED() __dead2;


//	Must LLVMContext be kept while using the execution engine? Keep it!
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


//	This pointer is passed as argument 0 to all compiled floyd functions and all runtime functions.

bool check_callers_fcp(llvm::Function& emit_f);
bool check_emitting_function(llvm::Function& emit_f);
llvm::Value* get_callers_fcp(llvm::Function& emit_f);
llvm::Type* make_frp_type(llvm::LLVMContext& context);



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
WIDE_RETURN_T make_wide_return_structptr(void* s);




////////////////////////////////		VEC_T


/*
	Vectors

	Encoded in LLVM as one 16 byte struct, VEC_T by value.

	- Vector instance is a 16 byte struct.
	- No RC or shared state -- always copied fully.
	- Mutation = copy entire vector every time.

	- The runtime handles all vectors as std::vector<uint64_t>. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64
		calloc(alloc_count, sizeof(uint64_t))
*/
struct VEC_T {
	~VEC_T();
	bool check_invariant() const;

	inline uint64_t get_allocation_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.allocation_word_count;
	}

	inline uint64_t get_element_count() const{
		QUARK_ASSERT(check_invariant());

		return alloc.element_count;
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
		return p[index];
	}


	////////////////////////////////		STATE
	heap_alloc_64_t alloc;
};

/*
enum class VEC_T_MEMBERS {
	element_ptr = 0,
	element_count = 1,
	magic = 2,
	element_bits = 3
};
*/
VEC_T* alloc_vec(heap_t& heap, uint64_t allocation_count, uint64_t element_count);
void vec_addref(VEC_T& vec);
void vec_releaseref(VEC_T* vec);


WIDE_RETURN_T make_wide_return_vec(VEC_T* vec);
VEC_T* wide_return_to_vec(const WIDE_RETURN_T& ret);




////////////////////////////////		DICT_T



/*
	Encoded in LLVM as an 8 byte struct. By value.
*/

struct DICT_BODY_T {
	std::map<std::string, runtime_value_t> map;
};

struct DICT_T {
	DICT_BODY_T* body_ptr;

	bool check_invariant() const{
		QUARK_ASSERT(this->body_ptr != nullptr);
		return true;
	}

	inline uint32_t size() const {
		QUARK_ASSERT(check_invariant());

		return body_ptr->map.size();
	}
};



//	Creates a new DICT_T with element_count. All elements are blank. Caller owns the result.
DICT_T make_dict();
void delete_dict(DICT_T& v);

enum class DICT_T_MEMBERS {
	body_ptr = 0
};


//llvm::Value* generate_dict_alloca(llvm::IRBuilder<>& builder, llvm::Value* dict_reg);

//	LLVM can't cast a struct-value to another struct value - need to store on stack and cast pointer instead.
//llvm::Value* generate__convert_wide_return_to_dict(llvm::IRBuilder<>& builder, llvm::Value* wide_return_reg);

WIDE_RETURN_T make_wide_return_dict(DICT_T* dict);
DICT_T* wide_return_to_dict(const WIDE_RETURN_T& ret);




////////////////////////////////		HELPERS




void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg);
void generate_struct_member_store(llvm::IRBuilder<>& builder, llvm::StructType& struct_type, llvm::Value& struct_ptr_reg, int member_index, llvm::Value& value_reg);

llvm::Type* deref_ptr(llvm::Type* type);


////////////////////////////////		llvm_arg_mapping_t

struct llvm_type_interner_t;

//	One element for each LLVM argument.
struct llvm_arg_mapping_t {
	llvm::Type* llvm_type;

	std::string floyd_name;
	typeid_t floyd_type;
	int floyd_arg_index;	//-1 is none. Several elements can specify the same Floyd arg index, since dynamic value use two.
	enum class map_type { k_floyd_runtime_ptr, k_simple_value, k_dyn_value, k_dyn_type } map_type;
};

struct llvm_function_def_t {
	llvm::Type* return_type;
	std::vector<llvm_arg_mapping_t> args;
	std::vector<llvm::Type*> llvm_args;
};

llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args);

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const floyd::typeid_t& function_type);



llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr);


////////////////////////////////		intern_type()


struct llvm_type_interner_t {
	llvm_type_interner_t(llvm::LLVMContext& context, const type_interner_t& interner);
	bool check_invariant() const;


	////////////////////////////////		STATE
	//	Notice: we match indexes of the lookup vectors between interner.interned and llvm_types.
	type_interner_t interner;
	std::vector<llvm::Type*> llvm_types;

	llvm::StructType* vec_type;
	llvm::StructType* dict_type;
	llvm::StructType* wide_return_type;
};

llvm::Type* intern_type(const llvm_type_interner_t& interner, const typeid_t& type);
llvm::StructType* make_wide_return_type(const llvm_type_interner_t& interner);
llvm::Type* make_vec_type(const llvm_type_interner_t& interner);
llvm::Type* make_dict_type(const llvm_type_interner_t& interner);


llvm::Type* make_function_type(const llvm_type_interner_t& interner, const typeid_t& function_type);
llvm::StructType* make_struct_type(const llvm_type_interner_t& interner, const typeid_t& type);


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
