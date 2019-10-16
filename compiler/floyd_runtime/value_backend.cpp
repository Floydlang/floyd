//
//  value_backend.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-18.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "value_backend.h"

#include <string>
#include <vector>

#include "types.h"
#include "json_support.h"
#include "compiler_basics.h"
#include "quark.h"


namespace floyd {





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
		<< " debug_info: " << get_debug_info(*e.alloc_ptr)
		<< " data[0]: " << e.alloc_ptr->data[0]
		<< " data[1]: " << e.alloc_ptr->data[1]
		<< " data[2]: " << e.alloc_ptr->data[2]
		<< " data[3]: " << e.alloc_ptr->data[3]
	);
}

void trace_heap(const heap_t& heap){
	QUARK_ASSERT(heap.check_invariant());

	if(false){
		QUARK_SCOPED_TRACE("HEAP");

		if(heap.record_allocs_flag){
#if HEAP_MUTEX
			std::lock_guard<std::recursive_mutex> guard(*heap.alloc_records_mutex);
#endif

			for(int i = 0 ; i < heap.alloc_records.size() ; i++){
				const auto& e = heap.alloc_records[i];
				trace_alloc(e);
			}
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

#if DEBUG
std::string get_debug_info(const heap_alloc_64_t& alloc){
	return alloc.debug_info;
}
#else
std::string get_debug_info(const heap_alloc_64_t& alloc){
	return "";
}
#endif

static inline void add_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	inc_rc(alloc);
}

static inline void release_ref(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	if(dec_rc(alloc) == 0){
		dispose_alloc(alloc);
	}
}


heap_alloc_64_t* alloc_64(heap_t& heap, uint64_t allocation_word_count, type_t debug_value_type, const char debug_string[]){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(debug_string != nullptr);

	const auto header_size = sizeof(heap_alloc_64_t);
	QUARK_ASSERT((header_size % 8) == 0);

#if HEAP_MUTEX
	std::lock_guard<std::recursive_mutex> guard(*heap.alloc_records_mutex);
#endif

	const auto malloc_size = header_size + allocation_word_count * sizeof(uint64_t);
	void* alloc0 = std::malloc(malloc_size);
	if(alloc0 == nullptr){
		throw std::exception();
	}

	auto alloc = new (alloc0) heap_alloc_64_t(&heap, allocation_word_count, debug_value_type, debug_string);
	QUARK_ASSERT(alloc->rc == 1);
	QUARK_ASSERT(alloc->check_invariant());
	if(heap.record_allocs_flag){
		heap.alloc_records.push_back({ alloc });
	}

	QUARK_ASSERT(alloc->check_invariant());
	QUARK_ASSERT(heap.check_invariant());
	return alloc;
}


QUARK_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap(false);
	QUARK_VERIFY(heap.check_invariant());
}

QUARK_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap(false);
	auto a = alloc_64(heap, 0, make_undefined(), "test");
	QUARK_VERIFY(a != nullptr);
	QUARK_VERIFY(a->check_invariant());
	QUARK_VERIFY(a->rc == 1);
#if DEBUG
	QUARK_VERIFY(get_debug_info(*a) == "test");
#endif

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
}

QUARK_TEST("heap_t", "add_ref()", "", ""){
	heap_t heap(false);
	auto a = alloc_64(heap, 0, make_undefined(), "test");
	add_ref(*a);
	QUARK_VERIFY(a->rc == 2);

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
}

QUARK_TEST("heap_t", "release_ref()", "", ""){
	heap_t heap(false);
	auto a = alloc_64(heap, 0, make_undefined(), "test");

	QUARK_VERIFY(a->rc == 1);
	release_ref(*a);
	QUARK_VERIFY(a->rc == 0);

	const auto count = heap.count_used();
	QUARK_VERIFY(count == 0);
}



void* get_alloc_ptr(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	const auto size = sizeof(heap_alloc_64_t);
	const auto p0 = reinterpret_cast<uint8_t*>(&alloc);
	const auto p1 = p0 + size;
	auto p = &alloc;
	const auto result = p + 1;
	QUARK_ASSERT(reinterpret_cast<uint8_t*>(result) == p1);
	return result;
}
const void* get_alloc_ptr(const heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

	auto p = &alloc;
	return p + 1;
}




