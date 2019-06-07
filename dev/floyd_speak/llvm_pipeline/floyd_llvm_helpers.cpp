//
//  floyd_llvm_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_helpers.h"

#include "ast_value.h"

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include "llvm/Bitcode/BitstreamWriter.h"


#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "host_functions.h"
#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "pass3.h"
#include "text_parser.h"
#include "ast_json.h"

#include "quark.h"


namespace floyd {





////////////////////////////////		heap_t



void trace_alloc(const heap_rec_t& e){
	QUARK_TRACE_SS(
		"used: " << e.in_use
		<< " rc: " << e.alloc_ptr->rc
		<< " debug[0]: " << e.alloc_ptr->debug_info[0]
		<< " data_a: " << e.alloc_ptr->data_a
	);
}

void trace_heap(const heap_t& heap){
	QUARK_ASSERT(heap.check_invariant());

	QUARK_SCOPED_TRACE("HEAP");
	for(int i = 0 ; i < heap.alloc_records.size() ; i++){
		const auto& e = heap.alloc_records[i];
		trace_alloc(e);
	}
}

void detect_leaks(const heap_t& heap){
	QUARK_ASSERT(heap.check_invariant());

	const auto leaks = heap.count_used();
	if(leaks > 0){
		QUARK_SCOPED_TRACE("LEAKS");

		trace_heap(heap);

#if 1
		if(leaks > 0){
			throw std::exception();
		}
#endif
	}
}


heap_t::~heap_t(){
	QUARK_ASSERT(check_invariant());

#if DEBUG
	const auto leaks = count_used();
	if(leaks > 0){
		QUARK_SCOPED_TRACE("LEAKS");
		trace_heap(*this);
	}
#endif

}



heap_alloc_64_t* alloc_64(heap_t& heap, uint64_t allocation_word_count){
	const auto header_size = sizeof(heap_alloc_64_t);
	QUARK_ASSERT(header_size == 64);

	const auto malloc_size = header_size + allocation_word_count * sizeof(uint64_t);
	void* alloc0 = std::malloc(malloc_size);
	if(alloc0 == nullptr){
		throw std::exception();
	}

	auto alloc = reinterpret_cast<heap_alloc_64_t*>(alloc0);

	alloc->allocation_word_count = allocation_word_count;
	alloc->rc = 1;
	alloc->magic = ALLOC_64_MAGIC;

	alloc->data_a = 0;
	alloc->data_b = 0;
	alloc->data_c = 0;

	alloc->heap64 = &heap;
	memset(&alloc->debug_info[0], 0x00, 16);

	heap.alloc_records.push_back({ alloc, true });

	QUARK_ASSERT(alloc->check_invariant());
	return alloc;
}

QUARK_UNIT_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap;
	QUARK_UT_VERIFY(heap.check_invariant());
}

QUARK_UNIT_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0);
	QUARK_UT_VERIFY(a != nullptr);
	QUARK_UT_VERIFY(a->check_invariant());
	QUARK_UT_VERIFY(a->allocation_word_count == 0);
	QUARK_UT_VERIFY(a->rc == 1);

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
}

QUARK_UNIT_TEST("heap_t", "add_ref()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0);
	add_ref(*a);
	QUARK_UT_VERIFY(a->rc == 2);

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
	release_ref(*a);
}

QUARK_UNIT_TEST("heap_t", "release_ref()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0);

	QUARK_UT_VERIFY(a->rc == 1);
	release_ref(*a);
	QUARK_UT_VERIFY(a->rc == 0);

	const auto count = heap.count_used();
	QUARK_UT_VERIFY(count == 0);
}



void* get_alloc_ptr(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	auto p = &alloc;
	return p + 1;
}
const void* get_alloc_ptr(const heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	auto p = &alloc;
	return p + 1;
}


bool heap_alloc_64_t::check_invariant() const{
	QUARK_ASSERT(magic == ALLOC_64_MAGIC);
	QUARK_ASSERT(heap64 != nullptr);
	QUARK_ASSERT(heap64->magic == HEAP_MAGIC);

//	auto it = std::find_if(heap64->alloc_records.begin(), heap64->alloc_records.end(), [&](heap_rec_t& e){ return e.alloc_ptr == this; });
//	QUARK_ASSERT(it != heap64->alloc_records.end());

	return true;
}

void add_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	alloc.rc++;
}

