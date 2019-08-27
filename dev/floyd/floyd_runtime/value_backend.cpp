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

#include "typeid.h"
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

		if(k_record_allocs){
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


heap_alloc_64_t* alloc_64(heap_t& heap, uint64_t allocation_word_count, const typeid_t& debug_value_type, const char debug_string[]){
	QUARK_ASSERT(heap.check_invariant());
	QUARK_ASSERT(debug_value_type.check_invariant());
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
	if(k_record_allocs){
		heap.alloc_records.push_back({ alloc });
	}

	QUARK_ASSERT(alloc->check_invariant());
	QUARK_ASSERT(heap.check_invariant());
	return alloc;
}


QUARK_UNIT_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap;
	QUARK_UT_VERIFY(heap.check_invariant());
}

QUARK_UNIT_TEST("heap_t", "alloc_64()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0, typeid_t::make_undefined(), "test");
	QUARK_UT_VERIFY(a != nullptr);
	QUARK_UT_VERIFY(a->check_invariant());
	QUARK_UT_VERIFY(a->rc == 1);
#if DEBUG
	QUARK_UT_VERIFY(get_debug_info(*a) == "test");
#endif

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
}

QUARK_UNIT_TEST("heap_t", "add_ref()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0, typeid_t::make_undefined(), "test");
	add_ref(*a);
	QUARK_UT_VERIFY(a->rc == 2);

	//	Must release alloc or heap will detect leakage.
	release_ref(*a);
	release_ref(*a);
}

QUARK_UNIT_TEST("heap_t", "release_ref()", "", ""){
	heap_t heap;
	auto a = alloc_64(heap, 0, typeid_t::make_undefined(), "test");

	QUARK_UT_VERIFY(a->rc == 1);
	release_ref(*a);
	QUARK_UT_VERIFY(a->rc == 0);

	const auto count = heap.count_used();
	QUARK_UT_VERIFY(count == 0);
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

	if(k_record_allocs){
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
	if(k_record_allocs){
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


uint64_t size_to_allocation_blocks(std::size_t size){
	const auto r = (size >> 3) + ((size & 7) > 0 ? 1 : 0);

	QUARK_ASSERT((r * sizeof(uint64_t) - size) >= 0);
	QUARK_ASSERT((r * sizeof(uint64_t) - size) < sizeof(uint64_t));

	return r;
}


////////////////////////////////	runtime_type_t



runtime_type_t make_runtime_type(int32_t itype){
	return runtime_type_t{ itype };
}



////////////////////////////////	runtime_value_t


QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(runtime_value_t);
	QUARK_UT_VERIFY(s == 8);
}



QUARK_UNIT_TEST("", "", "", ""){
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

runtime_value_t make_runtime_typeid(runtime_type_t type){
	return { .typeid_itype = type };
}

runtime_value_t make_runtime_struct(STRUCT_T* struct_ptr){
	return { .struct_ptr = struct_ptr };
}

runtime_value_t make_runtime_vector_cppvector(VECTOR_CPPVECTOR_T* vector_ptr){
	return { .vector_cppvector_ptr = vector_ptr };
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
	QUARK_ASSERT(str.vector_cppvector_ptr != nullptr);

	return str.vector_cppvector_ptr->get_element_count();
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




////////////////////////////////		VECTOR_CPPVECTOR_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_struct_size = sizeof(std::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size == 24);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto wr_struct_size = sizeof(WIDE_RETURN_T);
	QUARK_UT_VERIFY(wr_struct_size == 16);
}


VECTOR_CPPVECTOR_T::~VECTOR_CPPVECTOR_T(){
	QUARK_ASSERT(check_invariant());
}

bool VECTOR_CPPVECTOR_T::check_invariant() const {
	QUARK_ASSERT(this->alloc.check_invariant());
	QUARK_ASSERT(get_debug_info(alloc) == "cppvec");
	return true;
}

runtime_value_t alloc_vector_ccpvector2(heap_t& heap, uint64_t allocation_count, uint64_t element_count, const typeid_t& value_type){
	QUARK_ASSERT(heap.check_invariant());

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count, value_type, "cppvec");
	alloc->data[0] = element_count;

	auto vec = reinterpret_cast<VECTOR_CPPVECTOR_T*>(alloc);

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_cppvector_ptr = vec };
}


void dispose_vector_cppvector(const runtime_value_t& value){
	QUARK_ASSERT(sizeof(VECTOR_CPPVECTOR_T) == sizeof(heap_alloc_64_t));

	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value.vector_cppvector_ptr != nullptr);
	QUARK_ASSERT(value.vector_cppvector_ptr->check_invariant());

	auto heap = value.vector_cppvector_ptr->alloc.heap;
	dispose_alloc(value.vector_cppvector_ptr->alloc);
	QUARK_ASSERT(heap->check_invariant());
}