bool heap_alloc_64_t::check_invariant() const{
	QUARK_ASSERT(magic == ALLOC_64_MAGIC);
	QUARK_ASSERT(heap != nullptr);
	assert(heap != nullptr);
	QUARK_ASSERT(heap->magic == HEAP_MAGIC);

//	auto it = std::find_if(heap->alloc_records.begin(), heap->alloc_records.end(), [&](heap_rec_t& e){ return e.alloc_ptr == this; });
//	QUARK_ASSERT(it != heap->alloc_records.end());

	return true;
}



void dispose_alloc(heap_alloc_64_t& alloc){
	QUARK_ASSERT(alloc.check_invariant());

#if HEAP_MUTEX
	std::lock_guard<std::recursive_mutex> guard(*alloc.heap->alloc_records_mutex);
#endif

	if(alloc.heap->record_allocs_flag){
		auto it = std::find_if(alloc.heap->alloc_records.begin(), alloc.heap->alloc_records.end(), [&](heap_rec_t& e){ return e.alloc_ptr == &alloc; });
		QUARK_ASSERT(it != alloc.heap->alloc_records.end());

//		QUARK_ASSERT(it->in_use);
//		it->in_use = false;
	}
	
	//??? we don't delete the malloc() block in debug version.
#if DEBUG
	alloc.magic = 0xdeadbeef;
	alloc.data[0] = 0xdeadbeef'00000001;
	alloc.data[1] = 0xdeadbeef'00000002;
	alloc.data[2] = 0xdeadbeef'00000003;
	alloc.data[3] = 0xdeadbeef'00000004;

	alloc.allocation_word_count = 0xdeadbeef'00000005;

	alloc.heap = reinterpret_cast<heap_t*>(0xdeadbeef'00000005);
	alloc.debug_info = "disposed alloc";
#endif

	std::free(&alloc);
}



bool heap_t::check_invariant() const{
#if HEAP_MUTEX
	std::lock_guard<std::recursive_mutex> guard(*alloc_records_mutex);
#endif

#if 0
	for(const auto& e: alloc_records){
		QUARK_ASSERT(e.alloc_ptr != nullptr);
		QUARK_ASSERT(e.alloc_ptr->heap == this);
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
#endif
	return true;
}

int heap_t::count_used() const {
	QUARK_ASSERT(check_invariant());

	int result = 0;
#if HEAP_MUTEX
	std::lock_guard<std::recursive_mutex> guard(*alloc_records_mutex);
#endif
	if(record_allocs_flag){
		for(const auto& e: alloc_records){
			if(e.alloc_ptr->rc){
				result = result + 1;
			}
		}
		return result;
	}
	else{
		return 0;
	}
}




////////////////////////////////	runtime_type_t



runtime_type_t make_runtime_type(type_t type){
	return runtime_type_t{ type.get_data() };
}



////////////////////////////////	runtime_value_t


QUARK_TEST("", "", "", ""){
	const auto s = sizeof(runtime_value_t);
	QUARK_VERIFY(s == 8);
}



QUARK_TEST("", "", "", ""){
	auto y = make_runtime_int(8);
	auto a = make_runtime_int(1234);

	y = a;

	runtime_value_t c(y);
	(void)c;
}




runtime_value_t make_blank_runtime_value(){
	return make_runtime_int(0xdeadbee1);
}

runtime_value_t make_runtime_bool(bool value){
	return { .bool_value = (uint8_t)(value ? 1 : 0) };
}

runtime_value_t make_runtime_int(int64_t value){
	return { .int_value = value };
}

runtime_value_t make_runtime_double(double value){
	return { .double_value = value };
}

runtime_value_t make_runtime_typeid(type_t type){
	return { .typeid_itype = type.get_data() };
}

runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr){
	return { .struct_ptr = struct_ptr };
}

runtime_value_t make_runtime_vector_carray(VECTOR_CARRAY_T* vector_ptr){
	return { .vector_carray_ptr = vector_ptr };
}
runtime_value_t make_runtime_vector_hamt(VECTOR_HAMT_T* vector_hamt_ptr){
	return { .vector_hamt_ptr = vector_hamt_ptr };
}

runtime_value_t make_runtime_dict_cppmap(DICT_CPPMAP_T* dict_cppmap_ptr){
	return { .dict_cppmap_ptr = dict_cppmap_ptr };
}

runtime_value_t make_runtime_dict_hamt(DICT_HAMT_T* dict_hamt_ptr){
	return { .dict_hamt_ptr = dict_hamt_ptr };
}







uint64_t get_vec_string_size(runtime_value_t str){
	QUARK_ASSERT(str.vector_carray_ptr != nullptr);

	return str.vector_carray_ptr->get_element_count();
}

void copy_elements(runtime_value_t dest[], runtime_value_t source[], uint64_t count){
	for(auto i = 0 ; i < count ; i++){
		dest[i] = source[i];
	}
}





////////////////////////////////		WIDE_RETURN_T




WIDE_RETURN_T make_wide_return_2x64(runtime_value_t a, runtime_value_t b){
	return WIDE_RETURN_T{ a, b };
}




////////////////////////////////		VECTOR_CARRAY_T



QUARK_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(std::vector<int>);
	QUARK_VERIFY(vec_struct_size == 24);
}

