//
//  floyd_llvm.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm.h"

#include "compiler_helpers.h"
#include "pass3.h"

namespace floyd {



std::unique_ptr<llvm_ir_program_t> compile_to_ir_helper(llvm_instance_t& instance, const compilation_unit_t& cu){
	QUARK_ASSERT(instance.check_invariant());

	const auto pass3 = compile_to_sematic_ast__errors(cu);
	auto bc = generate_llvm_ir_program(instance, pass3, cu.source_file_path);
	return bc;
}


int64_t run_using_llvm_helper(const std::string& program_source, const std::string& file, const std::vector<std::string>& main_args){
	const auto cu = floyd::make_compilation_unit_nolib(program_source, file);
	const auto pass3 = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, pass3, file);
	const auto result = run_llvm_program(instance, *program, main_args);
	QUARK_TRACE_SS("Fib = " << result);
	return result;
}

}	//	floyd
