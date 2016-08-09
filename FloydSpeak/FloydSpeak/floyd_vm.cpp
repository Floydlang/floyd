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


#include "parser_evaluator.h"
#include "parser_value.h"
#include "parser_statement.h"
#include "parser_types.h"

#if true


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

using namespace floyd_parser;



//////////////////////////		vm_t


vm_t::vm_t(const floyd_parser::ast_t& ast) :
	_ast(ast)
{
	QUARK_ASSERT(ast.check_invariant());

	auto global_scope = scope_instance_t();
	global_scope._def = ast._global_scope.get();
	_scope_instances.push_back(make_shared<scope_instance_t>(global_scope));
	QUARK_ASSERT(check_invariant());
}

bool vm_t::check_invariant() const {
	QUARK_ASSERT(_ast.check_invariant());
	return true;
}









	/*
		alignment == 8: pos is roundet up untill nearest multiple of 8.
	*/
	std::size_t align_pos(std::size_t pos, std::size_t alignment){
		std::size_t rem = pos % alignment;
		std::size_t add = rem == 0 ? 0 : alignment - rem;
		return pos + add;
	}



QUARK_UNIT_TESTQ("align_pos()", ""){
	QUARK_TEST_VERIFY(align_pos(0, 8) == 0);
	QUARK_TEST_VERIFY(align_pos(1, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(2, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(3, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(4, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(5, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(6, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(7, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(8, 8) == 8);
	QUARK_TEST_VERIFY(align_pos(9, 8) == 16);
}

	std::vector<byte_range_t> calc_struct_default_memory_layout(const types_collector_t& types, const type_def_t& s){
		QUARK_ASSERT(types.check_invariant());
		QUARK_ASSERT(s.check_invariant());

		std::vector<byte_range_t> result;
		std::size_t pos = 0;
		for(const auto& member : s._struct_def->_members) {
			const auto identifier_data = types.lookup_identifier_deep(member._type->to_string());
			const auto type_def = identifier_data->_optional_def;
			QUARK_ASSERT(type_def);

			if(type_def->_base_type == k_int){
				pos = align_pos(pos, 4);
				result.push_back(byte_range_t(pos, 4));
				pos += 4;
			}
			else if(type_def->_base_type == k_bool){
				result.push_back(byte_range_t(pos, 1));
				pos += 1;
			}
			else if(type_def->_base_type == k_string){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(type_def->_base_type == k_struct){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(type_def->_base_type == k_vector){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(type_def->_base_type == k_function){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else{
				QUARK_ASSERT(false);
			}
		}
		pos = align_pos(pos, 8);
		result.insert(result.begin(), byte_range_t(0, pos));
		return result;
	}



/*
QUARK_UNIT_TESTQ("calc_struct_default_memory_layout()", "struct 2"){
	const auto a = types_collector_t();
	const auto b = define_test_struct5(a);
	const auto t = b.resolve_identifier("struct5");
	const auto layout = calc_struct_default_memory_layout(a, *t);
	int i = 0;
	for(const auto it: layout){
		const string name = i == 0 ? "struct" : t->_struct_def->_members[i - 1]._name;
		QUARK_TRACE_SS(it.first << "--" << (it.first + it.second) << ": " + name);
		i++;
	}
	QUARK_TEST_VERIFY(true);
//	QUARK_TEST_VERIFY(s2 == "<struct>{<string>x,<struct_1>y,<string>z}");
}
*/





shared_ptr<const floyd_parser::function_def_t> find_global_function(const vm_t& vm, const string& name){
	return vm._ast._global_scope->_types_collector.resolve_function_type(name);
}

struct vm_stack_frame {
	std::map<string, floyd_parser::value_t> locals;
	int _statement_index;
};

floyd_parser::value_t call_function(const vm_t& vm, shared_ptr<const floyd_parser::function_def_t> f, const vector<floyd_parser::value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	const auto r = floyd_parser::run_function(vm._ast, *f, args);
	return r;
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast({},
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(7));
}


QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = floyd_parser::program_to_ast({},
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}

QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = floyd_parser::program_to_ast({},
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(15));
}



QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast({},
		"int main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("-xyz-"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("-Hello, world!-"));
}

//### Check return value type.

QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast({},
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("--xyz<xyz>--"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("123") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("--123<123>--"));
}



//////////////////////////		TEST HELPERS



pair<vm_t, floyd_parser::value_t> run_program(const string& source){
	QUARK_ASSERT(source.size() > 0);
	auto ast = program_to_ast({}, source);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto r = call_function(vm, f, {});
	return {vm, r};
}


//////////////////////////		TEST STRUCT SUPPORT




QUARK_UNIT_TESTQ("struct", "Can define struct, instantiate it and read member data"){
	const auto a = run_program(
		"struct pixel { string s; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return p.s;"
		"}\n"
	);
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
	QUARK_TEST_VERIFY(a.second == value_t(""));
}

QUARK_UNIT_TESTQ("struct", "Struct member default value"){
	const auto a = run_program(
		"struct pixel { string s = \"one\"; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return p.s;"
		"}\n"
	);
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
	QUARK_TEST_VERIFY(a.second == value_t("one"));
}

QUARK_UNIT_TESTQ("struct", "Nesting structs"){
	const auto a = run_program(
		"struct pixel { string s = \"one\"; }"
		"struct image { pixel background_color; int width; int height; }"
		"string main(){\n"
		"	image i = image_constructor();"
		"	return i.background_color.s;"
		"}\n"
	);
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel"));
	QUARK_TEST_VERIFY(a.first._ast._global_scope->_types_collector.lookup_identifier_shallow("pixel_constructor"));
	QUARK_TEST_VERIFY(a.second == value_t("one"));
}

QUARK_UNIT_TESTQ("struct", "Can use struct as argument"){
	const auto a = run_program(
		"string get_s(pixel p){ return p.s; }"
		"struct pixel { string s = \"two\"; }"
		"string main(){\n"
		"	pixel p = pixel_constructor();"
		"	return get_s(p);"
		"}\n"
	);
	QUARK_TEST_VERIFY(a.second == value_t("two"));
}


/*

	Scenario
*/



//////////////////////////		run_main()


/*
	Quickie that compiles a program and calls its main() with the args.
*/
floyd_parser::value_t run_main(const string& source, const vector<floyd_parser::value_t>& args){
	QUARK_ASSERT(source.size() > 0);
	auto ast = program_to_ast({}, source);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto r = call_function(vm, f, args);
	return r;
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	const auto result = run_main(
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<floyd_parser::value_t>{floyd_parser::value_t("program_name 1 2 3 4")}
	);
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}


#endif