QUARK_UNIT_TEST("VECTOR_CPPVECTOR_T", "", "", ""){
	const auto vec_struct_size1 = sizeof(std::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size1 == 24);
}

QUARK_UNIT_TEST("VECTOR_CPPVECTOR_T", "", "", ""){
	heap_t heap;
	detect_leaks(heap);

	auto v = alloc_vector_ccpvector2(heap, 3, 3, typeid_t::make_undefined());
	QUARK_UT_VERIFY(v.vector_cppvector_ptr != nullptr);

	if(dec_rc(v.vector_cppvector_ptr->alloc) == 0){
		dispose_vector_cppvector(v);
	}

	QUARK_UT_VERIFY(heap.check_invariant());
	detect_leaks(heap);
}





////////////////////////////////		VECTOR_HAMT_T



QUARK_UNIT_TEST("", "", "", ""){
	const auto vec_size = sizeof(immer::vector<runtime_value_t>);
	QUARK_UT_VERIFY(vec_size == 32);
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

runtime_value_t alloc_vector_hamt(heap_t& heap, uint64_t allocation_count, uint64_t element_count, const typeid_t& value_type){
	QUARK_ASSERT(heap.check_invariant());

	auto vec = reinterpret_cast<VECTOR_HAMT_T*>(alloc_64(heap, 0, value_type, "vechamt"));

	QUARK_ASSERT(sizeof(immer::vector<runtime_value_t>) <= heap_alloc_64_t::k_data_bytes);
    new (&vec->alloc.data[0]) immer::vector<runtime_value_t>(allocation_count, runtime_value_t{ .int_value = (int64_t)0xdeadbeef12345678 } );

	QUARK_ASSERT(vec->check_invariant());
	QUARK_ASSERT(heap.check_invariant());

	return { .vector_hamt_ptr = vec };
}

runtime_value_t alloc_vector_hamt(heap_t& heap, const runtime_value_t elements[], uint64_t element_count, const typeid_t& value_type){
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

	heap_alloc_64_t* alloc = alloc_64(heap, 0, typeid_t::make_undefined(), "vechamt");

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

	heap_alloc_64_t* alloc = alloc_64(heap, 0, typeid_t::make_undefined(), "vechamt");
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





QUARK_UNIT_TEST("VECTOR_HAMT_T", "", "", ""){
	const auto vec_struct_size2 = sizeof(immer::vector<int>);
	QUARK_UT_VERIFY(vec_struct_size2 == 32);
}

QUARK_UNIT_TEST("VECTOR_HAMT_T", "", "", ""){
	heap_t heap;
	detect_leaks(heap);

	const runtime_value_t a[] = { make_runtime_int(1000), make_runtime_int(2000), make_runtime_int(3000) };
	auto v = alloc_vector_hamt(heap, a, 3, typeid_t::make_vector(typeid_t::make_int()));
	QUARK_UT_VERIFY(v.vector_hamt_ptr != nullptr);

	QUARK_UT_VERIFY(v.vector_hamt_ptr->get_element_count() == 3);
	QUARK_UT_VERIFY(v.vector_hamt_ptr->load_element(0).int_value == 1000);
	QUARK_UT_VERIFY(v.vector_hamt_ptr->load_element(1).int_value == 2000);
	QUARK_UT_VERIFY(v.vector_hamt_ptr->load_element(2).int_value == 3000);

	if(dec_rc(v.vector_hamt_ptr->alloc) == 0){
		dispose_vector_hamt(v);
	}

	QUARK_UT_VERIFY(heap.check_invariant());
	detect_leaks(heap);
}






////////////////////////////////		DICT_CPPMAP_T



QUARK_UNIT_TEST("", "", "", ""){
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

runtime_value_t alloc_dict_cppmap(heap_t& heap, const typeid_t& value_type){
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



QUARK_UNIT_TEST("", "", "", ""){
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

runtime_value_t alloc_dict_hamt(heap_t& heap, const typeid_t& value_type){
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

	heap_alloc_64_t* alloc = alloc_64(heap, 0, typeid_t::make_json(), "JSON");

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

STRUCT_T* alloc_struct(heap_t& heap, std::size_t size, const typeid_t& value_type){
	const auto allocation_count = size_to_allocation_blocks(size);

	heap_alloc_64_t* alloc = alloc_64(heap, allocation_count, value_type, "struct");

	auto vec = reinterpret_cast<STRUCT_T*>(alloc);
	return vec;
}

void dispose_struct(STRUCT_T& s){
	QUARK_ASSERT(sizeof(STRUCT_T) == sizeof(heap_alloc_64_t));
	QUARK_ASSERT(s.check_invariant());

	auto heap = s.alloc.heap;
	dispose_alloc(s.alloc);
	QUARK_ASSERT(heap->check_invariant());
}






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
			return make_runtime_bool(temp == 0 ? false : true);
		}
		runtime_value_t operator()(const typeid_t::int_t& e) const{
			const auto temp = *static_cast<const uint64_t*>(value_ptr);
			return make_runtime_int(temp);
		}
		runtime_value_t operator()(const typeid_t::double_t& e) const{
			const auto temp = *static_cast<const double*>(value_ptr);
			return make_runtime_double(temp);
		}
		runtime_value_t operator()(const typeid_t::string_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}

		runtime_value_t operator()(const typeid_t::json_type_t& e) const{
			return *static_cast<const runtime_value_t*>(value_ptr);
		}
		runtime_value_t operator()(const typeid_t::typeid_type_t& e) const{
			const auto value = *static_cast<const int32_t*>(value_ptr);
			return make_runtime_typeid(value);
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










runtime_type_t lookup_runtime_type(const value_backend_t& backend, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	for(const auto& e: backend.itype_to_typeid){
		if(e.second == type){
			return e.first;
		}
	}
	throw std::exception();
}

typeid_t lookup_type(const value_backend_t& backend, runtime_type_t itype){
	QUARK_ASSERT(backend.check_invariant());

	const auto type = backend.itype_to_typeid.at(itype);
	return type;
}




////////////////////////////////		VALUES


value_backend_t make_test_value_backend(){
	return value_backend_t(
		{},
		{},
		{}
	);
}


const std::pair<typeid_t, struct_layout_t>& find_struct_layout(const value_backend_t& backend, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(type.check_invariant());

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




void retain_vector_cppvector(value_backend_t& backend, runtime_value_t vec, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(is_vector_cppvector(type) || type.is_string());

	inc_rc(vec.vector_cppvector_ptr->alloc);
}
void retain_vector_hamt(value_backend_t& backend, runtime_value_t vec, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(is_vector_hamt(type));

	inc_rc(vec.vector_hamt_ptr->alloc);
}



void retain_dict_cppmap(value_backend_t& backend, runtime_value_t dict, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(is_dict_cppmap(type));

	inc_rc(dict.dict_cppmap_ptr->alloc);
}
void retain_dict_hamt(value_backend_t& backend, runtime_value_t dict, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(is_dict_hamt(type));

	inc_rc(dict.dict_hamt_ptr->alloc);
}

void retain_struct(value_backend_t& backend, runtime_value_t s, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(s.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_rc_value(type));
	QUARK_ASSERT(type.is_struct());

	inc_rc(s.struct_ptr->alloc);
}

void retain_value(value_backend_t& backend, runtime_value_t value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(is_rc_value(type)){
		if(type.is_string()){
			retain_vector_cppvector(backend, value, type);
		}
		else if(is_vector_cppvector(type)){
			retain_vector_cppvector(backend, value, type);
		}
		else if(is_vector_hamt(type)){
			retain_vector_hamt(backend, value, type);
		}
		else if(is_dict_cppmap(type)){
			retain_dict_cppmap(backend, value, type);
		}
		else if(is_dict_hamt(type)){
			retain_dict_hamt(backend, value, type);
		}
		else if(type.is_json()){
			inc_rc(value.json_ptr->alloc);
		}
		else if(type.is_struct()){
			retain_struct(backend, value, type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}






static void release_dict_deep(value_backend_t& backend, runtime_value_t dict0, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(dict0.check_invariant());
	QUARK_ASSERT(type.is_dict());

	if(is_dict_cppmap(type)){
		auto& dict = *dict0.dict_cppmap_ptr;
		if(dec_rc(dict.alloc) == 0){

			//	Release all elements.
			const auto element_type = type.get_dict_value_type();
			if(is_rc_value(element_type)){
				auto m = dict.get_map();
				for(const auto& e: m){
					release_deep(backend, e.second, element_type);
				}
			}
			dispose_dict_cppmap(dict0);
		}
	}
	else if(is_dict_hamt(type)){
		auto& dict = *dict0.dict_hamt_ptr;
		if(dec_rc(dict.alloc) == 0){

			//	Release all elements.
			const auto element_type = type.get_dict_value_type();
			if(is_rc_value(element_type)){
				auto m = dict.get_map();
				for(const auto& e: m){
					release_deep(backend, e.second, element_type);
				}
			}
			dispose_dict_hamt(dict0);
		}
	}
	else{
		QUARK_ASSERT(false);
	}
}

void release_vector_cppvector(value_backend_t& backend, runtime_value_t vec, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_string() || is_vector_cppvector(type));

	if(dec_rc(vec.vector_cppvector_ptr->alloc) == 0){
		//	Release all elements.
		{
			const auto element_type = type.get_vector_element_type();
			if(is_rc_value(element_type)){
				auto element_ptr = vec.vector_cppvector_ptr->get_element_ptr();
				for(int i = 0 ; i < vec.vector_cppvector_ptr->get_element_count() ; i++){
					const auto& element = element_ptr[i];
					release_deep(backend, element, element_type);
				}
			}
		}
		dispose_vector_cppvector(vec);
	}
}

void release_vector_hamt_elements_internal(value_backend_t& backend, runtime_value_t vec, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(is_vector_hamt(type));

	const auto element_type = type.get_vector_element_type();
	for(int i = 0 ; i < vec.vector_hamt_ptr->get_element_count() ; i++){
		const auto& element = vec.vector_hamt_ptr->load_element(i);
		release_deep(backend, element, element_type);
	}
}

static void release_vec_deep(value_backend_t& backend, runtime_value_t vec, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_string() || type.is_vector());

	if(type.is_string()){
		if(dec_rc(vec.vector_cppvector_ptr->alloc) == 0){
			//	String has no elements to release.

			dispose_vector_cppvector(vec);
		}
	}
	else if(is_vector_cppvector(type)){
		release_vector_cppvector(backend, vec, type);
	}
	else if(is_vector_hamt(type)){
		if(is_rc_value(type.get_vector_element_type())){
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

static void release_struct_deep(value_backend_t& backend, STRUCT_T* s, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(s != nullptr);
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_struct());

	if(dec_rc(s->alloc) == 0){
		const auto& struct_def = type.get_struct();
		const auto struct_base_ptr = s->get_data_ptr();

		const auto& struct_layout = find_struct_layout(backend, type);

		int member_index = 0;
		for(const auto& e: struct_def._members){
			if(is_rc_value(e._type)){
				const auto offset = struct_layout.second.offsets[member_index];
				const auto member_ptr = reinterpret_cast<const runtime_value_t*>(struct_base_ptr + offset);
				release_deep(backend, *member_ptr, e._type);
			}
			member_index++;
		}
		dispose_struct(*s);
	}
}

void release_deep(value_backend_t& backend, runtime_value_t value, const typeid_t& type){
	QUARK_ASSERT(backend.check_invariant());
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(is_rc_value(type)){
		if(type.is_string()){
			release_vec_deep(backend, value, type);
		}
		else if(type.is_vector()){
			release_vec_deep(backend, value, type);
		}
		else if(type.is_dict()){
			release_dict_deep(backend, value, type);
		}
		else if(type.is_json()){
			if(dec_rc(value.json_ptr->alloc) == 0){
				dispose_json(*value.json_ptr);
			}
		}
		else if(type.is_struct()){
			release_struct_deep(backend, value.struct_ptr, type);
		}
		else{
			QUARK_ASSERT(false);
		}
	}
}


}	//	floyd

