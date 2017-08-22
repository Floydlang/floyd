

# FUTURE BUILT-IN TYPES

- **text**			same as string but for user-texts.
	 				Always unicode denormalization C. Stores Unicode code points, not bytes.

- **code_point**	A Unicode code point.

- **hash**			160 bit SHA1 hash
- **exception**		A built in, fixed tree of exception types,
					including **runtime_exception** and **defect_exception**
- **path**			specified a number of named (or numbered) hopes through
	 				a nested data structure.
- **seq**
- **dyn**<>			dynamic type, tagged union.


# C TYPES
**C**			**Floyd**
float 			float
double			float80
int				always int32
int8_t			int8
int16_t			int16
int32_t			int32
uint32_t		typedef int32(



# FUTURE
- **protocol**		polymorhpism. Like an interface class.

- **rights**		Communicates between function and caller what access rights
					and what layer int the system this functions operates on.

# OPTIONAL VALUES
- **?**				the type postfix makes the value optional (can be null).
						int? a = 40
						assert(a.value == 40)
						int? b = null
						assert(b.null)


??? references needed? To make linked list?



# CORE TYPE FEATURES
- **diff(a, b)**									returns a path to the first members that diff between a and b. This is a true-deep operation.  ??? Research how git does diff. Better if diff() returns a seq that enumerates each diff deeply?

- ### add deduplicate feature / function / tag?


# Conversions
All data types with the same signature are can automatically be assign between.

??? Examples


# FOLD, FILTER, MAP, REDUCE

# FUNCTIONS


	### log()
	### assert()
	### Closures
	

# IF - THEN -ELSE

	You can use a lambda to do directly assign a value from an if-then-else:

		int v = () => {
			if(s == "one"){
				return 1;
			}
			else if(s == "two"){
				return 2;
			}
			else{
				return -1;
			}
		}



	??? Idea: allow unnamed helper functions, hidden from clients. With separate tests.


# schematic tables
a	->		1	0	0	0
b	->			1		0
c	->				1	0
x	<-		1	2	2	3








# HINTS-SYSTEM
	### add hints system for optimization
	### Add profiler features



# DICTIONARY refs
C++11
	map<int, char> m = {{1, 'a'}, {3, 'b'}, {5, 'c'}, {7, 'd'}};
	std::map<int, std::string> m{{1, "Hello"}, {2, "world"}, {4, "!!!"}};

	reference: https://mbevin.wordpress.com/2012/11/16/uniform-initialization/
Clojure
	{:a 1, :b 2, :c 3}
	(assoc {:a 1, :b 2} :b 3)
	reference: https://adambard.com/blog/clojure-in-15-minutes/

Dictionaries can contain null values


# SEQ
Seq is an interface built into every collection and struct. It allows you to read from the source, one entry at a time, in an immutable way.

	int f2(seq<int> data){
		a = data.first;
		b = data.rest;
	}

Vectors (and all collections, composites) support the seq interface so you can convert it to a seq:
		result = f2([int](1, 2, 3))
You can even do this with a struct: the members will be returned in order.

cons, first, and rest
http://www.braveclojure.com/core-functions-in-depth/
Lazy
into == adds stuff to a collection (empty or not) from a seq (or collection)

concat
dict
filter
reduce
take, take-white
drop, drop-while


seq<string> a = [string]( "one", "two", "three" );
assert(a.first == "one");
assert(a.rest == [string]( "two", "three" )));

b = into([string][], a.rest);

[string]


# FUNCTION DOCUMENTATION
How to add docs? Markdown. Tests, contracts etc.

	### Idea: Source code is markdown, add code segments:
		# This is my source file
			int my_const = "hello";
			int main(string args){
				return
			}


# FUNCTION CONTRACTS
Arguments and return values always specifies their contracts using an expression within []. You can use constants, function calls and all function arguments in the expressions.

	int [_ >=0 && _ <= a] function test1(int a, int b){
	}

	function1 = function {
		_: int [_ >= 0 && _ <= a]
		a: a >= 0 && a < 100, "The minor flabbergast"
	}
	
	### float< sin(float v<[_ >= 0.0f && _ <= 1.0f>);


# FUNCTION TESTS
	prove(function1) {
		"Whenever flabbergast is 0, b don't matter) and result is 0
		function1(0, 0) == 0
		function1(0, 1) == 0
		function1(0, 10000) == 0

		function1(1, 0) == 0
		function1(1, 1) == 8
	}