QUARK_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_VERIFY(wr_struct_size == 16);
}


VECTOR_CARRAY_T::~VECTOR_CARRAY_T(){
	QUARK_ASSERT(check_invariant());
}

bool VECTOR_CARRAY_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "cppvec");
	return true;
}

runtime_value_t alloc_vector_carray(heap_t& heap, uint64_t allocation_count, uint64_t element_count, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count, value_type, "cppvec");
	alloc->data[0] = element_count;

	auto vec = reinterpret_cast<VECTOR_CARRAY_T*>(alloc);

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_carray_ptr = vec };
}


void dispose_vector_carray(const runtime_value_t& value){
	QUARK_ASSERT(sizeof(VECTOR_CARRAY_T) == sizeof(heap_alloc_64_t));

	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.vector_carray_ptr != nullptr);
	QUARK_ASSERT(value.vector_carray_ptr->check_invariant());

	auto heap = value.vector_carray_ptr->alloc.heap;
	dispose_alloc(value.vector_carray_ptr->alloc);
	QUARK_ASSERT(heap->check_invariant());
}







QUARK_TEST("VECTOR_CARRAY_T", "", "", ""){
	const auto vec_struct_size1 = sizeof(std::vector<int>);
	QUARK_VERIFY(vec_struct_size1 == 24);
}

QUARK_TEST("VECTOR_CARRAY_T", "", "", ""){
	heap_t heap(false);
	detect_leaks(heap);

	auto v = alloc_vector_carray(heap, 3, 3, make_undefined());
	QUARK_VERIFY(v.vector_carray_ptr != nullptr);

	if(dec_rc(v.vector_carray_ptr->alloc) == 0){
		dispose_vector_carray(v);
	}

	QUARK_VERIFY(heap.check_invariant());
	detect_leaks(heap);
}





////////////////////////////////		VECTOR_HAMT_T



QUARK_TEST("", "", "", ""){
	const auto vec_size = sizeof(immer::vector<runtime_value_t>);
	QUARK_VERIFY(vec_size == 32);
}


VECTOR_HAMT_T::~VECTOR_HAMT_T(){
	QUARK_ASSERT(check_invariant());
}

bool VECTOR_HAMT_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "vechamt");
	const auto& vec = get_vecref();
//	QUARK_ASSERT(vec.impl().shift == 5);
	QUARK_ASSERT(vec.impl().check_tree());
	return true;
}

runtime_value_t alloc_vector_hamt(heap_t& heap, uint64_t allocation_count, uint64_t element_count, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());

	auto vec = reinterpret_cast<VECTOR_HAMT_T*>(alloc_64(heap, 0, value_type, "vechamt"));

	QUARK_ASSERT(sizeof(immer::vector<runtime_value_t>) <= heap_alloc_64_t::k_data_bytes);
    new (&vec->alloc.data[0]) immer::vector<runtime_value_t>(allocation_count, runtime_value_t{ .int_value = (int64_t)0xdeadbeef12345678 } );

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_hamt_ptr = vec };
}

runtime_value_t alloc_vector_hamt(heap_t& heap, const runtime_value_t elements[], uint64_t element_count, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(element_count == 0 || elements != nullptr);

	heap_alloc_64_t* alloc = alloc_64(heap, 0, value_type, "vechamt");

	auto vec = reinterpret_cast<VECTOR_HAMT_T*>(alloc);
	auto buffer_ptr = reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc->data[0]);

	QUARK_ASSERT(sizeof(immer::vector<runtime_value_t>) <= heap_alloc_64_t::k_data_bytes);
    auto vec2 = new (buffer_ptr) immer::vector<runtime_value_t>(&elements[0], &elements[element_count]);
	QUARK_ASSERT(vec2 == buffer_ptr);

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_hamt_ptr = vec };
}

