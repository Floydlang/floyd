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
	git commit -m "Commit message"
		command: "git"
		subcommand: "commit"
		flags
			m: ""
		extra_arguments
			"Commit message"
*/
command_line_args_t parse_command_line_args_subcommands(const std::vector<std::string>& args, const std::string& flags);



#endif /* command_line_parser_hpp */