??? Freeze complex inputs to function and serialzie them for later exploration, adding proofs and keeping as regression tests.

Special proof-construct (not a function but built in like "struct":
		on / off
		prove {
			a = test_object1();
			b = do_stuff
			verify(a == b)

		}
		Better: make all tests lists of expression vs expected results as a JSON.


# EQUIVALENCE: FUNCTIONS - DICTS - VECTORS
??? Explore functions vs vectors vs dicts. Integer keys vs float keys. sin(float a)?

You can use dicts, vector and functions the same way. Since dicts and vectors are immutable and functions are pure, they are equivalen when they have the same signature.

	string my_function(int a){
		return a == 1 ? "one", a == 2 ? "two", a == 3 ? "three";
	}
	my_dict = [string, int]( 1: "one", 2: "two", 3: "three");
	my_vector = [string]( "one", "two", "three" );




# RIGHTS
Communicates between function and caller what access rights and what layer int the system this functions operates on.
- This gives function can take one or several rights as argument. These are protocols that gives your function access to functions and privileges it doesn't otherwize have. A large composite program consists of layers of runtimes and _rights_ is the way for a function to communicate it requires access. This makes sure layring is consistent and explicit. You can easily make composite rights by listing several or using +

//	Requires the core functionallity to be available. Cannot be called from low-level init.
int f2(rights<core>, int a, int b){
}

//	Requires the UI system to be up and running, pumping Window system events etc.
int f3(rights<core, ui>, int a, int b){
}

int f4(rights<core, ui>, int a, int b){
}

rights ui {
	ui_state show_alert(ui_state ui, text title, text message, text button);
	ui_state show_alert(ui_state ui, text title, text message, text button1, text button2);
	ui_state show_progress(ui_state ui, text title);
}

rights core {
	log(string message);
	log_indent(string message);
	log_deintent();
	make_world();
}

## Function Internals
	function definition:
		input type
		output type
		namespace

	function_closure
		function_closure* parent_closure;
		function_def* f
		local_variables<local_var_name, dyn<all>>{}



# DYN
dyn<float, string> // one of these. Tagged union. a

	a = dyn<string, int>("test");
	assert(a == "test");
	assert(a.type == string);
	assert(a.type != int);
	assert(a.type != bool);

	b = dyn<string, int>(14);
	assert(a == 14);
	assert(a.type != string);
	assert(a.type == int);
	assert(a.type != bool);


# TYPEDEF
A typedef makes a new, unique type, by copying another (often complex) type.
	typedef [string, [bool]] book_list;
	### You also add additional invariant. sample_rate is an integer, but can only be positive.


# PATHS
You can make a path through nested data structures. The path is a built-in opaque type.

	let my_path = &my_document.text_block[3].footer_old.fragment[ix].text;
	### operations.
	### show as a string / file path.

See "JSON pointers" spec.


# PROTOCOLS
A protocol is a type that introduces zero to many functions signatures without having any implementation. The protocols have no state and no functionallity.

A struct can chose to implement one or several protocols.
A protocol does NOT mean an is-a hiearchy. It is rather a supports-a. An optional feature.

protocol tooltip_support {
	text get_text(this)
	color get_color(this)
	text_style get_style(this)
}

protocol mouse_support {
	T on_click(float x, float y)
	T on_release(float x, float y)
}

protocol seq {
	T first()
	seq rest()
}

//	Implements mouse_support and tooltip_support
struct my_window {
	mouse_support {
		T on_click(this, float x, float y){
			return this;
		}
		T on_release(this, float x, float y){
		}
	}
	tooltip_support {
		text get_text(this){
		}
		color get_color(this){
		}
		text_style get_style(this){
		}
	}
}


# sha1 type
# uuid type
# Uri type
# Date type

# TEMPLATES


# EXTERNALS


# CLOCKS


# CONCURRENCY


# FILE SYSTEM FEATURES


# OPTIONALS VALUES, NULL AND ?

"Haskell for example provides the Maybe monad instead, which forces the programmer to consider the "null case"."

int? a = b > 0 ? b : null;

	### Design a syntax for force client to always get and consider both null and value, like a visitor.
		b.resolve
		b.null 


# TEXT FEATURES
text
code_point
a = localizeable("Please insert data type into terminal...");

Tool extracts and switches text inside localizeable() automatically.
Grapehan
template_variable_injection(TEXT"Hello, \(name), do you want some \(food)?")


# BINARY DATA FEATURES




# ??? OPEN QUESTIONS


### SWITCH OPEN PROBLEM

switch(a){
	case "one":
		log("ONE");
	case "two":
		log("TWO");
	case "three":
		log("THREE");
	default
		log("...");
}

if(a == "one"){
	log("ONE");
}
else if(a == "two"){
	log("TWO");
}
else if(a == "three"){
	log("THREE");
}
else {
	log("...");
}

switch(a){
	case("one"){
		log("ONE");
	}
	case("two"){
		log("TWO");
	}
	case("three"){
		log("THREE");
	}
	default{
		log("...");
	}
}

switch(a){
	case(== 1){
		log("ONE");
	}
	case(< 3){
		log("TWO");
	}
	case(== 3){
		log("THREE");
	}
	case(> 3){
		log("...");
	}
}

dict<int, @>{
	1, {
		log("ONE")
	},
	2, {
		log("TWO")
	},
	3, {
		log("THREE")
	}
}[a]();

switch(a){
	(_ == 1) {
		log("ONE")
	}
	(_ == 2) {
		log("TWO")
	}
	(_ > 2){
		log("THREE")
	}
	(_ > 3){
		log("...")
	}
}

a,b ?
	(_0 == 0 : log("ONE")) :
	(_0 == _1 : log("TWO")) :
}


