//
//  llvm_code_gen.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "llvm_code_gen.h"

#include "parser_ast.h"

using namespace floyd_parser;


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
		const auto struct_def = s.get_struct_def();
		for(const auto& member : struct_def->_members) {
			const auto type_def = types.resolve_identifier(member._type->to_string());
			QUARK_ASSERT(type_def);

			base_type base = type_def->get_type();
			if(base == k_int){
				pos = align_pos(pos, 4);
				result.push_back(byte_range_t(pos, 4));
				pos += 4;
			}
			else if(base == k_bool){
				result.push_back(byte_range_t(pos, 1));
				pos += 1;
			}
			else if(base == k_string){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(base == k_struct){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(base == k_vector){
				pos = align_pos(pos, 8);
				result.push_back(byte_range_t(pos, 8));
				pos += 8;
			}
			else if(base == k_function){
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

