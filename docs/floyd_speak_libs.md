
# FUTURE -- MORE BUILT-IN TYPES


## cpu\_address_t

64-bit integer used to specify memory addresses and binary data sizes.

	typedef int cpu_address_t
	typedef cpu_address_t size_t


## file\_pos\_t

64-bit integer for specifying positions inside files.

	typedef int file_pos_t


## time\_ms\_t

64-bit integer counting miliseconds.

	typedef int64_t time_ms_t


## inbox\_tag\_t

This is used to tag message put in a green process inbox. It allows easy detection.

	struct inbox_tag_t {
		string tag_string
	}


## uuid_t

A universally unique identifier (UUID) is a 128-bit number used to identify information in computer systems. The term globaly unique identifier (GUID) is also used.

	struct uuid_t {
		int high64
		int low64
	}


## url_t

Internet URL.

	struct url_t {
		string absolute_url
	}


## quick\_hash\_t

64-bit hash value used to speed up lookups and comparisons.

	struct quick_hash_t {
		int hash
	}


## key_t

Efficent keying using 64-bit hash instead of a string. Hash can often be computed from string-key at compile time.

	struct key_t {
		quick_hash_t hash
	}


## date_t

Stores a UDT.

	struct date_t {
		string utd_date
	}


## sha1_t

128bit SHA1 hash number.

	struct sha1_t {
		string ascii40
	}


## relative\_path\_t 

File path, relative to some other path.

	struct relative_path_t {
		string fRelativePath
	}


## absolute\_path\_t

Absolute file path, from root of file system. Can specify the root, a directory or a file.

	struct absolute_path_t {
		string fAbsolutePath
	}


## binary_t

Raw binary data, 8bit per byte.

	struct binary_t {
		string bytes
	}


## text\_location\_t

Specifies a position in a text file.

	struct text_location_t {
		absolute_path_t source_file
		int line_number
		int pos_in_line
	};


## seq_t

This is the neatest way to parse strings without using an iterator or indexes.

	struct seq_t {
		string str
		size_t pos
	}

When you consume data at start of seq_t you get a *new* seq_t that holds
the rest of the string. No side effects.

This is a magic string were you can easily peek into the beginning and also get a new string
without the first character(s).

It shares the actual string data behind the curtains so is efficent.



## text_t

Unicode text. Opaque data -- only use library functions to process text_t. Think of text_t and Uncode text more like a PDF.

	struct text_t {
		binary_t data
	}


## text\_resource\_id

How you specify text resources.

	struct text_resource_id {
		quick_hash_t id
	}


## image\_id\_t

How you specify image resources.

	struct image_id_t {
		int id
	}


## color_t

Color. If you don't need alpha, set it to 1.0. Components are normally min 0.0 -- max 1.0 but you can use other ranges.

	struct color_t {
		float red
		float green
		float blue
		float alpha
	}


## vector2_t

2D position in cartesian coordinate system. Use to specify position on the screen and similar.

	struct vector2_t {
		float x
		float y
	}




# FUTURE -- BUILT-IN COLLECTION FUNCTIONS


```
sort()

DT diff(T v0, T v1)
T merge(T v0, T v1)
T update(T v0, DT changes)
```



# FUTURE -- BUILT-IN STRING FUNCTIONS

```
std::vector<string> split_on_chars(seq_t s, string match_chars)
/*
	Concatunates vector of strings. Adds div between every pair of strings. There is never a div at end.
*/
string concat_strings_with_divider([string] v, string div)


float parse_float(string pos)
double parse_double(string pos)
```




# FUTURE -- BUILT-IN TEXT AND TEXT RESOURCES FUNCTIONS

Unicode is used to support text in any human language. It's an extremely complex format and it's difficult to write code to manipulate or interpret it. It contains graphene clusters, code points, characters, compression etc.

Floyd's solution:

- a dedicated data type for holding Unicode text - text_t, separate from the string data type.

- use an opaque encoding of the text data -- always use the text-functions only.

Think of text_t more like PDF:s or JPEGs than strings of characters.


