//
//  format_table.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-13.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "format_table.h"


#include "quark.h"



/*
 from terminaltables import AsciiTable
 table_data = [
	 ['Heading1', 'Heading2'],
	 ['row1 column1', 'row1 column2'],
	 ['row2 column1', 'row2 column2']
 ]
 table = AsciiTable(table_data)
 print table.table
 +--------------+--------------+
 | Heading1     | Heading2     |
 +--------------+--------------+
 | row1 column1 | row1 column2 |
 | row2 column1 | row2 column2 |
 +--------------+--------------+

 table.inner_heading_row_border = False
 print table.table
 +--------------+--------------+
 | Heading1     | Heading2     |
 | row1 column1 | row1 column2 |
 | row2 column1 | row2 column2 |
 +--------------+--------------+

 table.inner_row_border = True
 table.justify_columns[1] = 'right'
 table.table_data[1][1] += '\nnewline'
 print table.table
 +--------------+--------------+
 | Heading1     |     Heading2 |
 +--------------+--------------+
 | row1 column1 | row1 column2 |
 |              |      newline |
 +--------------+--------------+
 | row2 column1 | row2 column2 |
 +--------------+--------------+

 +----------+------------+----------+----------+
 | Header 1 | Header 2   | Header3  | Header 4 |
 +==========+============+==========+==========+
 | row 1    | column 2   | column 3 | column 4 |
 +----------+------------+----------+----------+
 | row 2    | Cells span columns.              |
 +----------+----------------------------------+
 | row 3    | Cells      | - Cells             |
 +----------+ span rows. | - contain           |
 | row 4    |            | - blocks            |
 +----------+------------+---------------------+

 +----------+------+--------+
 |   name   | rank | gender |
 +----------+------+--------+
 |  Jacob   |  1   |  boy   |
 +----------+------+--------+
 | Isabella |  1   |  girl  |
 +----------+------+--------+
 |  Ethan   |  2   |  boy   |
 +----------+------+--------+
 |  Sophia  |  2   |  girl  |
 +----------+------+--------+
 | Michael  |  3   |  boy   |
 +----------+------+--------+

            " ----------------------------------------------------- ",
            "  a                 bb                ccc",
            " ===================================================== ",
            "  1                 2                 3",
            " ----------------------------------------------------- ",
            "  613.23236243236   613.23236243236   613.23236243236",
            " ----------------------------------------------------- ",

Planet      R (km)    mass (x 10^29 kg)
--------  --------  -------------------
Sun         696000           1.9891e+09
Earth         6371        5973.6
Moon          1737          73.5
Mars          3390         641.85


 | item   | qty   |
 |--------|-------|
 | spam   | 42    |
 | eggs   | 451   |
 | bacon  | 0     |



 ======  =====
 item      qty
 ======  =====
 spam       42
 eggs      451
 bacon       0
 ======  =====


 | item   | qty   |
 | name   |       |
 |--------|-------|
 | eggs   | 451   |
 | more   | 42    |
 | spam   |       |


 ===========================  ==========  ===========
 Table formatter                time, μs    rel. time
 ===========================  ==========  ===========
 csv to StringIO                     8.2          1.0
 join with tabs and newlines        10.8          1.3
 asciitable (0.8.0)                205.2         24.9
 tabulate (0.8.5)                  421.7         51.2
 PrettyTable (0.7.2)               787.2         95.6
 texttable (1.6.2)                1123.4        136.4
 ===========================  ==========  ===========


 |MODULE |FUNCTION |SCENARIO             |RESULT                  |
 |-------|---------|---------------------|------------------------|
 |       |subset() |skip first character |Floyd assertion failed. |
 |-------|---------|---------------------|------------------------|


https://beautifultable.readthedocs.io/en/latest/quickstart.html


............................
:   name   : rank : gender :
............................
:  Jacob   :  1   :  boy   :
: Isabella :  1   :  girl  :
:  Ethan   :  2   :  boy   :
:  Sophia  :  2   :  girl  :
: Michael  :  3   :  boy   :
............................


*/



std::string generate_line(const line_t& line, const std::vector<column_t>& columns){
	QUARK_ASSERT(columns.size() == line.columns.size());

	const auto bar_char = line.bar_char == 0x00 ? std::string("") : std::string(1, line.bar_char);

	std::string acc = bar_char;
	for(int c = 0 ; c < line.columns.size() ; c++){
		const auto s0 = line.columns[c];
		const auto s = std::string(columns[c].margin, line.pad_char) + s0 + std::string(columns[c].margin, line.pad_char);
		const auto wanted_width = columns[c].width + 1;

		const auto pad_count = wanted_width - size(s);
		const auto pad_string = std::string(pad_count, line.pad_char);

		const auto s2 = columns[c].align <= 0 ? s + pad_string : pad_string + s;

		acc = acc + s2 + bar_char;
	}
	return acc;
}

std::vector<column_t> fit_column(const std::vector<column_t>& start, const line_t& row){
	std::vector<column_t> acc = start;
	QUARK_ASSERT(row.columns.size() == acc.size());

	for(int column = 0 ; column < acc.size() ; column++){
		const auto acc_width = acc[column].width;
		const auto table_width = (int)row.columns[column].size() + start[column].margin * 2;
		acc[column].width = std::max(acc_width, table_width);
	}
	return acc;
}

std::vector<column_t> fit_columns(const std::vector<column_t>& start, const std::vector<line_t>& table){
	std::vector<column_t> acc = start;
	for(const auto& row: table){
		acc = fit_column(acc, row);
	}
	return acc;
}

std::vector<std::string> generate_table(const std::vector<line_t>& table, const std::vector<column_t>& columns){
	std::vector<std::string> report;
	for(const auto& line: table){
		const auto& line2 = generate_line(line, columns);
		report.push_back(line2);
	}
	return report;
}






std::string generate_table_type1(const std::vector<std::string>& headings, const std::vector<std::vector<std::string>>& matrix){
	const auto column_count = headings.size();
	const auto splitter = line_t( std::vector<std::string>(column_count, ""), '-', '|');

	std::vector<line_t> table = {
		line_t( headings, ' ', '|'),
		splitter
	};
	for(const auto& line: matrix){
		QUARK_ASSERT(line.size() == column_count);

		const auto line2 = line_t { line, ' ', '|' };
		table.push_back(line2);
	}
	table.push_back(splitter);

	const auto columns0 = std::vector<column_t>(column_count, column_t{ 0, -1, 0 });
	const auto columns = fit_columns(columns0, table);
	const auto r = generate_table(table, columns);

	std::stringstream ss;
	for(const auto& e: r){
		ss << e << std::endl;
	}
	return ss.str();
}