void release_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	if(alloc.rc == 0){
		throw std::exception();
	}

	alloc.rc--;

	if(alloc.rc == 0){
		auto it = std::find_if(alloc.heap64->alloc_records.begin(), alloc.heap64->alloc_records.end(), [&](heap_rec_t& e){ return e.alloc_ptr == &alloc; });
		QUARK_ASSERT(it != alloc.heap64->alloc_records.end());

		QUARK_ASSERT(it->in_use);
		it->in_use = false;

		//??? we don't delete the malloc() block in debug version.
	}
}

bool heap_t::check_invariant() const{
	for(const auto& e: alloc_records){
		QUARK_ASSERT(e.alloc_ptr != nullptr);
		QUARK_ASSERT(e.alloc_ptr->heap64 == this);
		QUARK_ASSERT(e.alloc_ptr->check_invariant());

		if(e.in_use){
			QUARK_ASSERT(e.alloc_ptr->rc > 0);
		}
		else{
			QUARK_ASSERT(e.alloc_ptr->rc == 0);
		}
	}
	return true;
}

int heap_t::count_used() const {
	QUARK_ASSERT(check_invariant());

	int result = 0;
	for(const auto& e: alloc_records){
		if(e.in_use){
			result = result + 1;
		}
	}
	return result;
}


uint64_t size_to_allocation_blocks(std::size_t size){
	const auto r = (size >> 3) + ((size & 7) > 0 ? 1 : 0);

	QUARK_ASSERT((r * sizeof(uint64_t) - size) >= 0);
	QUARK_ASSERT((r * sizeof(uint64_t) - size) < sizeof(uint64_t));

	return r;
}





////////////////////////////////	runtime_type_t


llvm::Type* make_runtime_type_type(llvm::LLVMContext& context){
	return llvm::Type::getInt32Ty(context);
/*
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt32Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
*/

}

runtime_type_t make_runtime_type(int32_t itype){
	return runtime_type_t{ itype };
}





////////////////////////////////	native_value_t


QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(runtime_value_t);
	QUARK_UT_VERIFY(s == 8);
}



QUARK_UNIT_TEST("", "", "", ""){
	auto y = make_runtime_int(8);
	auto a = make_runtime_int(1234);

	y = a;

	runtime_value_t c(y);
}


llvm::Type* make_runtime_value_type(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
}


runtime_value_t make_blank_runtime_value(){
	return make_runtime_int(0xdeadbee1);
}

runtime_value_t make_runtime_bool(bool value){
	return { .bool_value = value };
}

runtime_value_t make_runtime_int(int64_t value){
	return { .int_value = value };
}
runtime_value_t make_runtime_typeid(runtime_type_t type){
	return { .typeid_itype = type };
}
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr){
	return { .struct_ptr = struct_ptr };
}





char* get_vec_chars(runtime_value_t str){
	QUARK_ASSERT(str.vector_ptr != nullptr);

	return reinterpret_cast<char*>(str.vector_ptr->get_element_ptr());
}

uint64_t get_vec_string_size(runtime_value_t str){
	QUARK_ASSERT(str.vector_ptr != nullptr);

	return str.vector_ptr->get_element_count();
}




VEC_T* unpack_vec_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(types, arg_type);
#endif
	QUARK_ASSERT(type.is_vector());
	QUARK_ASSERT(arg_value.vector_ptr != nullptr);
	QUARK_ASSERT(arg_value.vector_ptr->check_invariant());

	return arg_value.vector_ptr;
}

DICT_T* unpack_dict_arg(const type_interner_t& types, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(types, arg_type);
#endif
	QUARK_ASSERT(type.is_dict());
	QUARK_ASSERT(arg_value.dict_ptr != nullptr);

	QUARK_ASSERT(arg_value.dict_ptr->check_invariant());

	return arg_value.dict_ptr;
}





base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto a = lookup_type(interner, type);
	const auto a_basetype = a.get_base_type();

	//??? We know ranges where type.itype maps to base_type -- no need to look up in type_interner.
	return a_basetype;
}


typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.first.itype == type; });
	if(it != interner.interned.end()){
		return it->second;
	}
	throw std::exception();
}

runtime_type_t lookup_runtime_type(const type_interner_t& interner, const typeid_t& type){
	const auto a = lookup_itype(interner, type);
	return make_runtime_type(a.itype);
}



////////////////////////////////	MISSING FEATURES




void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}





