//
//  compiler_basics.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_basics.h"

#include "text_parser.h"


namespace floyd {


const location_t k_no_location(std::numeric_limits<std::size_t>::max());






//	Return one entry per source line PLUS one extra end-marker. int tells byte offset of files that maps to this line-start.
//	Never returns empty vector, at least 2 elements.
std::vector<int> make_location_lookup(const std::string& source){
	if(source.empty()){
		return { 0 };
	}
	else{
		std::vector<int> result;
		int char_index = 0;
		result.push_back(char_index);
		for(const auto& ch: source){
			char_index++;

			if(ch == '\n' || ch == '\r'){
				result.push_back(char_index);
			}
		}
		const auto back_char = source.back();
		if(back_char == '\r' || back_char == '\n'){
			return result;
		}
		else{
			int end_index = static_cast<int>(source.size());
			result.push_back(end_index);
			return result;
		}
	}
}

QUARK_UNIT_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0 }));
}
QUARK_UNIT_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 3 }));
}
QUARK_UNIT_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa\nbbb\n");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 4, 8 }));
}

const std::string cleanup_line_snippet(const std::string& s){
	const auto line1 = skip(seq_t(s), "\t ");
	const auto split = split_on_chars(line1, "\n\r");
	QUARK_ASSERT(split.size() > 0);
	const auto line = split.front();
	return line;
}
QUARK_UNIT_TEST("", "cleanup_line_snippet()", "", ""){
	ut_verify(QUARK_POS, cleanup_line_snippet(" \tabc\n\a"), "abc");
}

location2_t find_loc_info(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc){
	QUARK_ASSERT(lookup.size() >= 2);
	QUARK_ASSERT(loc.offset <= lookup.back());

	int back = lookup.back();
	QUARK_ASSERT(back >= loc.offset);

	//	 EOF?
	if(back == loc.offset){
		const auto line_index = static_cast<int>(lookup.size()) - 1;
		const auto start = lookup[line_index];
		const auto end = lookup[line_index];
		const auto line = program.substr(start, end - start);
		const auto column = loc.offset - start;
		return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
	}
	else{
		int line_index = 0;
		while(lookup[line_index + 1] <= loc.offset){
			line_index++;
			QUARK_ASSERT(line_index < lookup.size());
		}

		const auto start = lookup[line_index];
		const auto end = lookup[line_index + 1];
		const auto line = cleanup_line_snippet(program.substr(start, end - start));
		const auto column = loc.offset - start;
		return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
	}
}

QUARK_UNIT_TEST("", "make_location_lookup()", "", ""){
	const std::vector<int> lookup = { 0, 1, 2, 18, 19, 21 };
	const std::string program = R"(

			{ let a = 10

		)";
	const auto r = find_loc_info(program, lookup, "path.txt", location_t(21));
}

location2_t find_source_line(const compilation_unit_t& cu, const location_t& loc){
	const auto program_lookup = make_location_lookup(cu.program_text);

	if(cu.prefix_source != ""){
		const auto corelib_lookup = make_location_lookup(cu.prefix_source);
		const auto corelib_end_offset = corelib_lookup.back();
		if(loc.offset < corelib_end_offset){
			return find_loc_info(cu.prefix_source, corelib_lookup, "corelib", loc);
		}
		else{
			const auto program_loc = location_t(loc.offset - corelib_end_offset);
			const auto loc2 = find_loc_info(cu.program_text, program_lookup, cu.source_file_path, program_loc);
			const auto result = location2_t(
				loc2.source_file_path,
				loc2.line_number,
				loc2.column,
				loc2.start,
				loc2.end,
				loc2.line,
				location_t(loc2.loc.offset + corelib_end_offset)
			);
			return result;
		}
	}
	else{
		return find_loc_info(cu.program_text, program_lookup, cu.source_file_path, loc);
	}
}



/*	const auto line_numbers2 = mapf<int>(
		statements_pos.line_numbers,
		[&](const auto& e){
			return e - pre_line_count;
		}
	);
*/



std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e){
	const auto loc2 = find_source_line(cu, e.location);
	const auto what1 = std::string(e.what());

	std::stringstream what2;
	what2 << what1 << " Line: " << std::to_string(loc2.line_number + 1) << " \"" << loc2.line << "\"";
	if(loc2.source_file_path.empty() == false){
		what2 << " file: " << loc2.source_file_path;
	}

	return { loc2, what2.str() };
}


}

