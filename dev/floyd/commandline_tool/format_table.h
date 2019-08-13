//
//  format_table.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef format_table_hpp
#define format_table_hpp


#include <string>
#include <vector>

struct column_t {
	int width;
	int align;
};

struct line_t {
	std::vector<std::string> columns;
	char pad_char;
};



std::string generate_line(const line_t& line, const std::vector<column_t>& columns);
std::vector<column_t> fit_column(const std::vector<column_t>& start, const line_t& row);
std::vector<column_t> fit_columns(const std::vector<column_t>& start, const std::vector<line_t>& table);
std::vector<std::string> generate_table(const std::vector<line_t>& table, const std::vector<column_t>& columns);



#endif /* format_table_hpp */
