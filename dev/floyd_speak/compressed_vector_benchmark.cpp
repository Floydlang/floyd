//
//  main.cpp
//  megastruct
//
//  Created by Marcus Zetterquist on 2019-01-28.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"

#include "variable_length_quantity.h"

#include <iostream>
#include <vector>



static const size_t k_test_element_count = 1 << 26;
//static const size_t k_test_element_count = 1 << 24;
#define ARGS_XYZ	->Arg(1 << 16)->Arg(k_test_element_count)




////////////////////////////////		BENCHMARK -- vector<int32_> vs variable_length_equantity




__attribute__((noinline))
std::vector<int32_t> make_random_vector_int32(int64_t count) {
	std::vector<int32_t> s;
	s.reserve(count);
	for (int i = 0; i < count; ++i){
//		const int32_t value = rand() % 65535;
		const int32_t value = rand() % 127;
		s.insert(s.end(), value);
	}
	return s;
}




static void BM_read_vector(benchmark::State& state) {
	std::vector<int32_t> data = make_random_vector_int32(state.range(0));
	for (auto _ : state) {
		long sum = 0;
		for (int j = 0; j < data.size(); ++j){
			const auto value = data[j];
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	//	Packed bytes.
	state.SetItemsProcessed(state.iterations() * data.size() * sizeof(int32_t));

	//	Unpacked bytes.
	state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(int32_t));
}

//BENCHMARK(BM_read_vector)->Ranges({{1 << 16, 4 << 16}, {1, 1}});
//BENCHMARK(BM_read_vector)->Arg(1 << 16);
//BENCHMARK(BM_read_vector)->RangeMultiplier(2)->Range(1 << 1, k_test_element_count);
BENCHMARK(BM_read_vector)ARGS_XYZ;



__attribute__((noinline))
std::vector<uint8_t> pack_vlq_vector(const std::vector<int32_t>& v){
	std::vector<uint8_t> result;
	result.reserve(static_cast<std::size_t>(v.size() * 2.5f));

	for(const auto e: v){
		const auto packed = pack_vlq(e);
		result.insert(result.end(), packed.begin(), packed.end());
	}

#if 0
	const size_t input_bytes = v.size() * sizeof(int32_t);
	const size_t output_bytes = result.size();
	const double compression = (double)output_bytes / (double)input_bytes;
	std::cout
		<< "pack_vlq_vector()"
		<< " input bytes: " << input_bytes
		<< " output bytes: " << output_bytes
		<< " out/in size: " << (compression * 100.0) << "%"
		<< std::endl;
#endif
	return result;
}



//	Fast integer array compression: https://github.com/lemire/FastPFor

static void BM_read_vlq_vector(benchmark::State& state) {
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(state.range(0)));

	for (auto _ : state) {
		long sum = 0;
		size_t pos = 0;
		for (int j = 0; j < data.size(); ++j){
			const auto e = unpack_vlq(&data[pos]);
			const auto value = e.first;
			pos = pos + e.second;
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	//	Packed bytes.
	state.SetItemsProcessed(state.iterations() * data.size());

	//	Unpacked bytes.
	state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(int32_t));
}

//BENCHMARK(BM_read_vector)->Ranges({{1 << 16, 4 << 16}, {1, 1}});
//BENCHMARK(BM_read_vector)->Arg(1 << 16);
//BENCHMARK(BM_read_vlq_vector)->RangeMultiplier(2)->Range(1 << 1, k_test_element_count);
BENCHMARK(BM_read_vlq_vector)ARGS_XYZ;




static void BM_read_vlq_vector2(benchmark::State& state) {
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(state.range(0)));

	for (auto _ : state) {
		long sum = 0;
		size_t pos = 0;
		for (int j = 0; j < data.size(); ++j){
			const auto value = unpack_vlq2(&data[0], pos);
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	//	Packed bytes.
	state.SetItemsProcessed(state.iterations() * data.size());

	//	Unpacked bytes.
	state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(int32_t));
}

BENCHMARK(BM_read_vlq_vector2)ARGS_XYZ;




__attribute__((noinline))
std::vector<uint8_t> use_low_8bits(const std::vector<int32_t>& v){
	std::vector<uint8_t> result;
	result.reserve(static_cast<std::size_t>(v.size() * 2.5f));

	for(const auto e: v){
		const uint8_t packed = e & 0xff;
		result.push_back(packed);
	}

#if 0
	const size_t input_bytes = v.size() * sizeof(int32_t);
	const size_t output_bytes = result.size();
	const double compression = (double)output_bytes / (double)input_bytes;
	std::cout
		<< "pack_vlq_vector()"
		<< " input bytes: " << input_bytes
		<< " output bytes: " << output_bytes
		<< " out/in size: " << (compression * 100.0) << "%"
		<< std::endl;
#endif
	return result;
}

static void BM_read_vlq_vector3(benchmark::State& state) {
	std::vector<uint8_t> data = use_low_8bits(make_random_vector_int32(state.range(0)));

	for (auto _ : state) {
		long sum = 0;
		size_t pos = 0;
		for (int j = 0; j < data.size(); ++j){
			const uint32_t value = data[pos];
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	//	Packed bytes.
	state.SetItemsProcessed(state.iterations() * data.size());

	//	Unpacked bytes.
	state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(int32_t));
}

BENCHMARK(BM_read_vlq_vector3)ARGS_XYZ;





#if 0
static void BM_read_vlq_vector4(benchmark::State& state) {
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(state.range(0)));

	for (auto _ : state) {
		long sum = 0;
		size_t pos = 0;
		for (int j = 0; j < data.size(); ++j){
			const auto value = unpack_vlq2(&data[0], pos);
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	//	Packed bytes.
	state.SetItemsProcessed(state.iterations() * data.size());

	//	Unpacked bytes.
	state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(int32_t));
}

BENCHMARK(BM_read_vlq_vector4)ARGS_XYZ;
#endif



BENCHMARK_MAIN();