/*
LLVM return struct byvalue:
http://lists.llvm.org/pipermail/llvm-dev/2009-June/023391.html
struct Big {
 int a, b, c;
int d;
};

struct Big foo() {
  struct Big result = {1, 2, 3, 4, 5};
  return result;
}


> "Note that the code generator does not yet fully support large return
> values. The specific sizes that are currently supported are dependent on the
> target. For integers, on 32-bit targets the limit is often 64 bits, and on
> 64-bit targets the limit is often 128 bits. For aggregate types, the current
> limits are dependent on the element types; for example targets are often
> limited to 2 total integer elements and 2 total floating-point elements."
>
> So, does this mean that I can't have a return type with more than two
> integers? Is there any other way to support longer return structure?

That's what it means (at least until someone like you contributes a
patch to support larger return structs). To work around it, imitate
what the C compiler does.

Try typing:

struct Big {
 int a, b, c;
};

struct Big foo() {
  struct Big result = {1, 2, 3};
  return result;
}
*/



/// Check a function for errors, useful for use when debugging a
/// pass.
///
/// If there are no errors, the function returns false. If an error is found,
/// a message describing the error is written to OS (if non-null) and true is
/// returned.
//bool verifyFunction(const Function &F, raw_ostream *OS = nullptr);

bool check_invariant__function(const llvm::Function* f){
	QUARK_ASSERT(f != nullptr);

	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	bool errors = llvm::verifyFunction(*f, &stream2);
	if(errors){
		f->print(stream2);

		QUARK_TRACE_SS("================================================================================");
		QUARK_TRACE_SS("\n" << dump);

		QUARK_ASSERT(false);
	}
	return !errors;
}

bool check_invariant__module(llvm::Module* module){
	QUARK_ASSERT(module != nullptr);

	std::string dump;
	llvm::raw_string_ostream stream2(dump);
	bool module_errors_flag = llvm::verifyModule(*module, &stream2, nullptr);
	if(module_errors_flag){
		QUARK_TRACE_SS(dump);

		const auto& functions = module->getFunctionList();
		for(const auto& e: functions){
			QUARK_ASSERT(check_invariant__function(&e));
			llvm::verifyFunction(e);
		}

		QUARK_ASSERT(false);
		return false;
	}

	return true;
}

bool check_invariant__builder(llvm::IRBuilder<>* builder){
	QUARK_ASSERT(builder != nullptr);

	auto module = builder->GetInsertBlock()->getParent()->getParent();
	QUARK_ASSERT(check_invariant__module(module));
	return true;
}





std::string print_module(llvm::Module& module){
	std::string dump;
	llvm::raw_string_ostream stream2(dump);

	stream2 << "\n" "MODULE" << "\n";
	module.print(stream2, nullptr);

/*
	Not needed, module.print() prints the exact list.
	stream2 << "\n" "FUNCTIONS" << "\n";
	const auto& functionList = module.getFunctionList();
	for(const auto& e: functionList){
		e.print(stream2);
	}
*/

	stream2 << "\n" "GLOBALS" << "\n";
	const auto& globalList = module.getGlobalList();
	int index = 0;
	for(const auto& e: globalList){
		stream2 << index << ": ";
		e.print(stream2);
		stream2 << "\n";
		index++;
	}

	return dump;
}


std::string print_type(llvm::Type* type){
	if(type == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		type->print(rso);
//		std::cout<<rso.str();
		return s;
	}
}

std::string print_function(const llvm::Function* f){
	if(f == nullptr){
		return "nullptr";
	}
	else{
		QUARK_ASSERT(check_invariant__function(f));

		std::string s;
		llvm::raw_string_ostream rso(s);
		f->print(rso);
//		std::cout<<rso.str();
		return s;
	}
}

std::string print_value0(llvm::Value* value){
	if(value == nullptr){
		return "nullptr";
	}
	else{
		std::string s;
		llvm::raw_string_ostream rso(s);
		value->print(rso);
//		std::cout<<rso.str();
		return s;
	}
}
std::string print_value(llvm::Value* value){
	if(value == nullptr){
		return "nullptr";
	}
	else{
		const std::string type_str = print_type(value->getType());
		const auto val_str = print_value0(value);
		return "[" + type_str + ":" + val_str + "]";
	}
}




////////////////////////////////		WIDE_RETURN_T




WIDE_RETURN_T make_wide_return_2x64(runtime_value_t a, runtime_value_t b){
	return WIDE_RETURN_T{ a, b };
}




////////////////////////////////	floyd_runtime_ptr



bool check_callers_fcp(const llvm_type_interner_t& interner, llvm::Function& emit_f){
	QUARK_ASSERT(interner.check_invariant());

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	QUARK_ASSERT(floyd_context_arg_ptr->getType() == make_frp_type(interner));
	return true;
}

