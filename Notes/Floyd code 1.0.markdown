# Floyd Programming Language Reference

Floyd is a fast and modern C-like program language that tries to make writing correct programs simpler and faster.

It uses pure functions, immutability and composition.

It as a unique concept to handle mutation of data, control of the outside world (files, network, UI) and controlling time.

Functions and classes are pure. A pure function can only call pure functions. This delegates all mutation / time to the top levels of your program. Here you need to call non-pure functions of the OS. You must tag your functions with "nonpure" to be able to call other nonpure functions.

This makes those risk. Have as little nonpure code as possible. Try to not have any conditional code inside nonpure code - this makes testing easier.


# BASIC TYPES
These are the privitive data types built into the language itself. The goals is that all the basics you need are already there in the language. This makes it easy to start making useful programs, you don't need to chose or build the basics. It allows composability since all libraries can rely on these types and communicate bewteen themselves using them. Reduces need for custom types and glue code.

- **float**					Same as float32
- **float32**				32 bit floating point
- **float80**				80 bit floating point
- **int**					Same as int64
- **int8**					8 bit integer
- **int16**
- **int32**
- **int64**
- **bool**					**true** or **false**
- **string**					built-in string type. 8bit pure (supports embedded nulls).
	 								Use for computer strings. Not localizable.
- **function**				function pointer. Functions can be Floyd functions or C functions.


### MORE TYPES
These are composites and collections of other types.

- **enum**		same as struct with only static constant data members
- **struct**		like C struct or class or tuple.
- **map**			look up values from a key. Localizable.
- **vector**		look up values from a 0-based continous range of integer indexes.


# CORE TYPE FEATURES
These are features built into every type: integer, string, struct, collections etc.

* string **serialize**(T a)					converts value T to json
* T **T.deserialize**(string s)			makes a value T from a json string
- **hash hash(v)**							computes a true-deep hash of the value
- **a = b** This true-deep copies the value b to the new name a.
	- Auto conversion: You can directly and silently convert structs between different types. It works if the destination type has a contructor that exactly matches the struct signature of the source struct.
- **a < b**										tests all member data in the order they appear in the struct.
- **a <= b**, **a > b**, **a >= b**	these are derivated of a < b


# PROOF
Floyd has serveral built in features to check code correctness. One of the core features is assert() which is a function that makes sure an expression is true or the program will be stopped. If the expression is false, the program is defect.

This is used through out this document to demonstrate language features.


# VALUES, VARIABLES AND CONSTANTS
All "variables" aka values are by default constants / immutable.
- Global variables
- Functiona arguments
- Local function variables
- Member variables of structs etc.
- Contents of collections
- 
	assert(floyd_verify(
		"int hello1(){
			a = "hello";
			a = "goodbye";	//	Compilation error - you cannot change variable a.
			return 3;
		}"
	) == -3);

Use _mutable_ to define a local variable that can be mutated. Only _local variables_ can be mutable.

	int hello2(){
		mutable a = "hello";
		a = "goodbye";	//	Changes variable a to "goodbye".
		return 3;
	}


# GLOBAL SCOPE
Used for function definitions, defining constants and structs.


# FUNCTIONS
Functions in Floyd are pure, or referential transparent. This means they can only read their input arguments and constants, never read or modify anything else that can change. It's not possible to call a function that returns different values at differenttimes, like get_time().

Function always have the same syntax, no matter if they are member functions of a struct, a constructor or a free function.

NOTE: A function is a thing that takes a struct/tuple as an argument and returns another struct/tuple or basic type.

Functions always return exactly one value. Use a tuple or struct to return more values.

A function without return value usually makes no sense since function cannot have side effects. Possible uses would be logging, asserting or throwing exceptions.

Example function definitions:

	int f1(string x){
		return 3;
	}

	int f2(int a, int b){
		return 5
	}

	int f3(){	
		return 100;
	}

	string f4(string x, bool y){
		return "<" + x + ">";
	}


# IF - THEN -ELSE
This is a normal if-else-if-else construct, like in most languages.

		if(s == "one"){
			return 1;
		}
		else if(s == "two"){
			return 2;
		}
		else{
			return -1;
		}

