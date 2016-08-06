//
//  text_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef text_parser_hpp
#define text_parser_hpp

#include <string>


typedef std::pair<std::string, std::string> seq;


///////////////////////////////		seq2


struct seq2 {
	public: seq2(const std::string& s);
	public: bool check_invariant() const;

	public: char first() const;
	public: const seq2 rest() const;
	public: std::size_t rest_size() const;

	private: seq2(const std::shared_ptr<const std::string>& str, std::size_t rest_pos);


	/////////////		STATE
	private: const std::shared_ptr<const std::string> _str;
	private: std::size_t _rest_pos;
};


//	Remove trailing comma, if any.
std::string remove_trailing_comma(const std::string& a);

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