bool check_emitting_function(const llvm_type_interner_t& interner, llvm::Function& emit_f){
	QUARK_ASSERT(interner.check_invariant());

	QUARK_ASSERT(check_callers_fcp(interner, emit_f));
	return true;
}

llvm::Value* get_callers_fcp(const llvm_type_interner_t& interner, llvm::Function& emit_f){
	QUARK_ASSERT(check_callers_fcp(interner, emit_f));

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	return floyd_context_arg_ptr;
}

llvm::Type* make_frp_type(const llvm_type_interner_t& interner){
	return interner.runtime_ptr_type;
}



////////////////////////////////		VEC_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(std::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size == 24);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_UT_VERIFY(wr_struct_size == 16);
}


VEC_T::~VEC_T(){
	QUARK_ASSERT(check_invariant());
}

bool VEC_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	return true;
}

VEC_T* alloc_vec(heap_t& heap, uint64_t allocation_count, uint64_t element_count){
	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count);
	alloc->data_a = element_count;
	alloc->debug_info[0] = 'V';
	alloc->debug_info[1] = 'E';
	alloc->debug_info[2] = 'C';

	auto vec = reinterpret_cast<VEC_T*>(alloc);
	return vec;
}

void vec_addref(VEC_T& vec){
	QUARK_ASSERT(vec.check_invariant());

	add_ref(vec.alloc);

	QUARK_ASSERT(vec.check_invariant());
}
void vec_releaseref(VEC_T* vec){
	QUARK_ASSERT(vec != nullptr);
	QUARK_ASSERT(vec->check_invariant());

	release_ref(vec->alloc);
}



WIDE_RETURN_T make_wide_return_vec(VEC_T* vec){
	return make_wide_return_2x64(runtime_value_t{.vector_ptr = vec}, runtime_value_t{.int_value = 0});
}

VEC_T* wide_return_to_vec(const WIDE_RETURN_T& ret){
	return ret.a.vector_ptr;
}






////////////////////////////////		DICT_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto size = sizeof(STDMAP);
	QUARK_ASSERT(size == 24);
}

bool DICT_T::check_invariant() const{
	QUARK_ASSERT(alloc.check_invariant());
	return true;
}

uint64_t DICT_T::size() const {
	QUARK_ASSERT(check_invariant());

	const auto& d = get_map();
	return d.size();
}

DICT_T* alloc_dict(heap_t& heap){
	heap_alloc_64_t* alloc = alloc_64(heap, 0);
	auto dict = reinterpret_cast<DICT_T*>(alloc);

	alloc->debug_info[0] = 'D';
	alloc->debug_info[1] = 'I';
	alloc->debug_info[2] = 'C';
	alloc->debug_info[3] = 'T';

	auto& m = dict->get_map_mut();
    new (&m) STDMAP();
	return dict;
}

void dict_addref(DICT_T& dict){
	QUARK_ASSERT(dict.check_invariant());

	add_ref(dict.alloc);

	QUARK_ASSERT(dict.check_invariant());
}
void dict_releaseref(DICT_T* dict){
	QUARK_ASSERT(dict != nullptr);
	QUARK_ASSERT(dict->check_invariant());

	//??? atomic needed!
	if(dict->alloc.rc == 1){
   		dict->get_map_mut().~STDMAP();
	}

	release_ref(dict->alloc);
}



WIDE_RETURN_T make_wide_return_dict(DICT_T* dict){
	return make_wide_return_2x64({ .dict_ptr = dict }, { .int_value = 0 });
}

DICT_T* wide_return_to_dict(const WIDE_RETURN_T& ret){
	return ret.a.dict_ptr;
}





////////////////////////////////		JSON_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto size = sizeof(STDMAP);
	QUARK_ASSERT(size == 24);
}

bool JSON_T::check_invariant() const{
	QUARK_ASSERT(alloc.check_invariant());
	QUARK_ASSERT(get_json().check_invariant());
	return true;
}

JSON_T* alloc_json(heap_t& heap, const json_t& init){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(init.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, 0);

	alloc->debug_info[0] = 'J';
	alloc->debug_info[1] = 'S';
	alloc->debug_info[2] = 'O';
	alloc->debug_info[3] = 'N';

	auto json = reinterpret_cast<JSON_T*>(alloc);
	auto copy = new json_t(init);
	json->alloc.data_a = reinterpret_cast<uint64_t>(copy);

	QUARK_ASSERT(json->check_invariant());
	return json;
}