In each body you can write any statements. There is no "break" keyword. { and } are required.


# QUESTION OPERATOR

	bool is_polite(string x){
		return x == "hello" ? "polite" : "rude"
	}
	assert(is_polity("hiya!") == false);
	assert(is_polity("hello") == true);


# EXPRESSIONS
		http://www.tutorialspoint.com/cprogramming/c_operators.htm
Comparisons are true-deep - they consider all members and also member structs and collections.


###	Arithmetic Operators
	
	+		Adds two operands.																					"A + B = 30"
	−		Subtracts second operand from the first.												"A − B = -10"
	*		Multiplies both operands.																		"A * B = 200"
	/		Divides numerator by de-numerator.														"B / A = 2"
	%		Modulus Operator and remainder of after an integer division.				"B % A = 0"


### Relational Operators
	a == b				true if a and b have the same value
	a != b				true if a and b have different values
	a > b				true if the value of a is greater than the value of b
	a < b				true if the value of a is smaller than the value of b
	>=
	<=


### Logical Operators
	&&
	||
	!


### Bitwise Operators

	One operand:
	!					inverts a bool: true becomes false, false becomes true.
	~
	^

	+
	-
	*
	/
	%
	+=
	-=
	++
	--


# LOOPS AND CONDITIONS
The foreach uses the seq-protocol to process all items in a seq, one by one.

foreach(a in stuff){
}
foreach(thing, [string]{ 1, 2, 3 })

int r = a < b ? 1 : 2;

switch() -- has modern update, no default or break.

	for(int i = 0 ; i < 100 ; i++){
	}

while(expression){
}


# STRUCTs
Structs are the central building blocks for composing data in Floyd. They are used for structs, classes, tuples. They are always value classes and immutable. Internally, value instances are often shared to conserve memory and performance. They are true-deep - there is no concept of pointers or references or shared structs (from the programmer's point of view).

	struct pixel { 
		int red;
		int green;
		int blue;
	};


**True-deep** is a Floyd term that means that all values and sub-values are considered, for example in equally checks. The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

You can chose to name members or not. An unnamed member can be accessed using its position in the struct: first member is "0", second is called "1" etc. Access like: "my_pixel.0". Named members are also numbered - a parallell name.


## Struct Signature
This is the properties exposed by a struct in the order they are listed in the struct.

* Users of the struct are not affected if you introduce a setter or getter to a property. They are also unaffected if you move a function from being a member function to a free function - they call them the same way.
* There is no implicit _this_.

Clients to a struct should not care if:
1) A struct property is a data member or a function, ever. Always the same syntax.
2) a function has private access or not to the struct. Always the same syntax.
3) a function is part of the struct or provided some other way. Always the same syntax.

This makes it easier to refactor code: you can change a function from non-member to member without affecting any client code!


## Member Data
All members have default value. This makes simple value classes extremely simple to create.
Clients can directly access all member variables, unless they are marked as "private".
You can take control over reading / writing members by replacing its default getter / setter with your own functions. 

	struct test {
		float f;
		float g = 3.1415f;
	};

Simple but fully functional struct. "f" will be initialized to 0.0f, g will be initialized to 3.1415f.

??? How to block mutation of member variable? Some members makes no sense to write in isolation.


## Constructors
These are member functions that are named the same as the struct that sets up the initial state of a struct value. All member variables are first automatically initialized to default values: either explicitly or using that type's default. A constructor overrides these defaults. The value does not exist to the rest of the program until the constructor finishes.

Two constructors are automatically generated:
	The first one takes no argument and initializes all members to their defaults
	The second one has one argument for each member, in the order they appear in the struct.


## Using default contructors:

	struct test {
		float f;
		float g = 3.1415f;
	}

	a = test();
	assert(a.f == 0.0f);
	assert(a.g == 3.1415f);

	b = test(1.1f, 2.2f);
	assert(b.f == 1.1f);
	assert(b.g == 2.2f);

You cannot stop these two contructors, but you can reimplement them if you wish.


