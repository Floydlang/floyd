//
//  core_api.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef core_api_hpp
#define core_api_hpp

#include <string>
#include <vector>

/*
	This is the IMPLEMENTATION of the Floyd runtime's core API. The VM calls these functions.
*/



struct core_api_runtime_t {
	core_api_runtime_t(int64_t os_process_id) :
		os_process_id(os_process_id)
	{
	}

	int64_t os_process_id;
};

struct green_process_runtime_t {
	green_process_runtime_t() :
		random_seed(234782348723)
	{
	}




	//////////////////////		STATE
	int64_t random_seed;
};

struct runtime_t {
	std::vector<green_process_runtime_t> process_runtimes;
};



#if 0
//////////////////////////		CORE 0.1

//-----------------		language / interpreter / compiler
to_string()
to_pretty_string()
typeof()
update()
size()
find()
exists()
erase()
push_back()
subset()
replace()
flatten_to_json()
unflatten_from_json()
get_json_type()
decode_json()
encode_json()
#endif

#if 0
//-----------------		language / interpreter / compiler

print()
send()
assert()

get_time_of_day()

get_env_path()
read_text_file()
write_text_file()

select()
probe()
#endif



//////////////////////////		MORE COLLECTION OPERATIONS


#if 0

# 1.5
sort()

DT diff(T v0, T v1)
T merge(T v0, T v1)
T update(T v0, DT changes)
#endif


#if 0





text_t lookup_text(text_resource_id id)

//	"Press $1 button  with $2!", [ "$1",
text_t populate_text_variables(text_resource_id id, [struct { string aky, string s}])




////////////////////////////////////////////////////////////////////			MATH


PI

math.rad (x)
math.deg (x)


double abs(double value)
double min(double a, double b)
double max(double a, double b)
double truncate(double valie, double min, double max)


//	Returns the arc cosine of x in radians.
double acos(double x)

//	Returns the arc sine of x in radians.
double asin(double x)

//	Returns the arc tangent of x in radians.
double atan(double x)

//	Returns the arc tangent in radians of y/x based on the signs of both values to determine the correct quadrant.
double atan2(double y, double x)

//	Returns the cosine of a radian angle x.
double cos(double x)

//	Returns the hyperbolic cosine of x.
double cosh(double x)

//	Returns the sine of a radian angle x.
double sin(double x)

//	Returns the hyperbolic sine of x.
double sinh(double x)

//	Returns the hyperbolic tangent of x.
double tanh(double x)

//	Returns the value of e raised to the xth power.
double exp(double x)

//	The returned value is the mantissa and the integer pointed to by exponent is the exponent. The resultant value is x = mantissa * 2 ^ exponent.
double frexp(double x, int *exponent)

//	Returns x multiplied by 2 raised to the power of exponent.
double ldexp(double x, int exponent)

//	Returns the natural logarithm (base-e logarithm) of x.
double log(double x)

//	Returns the common logarithm (base-10 logarithm) of x.
double log10(double x)

//	The returned value is the fraction component (part after the decimal), and sets integer to the integer component.
double modf(double x, double *integer)

//	Returns x raised to the power of y.
double pow(double x, double y)

//	Returns the square root of x.
double sqrt(double x)

//	Returns the smallest integer value greater than or equal to x.
double ceil(double x)

//	Returns the absolute value of x.
double fabs(double x)

//	Returns the largest integer value less than or equal to x.
double floor(double x)

//	Returns the remainder of x divided by y.
double fmod(double x, double y)





math.random ([m [, n]])
math.randomseed (x)




////////////////////////////			CORE



void on_assert_hook(runtime_i* runtime, source_code_location location, char expression[])


protocol trace_i {
	void trace_i__trace(char s[])
	void trace_i__open_scope(char s[])
	void trace_i__close_scope(char s[])
}


//	??? logging logs values, not strings. Strings are created on-demand.

//	??? Simple app that receives debug messages and prints them. Uses sockets.



#define QUARK_TRACE(s)
#define QUARK_TRACE_SS(s)
#define QUARK_SCOPED_TRACE(s)

#define QUARK_CONTEXT_TRACE(context, s)
#define QUARK_CONTEXT_TRACE_SS(context, s)
#define QUARK_CONTEXT_SCOPED_TRACE(context, s)

struct unit_test_def {
	source_code_location loc

	string _class_under_test
	string _function_under_test
	string _scenario
	string _expected_result

	unit_test_function _test_f
	bool _vip
}