void json_addref(JSON_T& json){
	QUARK_ASSERT(json.check_invariant());

	add_ref(json.alloc);

	QUARK_ASSERT(json.check_invariant());
}
void json_releaseref(JSON_T* json){
	QUARK_ASSERT(json != nullptr);
	QUARK_ASSERT(json->check_invariant());

	//??? atomic needed!
	if(json->alloc.rc == 1){
		delete &json->get_json();
		json->alloc.data_a = 666;
	}
	release_ref(json->alloc);
}

#if 0
WIDE_RETURN_T make_wide_return_json(JSON_T* json){
	return make_wide_return_2x64({ .json_ptr = json }, { .int_value = 0 });
}

JSON_T* wide_return_to_json(const WIDE_RETURN_T& ret){
	return ret.a.json_ptr;
}
#endif




////////////////////////////////		VEC_T




STRUCT_T::~STRUCT_T(){
	QUARK_ASSERT(check_invariant());
}

bool STRUCT_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	return true;
}

STRUCT_T* alloc_struct(heap_t& heap, std::size_t size){
	const auto allocation_count = size_to_allocation_blocks(size);

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count);
	alloc->debug_info[0] = 'S';
	alloc->debug_info[1] = 'T';
	alloc->debug_info[2] = 'R';
	alloc->debug_info[3] = 'U';
	alloc->debug_info[4] = 'C';
	alloc->debug_info[5] = 'T';

	auto vec = reinterpret_cast<STRUCT_T*>(alloc);
	return vec;
}

void struct_addref(STRUCT_T& vec){
	QUARK_ASSERT(vec.check_invariant());

	add_ref(vec.alloc);

	QUARK_ASSERT(vec.check_invariant());
}
void struct_releaseref(STRUCT_T* vec){
	QUARK_ASSERT(vec != nullptr);
	QUARK_ASSERT(vec->check_invariant());

	release_ref(vec->alloc);
}



WIDE_RETURN_T make_wide_return_structptr(STRUCT_T* s){
	return WIDE_RETURN_T{ { .struct_ptr = s }, { .int_value = 0 } };
}

STRUCT_T* wide_return_to_struct(const WIDE_RETURN_T& ret){
	return ret.a.struct_ptr;
}





////////////////////////////////		HELPERS




void generate_array_element_store(llvm::IRBuilder<>& builder, llvm::Value& array_ptr_reg, uint64_t element_index, llvm::Value& element_reg){
	QUARK_ASSERT(array_ptr_reg.getType()->isPointerTy());
	QUARK_ASSERT(array_ptr_reg.getType()->isPointerTy());

	auto element_type = array_ptr_reg.getType()->getPointerElementType();

	QUARK_ASSERT(element_type == element_reg.getType());

	auto element_index_reg = llvm::ConstantInt::get(builder.getInt64Ty(), element_index);
	const auto gep = std::vector<llvm::Value*>{
		element_index_reg
	};
	llvm::Value* element_n_ptr = builder.CreateGEP(element_type, &array_ptr_reg, gep, "");
	builder.CreateStore(&element_reg, element_n_ptr);
}



llvm::Type* deref_ptr(llvm::Type* type){
	QUARK_ASSERT(type != nullptr);

	if(type->isPointerTy()){
		llvm::PointerType* type2 = llvm::cast<llvm::PointerType>(type);
  		llvm::Type* element_type = type2->getElementType();
  		return element_type;
	}
	else{
		QUARK_ASSERT(false);
		return type;
	}
}


////////////////////////////////		llvm_arg_mapping_t


llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args){
	if(args.empty()){
		QUARK_ASSERT(def.args.size() == 1);
		return def;
	}
	else{
		//	Skip arg #0, which is "floyd_runtime_ptr".
		const auto floyd_arg_count = def.args.back().floyd_arg_index + 1;
		QUARK_ASSERT(floyd_arg_count == args.size());

		std::vector<llvm_arg_mapping_t> arg_results;

		for(int out_index = 0 ; out_index < def.args.size() ; out_index++){
			auto arg_copy = def.args[out_index];
			if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name;
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name + "-dynval";
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];

				arg_copy.floyd_name = floyd_arg._name + "-dyntype";
			}
			else{
				QUARK_ASSERT(false);
			}
			arg_results.push_back(arg_copy);
		}

		return llvm_function_def_t { def.return_type, arg_results, def.llvm_args };
	}
}

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto ret = function_type.get_function_return();
	llvm::Type* return_type = ret.is_any() ? make_wide_return_type(interner) : intern_type(interner, ret);

	const auto args = function_type.get_function_args();
	std::vector<llvm_arg_mapping_t> arg_results;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation in Floyd function type.
	arg_results.push_back({ make_frp_type(interner), "floyd_runtime_ptr", floyd::typeid_t::make_undefined(), -1, llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr });

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(arg.is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(arg.is_any()){
			arg_results.push_back({ make_runtime_value_type(context), std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_dyn_value });
			arg_results.push_back({ make_runtime_type_type(context), std::to_string(index), typeid_t::make_undefined(), index, llvm_arg_mapping_t::map_type::k_dyn_type });
		}
		else {
			auto arg_itype = intern_type(interner, arg);
			arg_results.push_back({ arg_itype, std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_known_value_type });
		}
	}

	std::vector<llvm::Type*> llvm_args;
	for(const auto& e: arg_results){
		llvm_args.push_back(e.llvm_type);
	}

	return llvm_function_def_t { return_type, arg_results, llvm_args };
}

static llvm_type_interner_t make_basic_interner(llvm::LLVMContext& context);


QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
	llvm_instance_t instance;
	const auto interner = make_basic_interner(instance.context);
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	auto& context = module->getContext();

	const auto r = map_function_arguments(
		context,
		interner,
		typeid_t::make_function(typeid_t::make_void(), {}, epure::pure)
	);

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
	llvm_instance_t instance;
	const auto interner = make_basic_interner(instance.context);
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	auto& context = module->getContext();

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isIntegerTy(64));

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
	llvm_instance_t instance;
	const auto interner = make_basic_interner(instance.context);
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	auto& context = module->getContext();

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 2);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int, DYN, bool)", ""){
	llvm_instance_t instance;
	const auto interner = make_basic_interner(instance.context);
	auto module = std::make_unique<llvm::Module>("test", instance.context);
	auto& context = module->getContext();

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int(), typeid_t::make_any(), typeid_t::make_bool() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 5);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);

	QUARK_UT_VERIFY(r.args[2].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[2].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[2].floyd_type.is_any());
	QUARK_UT_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_UT_VERIFY(r.args[3].llvm_type->isIntegerTy(32));
	QUARK_UT_VERIFY(r.args[3].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[3].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_UT_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_UT_VERIFY(r.args[4].floyd_name == "2");
	QUARK_UT_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_UT_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_UT_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}



llvm::GlobalVariable* generate_global0(llvm::Module& module, const std::string& symbol_name, llvm::Type& itype, llvm::Constant* init_or_nullptr){
//	QUARK_ASSERT(check_invariant__module(&module));
	QUARK_ASSERT(symbol_name.empty() == false);

	llvm::GlobalVariable* gv = new llvm::GlobalVariable(
		module,
		&itype,
		false,	//	isConstant
		llvm::GlobalValue::ExternalLinkage,
		init_or_nullptr ? init_or_nullptr : llvm::Constant::getNullValue(&itype),
		symbol_name
	);

//	QUARK_ASSERT(check_invariant__module(&module));

	return gv;
}



////////////////////////////////		intern_type()




//	Function-types are always returned as pointer-to-function types.
static llvm::Type* make_function_type_internal(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments(context, interner, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}

static llvm::StructType* make_exact_struct_type(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto m2 = intern_type(interner, m._type);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}


static llvm::StructType* make_wide_return_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	a
		llvm::Type::getInt64Ty(context),

		//	b
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

static llvm::StructType* make_generic_vec_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context)
	};

	llvm::StructType* s = llvm::StructType::create(context, members, "vec");
	return s;
}

static llvm::StructType* make_generic_dict_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "dict");
	return s;
}

static llvm::StructType* make_json_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "json");
	return s;
}

static llvm::StructType* make_generic_runtime_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt16Ty(context)
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "frp");
	return s;
}

static llvm::StructType* make_generic_struct_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "struct");
	return s;
}

