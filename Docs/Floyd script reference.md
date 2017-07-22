# Floyd Script Reference

Floyd Script is a fast and modern C-like program language that tries to make writing correct programs simpler and faster.

It uses pure functions, immutability and composition.

It as a unique concept to handle mutation of data, control of the outside world (files, network, UI) and controlling time.

Functions and classes are pure. A pure function can only call pure functions. This delegates all mutation / time to the top levels of your program. Here you need to call non-pure functions of the OS. You must tag your functions with "nonpure" to be able to call other nonpure functions.

This makes those risk. Have as little nonpure code as possible. Try to not have any conditional code inside nonpure code - this makes testing easier.

Mutable data can exist locally inside a function, but never leak out of the function. 


# BASIC TYPES
These are the privitive data types built into the language itself. The goals is that all the basics you need are already there in the language. This makes it easy to start making useful programs, you don't need to chose or build the basics. It allows composability since all libraries can rely on these types and communicate bewteen themselves using them. Reduces need for custom types and glue code.

- **int**					Same as int64
- **bool**					**true** or **false**
- **string**				built-in string type. 8bit pure (supports embedded nulls).
	- 							Use for machine strings, basic UI. Not localizable.

# COMPOSITE TYPES
These are composites and collections of other types.
- **struct**		like C struct or class or tuple.


# CORE TYPE FEATURES
These are features built into every type: integer, string, struct, collections etc.

- **a = b** 	This true-deep copies the value b to the new name a.
- **a != b**										derivated of a = b.
- **a < b**										tests all member data in the order they appear in the struct.
- **a <= b**, **a > b**, **a >= b**	these are derivated of a < b


# VALUES, VARIABLES AND CONSTANTS
All "variables" aka values are by default constants / immutable.
- Function arguments
- Local function variables
- Member variables of structs etc.

	assert(floyd_verify(
		"int hello1(){
			a = "hello";
			a = "goodbye";	//	Compilation error - you cannot change variable a.
			return 3;
		}"
	) == -3);


# GLOBAL SCOPE
Used for function definitions, and structs.


# FUNCTIONS
Functions in Floyd are pure, or referential transparent. This means they can only read their input arguments and constants, never read or modify anything else that can change. It's not possible to call a function that returns different values at different times, like get_time().

Functions always return exactly one value. Use a struct to return more values.

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

# EXPRESSIONS
Reference: http://www.tutorialspoint.com/cprogramming/c_operators.htm
Comparisons are true-deep - they consider all members and also member structs and collections.

###	Arithmetic Operators
```
+	Addition - adds two operands: "a = b + c", "a = b + c + d"
−	Subtracts second operand from the first. "a = b - c", "a = b - c - d"
*	Multiplies both operands: "a = b * c", "a = b * c * d"
/	Divides numerator by de-numerator: "a = b / c", "a = b / c / d"
%	Modulus Operator and remainder of after an integer division: "a = b / c", "a = b / c / d"
```

### Relational Operators
```
	a == b				true if a and b have the same value
	a != b				true if a and b have different values
	a > b				true if the value of a is greater than the value of b
	a < b				true if the value of a is smaller than the value of b
	a >= b
	a <= b
```

### Logical Operators
	a && b
	a || b

### Conditional Operator
condition ? a : b		When condition is true, this entire expression has the value of a. Else it has the value of b. Condition, a and b can all be complex expressions, with function calls etc.

	bool is_polite(string x){
		return x == "hello" ? "polite" : "rude"
	}
	assert(is_polity("hiya!") == false);
	assert(is_polity("hello") == true);

# STRUCTs
Structs are the central building blocks for composing data in Floyd. They are used in place of structs, classes, tuples in other languages. Structs are always values and immutable.

They are **true-deep**. True-deep is a Floyd term that means that all values and sub-values are always considered, in equality checks or assignment, for example. The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects because of several clients modifying the same struct.

Note: Internally, value instances are often shared to conserve memory and performance.

	struct pixel {
		int red;
		int green;
		int blue;
	};



## Struct Signature
This is the properties exposed by a struct in the order they are listed in the struct.


## Member Data
All members have default value. This makes simple value classes extremely simple to create.
Clients can directly access all member variables.

	struct test {
		float f;
		float g = 3.1415f;
	};

Above is "test", a simple but fully functional struct. "f" will be initialized to 0.0f, g will be initialized to 3.1415f.
	??? also init(f, g) like Swift
	??? No default constructor

## Constructors
These are member functions that are named the same as the struct that sets up the initial state of a struct value. All member variables are first automatically initialized to default values: either explicitly or using that type's default. A constructor overrides these defaults. The value does not exist to the rest of the program until the constructor finishes.


??? have a single constructor - the default constructor, that make a new, default value based on member default values. You cannot make your own constructors. Instead you created member make-functions that have access to the private state. This mean the value is always constructed, then mutated.
		struct pixel {
			pixel make_white(){
			}
		}

		struct pixel {
			int red = 255; int green = 255; int blue = 255;
			pixel make_red(){
				temp = pixel();
				temp
			}
		}
		//	New constructor. Non-member.
		pixel pixel(float float_gray){
		}



## Using default contructors:

	struct test {
		float f;
		float g = 3.1415f;
	}

	a = test();
	assert(a.f == 0.0f);
	assert(a.g == 3.1415f);


## Built in Features of Structs
Destruction: destruction is automatic when there are no references to a struct. Copying is done automatically, no need to write code for this. All copies behave as true-deep copies.
All structs automatically support the built in features of every data type like comparison, copies etc: see separate description.

Notice about optimizations: many accellerations are made behind the scenes:

- Causing a struct to be copied normally only bumps a reference counter and shares the data via a reference. This makes copy fast. This makes equality fast too -- the same objects is always equal.
- You cannot know that two structs with identical contents use the same storage -- if they are created independenlty from each other (not by copying) they are not necessarily deduplicated into the same object.

# AST
The encoding of a Floyd program into a AST (abstrac syntax tree) is fully documented. You can generate, manipulate and run these programs from within your own program.s

The tree is stored as standard JSON data. Each expression is a JSON-array where elements have defined meaning. You can nest expressions to any depth by specifying an expression instead of a constant.

### STATEMENTS

	[ "bind", identifier-string, expr ]
	[ "defstruct", identifier-string, expr ]
	[ "deffunc", identifier-string, expr ]
	[ "return", expr ]


### EXPRESSIONS
This is how all expressions are encoded into JSON format. "expr" is a placehold for another expression array. 
[ "k", expr, type ]

[ "negate", expr, type ]

[ "+", left_expr, right_expr, type ]
[ "-", left_expr, right_expr, type ]
[ "*", left_expr, right_expr, type ]
[ "/", left_expr, right_expr, type ]
[ "%", left_expr, right_expr, type ]

[ "<", left_expr, right_expr, type ]
[ "<=", left_expr, right_expr, type ]
[ ">", left_expr, right_expr, type ]
[ ">=", left_expr, right_expr, type ]

[ "||", left_expr, right_expr, type ]
[ "&&", left_expr, right_expr, type ]
[ "==", left_expr, right_expr, type ]
[ "!=", left_expr, right_expr, type ]

[ "?:", conditional_expr, a_expr, b_expr, type ]

[ "call", function_exp, [ arg_expr ], type ]

Resolve free variable using static scope
[ "@", string_name, type ]

Resolve member
[ "->", object_id_expr, string_member_name, type ]

Lookup member
[ "[-]", object_id_expr, key_expr, type ]


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
