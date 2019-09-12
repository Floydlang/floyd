//
//  command_line_parser.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef command_line_parser_hpp
#define command_line_parser_hpp


#include <string>
#include <vector>
#include <map>


////////////////////////////		HELPERS


std::vector<std::string> args_to_vector(int argc, const char * argv[]);
std::vector<std::string> split_command_line(const std::string& s);



////////////////////////////		COMMAND LINE ARGUMENTS


struct flag_info_t {
	enum class etype {
		simple_flag,
		flag_with_parameter,
		unknown_flag,
		flag_missing_parameter
	};
	etype type;
	std::string parameter;
};

inline bool operator==(const flag_info_t& lhs, const flag_info_t& rhs){
	return lhs.type == rhs.type && lhs.parameter == rhs.parameter;
}


//	Flags: x: means x supports parameter.
struct command_line_args_t {
	std::string command;
	std::string subcommand;

	//	Key: flag character, value: parameter or ""
	std::map<std::string, flag_info_t> flags;


	//	Extra arguments, after the flags have been parsed.
	std::vector<std::string> extra_arguments;
};





/*
	Example:
	INPUT: git commit -m "Commit message"
	RESULT:
		command: "git"
		subcommand: "commit"
		flags
			m: ""
		extra_arguments
			"Commit message"

	flags: specify each flag character to support, like "xit" to support -x, -i, -t.
	Parser supports merged flags, like -xi
	Supports flags with arguments, like -a:hello

	Does NOT support long flags, like --key. Future: use getopt_long()
*/

command_line_args_t parse_command_line_args(const std::vector<std::string>& args, const std::string& flags);
command_line_args_t parse_command_line_args_subcommand(const std::vector<std::string>& args, const std::string& flags);



#endif /* command_line_parser_hpp */