static llvm::Type* make_exact_type_internal(llvm::LLVMContext& context, llvm_type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		llvm::LLVMContext& context;
		llvm_type_interner_t& interner;
		const typeid_t& type;

		llvm::Type* operator()(const typeid_t::undefined_t& e) const{
			return llvm::Type::getInt16Ty(context);
		}
		llvm::Type* operator()(const typeid_t::any_t& e) const{
			return make_runtime_value_type(context);
		}

		llvm::Type* operator()(const typeid_t::void_t& e) const{
			return llvm::Type::getVoidTy(context);
		}
		llvm::Type* operator()(const typeid_t::bool_t& e) const{
			return llvm::Type::getInt1Ty(context);
		}
		llvm::Type* operator()(const typeid_t::int_t& e) const{
			return llvm::Type::getInt64Ty(context);
		}
		llvm::Type* operator()(const typeid_t::double_t& e) const{
			return llvm::Type::getDoubleTy(context);
		}
		llvm::Type* operator()(const typeid_t::string_t& e) const{
			return make_vec_type(interner)->getPointerTo();
		}

		llvm::Type* operator()(const typeid_t::json_type_t& e) const{
			return make_json_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::typeid_type_t& e) const{
			return make_runtime_type_type(context);
		}

		llvm::Type* operator()(const typeid_t::struct_t& e) const{
			return make_exact_struct_type(context, interner, type)->getPointerTo();
//			return get_generic_struct_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::vector_t& e) const{
			return make_vec_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::dict_t& e) const{
			return make_dict_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::function_t& e) const{
			return make_function_type_internal(context, interner, type);
		}
		llvm::Type* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
			return llvm::Type::getInt16Ty(context);
		}
	};
	return std::visit(visitor_t{ context, interner, type }, type._contents);
}



////////////////////////////////		llvm_type_interner_t()


static llvm_type_interner_t make_basic_interner(llvm::LLVMContext& context){
	type_interner_t temp;
	intern_type(temp, typeid_t::make_void());
	intern_type(temp, typeid_t::make_int());
	intern_type(temp, typeid_t::make_bool());
	intern_type(temp, typeid_t::make_string());
	return llvm_type_interner_t(context, temp);
}


llvm_type_interner_t::llvm_type_interner_t(llvm::LLVMContext& context, const type_interner_t& i){
	generic_vec_type = make_generic_vec_type_internal(context);
	generic_dict_type = make_generic_dict_type_internal(context);
	json_type = make_json_type_internal(context);
	generic_struct_type = make_generic_struct_type_internal(context);
	wide_return_type = make_wide_return_type_internal(context);
	runtime_ptr_type = make_generic_runtime_type_internal(context)->getPointerTo();

	for(const auto& e: i.interned){
		const auto llvm_type = make_exact_type_internal(context, *this, e.second);
		intern_type(interner, e.second);
		exact_llvm_types.push_back(llvm_type);
	}

	QUARK_ASSERT(check_invariant());
}

bool llvm_type_interner_t::check_invariant() const {
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(interner.interned.size() == exact_llvm_types.size());

	QUARK_ASSERT(generic_vec_type != nullptr);
	QUARK_ASSERT(generic_dict_type != nullptr);
	QUARK_ASSERT(json_type != nullptr);
	QUARK_ASSERT(generic_struct_type != nullptr);
	QUARK_ASSERT(wide_return_type != nullptr);
	return true;
}

