
# FUTURE -- MORE BUILT-IN TYPES





# FUTURE -- BUILT-IN COLLECTION FUNCTIONS


```
sort()

DT diff(T v0, T v1)
T merge(T v0, T v1)
T update(T v0, DT changes)
```



# FUTURE -- BUILT-IN STRING FUNCTIONS

```
[string] split_on_chars(seq_t s, string match_chars)
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

Makes a text from a normal string. Only 7-bit ASCII is supported.

	text_t text_from_ascii(string s)


## populate\_text\_variables()

Let's you safely assemble a Unicode text from a template that has insert-tags where to insert variable texts, in a way that is localizable. When localizing the template_text you can change the order of variables.

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


## utf8\_to\_text()

Converts a string with UTF8-text into a text_t.

	text_t utf8_to_text(string s)


## text\_to\_utf8()

Converts Unicode in text_t value to an UTF8 string.

	string text_to_utf8(text_t t, bool add_bom)






# FUTURE -- BUILT-IN URL FUNCTIONS


## is\_valid\_url()

	bool is_valid_url(string s)


## make\_url()

	url_t make_url(string s)


## split_url()

Splits a URL into its parts. This makes it easy to get to specific query strings or change a parts and reassemble the URL.

	url_parts_t split_url(url_t url)


## make_urls()

Makes a url_t from parts.

	url_t make_urls(url_parts_t parts)


## escape\_for\_url()

Prepares a string to be stored inside an URL. This requries some characters to be translated to special escapes.

| Character						| Result |
|---								| ---
| space							| "%20"
| !								| "%21"
| "								| "%22"

etc.

	string escape_for_url(string s)


## unescape\_from\_url()

This is to opposite of escape_for_url() - it takes a string from a URL and turns %20 into a space character and so on.

	string unescape_string_from_url(string s)






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
int get_random(random_t r)
random_t next(random_t r)
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
}

fs_environment_t get_environment()

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

	binary_t load_file(world w, absolute_path_t path)


## save\_binary\_file()

Will _create_ any needed directories in the save-path.

	void save_file(world w, absolute_path_t path, binary_t data)


## read\_unicode\_file()

Reads a text file into a text_t. It supports several encodings of the text file:

file_encoding:

-	0 = utf8
-	1 = Windows latin 1
-	2 = ASCII 7-bit
-	3 = UTF16 BIG ENDIAN
-	4 = UTF16 LITTLE ENDIAN
-	5 = UTF32 BIG ENDIAN
-	6 = UTF32 LITTLE ENDIAN

```
	text_t read_unicode_file(world w, absolute_path_t path, int file_encoding)
```

## write\_unicode\_file()

Writes text file in unicode format.

	void write_unicode_file(world w, absolute_path_t path, text_t t, int file_encoding, bool write_bom)


## does\_entry\_exist()

	bool does_entry_exist(world w, absolute_path_t path)


## create\_directories\_deep()

	void create_directories_deep(world w, absolute_path_t path)


## delete\_fs\_entry\_deep()

Deletes a file or directory. If the entry has children those are deleted too = delete folder also deletes is contents.

	void delete_fs_entry_deep(world w, absolute_path_t path)

## rename\_entry()

	void rename_entry(world w, absolute_path_t path, string n)


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
	
	directory_entry_info_t get_entry_info(world w, absolute_path_t path)


## get\_directory\_entries() and get\_directory\_entries\_deep()

Returns a vector of all the files and directories found at the path.

	struct directory_entry_t {
		enum EType {
			kFile,
			kDir
		}
		string fName
		EType fType
	}
	
	[directory_entry_t] get_directory_entries(world w, absolute_path_t path)

get_directory_entries_deep() works the same way, but will also traverse each found directory. Contents of sub-directories will be also be prefixed by the sub-directory names. All path names are relative to the input directory - not absolute to file system.

	[directory_entry_t] get_directory_entries_deep(world w, absolute_path_t path)


## to\_native\_path() and from\_native\_path()

Floyd's file and path function always use "/" as path divider. There may be other differences too.

Native-path is the current operating system's idea of how to store a path in a string. If you want to call OS functions, convert your Floyd path to a native path first.

- Windows is "\"
- Unix is "/"
- Mac OS 9 is ":"

```
	string to_native_path(world w, absolute_path_t path)
```
```
	absolute_path_t from_native_path(world w, string path)
