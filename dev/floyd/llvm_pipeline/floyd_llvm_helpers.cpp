//
//  floyd_llvm_helpers.cpp
//  Floyd
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

#include "immer/vector.hpp"
#include "immer/map.hpp"


#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "semantic_analyser.h"
#include "text_parser.h"

#include "floyd_llvm_types.h"

#include "quark.h"


namespace floyd {



runtime_value_t make_runtime_typeid(runtime_type_t type){
	return { .typeid_itype = type };
}


#if 0
void save_page(const std::string &url){
    // simulate a long page fetch
//    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::string result = "fake content";
	
    std::lock_guard<std::recursive_mutex> guard(g_pages_mutex);
    g_pages[url] = result;
}
#endif



////////////////////////////////		heap_t



void trace_alloc(const heap_rec_t& e){
	QUARK_TRACE_SS(""
//		<< "used: " << e.in_use
		<< " rc: " << e.alloc_ptr->rc
		<< " debug[0]: " << e.alloc_ptr->debug_info[0]
		<< " data_a: " << e.alloc_ptr->data_a
	);
}

void trace_heap(const heap_t& heap){
	QUARK_ASSERT(heap.check_invariant());

	if(false){
		QUARK_SCOPED_TRACE("HEAP");

		std::lock_guard<std::recursive_mutex> guard(*heap.alloc_records_mutex);

		for(int i = 0 ; i < heap.alloc_records.size() ; i++){
			const auto& e = heap.alloc_records[i];
			trace_alloc(e);
		}
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
			QUARK_ASSERT(false);
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
	QUARK_ASSERT(heap.check_invariant());

	const auto header_size = sizeof(heap_alloc_64_t);
	QUARK_ASSERT(header_size == 64);

	{
		std::lock_guard<std::recursive_mutex> guard(*heap.alloc_records_mutex);


		const auto malloc_size = header_size + allocation_word_count * sizeof(uint64_t);
		void* alloc0 = std::malloc(malloc_size);
		if(alloc0 == nullptr){
			throw std::exception();
		}

		auto alloc = reinterpret_cast<heap_alloc_64_t*>(alloc0);

		alloc->rc = 1;
		alloc->magic = ALLOC_64_MAGIC;

		alloc->data_a = 0;
		alloc->data_b = 0;
		alloc->data_c = 0;
		alloc->data_d = 0;

		alloc->heap64 = &heap;
		memset(&alloc->debug_info[0], 0x00, 8);

		heap.alloc_records.push_back({ alloc });

		QUARK_ASSERT(alloc->check_invariant());
		QUARK_ASSERT(heap.check_invariant());
		return alloc;
	}
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

int32_t dec_rc(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	const auto prev_rc = std::atomic_fetch_sub_explicit(&alloc.rc, 1, std::memory_order_relaxed);
	const auto rc2 = prev_rc - 1;

	if(rc2 < 0){
		QUARK_ASSERT(false);
		throw std::exception();
	}

	return rc2;
}
int32_t inc_rc(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	const auto prev_rc = std::atomic_fetch_add_explicit(&alloc.rc, 1, std::memory_order_relaxed);
	const auto rc2 = prev_rc + 1;
	return rc2;
}

void add_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	inc_rc(alloc);
}


void dispose_alloc(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	std::lock_guard<std::recursive_mutex> guard(*alloc.heap64->alloc_records_mutex);

	auto it = std::find_if(alloc.heap64->alloc_records.begin(), alloc.heap64->alloc_records.end(), [&](heap_rec_t& e){ return e.alloc_ptr == &alloc; });
	QUARK_ASSERT(it != alloc.heap64->alloc_records.end());

//	QUARK_ASSERT(it->in_use);
//	it->in_use = false;

	//??? we don't delete the malloc() block in debug version.
}


void release_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	if(dec_rc(alloc) == 0){
		dispose_alloc(alloc);
	}
}

bool heap_t::check_invariant() const{
	std::lock_guard<std::recursive_mutex> guard(*alloc_records_mutex);
	for(const auto& e: alloc_records){
		QUARK_ASSERT(e.alloc_ptr != nullptr);
		QUARK_ASSERT(e.alloc_ptr->heap64 == this);
		QUARK_ASSERT(e.alloc_ptr->check_invariant());

/*
		if(e.in_use){
			QUARK_ASSERT(e.alloc_ptr->rc > 0);
		}
		else{
			QUARK_ASSERT(e.alloc_ptr->rc == 0);
		}
*/

	}
	return true;
}

int heap_t::count_used() const {
	QUARK_ASSERT(check_invariant());

	int result = 0;
	std::lock_guard<std::recursive_mutex> guard(*alloc_records_mutex);
	for(const auto& e: alloc_records){
		if(e.alloc_ptr->rc){
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




runtime_value_t make_blank_runtime_value(){
	return make_runtime_int(0xdeadbee1);
}

runtime_value_t make_runtime_bool(bool value){
	return { .bool_value = value };
}

runtime_value_t make_runtime_int(int64_t value){
	return { .int_value = value };
}
runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr){
	runtime_value_t tmp;
	tmp.struct_ptr = struct_ptr;
	return   tmp;
	//return { .struct_ptr = struct_ptr };
}










VEC_T* unpack_vec_arg(const llvm_type_lookup& type_lookup, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(type_lookup, arg_type);
#endif
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(type.is_vector());
	QUARK_ASSERT(arg_value.vector_ptr != nullptr);
	QUARK_ASSERT(arg_value.vector_ptr->check_invariant());

	return arg_value.vector_ptr;
}

DICT_T* unpack_dict_arg(const llvm_type_lookup& type_lookup, runtime_value_t arg_value, runtime_type_t arg_type){
#if DEBUG
	const auto type = lookup_type(type_lookup, arg_type);
#endif
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(type.is_dict());
	QUARK_ASSERT(arg_value.dict_ptr != nullptr);

	QUARK_ASSERT(arg_value.dict_ptr->check_invariant());

	return arg_value.dict_ptr;
}







////////////////////////////////	MISSING FEATURES









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
		QUARK_TRACE_SS("\n" << stream2.str());

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
		QUARK_TRACE_SS(stream2.str());

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

	return stream2.str();
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
		return rso.str();
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
		return rso.str();
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



bool check_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(type_lookup.check_invariant());

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	QUARK_ASSERT(floyd_context_arg_ptr->getType() == make_frp_type(type_lookup));
	return true;
}

bool check_emitting_function(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(type_lookup.check_invariant());

	QUARK_ASSERT(check_callers_fcp(type_lookup, emit_f));
	return true;
}

llvm::Value* get_callers_fcp(const llvm_type_lookup& type_lookup, llvm::Function& emit_f){
	QUARK_ASSERT(check_callers_fcp(type_lookup, emit_f));

	auto args = emit_f.args();
	QUARK_ASSERT((args.end() - args.begin()) >= 1);
	auto floyd_context_arg_ptr = args.begin();
	return floyd_context_arg_ptr;
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
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count);
	alloc->data_a = element_count;
	alloc->data_b = allocation_count;
	alloc->debug_info[0] = 'V';
	alloc->debug_info[1] = 'E';
	alloc->debug_info[2] = 'C';

	auto vec = reinterpret_cast<VEC_T*>(alloc);

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return vec;
}

void dispose_vec(VEC_T& vec){
	QUARK_ASSERT(vec.check_invariant());

	dispose_alloc(vec.alloc);
	QUARK_ASSERT(vec.alloc.heap64->check_invariant());
}



WIDE_RETURN_T make_wide_return_vec(VEC_T* vec){
	return make_wide_return_2x64(runtime_value_t{.vector_ptr = vec}, runtime_value_t{.int_value = 0});
}




QUARK_UNIT_TEST("VEC_T", "", "", ""){
	const auto vec_struct_size1 = sizeof(std::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size1 == 24);
}

QUARK_UNIT_TEST("VEC_T", "", "", ""){
	heap_t heap;
	detect_leaks(heap);

	VEC_T* v = alloc_vec(heap, 3, 3);
	QUARK_UT_VERIFY(v != nullptr);

	if(dec_rc(v->alloc) == 0){
		dispose_vec(*v);
	}

	QUARK_UT_VERIFY(heap.check_invariant());
	detect_leaks(heap);
}





////////////////////////////////		VEC_HAMT_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(std::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size == 24);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_UT_VERIFY(wr_struct_size == 16);
}


