//
//  floyd_llvm_types.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_types_hpp
#define floyd_llvm_types_hpp

#include "ast.h"

#include <llvm/IR/Function.h>


namespace floyd {


llvm::Type* deref_ptr(llvm::Type* type);




/*
	floyd			C++			runtime_value_t			native func arg/return
	--------------------------------------------------------------------------------------------------------------------
	bool			bool		uint8					uint1
	int							int64_t					int64
	string			string		char*					char*
	vector[T]		vector<T>	VEC_T*					VEC_T*
	json_t			json_t		json_t*					int16*
*/



////////////////////////////////	runtime_type_t

/*
	An integer that specifies a unique type a type interner. Use this to specify types in running program.
	Avoid using floyd::typeid_t
*/

typedef int32_t runtime_type_t;

llvm::Type* make_runtime_type_type(llvm::LLVMContext& context);

runtime_type_t make_runtime_type(int32_t itype);

//	A type. It's encoded as a 64 bit integer in our LLVM code.
llvm::Type* make_runtime_value_type(llvm::LLVMContext& context);




////////////////////////////////		llvm_arg_mapping_t


//	One element for each LLVM argument.
struct llvm_arg_mapping_t {
	bool check_invariant() const {
		QUARK_ASSERT(llvm_type != nullptr);
		QUARK_ASSERT(floyd_name.empty() == false);
		QUARK_ASSERT(floyd_arg_index >= 0);
		return true;
	}


	llvm::Type* llvm_type;

	std::string floyd_name;
	typeid_t floyd_type;
	int floyd_arg_index;	//-1 is none. Several elements can specify the same Floyd arg index, since dynamic value use two.
	enum class map_type { k_floyd_runtime_ptr, k_known_value_type, k_dyn_value, k_dyn_type } map_type;
};




////////////////////////////////		llvm_function_def_t

//	Describes a complete LLVM function signature.

struct llvm_function_def_t {
	bool check_invariant() const {
		QUARK_ASSERT(return_type != nullptr);
		QUARK_ASSERT(args.size() == llvm_args.size());
		return true;
	}


	llvm::Type* return_type;
	std::vector<llvm_arg_mapping_t> args;
	std::vector<llvm::Type*> llvm_args;
};

llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args);





////////////////////////////////		llvm_type_interner_t()

/*
	LLVM Type interner: keeps a list of ALL types used statically in the program, their itype, their LLVM type and their Floyd type.

	Generic-type: vector (and string), dictionary, json and struct are passed around as 4 different
	types, not one for each vector type, struct type etc. These generic types are 64 bytes big, the same size as
	heap_alloc_64_t.
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

//	Returns the exact LLVM struct layout that maps to the struct members, without any alloc-64 header. Not a pointer.
llvm::StructType* get_exact_struct_type(const llvm_type_interner_t& interner, const typeid_t& type);

//	Returns the LLVM type used to pass this type of value around. It uses generic types for vector, dict and struct.
llvm::Type* get_exact_llvm_type(const llvm_type_interner_t& interner, const typeid_t& type);



//	Returns generic types.
llvm::Type* make_generic_vec_type(const llvm_type_interner_t& interner);
llvm::Type* make_generic_dict_type(const llvm_type_interner_t& interner);
llvm::Type* get_generic_struct_type(const llvm_type_interner_t& interner);

llvm::Type* make_json_type(const llvm_type_interner_t& interner);

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const floyd::typeid_t& function_type);

llvm::Type* make_frp_type(const llvm_type_interner_t& interner);




////////////////////////////////	type_interner_t helpers


//base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type);
//typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type);
runtime_type_t lookup_runtime_type(const llvm_type_interner_t& interner, const typeid_t& type);


}	// floyd

#endif /* floyd_llvm_types_hpp */