```





# FUTURE -- WORLD TCP COMMUNICATION

IDEA: Wrap CURL. https://en.wikipedia.org/wiki/CURL


FAQ:

- Q: How can I write server code that handles many concurrent clients?
- A: A server always supports many clients. Each reqeust goes it its inbox and can be handled one at a time.

- Q: How can a server work on several requests *concurrently* (many of them are usually IO-bound anyhow).
- A: Each process has a team-setting you can : have settings on process that tells how many instances of it runs in parallell. A process team. They each have their own state and inbox and runs in their own green-thread. Messages are multiplexed between the team members. ???

- Q: How can a client have many pending requests it is waits for.
- A: It queues them all and gets replies via its inbox.


Network calls are normally IO-bound - it takes a long time from sending a message to getting the result -- often billions and billions of you CPU' clock cycles.

1. Your code make a request message.
2. the request is sent via the operating system's sockets and TCP/IP stack through the network card or WIFI card.
3. The request is routed between nodes on the internet, each with their own hardware and software delays.
4. The destination devices receives the request via its network hardware.
5. The destination OS passes the request to a waiting socket, waking up its thread.
6. The destination thread does work and its own communication and database accesses etc to solve the request.
7. The destination thread calls the OS to send back a reply message.
8. The destination OS writes the reply to its hardware.
9. The reply is routed between nodes on the internet, each with their own hardware and software delays.
10. The reply appears on your hardware.
11. The OS wakes up your OS thread that waits on the socket.
12. Your OS thread wakes up and reads the bytes of the message.

If the message roundtrip above takes 10 ms, that is equivalent to 20.000.000 clock cycles for a 2 GHz CPU.

Floyd has two ways to deal IO-bound operations like this:

1. Make a synchronous call -- your code will stop until the IO operation is completed and you receive the reply message. Other processes and tasks can still run on your CPU.

2. Queue an asynchronous request. Floyd will record your request then return immediately so your code can continue executing. At some point in the future the reply comes back and your green-process will receive the reply message in its INBOX.


## server: tcp\_server\_t {}

Represents an active TCP server instance. It can receive request messages from the outside world.

	struct tcp_server_t {
		int id
	}


## tcp_request\_t {}

Describes a request messages. It's always a client that takes the initiative to create a request from a server.

	struct tcp_request_t {
		tcp_server_t server
		binary_t payload
	}



## tcp\_reply\_t {}

Describes a reply message. This is what a server gives back to a client, in respons to a request message.

	struct tcp_reply_t {
		binary_t payload
	}



## server: tcp\_server\_settings\_t

The configuration of a TCP server - which URL and port it listens to, which protocols to use.

	struct tcp_server_settings_t {
		url_t url
		int port
	
		//	domain: 1 = AF_INET (IPv4 protocol), 2 = AF_INET6 (IPv6 protocol).
		//	Requests arrive in INBOX as tcp_request_t.
		int domain
		inbox_tag_t inbox_tag
	}

## server: open\_tcp\_server()

Opens and creates an active TCP server in your program. You can create many if you want to.

	tcp_server_t open_tcp_server(world w, tcp_server_settings_t s)


## server: get\_settings()

Returns the settings of the TCP server. Use this in your server code to differe between several TCP server instances.

	tcp_server_settings_t get_settings(tcp_server_t s)


## server: close\_tcp\_server()

Close a server that you have opened. All servers must be closed. Each server can only be closed exactly once.

	void close_tcp_server(world w, tcp_server_t s)

## server: reply()

This is how your server code can send back a reply message, in response to a client's request.

	void reply(world w, tcp_request_t request, tcp_reply_t reply)


## client: tcp\_client\_t {}

Represents the client-end of a TCP connection. If you have a tcp_client_t, then you also have an up and running TCP session to a server.

	struct tcp_client_t {
		int id
	}


## client: open\_tcp\_client()

Open a TCP client connected to a server, as specified using the url. If this function succeedes, then you have an up and running connection to the server.

	tcp_client_t open_tcp_client(world w, url_t url, int port)

## client: close\_tcp\_client()

Close a TCP client, as previously opened using open_tcp_client_socket(). All clients must be closed exactly once.

	void close_tcp_client(world w, tcp_client_t s)

## client: send()

Send a request to the server and wait until we get its reply. This blocks you running thread of execution, but other code can use the CPU meanwhile.

	tcp_reply_t send(world w, tcp_client_t s, binary_t payload)

## client: queue()

Queues a request message to be sent to the server and then the function returns immediately, without waiting for the message to even be sent.

When later a reply is received, your green-process will get a message with a tcp_reply_t in its inbox. This allows your code to continue doing other stuff while the request travels around the world and is handled y the server etc. You can even queue more requests.

```
void queue(world w, tcp_client_t s, binary_t payload, inbox_tag_t inbox_tag)
```


## resolve_url()

Uses DNS to get the IP-address for a URL.

ip_address_t resolve(world w, url_t url)
void queue_resolve(world w, url_t url)


# FUTURE -- WORLD REST COMMUNICATION

REST uses HTTP commands to communicate between client and server and these is no open session -- each command is its own session.


## client: rest\_request\_t {}

Describes a complete REST request as a value, including any parameters.

```
struct rest_request_t {
	//	"GET", "POST", "PUT"
	string operation_type
	
	[string: string] headers
	[string: string] params
	binary_t raw_data
	
	//	 If not "", then will be inserted in HTTP request header as XYZ.
	string optional_session_token
}
```

## client: rest\_reply\_t {}

This is what you get back from the server.

```
struct rest_reply_t {
	rest_request_t request
	int html_status_code
	string response
	string internal_exception
}
```

## client: send()

Sends a REST request to a server and block until IO is complete.

	rest_reply_t send(world w, rest_request_t request)


## client: queue\_rest()

Queues a REST request a returns immediately. Your code can continue doing other things. When later a reply is received from the server, you will get a message with a rest_reply_t in your green-process inbox.

	void queue_rest(world w, rest_request_t request, inbox_tag_t inbox_tag)



## server: rest\_server\_t {}

Represents an active REST server instance. It can receive request messages from the outside world.

	struct rest_server_t {
		int id
	}


## server: rest\_server\_settings\_t

The configuration of a REST server - which URL and port it listens to.

	struct rest_server_settings_t {
		url_t url
		inbox_tag_t inbox_tag
	}


## server: open\_rest\_server()

Opens and creates an active REST server in your program. You can create many if you want to. It serves ONE domain, like "https://www.mysite.com". You can get requests on any of its subdomains.

	rest_server_t open_rest_server(world w, rest_server_settings_t s)


## server: get\_settings()

Returns the settings of the REST server. Use this in your server code to differ between several REST server instances.

	rest_server_settings_t get_settings(rest_server_t s)


## server: close\_rest\_server()

Close a server that you have opened. All servers must be closed. Each server can only be closed exactly once.

	void close_rest_server(world w, rest_server_t s)


## server: reply()

This is how your server code can send back a reply message, in response to a client's request.

	void reply(world w, rest_request_t request, rest_reply_t reply)

