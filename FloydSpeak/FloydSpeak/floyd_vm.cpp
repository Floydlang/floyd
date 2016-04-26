//
//  floyd_vm.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_vm.h"


#include "steady_vector.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


shared_ptr<const function_def_expr_t> find_global_function(const vm_t& vm, const string& name){
/*
	const auto it = std::find_if(vm._ast._top_level_statements.begin(), vm._ast._top_level_statements.end(), [=] (const statement_t& s) { return s._bind_statement != nullptr && s._bind_statement->_identifier == name; });
*/
	const auto it = vm._ast._functions._functions.find(name);
	if(it == vm._ast._functions._functions.end()){
		return nullptr;
	}

	return it->second;
}



struct vm_stack_frame {
	std::map<string, value_t> locals;
	int _statement_index;
};

value_t run(const ast_t& ast){
	vm_t vm(ast);

	const auto main_function = find_global_function(vm, "main");
	const auto r = run_function(vm._ast._functions, *main_function, vector<value_t>{value_t("program_name 1 2 3 4")});
	return r;
}

QUARK_UNIT_TESTQ("run()", "minimal program"){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t(7));
}

QUARK_UNIT_TESTQ("run()", "minimal program 2"){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t("123456"));
}

QUARK_UNIT_TESTQ("run()", "define additional function, call it several times"){
	auto ast = program_to_ast(
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t(15));
}
