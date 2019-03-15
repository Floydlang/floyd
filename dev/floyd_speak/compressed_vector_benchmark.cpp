
#include "gtest/gtest.h"
#include "benchmark/benchmark.h"

#include "variable_length_quantity.h"
#include "hardware_caps.h"

#include <iostream>
#include <vector>

#include "quark.h"


static const size_t k_64M = 1 << 26;
static const size_t k_32M = 1 << 25;
static const size_t k_testdata_size = k_32M;

//	state.PauseTiming();
//	state.ResumeTiming();



////////////////////////////////		HELPERS




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




////////////////////////////////		BENCHMARK -- vector<int32_> vs variable_length_equantity





static void BM_read_vector_uint32(benchmark::State& state) {
	const auto element_count = state.range(0);
	std::vector<int32_t> data = make_random_vector_int32(element_count);

	for (auto _ : state) {
		long sum = 0;
		for (int j = 0; j < element_count; ++j){
			const auto value = data[j];
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	state.counters["bytes/test"] = element_count * sizeof(int32_t);
	state.SetBytesProcessed(state.iterations() * element_count * sizeof(int32_t));
}
BENCHMARK(BM_read_vector_uint32)->Range((k_testdata_size >> 16) / sizeof(uint32_t), k_testdata_size / sizeof(uint32_t));


//	Fast integer array compression: https://github.com/lemire/FastPFor

static void BM_read_vlq_vector(benchmark::State& state) {
	const auto element_count = state.range(0);
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(element_count));

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

//	state.SetItemsProcessed(state.iterations() * data.size());

	state.SetBytesProcessed(state.iterations() * element_count * sizeof(int32_t));
}


static void BM_read_vlq_vector2(benchmark::State& state) {
	const auto element_count = state.range(0);
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(element_count));

	for (auto _ : state) {
		long sum = 0;
		size_t pos = 0;
		for (int j = 0; j < data.size(); ++j){
			const auto value = unpack_vlq2(&data[0], pos);
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	state.SetBytesProcessed(state.iterations() * element_count * sizeof(int32_t));
}
//BENCHMARK(BM_read_vlq_vector2)ARGS_XYZ;


static void BM_read_vector_uint8(benchmark::State& state) {
//	sleep(3);
	const auto element_count = state.range(0);
	std::vector<uint8_t> data = use_low_8bits(make_random_vector_int32(element_count));
//	std::vector<uint8_t> data(element_count, 0x13);

	for (auto _ : state) {
		long sum = 0;
		for (int j = 0; j < element_count; ++j){
			const uint32_t value = data[j];
			sum = sum + value;
		}
        benchmark::DoNotOptimize(sum);
	}

	state.counters["bytes/test"] = element_count * sizeof(int8_t);
	state.SetBytesProcessed(state.iterations() * element_count * sizeof(int8_t));
}
BENCHMARK(BM_read_vector_uint8)->Range((k_testdata_size >> 16) / sizeof(uint8_t), k_testdata_size / sizeof(uint8_t));


#if 0
static void BM_read_vlq_vector4(benchmark::State& state) {
	const auto element_count = state.range(0);
	std::vector<uint8_t> data = pack_vlq_vector(make_random_vector_int32(element_count));

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
	state.SetBytesProcessed(state.iterations() * element_count * sizeof(int32_t));
}
#endif




int main(int argc, char** argv) {
	const auto caps = floyd::read_hardware_caps();
	const auto caps_string = get_hardware_caps_string(caps);
	std::cout << caps_string;

	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
	::benchmark::RunSpecifiedBenchmarks();
}
//BENCHMARK_MAIN();

