//
//  floyd_llvm.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_llvm_hpp
#define floyd_llvm_hpp


#include <string>
#include <vector>

#include "compiler_helpers.h"
#include "compiler_basics.h"

namespace floyd {

struct config_t;
struct bench_t;
struct run_output_t;
struct benchmark_result2_t;


//	Compiles and runs the program. Returns results.
run_output_t run_program_helper(
	const std::string& program_source,
	const std::string& file,
	compilation_unit_mode mode,
	const compiler_settings_t& settings,
	const std::vector<std::string>& main_args,
	bool run_tests
);

std::vector<bench_t> collect_benchmarks_source(const std::string& program_source, const std::string& file, compilation_unit_mode mode, const compiler_settings_t& settings);
std::vector<benchmark_result2_t> run_benchmarks_source(const std::string& program_source, const std::string& file, compilation_unit_mode mode, const compiler_settings_t& settings, const std::vector<std::string>& tests);

}	// floyd

#endif /* floyd_llvm_hpp */
