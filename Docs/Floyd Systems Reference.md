

In your code you write probe(my_temp, "My intermediate value", "key-1") to let clients log my_temp.



# FLOYD SYSTEM REFERENCE


### SOFTWARE SYSTEM REFERENCE

Highest level of abstraction and describes something that delivers value to its users, whether they are human or not, can be composed of many computers working together.


### CONTAINER REFERENCE


This is a declarative file that describes the top-level structure of an app. Its contents looks like this:


Top level function

```
container_main()
```

This is th	e container's start function. It will find, create and and connect all resources, runtimes and other dependencies to boot up the container and to do executibe decisions and balancing for the container.



### COMPONENT REFERENCE



### CODE REFERENCE



### BUILT-IN COMPONENTS - REFERENCE


##### COMMAND LINE COMPONENT REFERENCE

??? TBD

	int on_commandline_input(string args) unpure
	void print(string text) unpure
	string readline() unpure


##### MULTIPLEXER COMPONENT

Contains internal pool of actors and distributes incoming messages to them to run messages in parallel.



### PROBES REFERENCE


##### LOG PROBE REFERENCE

??? TBD

- Pulse everytime a function is called
- Pulse everytime a clock ticks
- Record value of all clocks at all time, including process PC. Oscilloscope & log


##### PROFILE PROBE REFERENCE


##### WATCHDOG PROBE REFERENCE

??? TBD


##### BREAKPOINT PROBE REFERENCE

??? TBD


##### ASSERT PROBE REFERENCE


### TWEAKERS REFERECE


##### COLLECTION SELECTOR TWEAKER - REFERENCE

??? TBD

Defines rules for which collection backend to use when.


##### CACHE TWEAKER REFERENCE

??? TBD

A cache will store the result of a previous computation and if a future computation uses the same function and same inputs, the execution is skipped and the previous value is returned directly.

A cache is always a shortcut for a (pure) function. You can decide if the cache works for *every* invocation of a function or limit the cache to invocations of the function within specified parent part.

- Insert read cache
- Insert write cache
- Increase random access speed for dictionary
- Increase forward read speed, stride
- Increase backward read speed, stride


##### EAGER TWEAKER REFERENCE

??? TBD

Like a cache, but calculates its values *before* clients call the function. It can be used to create a static lookup table at app startup.


##### BATCH TWEAKER REFERENCE

??? TBD

When a function is called, this part calls the function with similar parameters while the functions instructions and its data sits in the CPU caches.

You supply a function that takes the parameters and make variations of them.

- Batching: make 64 value each time?
- Speculative batching with rewind.


##### LAZY TWEAKER REFERENCE

??? TBD

Make the function return a future and don't calculate the real value until client accesses it.
- Lazy-buffer


##### PRECALC TWEAKER REFERENCE

Prefaclucate at compile time and store statically in executable.
Precalculate at container startup time, store in FS.
- Precalculate / prefetch, eager


##### CONTENT DE-DUPLICATION TWEAKER REFERENCE

- Content de-duplication
- Cache in local file system
- Cache on Amazon S3


##### TRANSFORM MEMORY LAYOUT TWEAKER REFERENCE

??? TBD

Turn array of structs to struct of arrays etc.
- Rearrange nested composite (turn vec<pixel> to struct{ vec<red>, vec<green>, vec<blue> }


##### SET THREAD COUNT & PRIO TWEAKER REFERENCE
- Parallelize pure function



### THE FILE SYSTEM PART

??? TBD
All file system functions are blocking. If you want to do something else while waiting, run the file system calls in a separate clock. There are no futures, callbacks, async / await etc. It is very easy to write a function that does lots of processing and conditional file system calls etc and run it concurrently with other things.

??? Simple file API
	file_handle open_fileread(string path) unpure
	file_handle make_file(string path) unpure
	void close_file(file_handle h) unpure

	vec<ubyte> v = readfile(file_handle h, int start = 0, int size = 100) unpure
	delete_fsnode(string path) unpure

	make_dir(string path, string name) unpure
	make_file(string path, string name) unpure


### THE REST-API PART

??? TBD


### THE S3 BLOB PART

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistance. Uses SHA1 and content deduplication.


### THE LOCAL FS BLOB PART

??? TBD

Use to save a value to local file system efficiently. Only diffs are stored / loaded. Allows cross-session persistance. Uses SHA1 and content deduplication.



### THE ABI PART

??? TBD

This is a way to create a component from the C language, using the C ABI.



