






# COMPLETE TYPE SYSTEM

- construction
- statements vs expressions
- JSON compatible primitives types.

??? Make typeid a full blown thing. Allows you to design a new struct at runtime etc. Introspection.


| Example										| Result |
|:--											| :--
| int, string, float etc.						| IMPLEMENTED
| struct										| IMPLEMENTED
| function										| IMPLEMENTED
| dictionary									| IMPLEMENTED
| set											| FUTURE
| tuple											| TBD, unnamed struct
| enum											| TBD
| tagged union									| TBD, ???
| typedef										| TBD



Floyd typeid:
	int
	int id: 4
	[int]
	struct id:3
	struct def

A typeid either directly specifies a basic type, a composite stype or a custom_type based on another typeid
Two types are equal if they have the exact

TYPE
	bool
	int
	float
	string
	typeid
		TYPE: held type
	struct
		[MEMBER]
		MEMBER: TYPE member_name
	vector
		TYPE: element-type
	dict
		TYPE value-type
		key is always a string
	function
		TYPE return-value
		[ARGUMENT] - function arguments
		ARGUMENT: TYPE argument-name
	unresolved_type_identifier
		string: the identifier for the unknown type.
		??? Don't use typeid_t to store these! This is not an id of a type, it an unresolved identifier.
		Problem is that it can still be part of a complex typeid_t. 
	custom_type
		string: name-of-new-type, like "my_struct", "meters_t"
		string: compile-time path of the type declaration -- to make two identical custom_types unique.
		TYPE type-implementation.
		In the future, intern this and use a string-id.


When parsing code we support typenames were we don't know the actual type yet. Those typenames can be used to build more complex types out of.

When executing program, all those typenames needs to be resolved. Idea: make special compiler types: unresolved_typeid_t and resolved_typeid_t.



Struct declaration statement:

	struct struct1 {
		int count;
		string name;
		[string: string] tags;
	}

Result is equivalent to:

	typeid struct1 = typeid[
		"struct",
		[
			"struct1",
			[
				{ "type": "int", "name": "count" },
				{ "type": "string", "name": "name" },
				{ "type": ["dict", "string"], "name": "tags" }
			]
		]
	]


Function declaration statement:


	int test_xyz(float a, float b){ return a + b; }