VEC_HAMT_T::~VEC_HAMT_T(){
	QUARK_ASSERT(check_invariant());
}

bool VEC_HAMT_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	return true;
}

VEC_HAMT_T* alloc_vec_hamt(heap_t& heap, uint64_t allocation_count, uint64_t element_count){
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count);
	alloc->data_a = element_count;
	alloc->debug_info[0] = 'V';
	alloc->debug_info[1] = 'H';
	alloc->debug_info[2] = 'A';
	alloc->debug_info[3] = 'M';
	alloc->debug_info[4] = 'T';

	auto vec = reinterpret_cast<VEC_HAMT_T*>(alloc);

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return vec;
}

void dispose_vec_hamt(VEC_HAMT_T& vec){
	QUARK_ASSERT(vec.check_invariant());

	dispose_alloc(vec.alloc);
	QUARK_ASSERT(vec.alloc.heap64->check_invariant());
}



WIDE_RETURN_T make_wide_return_vec_hamt(VEC_HAMT_T* vec){
	return make_wide_return_2x64(runtime_value_t{.vector_hamt_ptr = vec}, runtime_value_t{.int_value = 0});
}




QUARK_UNIT_TEST("VEC_HAMT_T", "", "", ""){
	const auto vec_struct_size2 = sizeof(immer::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size2 == 32);
}

