//
//  compiler_basics.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef compiler_basics_hpp
#define compiler_basics_hpp

#include "quark.h"

namespace floyd {

////////////////////////////////////////		location2_t


struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
};


////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;


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


void throw_compiler_error(const location_t& location, const std::string& message) __dead2;
inline void throw_compiler_error(const location_t& location, const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(location, location2_t("", 0, 0, 0, 0, ""), message);
}

void throw_compiler_error(const location_t& location, const location2_t& location2, const std::string& message) __dead2;
inline void throw_compiler_error(const location_t& location, const location2_t& location2, const std::string& message){
	throw compiler_error(location, location2, message);
}

}	// floyd

#endif /* compiler_basics_hpp */