Result

	["int", ["float", "float"] test_xyz = FUNCTION_LITERAL;



Unnamed struct:

	a = struct { int, string, string }( 4, "five", "six");


Becomes:
	//	This is custom_type.
	typeid[
		"custom_type_name": "struct1",
		"implementation_type":
			[
				[
					{ "type": "int", "name": "" },
					{ "type": "string", "name": "" },
					{ "type": ["dict", "string"], "name": "" }
				]
			]
	]

	a = 
	typeid[
		[
			[
				{ "type": "int", "name": "" },
				{ "type": "string", "name": "" },
				{ "type": ["dict", "string"], "name": "" }
			]
		]
	](4, "five", "six");


??? make struct definition into an expression instead of a type.


int my_func(float a, float b){
	return a * b; ??? ANY DECLRATIONS ETC GO HERE:
}

[string:int] example_dictionary = [ "one": 1, "two": 2];
[string:int] example_dictionaryv2 = { "one": 1, "two": 2 };
[string] example_vector = [ "one", "two", "three" ];


struct1 example_struct1 = struct1{count: 4, name: "Kitty", ["red": "warning, "green": "no problemos"]};

(string:int) example_tuple = ("one", 13);



example_struct = struct

typedef float meters;

meters dist_home = 14
meter dist_home = meter(14)






	a = struct { int, int, string }( 4, 5, "six");


lambdas



### TYPEDEF
A typedef makes a new, unique type, by copying another (often complex) type.
	typedef [string, [bool]] book_list;
	### You also add additional invariant. sample_rate is an integer, but can only be positive.




### LAMBDAS

Lambdas aka closures are unnamed function-expressions. You make a little function that can be called later, without going through the trouble of making a full blown function. Lambdas also supports accessing variables outside its definition which is very useful when you.

**??? Lambdas are pure?!**

Lambdas are often short snippets of code passed to a functions, like a compare function passed to a sort function.

Function type X:

	bool (string a, string b)

Function implementation f1 of type X

	bool f1(string a, string b){ return a < b ; }

Variable of type X

	bool (string a, string b) f2 = f1

Example sort function that takes function as argument #2.

	[string] sort([string] values, bool (string a, string b) equal)

Equivalent calls to sort function:

	s1 = sort([ "one", "two", "three" ], f1)
	s2 = sort([ "one", "two", "three" ], bool (string a, string b){ return a < b; })
	s3 = sort([ "one", "two", "three" ], (a, b){ return a < b; })

	s4 = sort([ "one", "two", "three" ], { return $0 < $1; })


s3 and s4 - the type of the function is inferred.

s4: All functions can always access their argument using $0, $1 etc. We use this here since we didn't name the function arguments.


- No trailing closure syntax
- No implicit returns from single-expression closures

You can use a lambda to do directly assign a value from an if-then-else:

	string m = (int id){
		float temp = 0
		if (a == 1){
			return "FIRST!";
		}
		else{
			return "...more";
		}
	}(1)





### STRUCTs: Unnamed struct members (tuples)
All members also have number in the order they are listed in the struct, starting with 0. A second name for the same member.

You can chose to not name members. An unnamed member can be accessed using its position in the struct: first member is "0", second is called "1" etc. Access like: "my_pixel.0".

my_pixel[0] // allow clients to enumerate struct members as elements. If all members are the same type result is that type. If struct members are different types, result is a dyn<int, float>

**??? How to block mutation of member variable? Some members makes no sense to write in isolation.**


### STRUCTs: Unnnamed structs (tuples)
You can access the members using indexes instead of their names, for use as a tuple. Also, there is no need to name a struct.

	a = struct { int, int, string }( 4, 5, "six");
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
	
	b = struct { 4, 5, "six" };
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");














# BASIC TYPES
- **float32**				32 bit floating point
- **float80**				80 bit floating point
- **int8**					8 bit integer
- **int16**
- **int32**
- **int64**


### MORE TYPES

- **enum**		same as struct with only static constant data members

# CORE TYPE FEATURES
These are features built into every type: integer, string, struct, collections etc.

- **hash hash(v)**							computes a true-deep hash of the value



# PROOF & DOCS
Floyd has serveral built in features to check code correctness. One of the core features is assert() which is a function that makes sure an expression is true or the program will be stopped. If the expression is false, the program is defect.

This is used through out this document to demonstrate language features.



		
		# DOCS
				"Short": Calculates area of various basic shapes",
				"Features": "Squares, circles and dots",
				"Inputs": [
					"a": "a key that tells which shape to use."
						a == "": default shape, a dot
						a == "square": simple square, only size1 is used.
					"b": The size of the shape. For squares, this is the side. For circles, this is the radius
				],
				"output": Returns the area
			],
			"tests": [
				{
					"Scenario": "Minimal inputs"
					"Input": ["", 0]
					"Output": 0
				},
				{
					"Scenario": "Make sure the special case for PI works"
					"Input": ["hello", 3.14]
					"Output": 1
				}
			],
			"examples": [
				[
					int r = my_func("circle", 10.0)
					assert(r == 3.4)
				],
				[
					int r = my_func("square", 30.0)
					assert(r == 900.0)
				]
			]
		]
		
		int my_func(string a, float b){
		}
	





??? Function arguments = a tuple -- all functions takes ONE argument. Hide via syntax.

??? Treat function local variables as member of a struct. Mutating a variable gives us a completely new local state -- the old one still exists.





# MORE EXPRESSIONS
	!a


### Bitwise Operators
!a					inverts a bool: true becomes false, false becomes true.
~a
^a

### IF - EXPRESSION
??? This is like SWITCH or pattern matching.
??? Replace with expression:

		int val = if(s == "one"){
			return 1;
		}
		else if(s == "two"){
			return 2;
		}
		else{
			return -1;
		}

Use match for pattern matching? This syntax bloat is required since we have immutability.

Check out rust-lang match-expressions.


# LOOPS AND CONDITIONS


Alt:

	b = 3
	for(int a = 0 ; a < b ; a = a + 1){
		print(a + b);
	}


Swift 4:

	for name in names {
	    print("Hello, \(name)!")
	}
	for (animalName, legCount) in numberOfLegs {
		print("\(animalName)s have \(legCount) legs")
	}

	for index in 1...5 {
		print("\(index) times 5 is \(index * 5)")
	}

	for _ in 1...power {
		answer *= base
	}
	for tickMark in 0..<minutes {
	}


Python:
	for iterating_var in sequence:
		statements(s)
	for index in range(len(fruits)):
		print 'Current fruit :', fruits[index]
	for num in range(10,20):     #to iterate between 10 to 20


# Add better iteration / functional features

x = foreach(seq) { statements } process all items in a seq, one by one. Execution order undefined.

collection = map(e2 = expr(e), seq) - process sequence of elements one at a time, return new collection with equal number of elements.

collection = filter(bool expr, seq) -- process sequence of elements one at a time, return subset of those elements in new collection

collection = reduce(expr, v0, seq) -- process sequence of elements one at a time, aggregate value v0.

index = find1(e, vector) returns key of an element in a collection
index = find1(bool expr, vector) returns key of an element in a collection

key = find1(value, dict) returns key of an element in a collection
key = find1(bool expr, dict) returns key of an element in a collection

[index] = findall(e, vector) returns key of each element that matches expression.
[index] = findall(bool expr, vector) returns key of each element that matches expression.

r = foreach() process sequence of elements one at a time. ??? Where to store result?

for(;;){ statements }		--- mutable C-style for loop. Not recommended.
while(expr){ statements }	--- Not supported, use for loop instead.


Q: Can for loop evaluate condition once and convert to a count?
	Loop over a fixed number (int) of elements, calculated before loop.
	Loop over a sequence.
	Loop while expression is true.


Loop over collection or seq:

	foreach(thing, [string]{ 1, 2, 3 })
	
	
	for(const auto& e: my_vec){
		cout << e;
	}
	
	for(e in my_vec){
	}

	for name in names {
	    print("Hello, \(name)!")
	}

	for (animalName, legCount) in numberOfLegs {
	    print("\(animalName)s have \(legCount) legs")
	}

	for(const auto it = my_vec.begin() ; it != my_vec.end() ; i++){
	    print("Hello, \(*it)!")
	}

Loop over index:

	for index in 1...5 {
	    print("\(index) times 5 is \(index * 5)")
	}

	for (int index = 1 ; index <= 5 ; i++){
	    print("\(index) times 5 is \(index * 5)")
	}

	This is equivalent to [1,2,3,4,5].map(
		index in {print("\(index) times 5 is \(index * 5)")
	}

	var index = 1
	while(index <=5){
	    print("\(index) times 5 is \(index * 5)")
	    index = index + 1
	}
	
	for tickMark in 0..<minutes {
	    // render the tick mark each minute (60 times)
	}




for tickMark in stride(from: 0, to: minutes, by: minuteInterval) {
    // render the tick mark every 5 minutes (0, 5, 10, 15 ... 45, 50, 55)
}
Closed ranges: stride(from:through:by:)



Python. Number of iterations is set *before* loop starts.
for x in range(0, 3):
    print "We're on time %d" % (x)



Good syntax + compatible with c / c++ / Javascript

???


Related:
- Seq
- map(), filter(), reduce(), find()
- while


# filter()
	
	let digits = [1,4,10,15]
	let even = digits.filter { $0 % 2 == 0 }
	// [4, 10]
	

# map()

	let values = [2.0,4.0,5.0,7.0]
	let squares2 = values.map({
	  (value: Double) -> Double in
	  return value * value
	})

# reduce()

	let names = ["alan","brian","charlie"]
	let csv = names.reduce("===") {text, name in "\(text),\(name)"}
	// "===,alan,brian,charlie"

# find()

	let names = ["alan","brian","charlie"]
	let csv = names.find("===") {text, name in "\(text),\(name)"}
	// "===,alan,brian,charlie"




# ENUM
Works like expected from C, but can be extended. Always represents an integer.

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

A unique feature in Floyd is that you cannot specify the exact implementation of the collection, only its basic type: vector or dictionary. The precise data structure is selected by the runtime according to your recommendations and the optimiser / profiler.


# VECTOR COLLECTION
If you read many continous elements it's faster to use a SEQ with the vector - it allows the runtime to keep an pointer to the current position in the vector.


### OPTIMIZATIONS IN BACKEND
The runtime has several types of backends for a vector and choses for each vector *instances* - not each type:

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read and write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No write.



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



# More STRUCT features --- TODO NEXT

**??? Unify adressing nested dictionaries with structs and vectors. Functions too. Equivalent dict.member like dict["member"].**



# STRUCTs - Operator overloading --- TODO NEXT
This is useful for vector2d, mystring etc. You may need operator overloading for simple structs too -- feature not tied to advance structs.

- rect operator+(rect a, rect b) = default
- rect operator-(rect a, rect b) = default
- rect operator*(rect a, rect b) = default
- rect operator/(rect a, rect b) = default
- rect operator%(rect a, rect b) = default

- bool operator<(rect a, rect b) = default

- bool operator<=(rect a, rect b) -- if not implemented, Floyd uses: !operator<(b, c)
- bool operator==(rect a, rect b) -- if not implemented, Floyd uses: !(operator<(a, b) || operator(b, a))

#### Not overloadable, automatically use core equality functions
- bool operator>(rect a, rect b) --- compiler replaces this with !operator<=()
- bool operator>=(rect a, rect b) --- compiler replaces this with !operator<()
- bool operator!=(rect a, rect b) --- compiler replaces this with !operator==()


# Advanced STRUCTs --- TODO NEXT

**??? Maybe this isn't a struct but a class? Or something else? **

There is a second, more advanced flavor of struct where the struct has an invariant that needs protecting. It is quick to upgrade a simple struct to the advanced variant and no clients needs to be affected.

An invariant is a dependency between the member variables that must always be correct. Every function that modifies or creates this struct must make sure the invariant is always correct.

With an advanced struct all automatic behavior is by default turned off and you need to enable / code each explicitly.

GOAL:

- as few functions as possible have access to the internals of the invariant, including rect's implementation
- clients should not need changing if we go from simple struct to advanced
- clients should not need changing if we make a member variable part of the invariant
- clients should not need changing if a function is a altered to have access / not access to privates.

Advice: Encapsule invariant into the smallest struct possible: big structs may want to nest many structs instead of having complex invariant.

All you need to do upgrade a simple struct to an advance struct is to wrap its data in a state {}. Now all member access is blocked by clients, but you can still:
- copy
- delete
- compare

	struct rect {

		//	Internal data is wrapped in "state", which is a simple-struct.
		//	The state-struct has a init(float width, float height) and
		//	all other features of a basic struct but clients to rect cannot
		//	access.
		//	Member functions of rect can access state directly as a struct
		//	member called "state".
		state {
			float width;
			float height;
		}
	}

**??? improve state-syntax to make state a normal simple-struct that you can adress both type and member.**

The **state** is a built-in keyword and you can only have **one** state in a struct, which kicks it over into being an advanced struct. Advanced structs cannot have other member data besides its state.

It is possible to use any types inside the state, which lets just put other advanced structs inside rect.

To restore all previous functionality to make clients work as before:

	struct rect {
		//	Allow read & write of width & height. "default" is a
		//	convenience function for mapping to the state-member
		//	of the same name.
		float width: get = default, set = default
		float height: get = default, set = default
		init: default

		state {
			float width;
			float height;
		}
	}

This is what's really going on - the gets and sets use lambdas to add code to getters and setters.

	struct rect {
		float width:
			get = { return $0.state.width; },
			set = { return $0.state.width = $1; }
		float height:
			get = { return $0.state.height },
			set = { return $0.state.height = $1; }

		init(float width, float height){
			return state(width, height)
		}

		state {
			float width;
			float height;
		}
	}
**??? remove need for this.state.width. Better with this..width or this->width or this._.width**

Adding an invariant to the struct - area must always be width * height:

	//	Setters need to update area too!
	struct rect {
		float width:
			get = default,
			set = { s2 = $0.state.width = $1; s2.state.area = s2.state.width * s2.state.height; return s2; }
		float height:
			get = default,
			set = { s2 = $0.state.height = $1; s2.state.area = s2.state.width * s2.state.height; return s2; }
		float area:
			get = default

		init(float width, float height){
			return state(width, height, width * height)
		}

		state {
			float width;
			float height;
			float area;
		}
	}

You can add member functions. They can access the state directly:

	struct rect {
		float width:
			get = default,
			set = { s2 = $0.state.width = $1; s2.state.area = s2.state.width * s2.state.height; return s2; }
		float height:
			get = default,
			set = { s2 = $0.state.height = $1; s2.state.area = s2.state.width * s2.state.height; return s2; }
		float area:
			get = default

		init(float width, float height){
			return state(width, height, width * height)
		}

		state {
			float width;
			float height;
			float area;
		}

		//	Member function has access to state.
		bool is_empty(rect this){
			return this.state.area == 0
		}

		//	Can read state, create state.
		rect scale(rect this, float k){
			return state(this.state.width * k, this.state.height * k, this.state.width * k * this.state.height * k)
		}

		//	Can MUTATE state, create state.
		rect scale(rect this, float k){
			a = this.state.width = this.state.width * k
			b = a.state.height = a.state.height * k
			c = b.state.area = b.state.width * b.state.height
			return c
		}
	}

- There is no special implicit "this" argument to functions, you need to add it manually.
- It is not possible to change member variables of a value, instead you return a completely new instance of the struct.


### Unified access for member and non-members

Here are some non-member functions:

	rect make_square(float side){
		return rect(side, side)
	}

	bool is_square(rect this){
		return this.width == this.height
	}

	rect double_sides(rect this){
		return scale(this, 2)
	}

Member functions are called just like normal functions, no special -> syntax:

	a = rect(100, 100)
	assert(is_empty(a) == false)

	b = scale(rect(100, 100), 2)
	assert(b == rect(200, 200))

Accessing member variable (simple struct or advances struct):

	a = rect(4, 5)
	assert(a.width == 4)

>IDEA: Syntactic sugar for member variables:
>
>	width(a) a.width
>
>This makes it possible to implement struct getter/setter like this:**
>
>		rect {
>			float width(rect this){ return this.state.width; }
>			rect width(rect this, float width){ return this.state.width = width; }
>			...
>		}
>...and lets us remove set/get special syntax.


Syntactic sugar for member functions:

	is_empty(a) == a.is_empty()

Syntactic sugar for free functions:

	a = rect(6, 6);
	assert(is_square(a) == true)
	assert(a.is_square() == true)



# STRUCTs: paths
**??? Add paths to change deeply nested members?**
window_mgr_t m2 = window_mgr.window[3].root_view.child[0].title = "Welcome"
Add: assoc, assoc_inc, deassoc, deassoc_in. Works with path-objects.
Path object = vector of keys / indexes?

Path = is resolved everytime it is used -- it does NOT keep reference to target objects.

b = assoc_in(a, [ "window_mgr", "root_view", "child", "title"], "Welcome)

p = path(window_mgr.window[3].root_view.child[0].title)
p = &window_mgr.window[3].root_view.child[0].title
b = *p = "Welcome"


# STRUCTs: read-modify-write

**??? Idea - use special operator for assigning to struct member: "b = a.red <- 10;"**
**??? Add feature to read-modify-write value without listing path twice?**

		window_mgr_t m2 = window_mgr.window[3].root_view.child[0].title = "Welcome: " + $0

Use <- to read & modify? A generic version of C's "a += 3".
		a <= $0 + 3
		a <= { $0 + 3 }
Works like map() but for any nested variable.


Replace multiple members of struct:
	a = PaintWindow()
	b = a.update(title = "Hello", size = "medium")

...This makes a new PaintWindow identical to a, but with two members replaced.
Better syntax:

	b = a <= (title = "Hello", size = "medium")


# STRUCTs: Invariant function -- SOMEDAY MAYBE

Advances structs requires an invariant() function. It performs checks that the value isn't corrupt. If it fails the code is defect. The invariant function will automatically be called before a new value is exposed to client code: when returning a value from a constructor, from a mutating member function etc.

	invariant(pixel_t this){
		assert(this.red >= 0 && this.red <= 255)
	}

# STRUCTs EXAMPLES-FEATURE --- SOMEDAY MAYBE
When defining a data type (composite) you need to list 4 example instances. Can use functions to build them or just fill-in manually or a mix. These are used in example docs, example code and for unit testing this data types and *other* data types. You cannot change examples without breaking client tests higher up physical dependency graph.





# Functions -- alternative design:

(Int, Int) -> (Int)

??? idea: functions have ONE input and one output:

	bool f(string a, string b) --- this is a function that has (string a, string b) tuple as input, bool as output.
	(string a, string b) f(bool v)
	(string, string) f(bool v)


??? Idea: all functions can be implemented by an expression. Only use {} if you want to do local variables:

	//	Fully specified function definition.
	bool (string a, string b) f = bool(string a, string b){ return a < b; }

	//	Only need to specify function type once:
	f = bool(string a, string b){ return a < b; }
	bool (string a, string b) f = { return $0 < $1; 
	//	Special shorthand for C and Java and Javasript users.
	bool f(string a, string b){ return a < b; }

	bool f(string a, string b) = return $0 < $1;


