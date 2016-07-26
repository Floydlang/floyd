//
//  text_parser.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef text_parser_hpp
#define text_parser_hpp




#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "parser_types.h"
#include "parser_primitives.h"



typedef std::pair<std::string, std::string> seq;

struct seq2 {
	seq2 substr(std::size_t pos, std::size_t count = std::string::npos){
		return seq2();
	}
	
	const std::shared_ptr<const std::string> _str;
	std::size_t _pos;
};



seq read_while(const std::string& s, const std::string& match);
seq read_until(const std::string& s, const std::string& match);



std::pair<char, std::string> read_char(const std::string& s);

/*
	Returns "rest" if ch is found, else throws exceptions.
*/
std::string read_required_char(const std::string& s, char ch);
std::pair<bool, std::string> read_optional_char(const std::string& s, char ch);


bool peek_compare_char(const std::string& s, char ch);

bool peek_string(const std::string& s, const std::string& peek);


std::string trim_ends(const std::string& s);



#endif /* text_parser_hpp */