void dispose_vector_hamt(const runtime_value_t& vec){
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(sizeof(VECTOR_HAMT_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(vec.vector_hamt_ptr != nullptr);
	QUARK_ASSERT(vec.vector_hamt_ptr->check_invariant());

	auto& vec2 = vec.vector_hamt_ptr->get_vecref_mut();
	vec2.~vector<runtime_value_t>();

	auto heap = vec.vector_hamt_ptr->alloc.heap;
	dispose_alloc(vec.vector_hamt_ptr->alloc);
	QUARK_ASSERT(heap->check_invariant());
}

runtime_value_t store_immutable(const runtime_value_t& vec0, const uint64_t index, runtime_value_t value){
	QUARK_ASSERT(vec0.check_invariant());
	const auto& vec1 = *vec0.vector_hamt_ptr;
	QUARK_ASSERT(index < vec1.get_element_count());
	auto& heap = *vec1.alloc.heap;

	heap_alloc_64_t* alloc = alloc_64(heap, 0, make_undefined(), "vechamt");

	auto vec = reinterpret_cast<VECTOR_HAMT_T*>(alloc);

	auto buffer_ptr = reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc->data[0]);

	QUARK_ASSERT(sizeof(immer::vector<runtime_value_t>) <= heap_alloc_64_t::k_data_bytes);
    auto vec2 = new (buffer_ptr) immer::vector<runtime_value_t>();

	const auto& v2 = vec1.get_vecref().set(index, value);
	*vec2 = v2;

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_hamt_ptr = vec };
}

runtime_value_t push_back_immutable(const runtime_value_t& vec0, runtime_value_t value){
	QUARK_ASSERT(vec0.check_invariant());
	const auto& vec1 = *vec0.vector_hamt_ptr;
	auto& heap = *vec1.alloc.heap;

	heap_alloc_64_t* alloc = alloc_64(heap, 0, make_undefined(), "vechamt");
	auto vec = reinterpret_cast<VECTOR_HAMT_T*>(alloc);
	auto buffer_ptr = reinterpret_cast<immer::vector<runtime_value_t>*>(&alloc->data[0]);

	QUARK_ASSERT(sizeof(immer::vector<runtime_value_t>) <= heap_alloc_64_t::k_data_bytes);
    auto vec2 = new (buffer_ptr) immer::vector<runtime_value_t>();

	const auto& v2 = vec1.get_vecref().push_back(value);
	*vec2 = v2;

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_hamt_ptr = vec };
}




QUARK_TEST("VECTOR_HAMT_T", "", "", ""){
	const auto vec_struct_size2 = sizeof(immer::vector<int>);
	QUARK_VERIFY(vec_struct_size2 == 32);
}

QUARK_TEST("VECTOR_HAMT_T", "", "", ""){
	auto backend = make_test_value_backend();
	detect_leaks(backend.heap);

	const runtime_value_t a[] = { make_runtime_int(1000), make_runtime_int(2000), make_runtime_int(3000) };
	auto v = alloc_vector_hamt(backend.heap, a, 3, type_t::make_double());
	QUARK_VERIFY(v.vector_hamt_ptr != nullptr);

	QUARK_VERIFY(v.vector_hamt_ptr->get_element_count() == 3);
	QUARK_VERIFY(v.vector_hamt_ptr->load_element(0).int_value == 1000);
	QUARK_VERIFY(v.vector_hamt_ptr->load_element(1).int_value == 2000);
	QUARK_VERIFY(v.vector_hamt_ptr->load_element(2).int_value == 3000);

	if(dec_rc(v.vector_hamt_ptr->alloc) == 0){
		dispose_vector_hamt(v);
	}

	QUARK_VERIFY(backend.check_invariant());
	detect_leaks(backend.heap);
}






////////////////////////////////		DICT_CPPMAP_T



QUARK_TEST("", "", "", ""){
	const auto size = sizeof(CPPMAP);
	QUARK_ASSERT(size == 24);
}

bool DICT_CPPMAP_T::check_invariant() const{
	QUARK_ASSERT(alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "cppdict");
	return true;
}

uint64_t DICT_CPPMAP_T::size() const {
	QUARK_ASSERT(check_invariant());

	const auto& d = get_map();
	return d.size();
}

