//
//  compiler_basics.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef compiler_basics_hpp
#define compiler_basics_hpp

/*
	Infrastructure primitives under the compiler.
*/

#include "quark.h"

namespace floyd {


////////////////////////////////////////		function_id_t


struct function_id_t {
	std::string name;
};

inline bool operator==(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name == rhs.name;
}
inline bool operator!=(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name != rhs.name;
}
inline bool operator<(const function_id_t& lhs, const function_id_t& rhs){
	return lhs.name < rhs.name;
}

const function_id_t k_no_function_id = function_id_t { "" };



////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	location_t(const location_t& other) = default;
	location_t& operator=(const location_t& other) = default;

	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;



////////////////////////////////////////		location2_t


struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line, const location_t& loc) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line),
		loc(loc)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
	location_t loc;
};


////////////////////////////////////////		compilation_unit_t


struct compilation_unit_t {
	std::string prefix_source;
	std::string program_text;
	std::string source_file_path;
};



////////////////////////////////////////		compiler_error


class compiler_error : public std::runtime_error {
	public: compiler_error(const location_t& location, const location2_t& location2, const std::string& message) :
		std::runtime_error(message),
		location(location),
		location2(location2)
	{
	}

	public: location_t location;
	public: location2_t location2;
};



////////////////////////////////////////		throw_compiler_error()


void throw_compiler_error_nopos(const std::string& message) __dead2;
inline void throw_compiler_error_nopos(const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(k_no_location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}



void throw_compiler_error(const location_t& location, const std::string& message) __dead2;
inline void throw_compiler_error(const location_t& location, const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}

void throw_compiler_error(const location2_t& location2, const std::string& message) __dead2;
inline void throw_compiler_error(const location2_t& location2, const std::string& message){
	throw compiler_error(location2.loc, location2, message);
}


location2_t find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc);


////////////////////////////////////////		refine_compiler_error_with_loc2()


std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e);




////////////////////////////////	MISSING FEATURES



void NOT_IMPLEMENTED_YET() __dead2;
void UNSUPPORTED() __dead2;


}	// floyd

#endif /* compiler_basics_hpp */
