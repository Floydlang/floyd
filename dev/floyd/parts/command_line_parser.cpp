//
//  command_line_parser.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "command_line_parser.h"


#include "quark.h"

#include <getopt.h>



////////////////////////////		COMMAND LINE ARGUMENTS




std::vector<std::string> args_to_vector(int argc, const char * argv[]){
	std::vector<std::string> r;
	for(int i = 0 ; i < argc ; i++){
		const std::string s(argv[i]);
		r.push_back(s);
	}
	return r;
}

#ifdef __APPLE__
std::pair<std::string, std::vector<std::string> > extract_key(const std::vector<std::string>& args, const std::string& key){
	const auto it = std::find(args.begin(), args.end(), key);
	if(it != args.end()){
		auto copy = args;
		copy.erase(it);

		return { key, copy };
	}
	else{
		return { "", args };
	}
}
#endif

/*
% testopt
aflag = 0, bflag = 0, cvalue = (null)

% testopt -a -b
aflag = 1, bflag = 1, cvalue = (null)

% testopt -ab
aflag = 1, bflag = 1, cvalue = (null)

% testopt -c foo
aflag = 0, bflag = 0, cvalue = foo

% testopt -cfoo
aflag = 0, bflag = 0, cvalue = foo

% testopt arg1
aflag = 0, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -a arg1
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -c foo arg1
aflag = 0, bflag = 0, cvalue = foo
Non-option argument arg1

% testopt -a -- -b
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -b

% testopt -a -
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -
*/



command_line_args_t parse_command_line_args(const std::vector<std::string>& args, const std::string& flags){
	if(args.size() == 0){
		return {};
	}

	//	Includes a trailing null-terminator for each arg.
	int byte_count = 0;
	for(const auto& e: args){
		byte_count = byte_count + static_cast<int>(e.size()) + 1;
	}

	//	Make local writeable strings to argv to hand to getopt().
	std::vector<char*> argv;
	std::string concat_args;

	//	Make sure it's not reallocating since we're keeping pointers into the string.
	concat_args.reserve(byte_count);

	for(const auto& e: args){
		char* ptr = &concat_args[concat_args.size()];
		concat_args.insert(concat_args.end(), e.begin(), e.end());
		concat_args.push_back('\0');
		argv.push_back(ptr);
	}

	int argc = static_cast<int>(args.size());

	std::map<std::string, std::string> flags2;

	//	The variable optind is the index of the next element to be processed in argv. The system
	//	initializes this value to 1. The caller can reset it to 1 to restart scanning of the same argv,
	//	or when scanning a new argument vector.
	optind = 1;

	//	Disable getopt() from printing errors to stderr.
	opterr = 0;

    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    int opt = 0;
    while((opt = getopt(argc, &argv[0], flags.c_str())) != -1){
		const auto opt_string = std::string(1, opt);
		const auto optarg_string = optarg != nullptr ? std::string(optarg) : std::string();
		const auto optopt_string = std::string(1, optopt);

		//	Unknown flag.
		if(opt == '?'){
			flags2.insert({ optopt_string, "?" });
		}

		//	Flag with parameter.
		else if(optarg_string.empty() == false){
			flags2.insert({ opt_string, optarg_string });
		}

		//	Flag needs a value(???)
		else if(opt == ':'){
			flags2.insert({ ":", optarg_string });
		}

		//	Flag without parameter.
		else{
			flags2.insert({ opt_string, "" });
		}
    }

	std::vector<std::string> extras;

    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++){
		const auto s = std::string(argv[optind]);
		extras.push_back(s);
    }

	return { args[0], "", flags2, extras };
}

//	The shell actually strips the quotes, not C or C++.

command_line_args_t parse_command_line_args_subcommands(const std::vector<std::string>& args, const std::string& flags){
	const auto a = parse_command_line_args({ args.begin() + 1, args.end()}, flags);
	return { args[0], a.command, a.flags, a.extra_arguments };
}


void trace_command_line_args(const command_line_args_t& v){
	QUARK_SCOPED_TRACE("trace_command_line_args");

	{
		QUARK_SCOPED_TRACE("flags");
		for(const auto& e: v.flags){
			const auto s = e.first + ": " + e.second;
			QUARK_TRACE(s);
		}
	}
}

const auto k_git_command_examples = std::vector<std::string>({
	R"(git init)",

	R"(git clone /path/to/repository)",
	R"(git clone username@host:/path/to/repository)",
	R"(git add <filename>)",
	R"(git add *)",
	R"(git commit -m "Commit message")",

	R"(git push origin master)",
	R"(git remote add origin <server>)",
	R"(git checkout -b feature_x)",
	R"(git tag 1.0.0 1b2e1d63ff)",
	R"(git log --help)"
});


