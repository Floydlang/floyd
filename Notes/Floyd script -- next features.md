# BASIC TYPES
- **float32**				32 bit floating point
- **float80**				80 bit floating point
- **int8**					8 bit integer
- **int16**
- **int32**
- **int64**
- **function**				function pointer. Functions can be Floyd functions or C functions.


### MORE TYPES

- **dict**			look up values from a key. Localizable.
- **vector**		look up values from a 0-based continous range of integer indexes.
- **enum**		same as struct with only static constant data members

# CORE TYPE FEATURES
These are features built into every type: integer, string, struct, collections etc.

- **hash hash(v)**							computes a true-deep hash of the value



# PROOF & ASSERT
Floyd has serveral built in features to check code correctness. One of the core features is assert() which is a function that makes sure an expression is true or the program will be stopped. If the expression is false, the program is defect.

This is used through out this document to demonstrate language features.


# VALUES, VARIABLES AND CONSTANTS

- Global variables
- Contents of collections


Use _mutable_ to define a local variable that can be mutated. Only _local variables_ can be mutable.

	int hello2(){
		mutable a = "hello";
		a = "goodbye";	//	Changes variable a to "goodbye".
		return 3;
	}



# IF - THEN -ELSE -- STATEMENT & EXPRESSION
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




# MORE EXPRESSIONS
	!a

### Bitwise Operators
!a					inverts a bool: true becomes false, false becomes true.
~a
^a


# LOOPS AND CONDITIONS
The foreach uses the seq-protocol to process all items in a seq, one by one.

foreach(a in stuff){
}
foreach(thing, [string]{ 1, 2, 3 })

switch() -- has modern update, no default or break. ??? todo

	for(int i = 0 ; i < 100 ; i++){
	}

while(expression){
}




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

A unique feature in Floyd is that you cannot specify the exact implementation of the collection, only its basic type: vector or dictionary. The precise data structure is selected by the runtime according to your recommendations and the optimiser / profiler.

The collections are typesafe - a string-vector can only hold strings etc.

Every collection automatically support the core-type-features, like comparisons, serialization and hashing.


# VECTOR COLLECTION
A vector is a collection where you lookup your values using an index between 0 and (vector_size - 1). The items are ordered. Finding the correct value is constant-time.

If you read many continous elements it's faster to use a SEQ with the vector - it allows the runtime to keep an pointer to the current position in the vector.

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
	
There are many potential backends for a vector - the runtime choses for each vector *instances* - not each type:

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read an write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No read/write.

Vector Reference:

	vector(seq<t> in);
	vector [T][ 1, 2, 3, ... };
	vector<T> make(vector<T> other, int start, int end);
	vector<T> subset(vector<T> in, int start, int end);
	a = b + c;



# DICTIONARY COLLECTION
A collection that maps a key to a value. Unsorted. Like a c++ map. 
	//	Create a string -> int dictionary with three entries.
	a = [string: int]["red": 0, "blue": 100,"green": 255];

	//	Make dictionary where key is a string and value is an int. Initialize it.
	a = [string: int]("one": 1, "two": 2, "three": 3);

	//	Make a string->int dictionary where collection type is deducted from the initialization values.
	b = ["one": 1, "two": 2, "three", 3];

	c = [string: int]; //	Empty dictionary of string,int.
	

Dictionary Reference

	V my_dict.at(K key)
	V my_dict[K key]
	my_dict[K key] = value
	bool my_dict.empty()
	size_t my_dict.size()
	dict<K, V> my_dict.insert(K key, V value)
	size_t my_dict.count()
	dict<K, V> my_dict.erase(K key)



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