runtime_value_t alloc_dict_cppmap(heap_t& heap, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, 0, value_type, "cppdict");
	auto dict = reinterpret_cast<DICT_CPPMAP_T*>(alloc);

	auto& m = dict->get_map_mut();

	QUARK_ASSERT(sizeof(CPPMAP) <= heap_alloc_64_t::k_data_bytes);
    new (&m) CPPMAP();

	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(dict->check_invariant());

	return runtime_value_t { .dict_cppmap_ptr = dict };
}

void dispose_dict_cppmap(runtime_value_t& d){
	QUARK_ASSERT(sizeof(DICT_CPPMAP_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(d.dict_cppmap_ptr != nullptr);
	auto& dict = *d.dict_cppmap_ptr;

	QUARK_ASSERT(dict.check_invariant());

	dict.get_map_mut().~CPPMAP();
	auto heap = dict.alloc.heap;
	dispose_alloc(dict.alloc);
	QUARK_ASSERT(heap->check_invariant());
}






////////////////////////////////		DICT_HAMT_T



QUARK_TEST("", "", "", ""){
	const auto size = sizeof(HAMT_MAP);
	QUARK_ASSERT(size == 16);
}

bool DICT_HAMT_T::check_invariant() const{
	QUARK_ASSERT(alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "hamtdic");
	return true;
}

uint64_t DICT_HAMT_T::size() const {
	QUARK_ASSERT(check_invariant());

	const auto& d = get_map();
	return d.size();
}

runtime_value_t alloc_dict_hamt(heap_t& heap, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, 0, value_type, "hamtdic");
	auto dict = reinterpret_cast<DICT_HAMT_T*>(alloc);

	auto& m = dict->get_map_mut();

	QUARK_ASSERT(sizeof(HAMT_MAP) <= heap_alloc_64_t::k_data_bytes);
    new (&m) HAMT_MAP();

	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(dict->check_invariant());

	return runtime_value_t { .dict_hamt_ptr = dict };
}

void dispose_dict_hamt(runtime_value_t& d){
	QUARK_ASSERT(sizeof(DICT_HAMT_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(d.dict_hamt_ptr != nullptr);
	auto& dict = *d.dict_hamt_ptr;

	QUARK_ASSERT(dict.check_invariant());

	dict.get_map_mut().~HAMT_MAP();
	auto heap = dict.alloc.heap;
	dispose_alloc(dict.alloc);
	QUARK_ASSERT(heap->check_invariant());
}










////////////////////////////////		JSON_T



bool JSON_T::check_invariant() const{
	QUARK_ASSERT(alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "JSON");
	QUARK_ASSERT(get_json().check_invariant());
	return true;
}

JSON_T* alloc_json(heap_t& heap, const json_t& init){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(init.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, 0, make_json_type(), "JSON");

	auto json = reinterpret_cast<JSON_T*>(alloc);
	auto copy = new json_t(init);
	json->alloc.data[0] = reinterpret_cast<uint64_t>(copy);

	QUARK_ASSERT(json->check_invariant());
	QUARK_ASSERT(heap.check_invariant());
	return json;
}

void dispose_json(JSON_T& json){
	QUARK_ASSERT(sizeof(JSON_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(json.check_invariant());

	delete &json.get_json();
	json.alloc.data[0] = 0x00000000'00000000;

	auto heap = json.alloc.heap;
	dispose_alloc(json.alloc);
	QUARK_ASSERT(heap->check_invariant());
}





////////////////////////////////		STRUCT_T


//??? Store small structs inplace.


STRUCT_T::~STRUCT_T(){
	QUARK_ASSERT(check_invariant());
}

bool STRUCT_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "struct");
	return true;
}

STRUCT_T* alloc_struct(heap_t& heap, std::size_t size, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(value_type.check_invariant());

	const auto allocation_count = size_to_allocation_blocks(size);

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count, value_type, "struct");

	auto vec = reinterpret_cast<STRUCT_T*>(alloc);
	return vec;
}

STRUCT_T* alloc_struct_copy(heap_t& heap, const uint64_t data[], std::size_t size, type_t value_type){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(data != nullptr);
	QUARK_ASSERT(value_type.check_invariant());

	const auto allocation_count = size_to_allocation_blocks(size);

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count, value_type, "struct");

	auto vec = reinterpret_cast<STRUCT_T*>(alloc);
	std::memcpy(vec->get_data_ptr(), data, size);
	return vec;
}

void dispose_struct(STRUCT_T& s){
	QUARK_ASSERT(sizeof(STRUCT_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(s.check_invariant());

	auto heap = s.alloc.heap;
	dispose_alloc(s.alloc);
	QUARK_ASSERT(heap->check_invariant());
}





bool is_rc_value(const type_desc_t& desc){
	return desc.is_string() || desc.is_vector() || desc.is_dict() || desc.is_struct() || desc.is_json();
}



// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
runtime_value_t load_via_ptr2(const types_t& types, const void* value_ptr, const type_t& type){
	QUARK_ASSERT(value_ptr != nullptr);
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const void* value_ptr;

		runtime_value_t operator()(const undefined_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const any_t& e) const{
			UNSUPPORTED();
		}

		runtime_value_t operator()(const void_t& e) const{
			UNSUPPORTED();
		}
		runtime_value_t operator()(const bool_t& e) const{
			const auto temp = *static_cast<const uint8_t*>(value_ptr);
			return make_runtime_bool(temp == 0 ? false : true);
		}
		runtime_value_t operator()(const int_t& e) const{
			const auto temp = *static_cast<const uint64_t*>(value_ptr);
			return make_runtime_int(temp);
		}
		runtime_value_t operator()(const double_t& e) const{
			const auto temp = *static_cast<const double*>(value_ptr);
			return make_runtime_double(temp);
		}
		runtime_value_t operator()(const string_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}

		runtime_value_t operator()(const json_type_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_type_t& e) const{
			const auto value = *static_cast<const int32_t*>(value_ptr);
			return make_runtime_typeid(type_t(value));
		}

		runtime_value_t operator()(const struct_t& e) const{
			STRUCT_T* struct_ptr = *reinterpret_cast<STRUCT_T* const *>(value_ptr);
			return make_runtime_struct(struct_ptr);
		}
		runtime_value_t operator()(const vector_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const dict_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const function_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const symbol_ref_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
		runtime_value_t operator()(const named_type_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
	};
	return std::visit(visitor_t{ value_ptr }, get_type_variant(types, type));
}

// IMPORTANT: Different types will access different number of bytes, for example a BYTE. We cannot dereference pointer as a uint64*!!
void store_via_ptr2(const types_t& types, void* value_ptr, const type_t& type, const runtime_value_t& value){
	struct visitor_t {
		void* value_ptr;
		const runtime_value_t& value;

		void operator()(const undefined_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const any_t& e) const{
			UNSUPPORTED();
		}

		void operator()(const void_t& e) const{
			UNSUPPORTED();
		}
		void operator()(const bool_t& e) const{
			*static_cast<uint8_t*>(value_ptr) = value.bool_value;
		}
		void operator()(const int_t& e) const{
			*(int64_t*)value_ptr = value.int_value;
		}
		void operator()(const double_t& e) const{
			*static_cast<double*>(value_ptr) = value.double_value;
		}
		void operator()(const string_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}

		void operator()(const json_type_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const typeid_type_t& e) const{
			*static_cast<int32_t*>(value_ptr) = static_cast<int32_t>(value.typeid_itype);
		}

		void operator()(const struct_t& e) const{
			*reinterpret_cast<STRUCT_T**>(value_ptr) = value.struct_ptr;
		}
		void operator()(const vector_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const dict_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const function_t& e) const{
			*static_cast<runtime_value_t*>(value_ptr) = value;
		}
		void operator()(const symbol_ref_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
		void operator()(const named_type_t& e) const {
			QUARK_ASSERT(false); throw std::exception();
		}
	};
	std::visit(visitor_t{ value_ptr, value }, get_type_variant(types, type));
}








////////////////////////////////		value_backend_t




/*
static std::map<type_t, type_t> make_type_lookup(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::map<type_t, type_t> result;
	for(const auto& e: type_lookup.state.types){
		result.insert( { e.itype, e.type } );
	}
	return result;
}
*/

value_backend_t::value_backend_t(
	const std::vector<std::pair<link_name_t, void*>>& native_func_lookup,
	const std::vector<std::pair<type_t, struct_layout_t>>& struct_layouts,
	const types_t& types,
	const config_t& config
) :
	heap(config.trace_allocs),
	types(types),
	native_func_lookup(native_func_lookup),
	struct_layouts(struct_layouts),
	config(config)
{
	QUARK_ASSERT(config.check_invariant());

	for(type_lookup_index_t i = 0 ; i < types.nodes.size() ; i++){
		const auto type0 = lookup_type_from_index(types, i);
		const auto peek = peek2(types, type0);
		if(peek.is_vector()){
			const auto& type = peek.get_vector_element_type(types);
			child_type.push_back(type);
		}
		else if(peek.is_dict()){
			const auto& type = peek.get_dict_value_type(types);
			child_type.push_back(type);
		}
		else{
			child_type.push_back(make_undefined());
		}
	}

	QUARK_ASSERT(check_invariant());
}


type_t lookup_type_ref(const value_backend_t& backend, runtime_type_t type){
	QUARK_ASSERT(backend.check_invariant());

	return type_t(type);
}





const std::pair<type_t, struct_layout_t>& find_struct_layout(const value_backend_t& backend, type_t type){
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());
#endif

	const auto& vec = backend.struct_layouts;
	const auto it = std::find_if(
		vec.begin(),
		vec.end(),
		[&] (auto& e) {
			return e.first == type;
		}
	);
	if(it != vec.end()){
		return *it;
	}
	else{
		throw std::exception();
	}
}


////////////////////////////////		VALUES


value_backend_t make_test_value_backend(){
	return value_backend_t(
		{},
		{},
		types_t(),
		make_default_config()
	);
}



void retain_vector_carray(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(peek2(backend.types, type)));
	QUARK_ASSERT(is_vector_carray(backend.types, backend.config, type) || peek2(backend.types, type).is_string());

	inc_rc(vec.vector_carray_ptr->alloc);
}



void retain_dict_cppmap(value_backend_t& backend, runtime_value_t dict, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(peek2(backend.types, type)));
	QUARK_ASSERT(is_dict_cppmap(backend.types, backend.config, type));

	inc_rc(dict.dict_cppmap_ptr->alloc);
}
void retain_dict_hamt(value_backend_t& backend, runtime_value_t dict, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(peek2(backend.types, type)));
	QUARK_ASSERT(is_dict_hamt(backend.types, backend.config, type));

	inc_rc(dict.dict_hamt_ptr->alloc);
}

void retain_struct(value_backend_t& backend, runtime_value_t s, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(s.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(peek2(backend.types, type)));
	QUARK_ASSERT(peek2(backend.types, type).is_struct());

	inc_rc(s.struct_ptr->alloc);
}

void retain_value(value_backend_t& backend, runtime_value_t value, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto type_peek = peek2(backend.types, type);
	if(is_rc_value(type_peek)){
		if(type_peek.is_string()){
			retain_vector_carray(backend, value, type);
		}
		else if(is_vector_carray(backend.types, backend.config, type)){
			retain_vector_carray(backend, value, type);
		}
		else if(is_vector_hamt(backend.types, backend.config, type)){
			retain_vector_hamt(backend, value, type);
		}
		else if(is_dict_cppmap(backend.types, backend.config, type)){
			retain_dict_cppmap(backend, value, type);
		}
		else if(is_dict_hamt(backend.types, backend.config, type)){
			retain_dict_hamt(backend, value, type);
		}
		else if(type_peek.is_json()){
			inc_rc(value.json_ptr->alloc);
		}
		else if(type_peek.is_struct()){
			retain_struct(backend, value, type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}





void release_dict_cppmap(value_backend_t& backend, runtime_value_t dict0, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict0.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type).is_dict());
	QUARK_ASSERT(is_dict_cppmap(backend.types, backend.config, type));

	auto& dict = *dict0.dict_cppmap_ptr;
	if(dec_rc(dict.alloc) == 0){

		//	Release all elements.
		const auto element_type2 = lookup_dict_value_type(backend, type);
		if(is_rc_value(peek2(backend.types, element_type2))){
			auto m = dict.get_map();
			for(const auto& e: m){
				release_value(backend, e.second, element_type2);
			}
		}
		dispose_dict_cppmap(dict0);
	}
}

void release_dict_hamt(value_backend_t& backend, runtime_value_t dict0, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict0.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type).is_dict());
	QUARK_ASSERT(is_dict_hamt(backend.types, backend.config, type));

	auto& dict = *dict0.dict_hamt_ptr;
	if(dec_rc(dict.alloc) == 0){

		//	Release all elements.
		const auto element_type2 = lookup_dict_value_type(backend, type);
		if(is_rc_value(peek2(backend.types, element_type2))){
			auto m = dict.get_map();
			for(const auto& e: m){
				release_value(backend, e.second, element_type2);
			}
		}
		dispose_dict_hamt(dict0);
	}
}

