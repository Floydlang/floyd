//
//  floyd_llvm_codegen.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-03-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_codegen.h"
#include "ast_value.h"

#include "quark.h"

int main_fibonacci_loop(int argc, char* argv[]);
int main_fibonacci_resursive(int argc, char* argv[]);

void test_llvm(){
}


QUARK_UNIT_TEST("", "main_fibonacci_loop()", "", ""){
	char app_name[] = "dummy_app";
	char num[] = "20";
	char* args[] = { app_name, num };
	main_fibonacci_loop(2, args);
}

int run_using_llvm(const std::string& program, const std::string& file, const std::vector<floyd::value_t>& args){

		char app_name[] = "dummy_app";
		char num[] = "20";
		char* args2[] = { app_name, num };
		main_fibonacci_loop(2, args2);
	return 0;
}