QUARK_UNIT_TEST("VEC_HAMT_T", "", "", ""){
	heap_t heap;
	detect_leaks(heap);

	VEC_HAMT_T* v = alloc_vec_hamt(heap, 3, 3);
	QUARK_UT_VERIFY(v != nullptr);

	if(dec_rc(v->alloc) == 0){
		dispose_vec_hamt(*v);
	}

	QUARK_UT_VERIFY(heap.check_invariant());
	detect_leaks(heap);
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
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, 0);
	auto dict = reinterpret_cast<DICT_T*>(alloc);

	alloc->debug_info[0] = 'D';
	alloc->debug_info[1] = 'I';
	alloc->debug_info[2] = 'C';
	alloc->debug_info[3] = 'T';

	auto& m = dict->get_map_mut();
    new (&m) STDMAP();

	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(dict->check_invariant());

	return dict;
}

void dispose_dict(DICT_T& dict){
	QUARK_ASSERT(dict.check_invariant());

	dict.get_map_mut().~STDMAP();
	dispose_alloc(dict.alloc);
	QUARK_ASSERT(dict.alloc.heap64->check_invariant());
}



WIDE_RETURN_T make_wide_return_dict(DICT_T* dict){
	runtime_value_t tmp;
	tmp.dict_ptr=dict;
	return make_wide_return_2x64(tmp, { .int_value = 0 });
	//return make_wide_return_2x64({ .dict_ptr = dict }, { .int_value = 0 });
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
	QUARK_ASSERT(heap.check_invariant());
	return json;
}

void dispose_json(JSON_T& json){
	QUARK_ASSERT(json.check_invariant());

	delete &json.get_json();
	json.alloc.data_a = 666;
	dispose_alloc(json.alloc);

	QUARK_ASSERT(json.alloc.heap64->check_invariant());
}





////////////////////////////////		STRUCT_T




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

void dispose_struct(STRUCT_T& s){
	QUARK_ASSERT(s.check_invariant());

	dispose_alloc(s.alloc);

	QUARK_ASSERT(s.alloc.heap64->check_invariant());
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



////////////////////////////////		get_exact_llvm_type()












bool is_rc_value(const typeid_t& type){
	return type.is_string() || type.is_vector() || type.is_dict() || type.is_struct() || type.is_json();
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
			*static_cast<int32_t*>(value_ptr) = static_cast<int32_t>(value.typeid_itype);
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

llvm::Value* generate_cast_to_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& value, const typeid_t& floyd_type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(floyd_type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		llvm::LLVMContext& context;
		const llvm_type_lookup& type_lookup;
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
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::int_t& e) const{
			return &value;
		}
		llvm::Value* operator()(const typeid_t::double_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::BitCast, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::string_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::ZExt, &value, make_runtime_value_type(type_lookup), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, &value, make_runtime_value_type(type_lookup), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ builder, context, type_lookup, value }, floyd_type._contents);
}

llvm::Value* generate_cast_from_runtime_value2(llvm::IRBuilder<>& builder, const llvm_type_lookup& type_lookup, llvm::Value& runtime_value_reg, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	auto& context = builder.getContext();

	struct visitor_t {
		llvm::IRBuilder<>& builder;
		const llvm_type_lookup& type_lookup;
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
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type(type_lookup)->getPointerTo(), "");
		}

		llvm::Value* operator()(const typeid_t::json_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_json_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::Trunc, &runtime_value_reg, llvm::Type::getInt32Ty(context), "");
		}

		llvm::Value* operator()(const typeid_t::struct_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_generic_struct_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::vector_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_vec_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::dict_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, make_generic_dict_type(type_lookup)->getPointerTo(), "");
		}
		llvm::Value* operator()(const typeid_t::function_t& e) const{
			return builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, &runtime_value_reg, get_exact_llvm_type(type_lookup, type), "");
		}
		llvm::Value* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
		}
	};
	return std::visit(visitor_t{ builder, type_lookup, context, runtime_value_reg, type }, type._contents);
}






static const std::string k_floyd_func_link_prefix = "floydf_";


//	"hello" => "floyd_f_hello"
link_name_t make_floyd_func_link_name(const std::string& name){
	return link_name_t { k_floyd_func_link_prefix + name };
}
std::string unpack_floyd_func_link_name(const link_name_t& name){
	const auto left = name.name. substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.name.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}

std::string unpack_link_name(const link_name_t& name){
	const auto left = name.name.substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.name.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}


static const std::string k_runtime_func_link_prefix = "floydrt_";

//	"hello" => "floyd_rt_hello"
link_name_t make_runtime_func_link_name(const std::string& name){
	return link_name_t { k_runtime_func_link_prefix + name };
}

std::string unpack_runtime_func_link_name(const link_name_t& name){
	const auto left = name.name.substr(0, k_runtime_func_link_prefix.size());
	const auto right = name.name.substr(k_runtime_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_runtime_func_link_prefix);
	return right;
}






}	//	floyd

