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

////////////////////////////		COMMAND LINE ARGUMENTS


std::vector<std::string> args_to_vector(int argc, const char * argv[]);
std::vector<std::string> split_command_line(const std::string& s);

//	Flags: x: means x supports parameter.
struct command_line_args_t {
	std::string command;
	std::string subcommand;

	//	Key: flag character, value: parameter or ""
	std::map<std::string, std::string> flags;


	//	Extra arguments, after the flags have been parsed.
	std::vector<std::string> extra_arguments;
};

command_line_args_t parse_command_line_args(const std::vector<std::string>& args, const std::string& flags);

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

	Does NOT support long flags, like --key. Future: use getopt_long()
	Supports flags with arguments, like -a:hello
*/
command_line_args_t parse_command_line_args_subcommands(const std::vector<std::string>& args, const std::string& flags);



#endif /* command_line_parser_hpp */
