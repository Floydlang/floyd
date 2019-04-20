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
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

namespace floyd {




void NOT_IMPLEMENTED_YET() __dead2;
void UNSUPPORTED() __dead2;


//	Must LLVMContext be kept while using the execution engine? Keep it!
struct llvm_instance_t {
	bool check_invariant() const {
		return true;
	}

	llvm::LLVMContext context;
};



//	LLVM-functions pass GENs as two 64bit arguments.
//	Return: First is the type of the value. Second is tells if
/*
std::pair<llvm::Type*, bool> intern_type_generics(llvm::Module& module, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_function() == false);

	auto& context = module.getContext();

	if(type.is_internal_dynamic()){
		return { llvm::Type::getIntNTy(context, 64), llvm::Type::getIntNTy(context, 64) };
	}
	else{
		return { intern_type(module, type), nullptr };
	}
}
*/


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

/*
	FLOYD ARGS				LLVM ARGS
	----------				--------------------
							int32*		"floyd_runtime_ptr"
	int icecreams			int			icecreams
	----------				--------------------
							int32*		"floyd_runtime_ptr"
	string nick				int8*		nick
	----------				--------------------
							int32*		"floyd_runtime_ptr"
	DYN val					int64_t		val-DYNVAL
							int64_t		val-typei
	----------				--------------------
							int32*		"floyd_runtime_ptr"
	int icecreams			int			icecreams
	DYN val					int64_t		val-dynval
							int64_t		val-type
	string nick				int8*		nick

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




////////////////////////////////		VEC_T



//??? need word for "encoded". How data is stuffed into the LLVM instruction set..
//??? VEC_T for strings too!
/*
	Vectors

	- Vector instance is a 16 byte struct.
	- No RC or shared state -- always copied fully.
	- Mutation = copy entire vector every time.

	- The runtime handles all vectors as std::vector<uint64_t>. You need to pack and address other types of data manually.

	Invariant:
		alloc_count = roundup(element_count * element_bits, 64) / 64
		calloc(alloc_count, sizeof(uint64_t))
*/
struct VEC_T {
	uint64_t* element_ptr;
	uint32_t element_count;
	uint16_t magic;
	uint16_t element_bits;


	inline uint32_t size() const {
		return element_count;
	}
	inline uint64_t operator[](const uint32_t index) const {
		return element_ptr[index];
	}
};

bool check_invariant_vector(const VEC_T& v);

//	Creates a new VEC_T with element_count. All elements are blank. Caller owns the result.
VEC_T make_vec(uint32_t element_count);
void delete_vec(VEC_T& vec);

enum class VEC_T_MEMBERS {
	element_ptr = 0,
	element_count = 1,
	magic = 2,
	element_bits = 3
};


//	Makes a type for VEC_T.
llvm::Type* make_vec_type(llvm::LLVMContext& context);

llvm::Value* get_vec_ptr(llvm::IRBuilder<>& builder, llvm::Value* vec_byvalue);




////////////////////////////////		DICT_T


struct DICT_T {
	inline uint32_t size() const {
		return 666;
	}
};


////////////////////////////////		WIDE_RETURN_T

//	Used to return structs and bigger chunks of data from LLVM functions.
//	Can only be two members in LLVM struct, each an word wide.


//	??? Also use for arguments, not only return.
struct WIDE_RETURN_T {
	uint64_t a;
	uint64_t b;
};

enum class WIDE_RETURN_MEMBERS {
	a = 0,
	b = 1
};


llvm::Type* make_wide_return_type(llvm::LLVMContext& context);

WIDE_RETURN_T make_wide_return_2x64(uint64_t a, uint64_t b);

WIDE_RETURN_T make_wide_return_charptr(const char* s);

WIDE_RETURN_T make_wide_return_vec(const VEC_T& vec);
VEC_T wider_return_to_vec(const WIDE_RETURN_T& ret);




//	LLVM can't cast a struct-value to another struct value - need to store on stack and cast pointer instead.
//	Store the DYN to memory, then cast it to VEC and load it again.
llvm::Value* generate__convert_wide_return_to_vec(llvm::IRBuilder<>& builder, llvm::Value* wide_return_reg);





////////////////////////////////		llvm_arg_mapping_t


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

//	??? Improve map_function_arguments() it it correctly handles DYN returns.
llvm_function_def_t map_function_arguments(llvm::Module& module, const floyd::typeid_t& function_type);





////////////////////////////////		intern_type()

llvm::Type* make_function_type(llvm::Module& module, const typeid_t& function_type);

llvm::Type* intern_type(llvm::Module& module, const typeid_t& type);


//	??? Temp implementation of itype that supports base_types + vector[base_type] and dict[base_type] only.
int64_t pack_itype(const typeid_t& type);
typeid_t unpack_itype(int64_t type);


}	//	floyd

#endif /* floyd_llvm_helpers_hpp */