# FEATURE IDEAS
	### Add sysex-feature for doing custom extensionto languages that can later be adopted.
	### Clearly separate language constructs vs user code. 
	
	
	
#build in:
unicode
json
date
rest
sha1
url
file path

# NAMESPACES




# SOURCE CODE FILES
All source code is markdown. Open and edit using Markdown editor.
Code is insert as markdown code-segments
Comments & docs is one concept, disabling code is another concept. All code is always compiled to keep it working - no #if 0.

	proofs [
		//	Make some values for test.
		empty = [pixel_t]{};
		image1x1 = [pixel_t]{};

		smilie = from_json()


		[ make_image(0,0), [pixel_t]{}, "empty" ],
		[ make_image(0,0).width, 0, "checks for empty image" ],
		[ make_image(0,0).height, 0, "checks for empty image" ],
	]



# Embedded JSON
Embedd json inside source code file. Simple / no escaping needed. Use to paste data, test values etc. Round trip.

# SERIALIZATION

- **json**		built-in type that models the JSON format. You can initialize it from a JSON literals -- pasted json files, like this:
```
			json_value default_preferences = { "wtitle": "hello", "warnings_on": false }
```


* string **serialize**(T a)					converts value T to json
* T **T.deserialize**(string s)			makes a value T from a json string

Serializing a value is a built in mechanism. It is based on the seq-type.
It is always deep. ### Always shallow with interning when values are shared.

The result is always a normalized JSON stream. ???

??? De-duplication vs references vs equality vs diffing.
	### Make reading and writing Floyd code simple, especially data definitions.
//	Define serialised format of a type, usable as constant.

An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream.


## json keyword
This keyword lets you put plain JSON scripts directly into your source code without any need to escape the string. The result is a string value, not any special data type. The json format is validated.
??? how to generate json data / inject variables etc? Maybe better to just have raw-string support?

	json { "age": 10, "last_name": "zoom" };
	json [ "one", "two", "three" ];
	json "one";
	json {
		"age": 10,
		"last_name": "zoom"
	};

	assert(json { "age": 10, "last_name": "zoom" } == "{ \"age\": 10, \"last_name\": \"zoom\" }");	
There are built in features to pack and unpack the JSON data:

	my_json_string = json[
		"+",
		["load",["res_member", ["res_var", "p"], "s"]],
		["load", ["res_var", "a"]]
	];

	my_obj = unpack_json(my_json_string);
	assert(my_obj[0] == "+");
	assert(my_obj[2][0] == "load");

The returned value from unpack_json() is of type *dyn<string, number, vec<dyn>, dict<string,dyn>,bool>* and can hold any value returned.

If you are reading data you can do this:

	string expect_string_load = my_obj[0][2];
	// This throws exception if the value cannot be found or is not of type string = very convenient when parsing files.

	//	Here you get a default value if you cannot read the string.
	string except_string_load2 = try{ my_obj[0][2], "default_string" };


JSON AST -> script file convertion
No preprocessor in floyd - use AST / JSON



# JSON / SERIALIZATION

JavaScript Object Notation (JSON) Pointer: https://tools.ietf.org/html/rfc6901

c++ library -- cool: https://github.com/nlohmann/json


