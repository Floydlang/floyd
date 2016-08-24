//
//  llvm_code_gen.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef llvm_code_gen_hpp
#define llvm_code_gen_hpp


#include "quark.h"
#include <vector>

#include "parser_types_collector.h"

//////////////////////////////////////		type_identifier_t


typedef std::pair<std::size_t, std::size_t> byte_range_t;

/*
	s: all types must be fully defined, deeply, for this function to work.
	result, item[0] the memory range for the entire struct.
			item[1] the first member in the struct. Members may be mapped in any order in memory!
			item[2] the first member in the struct.
*/
std::vector<byte_range_t> calc_struct_default_memory_layout(const floyd_parser::types_collector_t& types, const floyd_parser::type_def_t& s);



#endif /* llvm_code_gen_hpp */