/*
usage: python [option] ... [-c cmd | -m mod | file | -] [arg] ...
Options and arguments (and corresponding environment variables):
-B     : don't write .py[co] files on import; also PYTHONDONTWRITEBYTECODE=x
-c cmd : program passed in as string (terminates option list)
-d     : debug output from parser; also PYTHONDEBUG=x
-E     : ignore PYTHON* environment variables (such as PYTHONPATH)
-h     : print this help message and exit (also --help)
-i     : inspect interactively after running script; forces a prompt even
         if stdin does not appear to be a terminal; also PYTHONINSPECT=x
-m mod : run library module as a script (terminates option list)
-O     : optimize generated bytecode slightly; also PYTHONOPTIMIZE=x
-OO    : remove doc-strings in addition to the -O optimizations
-R     : use a pseudo-random salt to make hash() values of various types be
         unpredictable between separate invocations of the interpreter, as
         a defense against denial-of-service attacks
-Q arg : division options: -Qold (default), -Qwarn, -Qwarnall, -Qnew
-s     : don't add user site directory to sys.path; also PYTHONNOUSERSITE
-S     : don't imply 'import site' on initialization
-t     : issue warnings about inconsistent tab usage (-tt: issue errors)
-u     : unbuffered binary stdout and stderr; also PYTHONUNBUFFERED=x
         see man page for details on internal buffering relating to '-u'
-v     : verbose (trace import statements); also PYTHONVERBOSE=x
         can be supplied multiple times to increase verbosity
-V     : print the Python version number and exit (also --version)
-W arg : warning control; arg is action:message:category:module:lineno
         also PYTHONWARNINGS=arg
-x     : skip first line of source, allowing use of non-Unix forms of #!cmd
-3     : warn about Python 3.x incompatibilities that 2to3 cannot trivially fix
file   : program read from script file
-      : program read from stdin (default; interactive mode if a tty)
arg ...: arguments passed to program in sys.argv[1:]

Other environment variables:
PYTHONSTARTUP: file executed on interactive startup (no default)
PYTHONPATH   : ':'-separated list of directories prefixed to the
               default module search path.  The result is sys.path.
PYTHONHOME   : alternate <prefix> directory (or <prefix>:<exec_prefix>).
               The default module search path uses <prefix>/pythonX.X.
PYTHONCASEOK : ignore case in 'import' statements (Windows).
PYTHONIOENCODING: Encoding[:errors] used for stdin/stdout/stderr.
PYTHONHASHSEED: if this variable is set to 'random', the effect is the same
   as specifying the -R option: a random value is used to seed the hashes of
   str, bytes and datetime objects.  It can also be set to an integer
   in the range [0,4294967295] to get hash values with a predictable seed.
*/



std::vector<std::string> split_command_line(const std::string& s){
	std::vector<std::string> result;
	std::string acc;

	auto pos = 0;
	while(pos < s.size()){
		if(s[pos] == ' '){
			if(acc.empty()){
			}
			else{
				result.push_back(acc);
				acc = "";
			}
			pos++;
		}
		else if (s[pos] == '\"'){
			acc.push_back(s[pos]);

			pos++;
			while(s[pos] != '\"'){
				acc.push_back(s[pos]);
				pos++;
			}

			acc.push_back(s[pos]);
			pos++;
		}
		else{
			acc.push_back(s[pos]);
			pos++;
		}
	}

	if(acc.empty() == false){
		result.push_back(acc);
		acc = "";
	}

	return result;
}

QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("");
	QUARK_UT_VERIFY(result == std::vector<std::string>{});
}
QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("  ");
	QUARK_UT_VERIFY(result == std::vector<std::string>{});
}
QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line(" one ");
	QUARK_UT_VERIFY(result == std::vector<std::string>{ "one" });
}
QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("one two");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", "two" }));
}
QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("one two     three");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", "two", "three" }));
}

QUARK_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line(R"(one "hello world"    three)");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", R"("hello world")", "three" }));
}




QUARK_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.empty() && result.extra_arguments.empty());
}

QUARK_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "file1.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.empty());
	QUARK_UT_VERIFY(result.extra_arguments == std::vector<std::string>({"file1.txt"}));
}

QUARK_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("l")->second == "");
	QUARK_UT_VERIFY(result.flags.find("r")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY(result.extra_arguments.size() == 0);
}


//	getopt() uses global state, make sure we can call it several times OK (we reset it).
QUARK_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("l")->second == "");
	QUARK_UT_VERIFY(result.flags.find("r")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY(result.extra_arguments.size() == 0);

	const auto result1 = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.flags == result1.flags && result.extra_arguments == result.extra_arguments);
}


QUARK_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-i", "-f", "doc.txt", "extra_one", "extra_two" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "extra_one", "extra_two" }));
}



QUARK_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[0]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "init");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{}));
}
QUARK_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[1]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "clone");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "/path/to/repository" }));
}

QUARK_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[4]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "add");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "*" }));
}
QUARK_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[5]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "commit");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{ {"m","?"}} ));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ R"("Commit message")" }));
}