## Explicit constructors

	struct pixel { 
		pixel pixel(int gray){
			//	Use built in constructor 2.
			return pixel.make(gray, gray, gray);
		}

		int red;
		int green;
		int blue;
	};

	//	Construct a new gray pixel, called a. Check its values. 
	a = pixel(100);
	assert(a.red == 100 && a.green == 100 && a.blue == 100);

	//	Make a pixel from red-green-blue values and read it back. Compare it to another pixel with same values.	
	b = pixel(10, 20, 30);
	assert(b.red == 10 && b.green == 20 && b.blue == 30);
	assert(b == pixel(10, 20, 30);


## Member Functions
Member functions have access to all private members, external functions only to public members of the struct.
You call a member function just like a normal function, draw(my_struct, 10).
There is no special implicit "this" argument to functions, you need to add it manually.
It is not possible to change member variables, instead you return a completely new instance of the struct.

	struct pixel { 
		/*
			This is how the implicitly generated getter for member "red" looks.
			You can make your own to override it. Any function that takes a single argument of
			a type can be called as if it's a readable propery of the struct.
		*/
		int red(pixel p){
			return red;
		}

		/*
			This is how the implicitly generated setter for member "red" looks.
			You can make your own to override it. Any function that takes a two
			arguments (struct type + one more) can be called as if it's a writable propery of the struct.
		*/
		pixel red(pixel p, int value){
			return pixel(value, p.green, p.blue);
		}

		int magneta(pixel p){
			return (p.red + p.green) * 0.3;
		}

		//	Member functions.
		int intensity(pixel this){ return (red + green + blue) / 3; };

		//	Member values.
		int red;
		int green;
		int blue;

		//	Constant pixels.
		black = pixel(0, 0, 0);
		white = pixel(255, 255, 255);
	};

	//	External function.
	int get_sum(pixel p){ return p.red + p.green + p.blue; };

	my_pixel = pixel(100, 200, 50);
	int red = my_pixel.red;

	sum1 = get_sum(my_pixel);
	intensity1 = get_intensity(my_pixel);


## Alternative function call style
??? Bad to have several ways to call functions...
Any function with a pixel as first argument (member functions and non-member functions) can all be called like this.

	sum1 = get_sum(my_pixel);
	sum2 = my_pixel.get_sum();

	intensity1 = get_intensity(my_pixel);
	intensity2 = my_pixel.get_intensity();

	int green1 = my_pixel.green;
	int green2 = green(my_pixel);
	int green3 = my_pixel.green(my_pixel);

	pixel2 = my_pixel.green = 60;
	pixel3 = green(my_pixel, 60);
	pixel4 = my_pixel.green(my_pixel, 60);


## Persistence - mutating / changing an immutable struct
* When client attempts to write to a member variable of a struct it gets a new copy of the struct, with the change applied. This is the default behaviour.
* If a struct wants to control the mutation, you can add your own setter function or other mutating function. Either as a member function or an external function.

Changing member variable of a struct:

	a = pixel(255, 255, 255);

	//	Storing into a member variable is an expression that returns a new value,
	//	not a statement that modifies the existing value.
	a.red = 10;
	assert(a == pixel(255, 255, 255));	//	a is unmodified!

	//	Capture the new, updated pixel value	in a new variable called b.
	b = a.red <- 10;

	//	Now we have the original, unmodified a and the new, updated b.
	assert(a == pixel(255, 255, 255));
	assert(b == pixel(10, 255, 255));

This works with nested values too:

	//	Define an image-struct that holds some stuff, including a pixel struct.
	struct image { pixel background; int width; int height; };

	//	Make an image-value.
	i = image(pixel(1, 2, 3, 255), 512, 256);

	int a = i.image.red;
	assert(a == 1);

	//	Update the red member inside the image's pixel. The result is a brand new image!
	b = i.image.red <- 4;
	assert(a.image.red == 1);
	assert(b.image.red == 4);


## Built in Features of Structs
Destruction: destruction is automatic when there are no references to a struct. Copying is done automatically, no need to write code for this. All copies behave as true-deep copies.
All structs automatically support the built in features of every data type like comparison, copies etc: see separate description.

Notice about optimizations: many accellerations are made behind the scenes:

- causing a struct to be copied normally only bumps a reference counter and shares the data via a reference. This makes copy fast. This makes equality fast too -- the same objects is always equal.
- You cannot know that two structs with identical contents use the same storage -- if they are created independenlty from each other (not by copying) they are not necessarily deduplicated into the same object.
- There is no way for client code to see if the struct instances are _the same_ - that is access the same memory via two separate pointers.


# TUPLES (unnnamed structs)
You can access the members using indexes instead of their names, for use as a tuple.

	a = struct { int, int, string }( 4, 5, "six");
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
	
	b = struct { 4, 5, "six" };
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");


# ENUM
Works like expected from C, but can be extended.

enum preset_colo {
	red = 2,
	green,
	blue
}

enum preset_color_ex : preset color {
	yellow,
}


# ABOUT COLLECTIONS
A fixed set of collections are built right into Floyd. They can store any Floyd value, including structs, tuples, string, integers and even other collections. They are all immutable / persistent.

A unique feature in Floyd is that you cannot specify the exact implementation of the collection, only its basic type: vector or map. The precise data structure is selected by the runtime according to your recommendations and the optimiser / profiler.

The collections are typesafe - a string-vector can only hold strings etc.

Every collection automatically support the core-type-features, like comparisons, serialization and hashing.


# VECTOR
A vector is a collection where you lookup your values using an index between 0 and (vector_size - 1). The items are ordered. Finding the correct value is constant-time. If you read many continous elements it's faster to use a SEQ with the vector - it allows the runtime to keep an pointer to the current position in the vector.

	a = [int]();		//	Create empty vector of ints.
	b = [int](1, 2, 3);		//	Create a vector with three ints.
	b = [1, 2, 3];				//	Shortcut to create a vector with three ints. Int-type is deducted from value.
	a = [string]();					//	Empty vector of strings
	b = [string] ( "one", "two", "three");		//	Vector initialized to 3 strings.
	b = ["one", "two", "three"];		//	Shortcut to create a vector with 3 strings. String-type is dedu

The vector is persistent so you can write elements to it, but you always get a new vector back - the original is unmodified:

	a = ["one", "two", "three" ];
	b = a[1] = "zwei";
	assert(a == ["one", "two", "three" ] && b == ["one", "zeei", "three" ]);

There are many potential backends for a vector - the runtime choses for each vector instances (not each type!):

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read an write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No read/write.

Vector Reference:

	vector(seq<t> in);
	vector [T][ 1, 2, 3, ... };
	vector<T> make(vector<T> other, int start, int end);
	seq subset(vector<T> in, int start, int end);
	a = b + c;


# MAP
A collection that maps a key to a value.Unsorted. Like a dictionary. 
	//	Create a string -> int map with three entries.
	a = [string: int]["red": 0, "blue": 100,"green": 255];

	//	Make map where key is a string and value is an int. Initialize it.
	a = [string: int]("one": 1, "two": 2, "three": 3);

	//	Make a string->int map where collection type is deducted from the initialization values.
	b = ["one": 1, "two": 2, "three", 3];

Map Reference

	V my_map.at(K key)
	V my_map[K key]
	my_map[K key] = value
	bool my_map.empty()
	size_t my_map.size()
	map<K, V> my_map.insert(K key, V value)
	size_t my_map.count()
	map<K, V> my_map.erase(K key)


# SERIALIZATION
Serializing a value is a built in mechanism. It is based on the seq-type.
It is always deep. ### Always shallow with interning when values are shared.

The result is always a normalized JSON stream. ???

??? De-duplication vs references vs equality vs diffing.
	### Make reading and writing Floyd code simple, especially data definitions.
//	Define serialised format of a type, usable as constant.

 ### floyd code = stored / edited in the serialized format.

An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream.


# IMPORT / DEFINE LIBRARIES

# RUNTIME ERRORS / EXCEPTIONS
Principles and policies.
Functions.
	??? Only support this in mutating parts and externals?


# C GLUE


# RESERVED KEYWORDS
assert
bool
catch
char???
clock
code_point
defect_exception
deserialize()
diff()
double
dyn
dyn**<>
else
ensure
enum
exception
false
float
float32
float80
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
namespace???
null
path
private
property
protocol
prove
require
return
rights
runtime_exception
seq
serialize()
string
struct
swap
switch
tag
test
text
this
true
try
typecast
typedef
typeof
vector
while
