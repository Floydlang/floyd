//
//  floyd_llvm_types.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_types_hpp
#define floyd_llvm_types_hpp

#include "floyd_ast.h"
#include "value_backend.h"

//#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>


namespace floyd {


llvm::Type* deref_ptr(llvm::Type* type);

//	Returns true if we pass values of this type around via pointers (not by-value).
bool pass_as_ptr(const type_desc_t& type);


/*
floyd			C++			runtime_value_t			native func arg/return
--------------------------------------------------------------------------------------
bool			bool		uint8					uint1
int							int64_t					int64
string			string		char*					char*
vector[T]		vector<T>	VEC_T*					VEC_T*
json_t			json_t		json_t*					int16*

NOTICE: Types using symbols directly or indirectly are not used in LLVM type and returns nullptr.
*/




////////////////////////////////		llvm_arg_mapping_t


//	One element for each LLVM argument.
struct llvm_arg_mapping_t {
	bool check_invariant() const {
		QUARK_ASSERT(llvm_type != nullptr);
		QUARK_ASSERT(floyd_name.empty() == false);
		QUARK_ASSERT(floyd_arg_index >= 0);
		return true;
	}


	////////////////////////////////		STATE

	llvm::Type* llvm_type;

	std::string floyd_name;
	type_t floyd_type;

	// -1 is none. Several elements can specify the same Floyd arg index,
	//	since dynamic value use two.
	int floyd_arg_index;

	enum class map_type { k_floyd_runtime_ptr, k_known_value_type, k_dyn_value, k_dyn_type } map_type;
};



////////////////////////////////		llvm_function_signature_t

//	Describes a complete LLVM function signature.
// ??? rename llvm_function_type_t

struct llvm_function_signature_t {
	bool check_invariant() const {
		QUARK_ASSERT(return_type != nullptr);
		return true;
	}


	////////////////////////////////		STATE

	llvm::Type* return_type;
	std::vector<llvm_arg_mapping_t> args;
};

llvm_function_signature_t name_args(const llvm_function_signature_t& def, const std::vector<member_t>& args);



////////////////////////////////		type_entry_t


//	LLVM-specific info for each type_t. Keep a parallell vector with these, alongside the types. If the type makes no sense for codegen, for example uses an unresolved symbol -- then it's marked as use_flag = false

struct type_entry_t {
	type_entry_t() :
		use_flag(false),
		llvm_type_specific(nullptr),
		llvm_type_generic(nullptr),
		optional_function_signature {}
	{
		QUARK_ASSERT(check_invariant());
	}

	type_entry_t(
		bool use_flag,
		llvm::Type* llvm_type_specific,
		llvm::Type* llvm_type_generic,
		std::shared_ptr<const llvm_function_signature_t> optional_function_signature
	) :
		use_flag(use_flag),
		llvm_type_specific(llvm_type_specific),
		llvm_type_generic(llvm_type_generic),
		optional_function_signature(optional_function_signature)
	{
	}

	bool check_invariant() const {
		return true;
	}


	////////////////////////////////		STATE

	bool use_flag;
	llvm::Type* llvm_type_specific;
	llvm::Type* llvm_type_generic;
	std::shared_ptr<const llvm_function_signature_t> optional_function_signature;
};



////////////////////////////////		state_t


//	You must create structs *once* in LLVM. Several identical struct types won't match up.
struct state_t {
	public: llvm::StructType* generic_vec_type;
	public: llvm::StructType* generic_dict_type;
	public: llvm::StructType* generic_struct_type;
	public: llvm::StructType* json_type;

//	public: llvm::StructType* wide_return_type;
	public: llvm::Type* runtime_ptr_type;
	public: llvm::Type* runtime_type_type;
	public: llvm::Type* runtime_value_type;

	public: types_t types;

	//	Elements match the elements inside types.
	public: std::vector<type_entry_t> type_entries;
};


////////////////////////////////		llvm_type_lookup

/*
	LLVM Type types: keeps a list of ALL types used statically in the program, their type,
	their LLVM type and their Floyd type.

	Generic-type: vector (and string), dictionary, json and struct are passed around as 4 different
	types, not one for each vector type, struct type etc. These generic types are 64 bytes big,
	the same size as heap_alloc_64_t.
*/

struct llvm_type_lookup {
	llvm_type_lookup(llvm::LLVMContext& context, const types_t& types);
	bool check_invariant() const;

	const type_entry_t& find_from_type(const type_t& type) const;


	////////////////////////////////		STATE

	public: state_t state;
};

void trace_llvm_type_lookup(const llvm_type_lookup& type_lookup);

llvm::Type* make_runtime_type_type(const llvm_type_lookup& type_lookup);
llvm::Type* make_runtime_value_type(const llvm_type_lookup& type_lookup);

type_t lookup_type(const llvm_type_lookup& type_lookup, const type_t& type);


//	Returns the exact LLVM struct layout that maps to the struct members, without
//	any alloc-64 header. Not a pointer.
llvm::StructType* get_exact_struct_type_byvalue(const llvm_type_lookup& i, const type_t& type);



//	Returns the LLVM type used to pass this type of value around. It uses generic types for
//	vector, dict and struct.
//	Small types are by-value, large types are pointers.
/*
|type_t                                        |llvm_type_specific     |llvm_type_generic |
|----------------------------------------------|-----------------------|------------------|
|undef                                         |i16                    |nullptr           |
|any                                           |i64                    |nullptr           |
|void                                          |void                   |nullptr           |
|bool                                          |i1                     |nullptr           |
|int                                           |i64                    |nullptr           |
|double                                        |double                 |nullptr           |
|string                                        |%vec*                  |%vec*             |
|json                                          |%json*                 |nullptr           |
|typeid                                        |i64                    |nullptr           |
|unresolved:                                   |i16                    |nullptr           |
|function int(any) pure                        |i64 (%frp*, i64, i64)* |nullptr           |
|[string]                                      |%vec*                  |%vec*             |
|function int(int,int) pure                    |i64 (%frp*, i64, i64)* |nullptr           |
|struct {int dur;json more;}                   |{ i64, %json* }*       |%struct*          |
|[struct {int dur;json more;}]                 |%vec*                  |%vec*             |
|function [struct {int dur;json more;}]() pure |%vec* (%frp*)*         |nullptr           |
|----------------------------------------------|-----------------------|------------------|
*/
llvm::Type* get_llvm_type_as_arg(const llvm_type_lookup& type_lookup, const type_t& type);

llvm::FunctionType* get_llvm_function_type(const llvm_type_lookup& type_lookup, const type_t& type);


//	Returns generic types.
llvm::Type* make_generic_vec_type_byvalue(const llvm_type_lookup& type_lookup);
llvm::Type* make_generic_dict_type_byvalue(const llvm_type_lookup& type_lookup);
llvm::Type* get_generic_struct_type_byvalue(const llvm_type_lookup& type_lookup);

llvm::Type* make_json_type_byvalue(const llvm_type_lookup& type_lookup);
llvm::Type* make_frp_type(const llvm_type_lookup& type_lookup);


}	// floyd

#endif /* floyd_llvm_types_hpp */