print(a.red)
print(a)	— round trip: JSON? ??? deep vs shallow?
	red: 100
	green: 200
	blue: 210
	alpha: 255

json(a) -- output:
	{
		“red”: 100,
		“green”: 200,
		“blue”:210,
		“alpha”: 255
	}

c = json(
	“{
		“red”: 100,
		“green”: 200,
		“blue”:210,
		“alpha”: 255
	}”


	### Define format for source-as-JSON roundtrip. Normalized source code format. Goal for JSON is easy machine transformation.
!!! JSON does not support multi-line strings.
	### Encode using JSON! Easy to copy-paste, user JSON validators etc:

	"hello[\"troll\"].kitty[10].cat" =>
	"(@load (@res_member (@lookup (@res_member (@lookup (@res_var 'hello') (@k <string>'troll')) 'kitty') (@k <int>10)) 'cat'))"


	["@res_member",
		["@lookup",
			["@res_member",
				["@lookup",
					["@res_member", "nullptr", "hello"],
					["@k", "<string>", "troll"]
				],
				"kitty"
			],
			["@k", "<int>", "10"]
		],
		"cat"
	]


??? nest JSON? Expand or store as string constants?
??? Can JSON be used to store and edit Floyd script?
"Floyd speak"
??? Can JSON be used to serialize all floyd data structures? De-duplication?
??? Call stack and local variables and heap are also just state, just like all other values.





# IMPORT / DEFINE LIBRARIES

# RUNTIME ERRORS / EXCEPTIONS
Principles and policies.
Functions.
	??? Only support this in mutating parts and externals?


# C GLUE


# MISC FEATURE IDEAS
- Allow adding invariant to _any_ type and getting a new, safer type. Cool for collections (they are sorted!) or integers (range is 0 to 7). Combine this with typedef feature.




#EXCEPTIONS

All runtime errors are handled using exceptions. There is a fixed set of exception types that can be thrown. Nothing else can be thrown or caught. You also tell which “subject” = type of target object. The available subject types can be extended by clients.


int CalcStuffy(int a, int b){
	if(abc){
		throw out_of_memory(x_ram)
	}
}

int DoStuff(int x, int y){
	try {
		z = CalcStuff(1 + x, 2 + x);
	}
	catch{
		z = CalcStuff(1 - x, 2 + x);
	}
}

int OnButton(uri path){
	try {
		f = OpenFile(“asdsd”);
		s = ParseFile(f);
		SaveFile(path, s);
	}
	catch(runtime_out_of_memory e){
		if(e._type == x_persistent_storage){
			ShowAlert_HarddiskFull();
		}
		else{
			rethrow;
		}
	}
}


# Built in runtime errors
runtime_out_of_memory
runtime_read_error
runtime_write_error
runtime_illegal_format_error
runtime_unsupported_version_error

//	Built in logic errors
logic_bounds_error
logic_argument_error
logic_assert_error
logic_test_failure

//	Subjects. Extendable.
x_file
x_ram
x_socket
x_persistent_storage

#ddf
EXCEPTIONS vs normal program flow
Where to draw the line?

""Exceptions are for handling the following situation: you can not find out in advance whether an operation will succeed without trying it." - First I note that this criteria applies equally well to returning an error code. That aside..."

1) Main flow: this is the plan A - the work that should be done. It must not trigger exceptions to perform its work. Example: check if you are read the entire file before reading more - do not rely on exceptions for the main flow.

2) Function contract describes different alternative behaviors on runtime errors.

3) Function contract defines what inputs are illegal and defects.

If an error CAN potentially happen in function (= almost all functions) it can OPTIONALLY define explicit exceptions that will be thrown for those cases. The function must guarantee these exceptions are thrown for these cases. It is also given that other, unknown exceptions may be thrown too.

There are be special no-throw functions that are guaranteed to never fail / throw exceptions. These are rare.

often changing the normal flow of program execution.

- If there is no practical / meaningful way to handle the problem as the normal flow of the program.

- Opening a file can fail if file has been deleted. You can preflight that by checking if file exists first.

Example
- While your program reads a large file from disk into RAM and the disks USB-cable is unplugged.
	- It is not

	
	
## Auto conversion --- SOMEDAY MAYBE
You can directly and silently convert structs between different types. It works if the destination type has a contructor that exactly matches the struct signature of the source struct.



??? invent way to separate member variable names, locals and arguments and globals so they don't collide. Use ".red = red"?
We 