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



const std::string test_whitespace_chars = " \n\t\r";


///////////////////////////////		seq_t

/*
	This is a magic string were you can easily peek into the beginning and
	also get a new string without the first character(s).

	It shares the actual string data behind the curtains so is efficent.
*/

struct seq_t {
	public: explicit seq_t(const std::string& s);
	public: bool check_invariant() const;


	//	Peek at first char (as C-character). Throws exception if empty().
	public: char first1_char() const;

	//	Peeks at first char (you get a string). Throws if there is no more char to read.
	public: std::string first1() const;
	public: std::string first() const { return first1(); }

	//	Skips 1 char.
	//	You get empty if rest_size() == 0.
	public: seq_t rest1() const;
	public: seq_t rest() const { return rest1(); }



	//	Returned string can be "" or shorter than chars if there aren't enough chars.
	//	Won't throw.
	public: std::string first(size_t chars) const;

	//	Skips n characters.
	//	Limited to rest_size().
	public: seq_t rest(size_t skip) const;


	//	Returns entire string. Equivalent to x.rest(x.size()).
	public: std::string get_s() const;
	public: std::size_t size() const;

	//	If true, there are no more characters.
	public: bool empty() const;

	private: seq_t(const std::shared_ptr<const std::string>& str, std::size_t pos);

	public: bool operator==(const seq_t& other) const;

	//	Returns point to entire string.
	const char* c_str() const;


	/////////////		STATE
	private: const char* FIRST_debug = nullptr;
	private: const char* REST_debug = nullptr;
	private: std::shared_ptr<const std::string> _str;
	private: std::size_t _pos;
};


std::pair<std::string, seq_t> read_while(const seq_t& p1, const std::string& match);
std::pair<std::string, seq_t> read_until(const seq_t& p1, const std::string& match);

//	If p starts with wanted_string, return true and consume those chars. Else return false and the same seq_t.
std::pair<bool, seq_t> peek(const seq_t& p, const std::string& wanted_string);



//	Remove trailing comma, if any.
std::string remove_trailing_comma(const std::string& a);


std::pair<char, std::string> read_char(const std::string& s);

/*
	Returns "rest" if ch is found, else throws exceptions.
*/
seq_t read_required_char(const seq_t& s, char ch);
std::pair<bool, seq_t> read_optional_char(const seq_t& s, char ch);


std::string read_required_string(const std::string& s, const std::string& wanted);


std::string trim_ends(const std::string& s);

float parse_float(const std::string& pos);




std::string quote(const std::string& s);

std::string float_to_string(float value);
std::string double_to_string(double value);


#endif /* text_parser_hpp */