## lookup\_text()

Reads text from resource file (cached in RAM).
This is a pure function, it expected to give the same result every time.

	text_t lookup_text(text_resource_id id)


## text\_from\_ascii()

Makes a text from a normal string. Only 7bit ASCII is supported.

	text_t text_from_ascii(string s)


## populate\_text\_variables()

Let's you safely assemble a Unicode text from a template that has insert-tags where to insert variable texts, in a way that is localizable. Localizing the template_text can change order of variables.

	text_t populate_text_variables(text_t template_text, [string: text_t])


Example 1:

```
populate_text_variables(
	"Press ^1 button with ^2!",
	{ "^1": text_from_ascii("RED"), "^2": text_from_ascii("pinkie")}
)
```
**Output: "Press RED button with pinkie!"**

Example 2 -- reorder:

```
populate_text_variables(
	"Använd ^2 för att trycka på den ^1 knappen!",
	{ "^1": text_from_ascii("röda"), "^2": text_from_ascii("lillfingret")}
)
```
**Output: "Använd lillfingret för att trycka på den röda knappen!"**


## read\_unicode\_file()

Reads a text file into a text_t. It supports several encodings of the text file:

file_encoding:

-	0 = utf8
-	1 = Windows latin 1
-	2 = ASCII 7bit
-	3 = UTF16 BIG ENDIAN
-	4 = UTF16 LITTLE ENDIAN
-	5 = UTF32 BIG ENDIAN
-	6 = UTF32 LITTLE ENDIAN

```
	text_t read_unicode_file(absolute_path_t path, int file_encoding)
```

## write\_unicode\_file()

Writes text file in unicode format.

	void write_unicode_file(absolute_path_t path, text_t t, int file_encoding, bool write_bom)


## utf8\_to\_text()

Converts a string with UTF8-text into a text_t.

	text_t utf8_to_text(string s)


## text\_to\_utf8()

Converts Unicode in text_t value to an UTF8 string.

	string text_to_utf8(text_t t, bool add_bom)






# FUTURE -- BUILT-IN URL FUNCTIONS


## is\_valid\_url()

	bool is_valid_url(const std::string& s);


## make\_url()

	url_t make_url(const std::string& s);


## url\_parts\_t {}

This struct contains an URL separate to its components.

	struct url_parts_t {
		std::string protocol;
		std::string domain;
		std::string path;
		[string:string] query_strings;
	};

Example 1:

	url_parts_t("http", "example.com", "path/to/page", {"name": "ferret", "color": "purple"})

	Output: "http://example.com/path/to/page?name=ferret&color=purple"


## split_url()

Splits a URL into its parts. This makes it easy to get to specific query strings or change a parts and reassemble the URL.

	url_parts_t split_url(const url_t& url);


## make_urls()

Makes a url_t from parts.

	url_t make_urls(const url_parts_t& parts);


## escape\_for\_url()

Prepares a string to be stored inside an URL. This requries some characters to be translated to special escapes.

| Character						| Result |
|---								| ---
| space							| "%20"
| !								| "%21"
| "								| "%22"

etc.

	std::string escape_for_url(const std::string& s);


## unescape\_from\_url()

This is to opposite of escape_for_url() - it takes a string from a URL and turns %20 into a space character and so on.

	std::string unescape_string_from_url(const std::string& s);






# FUTURE -- BUILT-IN SHA1 FUNCTIONS
```

sha1_t calc_sha1(string s)
sha1_t calc_sha1(binary_t data)

//	SHA1 is 20 bytes.
//	String representation uses hex, so is 40 characters long.
//	"1234567890123456789012345678901234567890"
let sha1_bytes = 20
let sha1_chars = 40

string sha1_to_string(sha1_t s)
sha1_t sha1_from_string(string s)

```





# FUTURE - BUILT-IN RANDOM FUNCTIONS

```
struct random_t {
	int value
}

random_t make_random(int seed)
int get_random(random_t& r)
random_t next(random_t& r)
```






# FUTURE - BUILT-IN MATH FUNCTIONS