llvm::Type* intern_type(const llvm_type_interner_t& i, const typeid_t& type){
	if(type.is_vector()){
		return i.generic_vec_type->getPointerTo();
	}
	else if(type.is_dict()){
		return i.generic_dict_type->getPointerTo();
	}
	else if(type.is_struct()){
		return i.generic_struct_type->getPointerTo();
	}
	else{
		const auto it = std::find_if(i.interner.interned.begin(), i.interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
		if(it == i.interner.interned.end()){
			throw std::exception();
		}
		const auto index = it - i.interner.interned.begin();
		QUARK_ASSERT(index >= 0 && index < i.exact_llvm_types.size());
		return i.exact_llvm_types[index];
	}
}


//??? Make intern_type() return vector, struct etc. directly, not getPointerTo().
llvm::StructType* get_exact_struct_type(const llvm_type_interner_t& i, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	const auto it = std::find_if(i.interner.interned.begin(), i.interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
	if(it == i.interner.interned.end()){
		throw std::exception();
	}
	const auto index = it - i.interner.interned.begin();
	QUARK_ASSERT(index >= 0 && index < i.exact_llvm_types.size());
	auto result = i.exact_llvm_types[index];

	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
//	return make_exact_struct_type(context, interner, type)->getPointerTo();

/*
	auto result = intern_type(interner, type);
	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
*/

}

llvm::StructType* make_wide_return_type(const llvm_type_interner_t& interner){
	return interner.wide_return_type;
}

llvm::Type* make_vec_type(const llvm_type_interner_t& interner){
	return interner.generic_vec_type;
}

llvm::Type* make_dict_type(const llvm_type_interner_t& interner){
	return interner.generic_dict_type;
}

llvm::Type* make_json_type(const llvm_type_interner_t& interner){
	return interner.json_type;
}

llvm::Type* get_generic_struct_type(const llvm_type_interner_t& interner){
	return interner.generic_struct_type;
}

llvm::Type* get_generic_runtime_type(const llvm_type_interner_t& interner){
	return interner.generic_struct_type;
}




llvm::Type* make_function_type(const llvm_type_interner_t& interner, const typeid_t& function_type){
	return intern_type(interner, function_type);
}

llvm::StructType* make_struct_type(const llvm_type_interner_t& interner, const typeid_t& type){
	auto struct_ptr = intern_type(interner, type);
	auto struct_byvalue = deref_ptr(struct_ptr);
	return llvm::cast<llvm::StructType>(struct_byvalue);
}



bool is_rc_value(const typeid_t& type){
	return type.is_string() || type.is_vector() || type.is_dict() || type.is_struct() || type.is_json_value();
}










// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
runtime_value_t load_via_ptr2(const void* value_ptr, const typeid_t& type){
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const void* value_ptr;

		runtime_value_t operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		runtime_value_t operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const typeid_t::bool_t& e) const{
			const auto temp = *static_cast<const uint8_t*>(value_ptr);
			return runtime_value_t{ .bool_value = temp };
		}
		runtime_value_t operator()(const typeid_t::int_t& e) const{
			const auto temp = *static_cast<const uint64_t*>(value_ptr);
			return make_runtime_int(temp);
		}
		runtime_value_t operator()(const typeid_t::double_t& e) const{
			const auto temp = *static_cast<const double*>(value_ptr);
			return runtime_value_t{ .double_value = temp };
		}
		runtime_value_t operator()(const typeid_t::string_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto value = *static_cast<const int32_t*>(value_ptr);
			return runtime_value_t{ .typeid_itype = value };
		}

		runtime_value_t operator()(const typeid_t::struct_t& e) const{
			STRUCT_T* struct_ptr = *reinterpret_cast<STRUCT_T* const *>(value_ptr);
			return make_runtime_struct(struct_ptr);
		}
		runtime_value_t operator()(const typeid_t::vector_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::dict_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::function_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ value_ptr }, type._contents);
}

// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
void store_via_ptr2(void* value_ptr, const typeid_t& type, const runtime_value_t& value){
	struct visitor_t {
		void* value_ptr;
		const runtime_value_t& value;

		void operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		void operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const typeid_t::bool_t& e) const{
			*static_cast<uint8_t*>(value_ptr) = value.bool_value;
		}
		void operator()(const typeid_t::int_t& e) const{
			*(int64_t*)value_ptr = value.int_value;
		}
		void operator()(const typeid_t::double_t& e) const{
			*static_cast<double*>(value_ptr) = value.double_value;
		}
		void operator()(const typeid_t::string_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}

		void operator()(const typeid_t::json_type_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::typeid_type_t& e) const{
			*static_cast<int32_t*>(value_ptr) = value.typeid_itype;
		}

		void operator()(const typeid_t::struct_t& e) const{
			*reinterpret_cast<STRUCT_T**>(value_ptr) = value.struct_ptr;
		}
		void operator()(const typeid_t::vector_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::dict_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::function_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	std::visit(visitor_t{ value_ptr, value }, type._contents);
}

llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::LLVMContext& context;
		llvm::IRBuilder<>& builder;
		llvm::Value& value;

		llvm::Value* operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		llvm::Value* operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::bool_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &value;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(context), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(context), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ context, builder, value }, floyd_type._contents);
}

llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_interner_t& interner, llvm::Value& runtime_value_reg, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		const llvm_type_interner_t& interner;
		llvm::LLVMContext& context;
		llvm::Value& runtime_value_reg;
		const typeid_t& type;

		llvm::Value* operator()(const typeid_t::undefined_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::any_t& e) const{
			UNSUPPORTED();
		}

		llvm::Value* operator()(const typeid_t::void_t& e) const{
			UNSUPPORTED();
		}
		llvm::Value* operator()(const typeid_t::bool_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt1Ty(context), "");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &runtime_value_reg;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &runtime_value_reg, llvm::Type::getDoubleTy(context), "");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_vec_type(interner)->getPointerTo(), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_json_type(interner)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt32Ty(context), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_generic_struct_type(interner)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_vec_type(interner)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_dict_type(interner)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_function_type(interner, type), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ builder, interner, context, runtime_value_reg, type }, type._contents);
}


}	//	floyd