void release_dict(value_backend_t& backend, runtime_value_t dict, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type).is_dict());

	if(is_dict_cppmap(backend.types, backend.config, type)){
		release_dict_cppmap(backend, dict, type);
	}
	else if(is_dict_hamt(backend.types, backend.config, type)){
		release_dict_hamt(backend, dict, type);
	}
	else{
		QUARK_ASSERT(false);
	}
}





void release_vector_carray_pod(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(backend.types, type);
	QUARK_ASSERT(peek.is_string() || is_vector_carray(backend.types, backend.config, type));

	if(peek.is_vector()){
		QUARK_ASSERT(is_rc_value(peek2(backend.types, lookup_vector_element_type(backend, type))) == false);
	}

	if(dec_rc(vec.vector_carray_ptr->alloc) == 0){
		dispose_vector_carray(vec);
	}
}

void release_vector_carray_nonpod(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type).is_string() || is_vector_carray(backend.types, backend.config, type));
	QUARK_ASSERT(is_rc_value(peek2(backend.types, lookup_vector_element_type(backend, type))) == true);

	if(dec_rc(vec.vector_carray_ptr->alloc) == 0){
		//	Release all elements.
		{
			const auto element_type = lookup_vector_element_type(backend, type);

			auto element_ptr = vec.vector_carray_ptr->get_element_ptr();
			for(int i = 0 ; i < vec.vector_carray_ptr->get_element_count() ; i++){
				const auto& element = element_ptr[i];
				release_value(backend, element, element_type);
			}
		}
		dispose_vector_carray(vec);
	}
}

