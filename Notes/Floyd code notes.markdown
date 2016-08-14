# FLOYD RUNTIME
- Static profiler - knows ballpark performance and complexity directly in editor. Optimize complexity when data sets are big, else brute force.
- Runtime does optimizations in the same way as Pentium does optimizations at runtime. Caches, write buffers, speculation, batching, concurrency. Nextgen stuff.
- No aliasing or side-effects = maximum performance.
- ### Compile for GPUs too.


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


# FUNCTIONS


	### The arguments for a function is a struct. Function internally only take one argument. The args-struct is called "args" and can be accessed like any value.
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


	
# STRUCTs

## Invariant
_invariant_ is required for structs that have dependencies between members (that have setters or mutating members) and is an expression that must always be true - or the song-struct is defect. The invariant function will automatically be called before a new value is exposed to client code: when returning a value from a constructor, from a mutating member function etc.

	### Allow adding invariant to _any_ type and getting a new, safer type. Cool for collections (they are sorted!) or integers (range is 0 to 7). Combine this with typedef feature.

	### define invariant will block default constructors 

---Notice: a struct can expose a set of read properties but have a different set of arguments to its constructor(s). This in affect makes the constructor / property getters _refine_ the input arguments -- the struct is not a passive container of values but a function too. **??? Good or bad?

- **Data member** = memory slot.
- **Property** = member exposed to outside - either a memory slot or a get/set.
- **Private member data** = memory slot hidden from clients.
All member functions works like C++ static members and takes *this* as their first, explict argument. There is nothing special about _this_ - it's only a convention.


??? invent way to separate member variable names, locals and arguments and globals so they don't collide. Use ".red = red"?


######################################
### separate clients from how struct members & data is implementeed. Allow refactoring.


### Separate needs into clearer parts:
	1) private / public is access control for clients and for struct implementation.
	
	2) Clients should not be affected with if a struct value is data or calculated.




# STRUCT EXAMPLES
When defining a data type (composite) you need to list 4 example instances. Can use functions to build them or just fill-in manually or a mix. These are used in example docs, example code and for unit testing this data types and *other* data types. You cannot change examples without breaking client tests higher up physical dependncy graph.



??? split / join cables from visual language

# Struct Examples

	struct song {
		float length = 0;
		float? last_pos = null;
		dyn<int, string> nav_info = "off";

		song scale_song(song original){
			mut temp = original;
			temp.length = temp.length / 2.0f;
			temp.last_pos = temp.last_pos / 2.0f;
			temp.internal_play_pos = internal_play_pos / 2.0f;
		}

		invariant(song a) {
			a.length > 0
			a.last_pos == null || a.last_pos <= length;
		}

		song(){
			return song();
		}
		song(float bars (_ >=0 && _ <= 128.0f) ){
			mut temp = song();
			temp.length = bars * 192;
			return temp;
		}
	}



# HINTS-SYSTEM
	### add hints system for optimization
	### Add profiler features



# MAP
??? Rename "dict"?
C++11
	map<int, char> m = {{1, 'a'}, {3, 'b'}, {5, 'c'}, {7, 'd'}};
	std::map<int, std::string> m{{1, "Hello"}, {2, "world"}, {4, "!!!"}};

	reference: https://mbevin.wordpress.com/2012/11/16/uniform-initialization/
Clojure
	{:a 1, :b 2, :c 3}
	(assoc {:a 1, :b 2} :b 3)
	reference: https://adambard.com/blog/clojure-in-15-minutes/



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
map
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






# EQUIVALENCE: FUNCTIONS - MAPS - VECTORS
??? Explore functions vs vectors vs maps. Integer keys vs float keys. sin(float a)?

You can use maps, vector and functions the same way. Since maps and vectors are immutable and functions are pure, they are equivalen when they have the same signature.

	string my_function(int a){
		return a == 1 ? "one", a == 2 ? "two", a == 3 ? "three";
	}
	my_map = [string, int]( 1: "one", 2: "two", 3: "three");
	my_vector = [string]( "one", "two", "three" );


# LAMBDAS
foreach(stuff, a => return a + 1)
foreach(stuff, () => {})
foreach(stuff, (a, b) => { return a + 1; })
They are closures.


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

map<int, @>{
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

The returned value from unpack_json() is of type *dyn<string, number, vec<dyn>, map<string,dyn>,bool>* and can hold any value returned.

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
