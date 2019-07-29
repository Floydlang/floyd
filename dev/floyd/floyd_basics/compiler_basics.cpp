//
//  compiler_basics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_basics.h"

#include "text_parser.h"
#include "parser_primitives.h"

namespace floyd {


const location_t k_no_location(std::numeric_limits<std::size_t>::max());




using std::string;



//////////////////////////////////////////////////		base_type


string base_type_to_opcode(const base_type t){
	if(t == base_type::k_undefined){
		return "**undef**";
	}
	else if(t == base_type::k_any){
		return "any";
	}

	else if(t == base_type::k_void){
		return "void";
	}
	else if(t == base_type::k_bool){
		return "bool";
	}
	else if(t == base_type::k_int){
		return "int";
	}
	else if(t == base_type::k_double){
		return "double";
	}
	else if(t == base_type::k_string){
		return "string";
	}
	else if(t == base_type::k_json){
		return "json";
	}

	else if(t == base_type::k_typeid){
		return "typeid";
	}

	else if(t == base_type::k_struct){
		return "struct";
	}
	else if(t == base_type::k_vector){
		return "vector";
	}
	else if(t == base_type::k_dict){
		return "dict";
	}
	else if(t == base_type::k_function){
		return "func";
	}
	else if(t == base_type::k_unresolved){
		return "**unknown-identifier**";
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_undefined) == "**undef**");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_any) == "any");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_void) == "void");
}

QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_bool) == "bool");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_int) == "int");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_double) == "double");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_string) == "string");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_json) == "json");
}

QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_typeid) == "typeid");
}

QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_struct) == "struct");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_vector) == "vector");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_dict) == "dict");
}
QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_function) == "func");
}

QUARK_UNIT_TEST("", "base_type_to_opcode(base_type)", "", ""){
	QUARK_TEST_VERIFY(base_type_to_opcode(base_type::k_unresolved) == "**unknown-identifier**");
}

base_type opcode_to_base_type(const std::string& s){
	QUARK_ASSERT(s != "");

	if(s == "**undef**"){
		return base_type::k_undefined;
	}
	else if(s == "any"){
		return base_type::k_any;
	}
	else if(s == "void"){
		return base_type::k_void;
	}
	else if(s == "bool"){
		return base_type::k_bool;
	}
	else if(s == "int"){
		return base_type::k_int;
	}
	else if(s == "double"){
		return base_type::k_double;
	}
	else if(s == "string"){
		return base_type::k_string;
	}
	else if(s == "typeid"){
		return base_type::k_typeid;
	}
	else if(s == "json"){
		return base_type::k_json;
	}

	else if(s == "struct"){
		return base_type::k_struct;
	}
	else if(s == "vector"){
		return base_type::k_vector;
	}
	else if(s == "dict"){
		return base_type::k_dict;
	}
	else if(s == "func"){
		return base_type::k_function;
	}
	else if(s == "**unknown-identifier**"){
		return base_type::k_unresolved;
	}

	else{
		QUARK_ASSERT(false);
	}
}


void ut_verify(const quark::call_context_t& context, const base_type& result, const base_type& expected){
	ut_verify(
		context,
		base_type_to_opcode(result),
		base_type_to_opcode(expected)
	);
}



//////////////////////////////////////////////////		location_t





std::pair<json_t, location_t> unpack_loc(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;

		const auto elements = s.get_array();
		const std::vector<json_t> elements2 = { elements.begin() + 1, elements.end() };
		const auto statement = json_t::make_array(elements2);

		return { statement, source_offset };
	}
	else{
		return { s, k_no_location };
	}
}

location_t unpack_loc2(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;
		return source_offset;
	}
	else{
		return k_no_location;
	}
}



//////////////////////////////////////////////////		MACROS



void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}


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
	const auto line = split.size() > 0 ? split.front() : "";
	return line;
}
QUARK_UNIT_TEST("", "cleanup_line_snippet()", "", ""){
	ut_verify(QUARK_POS, cleanup_line_snippet(" \tabc\n\a"), "abc");
}

location2_t make_loc2(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc, int line_index){
	const auto start = lookup[line_index];
	const auto end = lookup[line_index + 1];
	const auto line = cleanup_line_snippet(program.substr(start, end - start));
	const auto column = loc.offset - start;
	return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
}


location2_t find_loc_info(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc){
	QUARK_ASSERT(lookup.size() >= 2);
	QUARK_ASSERT(loc.offset <= lookup.back());

	int last_line_offset = lookup.back();
	QUARK_ASSERT(last_line_offset >= loc.offset);

	//	 EOF? Use program's last non-blank line.
	if(last_line_offset == loc.offset){
		auto line_index = static_cast<int>(lookup.size()) - 2;
		auto loc2 = make_loc2(program, lookup, file, loc, line_index);
		while(line_index >= 0 && loc2.line ==""){
			line_index--;
			loc2 = make_loc2(program, lookup, file, loc, line_index);
		}
		return loc2;
	}
	else{
		int line_index = 0;
		while(lookup[line_index + 1] <= loc.offset){
			line_index++;
			QUARK_ASSERT(line_index < lookup.size());
		}
		return make_loc2(program, lookup, file, loc, line_index);
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
	const auto line_snippet = parser::reverse(parser::skip_whitespace(parser::reverse(loc2.line)));
	what2 << what1 << " Line: " << std::to_string(loc2.line_number + 1) << " \"" << line_snippet << "\"";
	if(loc2.source_file_path.empty() == false){
		what2 << " file: " << loc2.source_file_path;
	}

	return { loc2, what2.str() };
}




void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
}

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify(context, result.first, expected.first);
		ut_verify(context, result.second.str(), expected.second.str());
	}
}




////////////////////////////////	make_ast_node()



json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

QUARK_UNIT_TEST("", "make_ast_node()", "", ""){
	const auto r = make_ast_node(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify(QUARK_POS, r, json_t::make_array({ 1234.0, "def-struct" }));
}


}	// floyd