void release_vector_hamt_elements_internal(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_vector_hamt(backend.types, backend.config, type));

	const auto element_type = lookup_vector_element_type(backend, type);
	for(int i = 0 ; i < vec.vector_hamt_ptr->get_element_count() ; i++){
		const auto& element = vec.vector_hamt_ptr->load_element(i);
		release_value(backend, element, element_type);
	}
}

void release_vec(value_backend_t& backend, runtime_value_t vec, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& peek = peek2(backend.types, type);
	QUARK_ASSERT(peek.is_string() || peek.is_vector());

	if(peek.is_string()){
		if(dec_rc(vec.vector_carray_ptr->alloc) == 0){
			//	String has no elements to release.

			dispose_vector_carray(vec);
		}
	}
	else if(is_vector_carray(backend.types, backend.config, type)){
		const auto element_type = lookup_vector_element_type(backend, type);
		if(is_rc_value(peek2(backend.types, element_type))){
			release_vector_carray_nonpod(backend, vec, type);
		}
		else{
			release_vector_carray_pod(backend, vec, type);
		}
	}
	else if(is_vector_hamt(backend.types, backend.config, type)){
		const auto element_type = lookup_vector_element_type(backend, type);
		if(is_rc_value(peek2(backend.types, element_type))){
			release_vector_hamt_nonpod(backend, vec, type);
		}
		else{
			release_vector_hamt_pod(backend, vec, type);
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}

//??? prep data, inluding member types -- so we don't need to acces struct_def and its type_t:s.
void release_struct(value_backend_t& backend, runtime_value_t str, type_t type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(str.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(peek2(backend.types, type).is_struct());

	auto s = str.struct_ptr;

	if(dec_rc(s->alloc) == 0){
		const auto& struct_def = peek2(backend.types, type).get_struct(backend.types);
		const auto struct_base_ptr = s->get_data_ptr();

		const auto& struct_layout = find_struct_layout(backend, type);

		int member_index = 0;
		for(const auto& e: struct_def._members){
			const auto member_type = e._type;
			if(is_rc_value(peek2(backend.types, member_type))){
				const auto offset = struct_layout.second.members[member_index].offset;
				const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
				release_value(backend, *member_ptr, member_type);
			}
			member_index++;
		}
		dispose_struct(*s);
	}
}

void release_value(value_backend_t& backend, runtime_value_t value, type_t type){
#if DEBUG
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());
#endif
	QUARK_ASSERT(is_rc_value(peek2(backend.types, type)));

	const auto& peek = peek2(backend.types, type);

	if(peek.is_string()){
		release_vec(backend, value, type);
	}
	else if(peek.is_vector()){
		release_vec(backend, value, type);
	}
	else if(peek.is_dict()){
		release_dict(backend, value, type);
	}
	else if(peek.is_json()){
		if(dec_rc(value.json_ptr->alloc) == 0){
			dispose_json(*value.json_ptr);
		}
	}
	else if(peek2(backend.types, type).is_struct()){
		release_struct(backend, value, type);
	}
	else{
		QUARK_ASSERT(false);
	}
}


}	//	floyd