```
let pi_constant = 3.141592653589793238462

//	Convert an angle in degrees to radians.
radians (x)

//	Convert an angle in radians to degrees.
degrees(x)

//	Returns absolute value. If value is negative it becomes positive.
double abs(double value)

//	Returns the smaller of a and b
double min(double a, double b)

//	Returns the larger of a and b
double max(double a, double b)


//	Returns value but clipped to the span min -- max.
double truncate(double value, double min, double max)


//	Returns the arc cosine of x in radians.
double acos(double x)

//	Returns the arc sine of x in radians.
double asin(double x)

//	Returns the arc tangent of x in radians.
double atan(double x)

//	Returns the arc tangent in radians of y/x based on the signs of
//	both values to determine the correct quadrant.
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

//	The returned value is the mantissa and the integer pointed to by
//	exponent is the exponent. The resultant value is x = mantissa * 2 ^ exponent.
double frexp(double x, int *exponent)

//	Returns x multiplied by 2 raised to the power of exponent.
double ldexp(double x, int exponent)

//	Returns the natural logarithm (base-e logarithm) of x.
double log(double x)

//	Returns the common logarithm (base-10 logarithm) of x.
double log10(double x)

//	The returned value is the fraction component (part after the decimal),
//	and sets integer to the integer component.
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

```





# FUTURE - BUILT-IN FILE PATH FUNCTIONS


```
struct fs_environment_t {
	//	A writable directory were we can store files for next run time.
	absolute_path_t persistence_dir

	absolute_path_t app_resource_dir
	absolute_path_t executable_dir

	absolute_path_t test_read_dir
	absolute_path_t test_write_dir

	absolute_path_t preference_dir
	absolute_path_t desktop_dir
};

fs_environment_t get_environment();

absolute_path_t get_parent_directory(absolute_path_t path)

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
```






# FUTURE - BUILT-IN STRING FUNCTIONS

```
struct seq_out {
	string s
	seq_t seq
}

seq_t make_seq(string s)
bool check_invariant(seq_t s)


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


//	Returns entire string. Equivalent to x.rest(x.size()).
//	Notice: these returns what's left to consume of the original string, not the full original string.
string str(seq_t s)
size_t size(seq_t s)

//	If true, there are no more characters.
bool empty(seq_t s)



//	Skip any leading occurrances of these chars.
seq_t skip(seq_t p1, string chars)

seq_out read_while(seq_t p1, string chars)
seq_out read_until(seq_t p1, string chars)

seq_out split_at(seq_t p1, string str)

//	If p starts with wanted_string, return true and consume those chars. Else return false and the same seq_t.
std::pair<bool, seq_t> if_first(seq_t p, string wanted_string)

bool is_first(seq_t p, string wanted_string)


seq_out read_char(seq_t s)

/*
	Returns "rest" if s is found, else throws exceptions.
*/
seq_t read_required(seq_t s, string req)

std::pair<bool, seq_t> read_optional_char(seq_t s, char ch)
```






# FUTURE -- WORLD FILE SYSTEM FUNCTIONS

These functions allow you to access the OS file system. They are all impure. Temporary files are sometimes used to make the functions revertable on errors.

Floyd uses unix-style paths in all its APIs. It will convert these to native paths with accessing the OS.


??? Keep file open, read/write part by part.
??? Paths could use [string] instead.


## load\_binary\_file()

	binary_t load_file(absolute_path_t path)


## save\_binary\_file()

Will _create_ any needed directories in the save-path.

	void save_file(absolute_path_t path, binary_t data)


## does\_entry\_exist()

	bool does_entry_exist(absolute_path_t path)


## create\_directories\_deep()

	void create_directories_deep(absolute_path_t path)


## delete\_fs\_entry\_deep()

Deletes a file or directory. If the entry has children those are deleted too = delete folder also deletes is contents.

	void delete_fs_entry_deep(absolute_path_t path)

## rename\_entry()

	void rename_entry(absolute_path_t path, string n)


