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


const std::string test_whitespace_chars = " \n\t\r";

///////////////////////////////		seq_t


struct seq_t {
	public: explicit seq_t(const std::string& s);
	public: bool check_invariant() const;

	public: char first_char() const;

	//	Throws if there is no more char to read.
	public: std::string first() const;

	//	Returned string can be "" or shorter than chars if there aren't enough chars.
	public: std::string first(size_t chars) const;

	//	Nothing happens if rest_size() == 0.
	public: seq_t rest() const;

	//	Limited to rest_size().
	public: seq_t rest(size_t skip) const;

	//	Returns first + rest as one string.
	public: std::string get_all() const;

	//	Skips first char.
	public: std::size_t rest_size() const;

	//	If true, there is no first-char and rest is empty.
	public: bool empty() const;

	private: seq_t(const std::shared_ptr<const std::string>& str, std::size_t pos);

	public: bool operator==(const seq_t& other) const;

	//	Returns point to first char and rest.
	const char* c_str() const;

	/////////////		STATE
	private: const char* FIRST_debug = nullptr;
	private: const char* REST_debug = nullptr;
	private: std::shared_ptr<const std::string> _str;
	private: std::size_t _pos;
};


std::pair<std::string, seq_t> read_while(const seq_t& p1, const std::string& match);
std::pair<std::string, seq_t> read_while_not(const seq_t& p1, const std::string& match);





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
std::string read_required_string(const std::string& s, const std::string& wanted);


std::string trim_ends(const std::string& s);

float parse_float(const std::string& pos);


/*
	s[0] == start_char

	Supports nesting.
	Removes outer start_char and end_char

	{ "{ hello }xxx", '{', '}' } => { " hello ", "xxx" }
*/

seq get_balanced_pair(const std::string& s, char start_char, char end_char);



#endif /* text_parser_hpp */