template <typename T> void ut_compare2(T result, T expected, source_code_location code_location){
inline void run_tests(unit_test_registry registry, [string] source_file_order){



/////////////////////////////////////////			sha1


bool operator<(sha1_t iLHS, sha1_t iRHS)

//	strlen("sha1 = ")
size_t kSHA1Prefix = 7

//	iData can be nullptr if size is 0.
sha1_t calc_sha1(std::uint8_t iData[], size_t iSize)

sha1_t calc_sha1(string iS)

//	String is always exactly 40 characters long.
//	"1234567890123456789012345678901234567890"
static size_t kSHA1StringSize = sha1_t::kHashSize * 2

string sha1_to_string_plain(sha1_t iSHA1)
sha1_t sha1_from_string_plain(string iString)


/**
	"sha1 = 1234567890123456789012345678901234567890"
*/
bool is_valid_tagged_sha1(string iString)
string sha1_to_string_tagged(sha1_t iSHA1)
sha1_t sha1_from_string_taggeed(string iString)





///////////////////////////////		String utils



string test_whitespace_chars = " \n\t\r"

string trim_ends(string s)
string quote(string s)

std::vector<string> split_on_chars(seq_t s, string match_chars)
/*
	Concatunates vector of strings. Adds div between every pair of strings. There is never a div at end.
*/
string concat_strings_with_divider([string] v, string div)


float parse_float(string pos)
double parse_double(string pos)



///////////////////////////////		seq_t


/*
	This is a magic string were you can easily peek into the beginning and
	also get a new string without the first character(s).

	It shares the actual string data behind the curtains so is efficent.
*/

struct seq_t {
	string FIRST_debug
	string str
	size_t _pos
}


seq_t make_seq(string s)
bool check_invariant(seq_t s)


//	Peek at first char (as C-character). Throws exception if empty().
char first1_char(seq_t s)

//	Peeks at first char (you get a string). Throws if there is no more char to read.
string first1(seq_t s)

//	Skips 1 char.
//	You get empty if rest_size() == 0.
seq_t rest1(seq_t s)


//	Returned string can be "" or shorter than chars if there aren't enough chars.
//	Won't throw.
string first(seq_t s, size_t chars)

//	Skips n characters.
//	Limited to rest_size().
seq_t rest(seq_t s, size_t count)

seq_t back(seq_t s, size_t count)


//	Returns entire string. Equivalent to x.rest(x.size()).
//	Notice: these returns what's left to consume of the original string, not the full original string.
string get_s(seq_t s)
string str(seq_t s)
size_t size(seq_t s)

//	If true, there are no more characters.
bool empty(seq_t s)

//	Returns true if both are based on the same internal string.
bool related(seq_t a, seq_t b)



struct seq_out {
	string s
	seq_t seq
}


seq_t skip(seq_t p1, string chars)

seq_out read_while(seq_t p1, string chars)
seq_out read_until(seq_t p1, string chars)

seq_out split_at(seq_t p1, string str)

//	If p starts with wanted_string, return true and consume those chars. Else return false and the same seq_t.
std::pair<bool, seq_t> if_first(seq_t p, string wanted_string)

bool is_first(seq_t p, string wanted_string)


string get_range(seq_t a, seq_t b)

seq_out read_char(seq_t s)

/*
	Returns "rest" if s is found, else throws exceptions.
*/
seq_t read_required_char(seq_t s, char ch)

/*
	Returns "rest" if s is found, else throws exceptions.
*/
seq_t read_required(seq_t s, string req)

std::pair<bool, seq_t> read_optional_char(seq_t s, char ch)






////////////////////////////////		COLLECTION OPERATIONS, PARALLELLISM



[int:R] supermap(tasks: [T, [int], f: R (T, [R]))
[T] map([T] elements, f)
[T] reduce([T], F f)
R fold([T], R init, F f)






//////////////////////////////		SOCKETS





server_socket_t
client_socket_t

rest_message_t

cookie_t
oath_t


struct open_socket {
	int socket_id
}


open_socket open_socket(string url)
void close_socket(open_socket s)
void write_sync(open_socket_t s, string message)
void write_async(open_socket_t s, string message, message_t result_template)

/*
???

Blocking operations (blocks green-process).
Non-blocking operations post result as custom message to green-process.
*/





//////////////////////////////		FILE HANDLING





//??? Keep file open, read/write bits.

binary_t load_file(absolute_path_t path)

//	Will _create_ any needed directories in the save-path.
void save_file(absolute_path_t path, binary_t data)

string load_text_file(absolute_path_t path)

//	Will _create_ any needed directories in the save-path.
void save_text_file(absolute_path_t path, string data)

bool does_entry_exist(absolute_path_t path)


//	A writable directory were we can store files for next run time.
absolute_path_t get_cache_dir()

absolute_path_t get_preference_dir()

absolute_path_t get_desktop_dir()

absolute_path_t get_app_resource_dir()
absolute_path_t get_app_rw_dir()

absolute_path_t get_executable_dir()

absolute_path_t get_test_read_dir()
absolute_path_t get_test_write_dir()




void create_directories_deep(absolute_path_t path)

//	Deletes a file or directory. If the entry has children those are deleted too = delete folder also deletes is contents.
void delete_fs_entry_deep(absolute_path_t path)

void rename_entry(absolute_path_t path, string n)


absolute_path_t get_parent_directory(absolute_path_t path)

struct directory_entry_info_t {
	enum EType {
		kFile,
		kDir
	}

	//	Dates are undefined unit, but can be compared.
//	int fCreationDate
	EType fType
	int fModificationDate
	file_pos_t fFileSize
}

directory_entry_info_t get_entry_info(absolute_path_t path)


struct directory_entry_t {
	enum EType {
		kFile,
		kDir
	}
	string fName
	EType fType
}

std::vector<directory_entry_t> get_directory_entries(absolute_path_t dir_path)

//	Returns a entiry directory tree deeply.
//	Contents of sub-directories will be also be prefixed by the sub-directory names.
//	All path names are relative to the input directory - not absolute to file system.
std::vector<directory_entry_t> get_directory_entries_deep(absolute_path_t dir_path)


//	Converts all forward slashes "/" to the path separator of the current operating system:
//		Windows is "\"
//		Unix is "/"
//		Mac OS 9 is ":"
string to_native_path(absolute_path_t path)
absolute_path_t get_native_path(string path)


struct path_parts_t {
	//	"/Volumes/MyHD/SomeDir/"
	string fPath

	//	"MyFileName"
	string fName

	//	Returns "" when there is no extension.
	//	Returned extension includes the leading ".".
	//	Examples:
	//		".wav"
	//		""
	//		".doc"
	//		".AIFF"
	string fExtension
}

path_parts_t split_path(absolute_path_t path)


/*
	base								relativePath
	"/users/marcus/"					"resources/hello.jpg"			"/users/marcus/resources/hello.jpg"
*/
absolute_path_t make_path(path_parts_t parts)



//??? temp file support. Always store to temp files.



#endif

#endif /* core_api_hpp */
