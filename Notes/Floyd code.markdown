# Floyd Programming Language

### GOALS
1. Floyd prefers C-like / Java syntax to appeal to imperative programmers.
2. Automatic - Avoid having to repeat things: no need to implement equal() etc manually. No need to initialize every member in each constructor etc.
3. Explicit - One simple and clear way to do everything, little opportunity to do creative coding. Built-in specific mechanisms for all common stuff.
4. Composability
5. Performance: built in profiling and hardware cache. Ballpark C++ performance.
6. Solid code and techniques
7. Normalized source code format allows round-trip tools / visual editing

	
# Reference
	
	
# BASIC TYPES
These are the privitive data types built into the language itself. The goals is that all the basics you need are already there in the language.
- This makes it easy to start making useful programs, you don't need to chose or build the baiscs.
- It allows composability since all libraries can rely on these types and communicate bewteen themselves using them. Reduces need for custom types and glue code.

- **float**			Same as float64
- **float32**		32 bit floating point
- **float64**		64 bit floating point
- **int**			Same as int64
- **int8**			8 bit integer
- **int16**
- **int32**
- **int64**
- **bool**			**true** or **false**
- **enum**			(same as struct with only static constant data members)

- **struct**		like C struct or class or tuple.

- **string**		built-in string type. 8bit pure (supports emedded nulls).
	- 					Use for computer strings. Not localizable.
- **text**			same as string but for user-texts.
	- 				Always unicode denormalization C. Stores Unicode code points, not bytes.
- **map**			look up values from a key. Localizable.
- **vector**		look up values from a 0 based continous range of integer indexes.

- **dyn**<>			dynamic type, tagged union.

- **?**				the type postfix makes the value optional (can be null).
- **null**			represents nothing / missing value.
- **hash**			160 bit SHA1 hash
- **exception**		A built in, fixed tree of exception types,
					including **runtime_exception** and **defect_exception**
- **path**			specified a number of named (or numbered) hopes through
	- 				a nested data structure.
- **protocol**

- **rights**		Communicates between function and caller what access rights
					and what layer int the system this functions operates on.