## get\_entry\_info()

	struct directory_entry_info_t {
		enum EType {
			kFile,
			kDir
		}
	
		date_t creation_date
		date_t modification_date
		EType type
		file_pos_t file_size
		relative_path_t path
		string name
	}
	
	directory_entry_info_t get_entry_info(absolute_path_t path)


## get\_directory\_entries(), get\_directory\_entries\_deep()

Returns a vector of all the files and directories found at the path.

	struct directory_entry_t {
		enum EType {
			kFile,
			kDir
		}
		string fName
		EType fType
	}
	
	std::vector<directory_entry_t> get_directory_entries(absolute_path_t path)

get_directory_entries_deep() works the same way, but will also traverse each found directory. Contents of sub-directories will be also be prefixed by the sub-directory names. All path names are relative to the input directory - not absolute to file system.

	std::vector<directory_entry_t> get_directory_entries_deep(absolute_path_t path)


## NATIVE PATHS

Converts all forward slashes "/" to the path separator of the current operating system:
	Windows is "\"
	Unix is "/"
	Mac OS 9 is ":"

string to_native_path(absolute_path_t path)
absolute_path_t get_native_path(string path)






# FUTURE -- WORLD TCP COMMUNICATION

FAQ:

- ??? How can server code handle many clients in parallell? Use one green thread per request?
- How can a client have many pending requests it waits for. It queues them all and calls select() on inbox.


Network calls are normally IO-bound - it takes a long time from sending a message to getting the result -- often billions and billions of CPU clock cycles. When performing a call that is IO-bound you have two choices to handle this fact:

1. Make a synchronous call -- your code will stop until the IO operation is completed. Other processes and tasks can still run.
	- the message has been sent via sockets
	- destination devices has processed or failed to handle the message
	- the destination has transmitted a reply back.


2. Make an async call and privide a tag. Floyd will queue up your request then return immediately so your code can continue executing.

These functions are called "queue()". At a future time when there is a reply, your green-process will receive a special message in its INBOX.


```
struct tcp_server_t {
	int id;
};

struct tcp_request_t {
	tcp_server_t server;
	binary_t payload;
};

struct tcp_reply_t {
	binary_t payload;
};

struct tcp_server_settings_t {
	url_t url;
	int port;

	//	domain: 1 = AF_INET (IPv4 protocol), 2 = AF_INET6 (IPv6 protocol).
	//	Requests arrive in INBOX as tcp_request_t.
	int domain;
	inbox_tag_t inbox_tag;
};

tcp_server_t open_tcp_server(const tcp_server_settings_t& s);
tcp_server_settings_t get_settings(const tcp_server_t& s);
void close_tcp_server(const tcp_server_t& s);

void reply(const tcp_request_t& request, const tcp_reply_t& reply);



struct tcp_client_t {
	int id;
};

tcp_client_t open_tcp_client_socket(const url_t& url, int port);
void close_tcp_client_socket(const tcp_client_t& s);

//	Blocks until IO is complete.
tcp_reply_t send(const tcp_client_t& s, const binary_t& payload);

Returns at once. When later a reply is received, you will get a message

with a tcp_reply_t in your green-process INBOX.

void queue(const tcp_client_t& s, const binary_t& payload, const inbox_tag_t& inbox_tag);

```





# FUTURE -- WORLD REST COMMUNICATION

REST uses HTTP commands to communicate between client and server and these is no open session -- each command is its own session.

```
struct rest_request_t {
	//	"GET", "POST", "PUT"
	std::string operation_type;

	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> params;
	binary_t raw_data;

	//	 If not "", then will be inserted in HTTP request header as XYZ.
	std::string optional_session_token;
};

struct rest_reply_t {
	rest_request_t request;
	int html_status_code;
	std::string response;
	std::string internal_exception;
};

//	Blocks until IO is complete.
rest_reply_t send(const rest_request_t& request);

//	Returns at once. When later a reply is received, you will get a message with a rest_reply_t in your green-process INBOX.
void queue_rest(const rest_request_t& request, const inbox_tag_t& inbox_tag);

```
