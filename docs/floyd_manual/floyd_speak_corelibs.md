# FLOYD SPEAK CORE LIBRARY

This is a small set of functions and types you can rely on always being available to your Floyd programs.



# WORKING WITH HASHES - SHA1



# calc\_string\_sha1()

Calculates a SHA1 hash for the contents in a string.

```
sha1_t calc_string_sha1(string s)
```

# calc\_binary\_sha1()

Calculates a SHA1 hash for a block binary data.

```
sha1_t calc_binary_sha1(binary_t d)
```




# IMPURE FUNCTIONS




## get\_time\_of\_day()

Returns the computer's realtime clock, expressed in the number of milliseconds since system start. Useful to measure program execution. Sample get_time_of_day() before and after execution and compare them to see duration.

	int get_time_of_day() impure




# FILE SYSTEM FUNCTIONS

These functions allow you to access the OS file system. They are all impure.

Floyd uses unix-style paths in all its APIs. It will convert these to native paths with accessing the OS.

fsentry = File System Entry. This is a node in the file system's graph of files and directories. It can either be a file or a directory.


## read\_text\_file()

Reads a text file from the file system and returns it as a string.

	string read_text_file(string abs_path) impure

Throws exception if file cannot be found or read.



## write\_text\_file()

Write a string to the file system as a text file. Will create any missing directories in the absolute path.

	void write_text_file(string abs_path, string data) impure



## get\_fsentries_shallow() and get\_fsentries\_deep()

Returns a vector of all the files and directories found at the absolute path.

	struct fsentry_t {
		string type
		string name
		string abs_parent_path
	}
	
	[fsentry_t] get_fsentries_shallow(string abs_path) impure

- type: either "file" or "dir".
- name: name of the leaf directory or file.
- abs\_parent\_path: the path to the entires parent.

get_fsentries_deep() works the same way, but will also traverse each found directory. Contents of sub-directories will be also be prefixed by the sub-directory names. All path names are relative to the input directory - not absolute to file system.

	[fsentry_t] get_fsentries_deep(string abs_path) impure



## get\_fsentry\_info()

Information about an entry in the file system. Can be a file or a directory.

	struct fsentry_info_t {
		string type
		string name
		string abs_parent_path

		string creation_date
		string modification_date
		int file_size
	}
	
	fsentry_info_t get_fsentry_info(string abs_path) impure

- type: either "file" or "dir".
- name: name of the leaf directory or file.
- abs\_parent\_path: the path to the entires parent.
- creation_date: creation date of the entry.
- modification_date: latest modification date of the entry.
- file_size: size of the file or directory.


## get\_fs\_environment()

Returns important root locations in the host computer's file system.

Notice: some of these directories can change while your program runs.

```
	struct fs_environment_t {
		string home_dir
		string documents_dir
		string desktop_dir

		string hidden_persistence_dir
		string preferences_dir
		string cache_dir
		string temp_dir

		string executable_dir
	}
```

```
fs_environment_t get_fs_environment() impure
```


##### home_dir
User's home directory. You don't normally store anything in this directory, but in one of the sub directories.

Example: "/Users/bob"

- User sees these files.

##### documents_dir
User's documents directory.

Example: "/Users/bob/Documents"

- User sees these files.

##### desktop_dir
User's desktop directory.

Example: "/Users/bob/Desktop"

- User sees these files.

##### hidden\_persistence\_dir
Current logged-in user's Application Support directory.
App creates data here and manages it on behalf of the user and can include files that contain user data.

Example: "/Users/marcus/Library/Application Support"

- Notice that this points to a directory shared by many applications: store your data in a sub directory!
- User don't see these files.

##### preferences_dir
Current logged-in user's preference directory.

Example: "/Users/marcus/Library/Preferences"

- Notice that this points to a directory shared by many applications: store your data in a sub directory!
- User don't see these files.

##### cache_dir
Current logged-in user's cache directory.

Example: "/Users/marcus/Library/Caches"

- Notice that this points to a directory shared by many applications: store your data in a sub directory!
- User don't see these files.

##### temp_dir
Temporary directory. Will be erased soon. Don't expect to find your files here next time your program starts or in 3 minutes.

- Notice that this points to a directory shared by many applications: store your data in a sub directory!
- User don't see these files.

##### executable_dir
Directory where your executable or bundle lives. This is usually read-only - you can't modify anything in this directory. You might use this path to read resources built into your executable or Mac bundle.

Example: "/Users/bob/Applications/MyApp.app/"



## does\_fsentry\_exist()

Checks if there is a file or directory at specified path.

	bool does_fsentry_exist(string abs_path) impure


## create\_directory\_branch()

Creates a directory at specified path. If the parents directories don't exist, then those will be created too.

	void create_directory_branch(string abs_path) impure


## delete\_fsentry\_deep()

Deletes a file or directory. If the entry has children those are deleted too - delete folder also deletes is contents.

	void delete_fsentry_deep(string abs_path) impure


## rename\_fsentry()

Renames a file or directory. If it is a directory, its contents is unchanged.
After this call completes, abs_path no longer references an entry.

	void rename_fsentry(string abs_path, string n) impure

Example:

Before:
	In the file system: "/Users/bob/Desktop/original_name.txt"
	abs_path: "/Users/bob/Desktop/original_name.txt"
	n: "name_name.txt"

After:
	world: "/Users/bob/Desktop/name_name.txt"





# CORE TYPES
A bunch of common data types are built into Floyd. This is to make composition easier and avoid the noise of converting all simple types between different component's own versions.



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




## uuid_t

A universally unique identifier (UUID) is a 128-bit number used to identify information in computer systems. The term globaly unique identifier (GUID) is also used.

	struct uuid_t {
		int high64
		int low64
	}


## ip\_address\_t

Internet IP adress in using IPv6 128-bit number.

	struct ip_address_t {
		int high64
		int low_64_bits
	}


## url_t

Internet URL.

	struct url_t {
		string absolute_url
	}


## url\_parts\_t {}

This struct contains an URL separate to its components.

	struct url_parts_t {
		string protocol
		string domain
		string path
		[string:string] query_strings
		int port
	}

Example 1:

	url_parts_t("http", "example.com", "path/to/page", {"name": "ferret", "color": "purple"})

	Output: "http://example.com/path/to/page?name=ferret&color=purple"


## quick\_hash\_t

64-bit hash value used to speed up lookups and comparisons.

	struct quick_hash_t {
		int hash
	}


## key_t

Efficent keying using 64-bit hash instead of a string. Hash can often be computed from string key at compile time.

	struct key_t {
		quick_hash_t hash
	}


## date_t

Stores a UDT.

	struct date_t {
		string utc_date
	}


## sha1_t

128-bit SHA1 hash number.

	struct sha1_t {
		string ascii40
	}


## relative\_path\_t 

File path, relative to some other path.

	struct relative_path_t {
		string relative_path
	}


## absolute\_path\_t

Absolute file path, from root of file system. Can specify the root, a directory or a file.

	struct absolute_path_t {
		string absolute_path
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
	}


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







# FUTURE -- IMPURE FUNCTIONS

## probe()

TODO POC

	probe(value, description_string, probe_tag) impure

In your code you write probe(my_temp, "My intermediate value", "key-1") to let clients log my_temp. The probe will appear as a hook in tools and you can chose to log the value and make stats and so on. Argument 2 is a descriptive name, argument 3 is a string-key that is scoped to the function and used to know if several probe()-statements log to the same signal or not.



## select()

TBD POC

Called from a process function to read its inbox. It will block until a message is received or it times out.