# STRUCTs
Structs are the central building blocks for composing data in Floyd. They are used for structs, classes, tuples. They are always value classes and immutable. Internally, value instances are often shared to conserve memory and performance. They are true-deep - there is no concept of pointers or references or shared structs (from the programmer's point of view). **True-deep** is a Floyd term that means that all values and sub-values are considered equally.

The order of the members inside the struct (or collection) is important for comparisons since those are done member by member.

You can chose to name members or not. An unnamed member can be accessed using its position in the struct: first member is 0.

Notice: a struct can expose a set of read properties but have a different set of arguments to its constructor(s). This in affect makes the constructor / property getters _refine_ the input arguments -- the struct is not a passive container of values but a function too. **??? Good or bad?

- **Data member** = memory slot.
- **Property** = member exposed to outside - either a memory slot or a get/set.
- Private member data = memory slot hidden from clients.
All member functions works like C++ static members and takes *this* as their first, excplict argument. There is nothing special about _this_ - it's only a convention.

## Struct Signature
This is the properties exposed by a struct in the order they are listed in the struct. This is affected by private and properties.

* Users of the struct are not affected if you introduce a setter or getter to a property. They are also unaffected if you move a function from being a member function to a free function - they call them the same way.
* There is no implicit _this_.

## Member-data
* All members have default value.
* All data is always read-only for clients. Member functions can mutate local-variable instances of the struct.
* Clients can directly access all member variables. But you can explicitly hide member data by prefixing with "private".
* You can take control over reading / writing members by replacing its default value with a get / set function. 

## Invariant
_invariant_ is required for structs that have dependencies between members (that have setters or mutating members) and is an expression that must always be true - or the song-struct is defect. The invariant function will automatically be called before a new value is exposed to client code: when returning a value from a constructor, from a mutating member function etc.

	### Allow adding invariant to _any_ type and getting a new, safer type. Cool for collections (they are sorted!) or integers (range is 0 to 7).

### Built in Features of Structs
Destruction: destruction is automatic when there are no references to a struct. Copying is done automatically, no need to write code for this. All copies behave as true-deep copies.
All structs automatically support the built in features of every data type like comparison, copies etc: see separate description.

Notice about optimizations: many accellerations are made behind the scenes:
- causing a struct to be copied normally only bumps a reference counter and shares the data via a reference. This makes copy fast. This makes equality fast too -- the same objects is always equal.
- You cannot know that two structs with identical contents use the same storage -- if they are created independenlty from each other (not by copying) they are not necessarily deduplicated into the same object.
- There is no way for client code to see if the struct instances are _the same_ - that is access the same memory via two separate pointers.

### Mutation
* When client attempts to write to a member variable of a struct, it gets a new copy of the struct, with the change applied. This is the default behaviour.
* If a struct wants to control the mutation, you can add your own setter function as a member function.
* You can add any type of mutating functions, not just setters.
* It is often (but not always) possible to make an free function that mutates a value. If it can read all data from the original and can control all member data of the newly constructed value.

### Tuples (unnnamed structs)
You can access the members using indexes instead of their names, for use as a tuple.

	a = struct { int, int, string }( 4, 5, "six");
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
	
	b = struct { 4, 5, "six" };
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");

#### Anonymous structs
Unnamed structs / tuples with the same signature are the considered the same type and you can automatically assign between them.






### Features of Values - Structs, Basic types, collections - every type
Built into every data structure: basic types, structs, string, collections etc.
* string **serialize**(T a)
* T **T.deserialize**(string s)
- **hash hash(v)** -- built in function that computes a true-deep hash of the value.
	x = make_some_object()
	my_hash = get_hash(x)
- **diff(a, b)** -- this is an automatic diff-function that returns a path to the first members that diff between a and b. This is a true-deep operation. ### Research how git does it.
- **a = b** This true-deep copies the value b to the new name a.
	- Auto conversion: You can directly and silently convert structs between different types. It works if the destination type has a contructor that exactly matches the struct signature of the source struct.
- **a < b**: tests all member data in the order they appear in the struct. Only public data and public getters are checked.
- **a <= b**, **a > b**, **a >= b** -- these are derivated of a < b

- ### add deduplicate feature / function / tag?


## STRUCT EXAMPLES

	struct song {
		float length = 0;
		float? last_pos = null;
		dyn<int, string> nav_info = "off";

		//	Demonstrate replacing a member property with accessors, without affecting clients.
		bool selected {
			get { return !selected_internal }
			set { return assoc(
		}
		private bool selected_internal = true;

		private float internal_play_pos = 0.0f;

		float play_pos(song original) {
			return internal_play_pos / 1000.0f;
		}
		float play_pos2(song original) {
			return internal_play_pos;
		}

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
		song(float bars ( >=0 && @ <= 128.0f) ){
			mut temp = song();
			temp.length = bars * 192;
			return temp;
		}
	}








#### Enums
Works like usual, but can be extended.

enum preset_colo {
	red = 2,
	green,
	blue
}

enum preset_color_ex : preset color {
	yellow,
}


### Variables and constants
All variables are by default constants / immutable.

	assert(floyd_verify(
		"int hello1(){
			a = "hello";
			a = "goodbye";	//	Compilation error - you cannot change variable a.
			return 3;
		}"
	) == -3);

Use _mut_ to define a local variable that can be mutated. Only _local variables_ can be mut.

	int hello2(){
		mut a = "hello";
		a = "goodbye";	//	Changes variable a to "goodbye".
		return 3;
	}

You can postfix type with ? to make it optional.


# ABOUT COLLECTIONS
A fixed set of collections are built right into Floyd. They can store any Floyd value, including structs, tuples, string, integers and even other collections. They are all immutable / persistent.

A unique feature in Floyd is that you cannot specify the exact implementation of the collection, only its basic type: vector or map. The precise data structure is selected by the runtime according to your recommendations and the optimiser / profiler.

## VECTOR
A vector is a collection where you lookup your values using an index between 0 and (vector_size - 1). The items are ordered. Finding the correct value is constant-time. There are many potential backends for a vector:

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read an write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No read/write.

??? Explore functions vs vectors vs maps. Integer keys vs float keys. sin(float a)?

	b = [int](1, 2, 3);

## MAPS
map<K, V>

	struct {
		my_map = [string, int]({ "one", 1 }, { "two", 2 });
		my_map2 [string, int]({ "one", 1 }, { "two", 2 });
	}
	int function test_map([string, int] p1);
	a = {string, int}({ "one", 1 }, { "two", 2 }, { "three", 3 });

## SEQ
Seq is an interface built into every collection and struct. It allows you to read from the source, one entry at a time, in an immutable way.

	int f2(seq<int> data){
		a = data.first;
		b = data.rest;
	}

	//	Vectos (and all collections, compoisites) support the seq interface so you can convert it to a seq:
	result = f2([int](1, 2, 3))

# FUNCTIONS
Function always have the same syntax, no matter if they are member functions of a struct, a constructor or a free function.

NOTE: A function is a thing that takes a struct/tuple as an argument and returns another struct/tuple or basic type.

	### The arguments for a function is a struct. Function internally only take one argument.

Closures

## Internals
	function definition:
		input type
		output type
		namespace

	function_closure
		function_closure* parent_closure;
		function_def* f
		local_variables<local_var_name, dyn<all>>{}
		
Functions always return exactly one value. Use a tuple to return more values. A function without return value makes no sense since function cannot have side effects.

You can use maps, vector and functions the same way. Since maps and vectors are immutable and functions are pure, they are equivalen when they have the same signature.

	string my_function(int a){
		return a == 1 ? "one", a == 2 ? "two", a == 3 ? "three";
	}
	my_map = [string, int]( 1: "one", 2: "two", 3: "three");
	my_vector = [string]( "one", "two", "three" );

How to add docs? Markdown. Tests, contracts etc.

## FUNCTION CONTRACTS
Arguments and return values always specifies their contracts using an expression within []. You can use constants, function calls and all function arguments in the expressions.

	int [_ >=0 && _ <= a] function test1(int a, int b){
	}

	function1 = function {
		_: int [_ >= 0 && _ <= a]
		a: a >= 0 && a < 100, "The minor flabbergast"

	}

## FUNCTION PROOFS
	prove(function1) {
		"Whenever flabbergast is 0, b don't matter) and result is 0
		function1(0, 0) == 0
		function1(0, 1) == 0
		function1(0, 10000) == 0

		function1(1, 0) == 0
		function1(1, 1) == 8
	}

??? Freeze complex inputs to function and serialzie them for later exploration, adding proofs and keeping as regression tests.

Example function definitions:

	int f1(dyn<string,int> x){
	}

	int f2(int a, int b){
	}

	int f3(){	
	}

	int f4(string x, bool y)


## Lambdas
foreach(stuff, a => return a + 1)
foreach(stuff, () => {})
foreach(stuff, (a, b) => { return a + 1; })
They are closures.


int main(string args){
	log(args);
	return 3;
}


## RIGHTS
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



### Dynamic type / tagged union
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


### Typedef
A typedef makes a new, unique type, by copying another (often complex) type.
	typedef [string, [bool]] book_list;
	typedef 

### Paths
You can make a path through nested data structures. The path is a built-in opaque type.

# Value serialization
Serializing a value is a built in mechanism. It is based on the seq-type.
It is always deep. ### Always shallow with interning when values are shared.

The result is always a normalized JSON stream.

??? De-duplication vs references vs equality vs diffing.

//	Define serialised format of a type, usable as constant.

 ### floud code = stored / edited in the serialized format.

An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream.

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

### Statements
if(){
}
else if(){
}
else{
}

foreach(a in stuff){
}
foreach(stuff, () => {})

return a < b ? 1 : 2

assert()
test
log()
prove


switch()





### Reserved Keywords



|   	|   	|   	|   	|   	|
|---	|---	|---	|---	|---	|
|   	|   	|   	|   	|   	|
|   	|   	|   	|   	|   	|
|   	|   	|   	|   	|   	|


?
assert
bool
catch
char???
code_point
clock
deserialize()
diff()
double
dyn
dyn**<>
else
ensure
enum
false
float
foreach
hash
hash()
if
int
int16
int32
int64
int8
invariant
log
map
mutable
namespace?
null
path
private
protocol
prove
return
require
seq
serialize()
string
struct
swap
switch
tag
test
text
true
try
typedef
typeof
typecast
vector
while






# Text features
text
code_point
a = localizeable("Please insert data type into terminal...");

Tool extracts and switches text inside localizeable() automatically.

# Binary data features

# File features








# ??? OPEN QUESTIONS
	### fixed_size_vector

??? Global constants / static?
Namespaces
Serializing abstraction
Externals


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

return a < b ? 1 : 2

a,b ?
	(_0 == 0 : log("ONE")) :
	(_0 == _1 : log("TWO")) :
}
