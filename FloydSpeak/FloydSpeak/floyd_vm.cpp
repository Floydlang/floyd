//
//  floyd_vm.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_vm.hpp"


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




shared_ptr<make_function_expression_t> find_global_function(const vm_t& vm, const string& name){
	const auto it = std::find_if(vm._ast._top_level_statements.begin(), vm._ast._top_level_statements.end(), [=] (const statement_t& s) { return s._bind_statement != nullptr && s._bind_statement->_identifier == name; });
	if(it == vm._ast._top_level_statements.end()){
		return nullptr;
	}

	const auto f = it->_bind_statement->_expression._make_function_expression;
	return f;
}

pair<value_t, vm_t> run_function(const vm_t& vm, const make_function_expression_t& f, const vector<value_t>& args){
	const auto body = f._body;

	std::map<string, value_t> locals;
	int statement_index = 0;
	while(statement_index < body._statements.size()){
		const auto statement = body._statements[statement_index];
		if(statement._bind_statement){
			const auto s = statement._bind_statement;
			const auto name = s->_identifier;
			if(locals.count(name) != 0){
				throw std::runtime_error("local constant already exists");
			}
			const auto result = evaluate_compiletime_constants(s->_expression);
			if(!result._constant){
				throw std::runtime_error("unknown variables");
			}
			locals[name] = *result._constant;
		}
		else if(statement._return_statement){
//			const auto result = evaluate_compiletime_constants()
			return pair<value_t, vm_t>(value_t(11), vm);
		}
		else{
			QUARK_ASSERT(false);
		}
		statement_index++;
	}
	throw std::runtime_error("function missing return statement");
}

value_t run(const ast_t& ast){
	vm_t vm(ast);

	const auto main_function = find_global_function(vm, "main");
	const auto r = run_function(vm, *main_function, vector<value_t>{value_t("program_name 1 2 3 4")});
	return r.first;
}

QUARK_UNIT_TEST("", "run()", "", ""){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);

	value_t result = run(ast);
//	QUARK_TEST_VERIFY(result == value_t(7));
}
