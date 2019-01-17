![](floyd_speak_logo.png)

# FLOYD SPEAK

Floyd Speak is the programming language part of Floyd. It's an alternative to Java and C++, Javascript and Python. Using Floyd Speak you write functions and data types. You make complex data structures, setup concurrency and parallelism and communicate with the world around your program.

![](floyd_speak_cheat_sheet.png)


# DATA TYPES

These are the primitive data types built into the language itself. The building blocks of all Floyd programs.

The goal is that all the basics you need are already there in the language and the core library. This makes it easy to start making meaningful programs. It also allows composability since all Floyd code can rely on these types and communicate between themselves using these types. This greatly reduces the need to write glue code that converts between different librarys' string classes and logging and so on.

|TYPE		  	| USE
|:---				|:---	
|__bool__			|__true__ or __false__
|__int__			| Signed 64 bit integer
|__double__		| 64-bit floating point number
|__string__		| Built-in string type. 8-bit pure (supports embedded zeros). Use for machine strings, basic UI. Not localizable. Typically used for Windows Latin1, UTF-8 or ASCII.
|__typeid__		| Describes the *type* of a value.
|__function__	| A function value. Functions can be Floyd functions or C functions. They are callable.
|__struct__		| Like C struct or classes or tuples. A value object.
|__protocol__	| A value that hold a number of callable functions. Also called interface or abstract base class.
|__vector__		| A continuous array of elements addressed via indexes.
|__dictionary__	| Lookup values using string keys.
|__json_value__	| A value that holds a JSON-compatible value, can be a big JSON tree.
|TODO POC: __sum-type__		| Tagged union.
|TODO 1.0: __float__		| 32-bit floating point number
|TODO 1.0: __int8__		| 8-bit signed integer
|TODO 1.0: __int16__		| 16-bit signed integer
|TODO 1.0: __int32__		| 32-bit signed integer

Notice that string has many qualities of an array of characters. You can ask for its size, access characters via [], etc.


# SOURCE CODE FILES

Floyd Speak files are always utf-8 files with no BOM. Their extension is ".floyd".

# COMMAND LINE

|COMMAND		  	| MEANING
|:---				|:---	
| floyd run mygame.floyd | compile and run the floyd program "mygame.floyd"
| floyd compile mygame.floyd | compile the floyd program "mygame.floyd" to an AST, in JSON format
| floyd help		| Show built in help for command line tool
| floyd runtests	| Runs Floyds internal unit tests
| floyd benchmark 		| Runs Floyd built in suite of benchmark tests and prints the results.
| floyd -t mygame.floyd		| the -t turns on tracing, which shows Floyd compilation steps and internal states



# CONCEPT: PURE AND IMPURE FUNCTIONS

Functions in Floyd are by default *pure*, or *referential transparent*. This means they can only read their input arguments and constants, never read or modify anything: not global variables, not by calling another, impure function.

It's not possible to call a pure function with a set of arguments and later call it again with the same arguments and get a different result.

A function like get_time() is impure.

While a function executes, it perceives the outside world to stand still.

Functions always return exactly one value. Use a struct to return more values.

Example definitions of functions:

```
func int f1(string x){
	return 3
}

func int f2(int a, int b){
	return 5
}

func int f3(){	
	return 100
}

func string f4(string x, bool y){
	return "<" + x + ">"
}
```

Function types:

```
func bool (string, double)
```


This is a function that takes a function value as argument:

```
func int f5(func bool (string, string))
```

This is a function that returns a function value:

```
func bool (string, string) f5(int x)
```

All arguments to a function are read-only -- there are no output arguments.


### IMPURE FUNCTIONS

You can tag a function to be impure using the "impure" keyword.

```
func int f1(string x) impure {
	return 3
}
```

This mean the function has side effects or gets information somewhere that can change over time.

- A pure function cannot call any impure functions!
- An impure function can call both pure and impure functions.

Limit the amount of impure code!


### GRAY PURE FUNCTIONS

This is a type of function that *has side effects or state* -- but the calling functions cannot observe this so from their perspective it is pure. Examples are memory allocators and memory pools and logging program execution.

Why is the OK? Well to be picky there are no pure functions, since calling a pure function makes your CPU emit more heat and consumes real-world time, makes other programs run slower, consumes memory bandwidth. But a pure function cannot observe those side effects either.


# CONCEPT: DEEP BY VALUE

All values and aggregated members values are always considered in operations in any type of nesting of structs and values and collections. This includes equality checks or assignment for example.

The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

These are features built into every type: integer, string, struct, dictionary:

|EXPRESSION		| EXPLANATION
|:---				|:---	
|__a = b__ 		| This true-deep copies the value b to the new name a.
|__a == b__		| a exactly the same as b
|__a != b__		| a different to b
|__a < b__		| a smaller than b
|__a <= b__ 		| a smaller or equal to b
|__a > b__ 		| a larger than b
|__a >= b__ 		| a larger or equal to b

This also goes for print(), map(), to\_string(), value\_to\_jsonvalue(), send() etc.

Example: your application's entire state may be stored in *one* value in a struct containing other structs and vectors and so on. This value can still be copied around quickly, it is automatically sortable, convertable to JSON or whatever.


# CONCEPT: NO POINTERS

There are no pointers or references in Floyd. You copy values around deeply instead. Even a big value like your games entire world or your word processor's entire document. Behind the curtains Floyd uses pointers extensively to make this fast.

Removing the concept of pointers makes programming easier. There are no dangling pointers, aliasing problems or defensive copying and other classic problems. It also makes it simpler for the runtime and compiler to generate extremely fast code.

# CONCEPT: STATIC TYPING

Every value and variable and identifier has a static type: a type that is defined at compile time, before the program runs. This is how Java, C++ and Swift works. Javascript, Python and Ruby does not use static typing.


# CONCEPT: IMMUTABLE VALUES VS VARIABLES

All values in Floyd are immutable -- you make new values based on previous values but you don't drectly modify old values. Internally Floyd uses clever mechanisms to make this fast and avoids copying data too much. It's perfectly good to replace a character in a 3 GB long string and get a new 3 GB string as a result. Almost all of the characters will be stored only once.

The only exception is local variables that can be forced to be mutable.

(Also each green-process has one mutable value too.)


- Function arguments
- Function local variables
- Member variables of structs.

When defining a variable you can often skip telling which type it is, since the type can be deduced by the Floyd compiler.

Explicit

```
let int x = 10
```

Implicit

```
let y = 11
```


Example:

```
int main(){
	let a = "hello"
	a = "goodbye"	//	Runtime error - you cannot change variable a.
	return 3
}
```

You can use "mutable" to make a local variable changeable.

```
int main(){
	mutable a = "hello"
	a = "goodbye"	//	Changes variable a to "goodbye".
	return 3
}
```



# CONCEPT: GLOBAL SCOPE

Here you normally define functions, structs and global constants. The global scope can have almost any statement and they execute at program start. Simple programs can do without defining any functions at all.



# CONCEPT: SOFTWARE-SYSTEM

This keyword is part of Floyd Systems -- a way to define how all the containers and components and processes are interfacting.

```
software-system {
	"name": "My Arcade Game",
	"desc": "Space shooter for mobile devices, with connection to a server.",
	"containers": {}
}
```

Read more about this in the Floyd Systems documentation. It allows you setup concurrency.






# EXPRESSIONS

An expression is how you calculate new values. The output of an expression is always another value.

Comparisons are deep: for a composite values they consider all members values and their member values. This goes for struct members and collections.

## ARITHMETIC OPERATORS

How to add and combine values:

|OPERATOR		| EXPLANATION
|:---			|:---	
|+	|Addition - adds two operands: "a = b + c", "a = b + c + d"
|−	|Subtracts second operand from the first. "a = b - c", "a = b - c - d"
|*	|Multiplies both operands: "a = b * c", "a = b * c * d"
|/	|Divides numerator by de-numerator: "a = b / c", "a = b / c / d"
|%	|Modulus Operator and remainder of after an integer division: "a = b / c", "a = b / c / d"


## RELATIONAL OPERATORS

Used to compare two values. The result is true or false:

|OPERATOR		| EXPLANATION
|:---			|:---	
|	a == b	|	true if a and b have the same value
|	a != b	|	true if a and b have different values
|	a > b	|	true if the value of a is greater than the value of b
|	a < b	|	true if the value of a is smaller than the value of b
|	a >= b	|	true if a is greater than or equal to b
|	a <= b	|	true if a is less than or equal to b


## LOGICAL OPERATORS

Used to compare two values. The result is true or false:

|OPERATOR		| EXPLANATION
|:---			|:---	
| a && b		|	true if a is true and b is true
| a \|\| b		|	true if a is true or b is true


## CONDITIONAL OPERATOR
When the condition is true, this entire expression has the value of a. Else it has the value of b. Also called ternary operator, because it has three parts.

```
condition ? a : b
```

Condition, a and b can all be complex expressions, with function calls, etc.

```
func bool is_polite(string x){
	return x == "hello" ? "polite" : "rude"
}
assert(is_polite("hiya!") == false)
assert(is_polite("hello") == true)
```


# IF - THEN - ELSE -- STATEMENTS

This is a normal if-elseif-else feature, like in most languages. Brackets are required always.

```
if (s == "one"){
	return 1
}
```

You can add an else body like this:

```
if(s == "one"){
	return 1
}
else{
	return -1
}
```

Else-if lets you avoid big nested if-else statements and do this:

```
if(s == "one"){
	return 1
}
else if(s == "two"){
	return 2
}
else{
	return -1
}
```

In each body you can write any statements. There is no "break" keyword.



# MATCH STATEMENT

TODO POC


# FOR LOOPS

For loops are used to execute a body of statements many times. The number of times is calculated *before* the first time the body is called. Many other languages evaluates the condition for each loop iteration. In Floyd you use a while-loop for that.

Example: Closed range that starts with 1 and ends with 5:

```
for (index in 1 ... 5) {
	print(index)
}
```

Example: Open range that starts with 1 and ends with 59:

```
for (tickMark in 0 ..< 60) {
}
```

You can use expressions to define the range:, not only constants:

```
for (tickMark in a ..< string.size()) {
}
```

- ..< defines an *open range*. Up to the end value but *excluding the end value*.
- ..- defines a *closed range*. Up to and *including* the end.


# WHILE LOOPS

Perform the loop body while the expression is true.

```
while (my_array[a] != 3){
}
```

The condition is executed each time before body is executed. If the condition is false initially, then zero loops will run. If you can calculate the number of loop iteration beforehand, then prefer to use for-loop since it expresses that better and also can give better performance.


# STRING DATA TYPE

This is a pure 8-bit string type. It is immutable. You can compare with other strings, copy it using = and so on. There is a small kit of functions for changing and processing strings.

The encoding of the characters in the string is undefined. You can put 7-bit ASCII in them or UTF-8 or something else. You can also use them as fast arrays of bytes.

You can make string literals directly in the source code like this:

```
let a = "Hello, world!"
```

All comparison expressions work, like a == b, a < b, a >= b, a != b and so on.

You can access a random character in the string, using its integer position, where element 0 is the first character, 1 the second etc.

```
let a = "Hello"[1]
assert(a == "e")
```

Notice 1: You cannot modify the string using [], only read. Use update() to change a character.
Notice 2: Floyd returns the character as an int, which is 64 bit signed.

You can append two strings together using the + operation.

```
let a = "Hello" + ", world!"
assert(a == "Hello, world!")
```

### ESCAPE SEQUENCES

String literals in Floyd code cannot contain any character, because that would make it impossible for the compiler to understand what is part of the string and what is the program code. Some special characters cannot be entered directly into string literals and you need to use a special trick, an escape sequence.

You can use these escape characters in string literals by entering \n or \' or \" etc.

|ESCAPE SEQUENCE	| RESULT CHARACTER, AS HEX		| ASCII MEANING
|:---				|:---						|:---
| \a		| 0x07	| BEL, bell, alarm, \a
| \b		| 0x08	| BS, backspace, \b
| \f		| 0x0c	| FF, NP, form feed, \f
| \n		| 0x0a	| Newline (Line Feed)
| \r		| 0x0d	| Carriage Return
| \t		| 0x09	| Horizontal Tab
| \v		| 0x0b	| Vertical Tab
| \\\\	| 0x5f	| Backslash
| \'		| 0x27	| Single quotation mark
| \\"	| 0x22	| Double quotation mark

##### EXAMPLES

|CODE		| OUTPUT | NOTE
|:---			|:--- |:---
| print("hello") | hello
| print("What does \\"blobb\\" mean?") | What does "blobb" mean? | Allows you to insert a double quotation mark into your string literal without ending the string literal itself.


Floyd string literals do not support insert hex sequences or unicode code points.


##### CORE FUNCTIONS

- __print()__: prints a string to the default output of the app.
- __update()__: changes one character of the string and returns a new string.
- __size()__: returns the number of characters in the string, as an integer.
- __find()__: searches from left to right after a substring and returns its index or -1
- __push_back()__: appends a character or string to the right side of the string. The character is stored in an int.
- __subset__: extracts a range of characters from the string, as specified by start and end indexes. aka substr()
- __replace()__: replaces a range of a string with another string. Can also be used to erase or insert.



# VECTOR DATA TYPE

A vector is a collection of values where you lookup the values using an index between 0 and (vector size - 1). The items in a vector are called "elements". The elements are ordered. Finding an element at an index uses constant time. In other languages vectors are called "arrays" or even "lists".


You can make a new vector and specify its elements directly, like this:

```
let a = [ 1, 2, 3]
```

You can also calculate its elements, they don't need to be constants:

```
let a = [ calc_pi(4), 2.1, calc_bounds() ]
```

You can put ANY type of value into a vector: integers, doubles, strings, structs, other vectors and so on. But all elements must be the same type inside a specific vector.

You can copy vectors using =. All comparison expressions work, like a == b, a < b, a >= b, a != b and similar. Comparisons will compare each element of the two vectors.

This lets you access a random element in the vector, using its integer position.

```
let a = [10, 20, 30][1]
assert(a == 20)
```

Notice: You cannot modify the vector using [], only read. Use update() to change an element.

You can append two vectors together using the + operation.

```
let a = [ 10, 20, 30 ] + [ 40, 50 ]
assert(a == [ 10, 20, 30, 40, 50 ])
```

##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one element of the vector and returns a new vector.
- __size()__: returns the number of elements in the vector, as an integer.
- __find()__: searches from left to right after a sub-vector and returns its index or -1
- __push_back()__: appends an element to the right side of the vector.
- __subset__: extracts a range of elements from the vector, as specified by start and end indexes.
- __replace()__: replaces a range of a vector with another vector. Can also be used to erase or insert.



# DICTIONARY DATA TYPE

A collection of values where you identify the elemts using string keys. It is not sorted. In C++ you would use a std::map. 

You make a new dictionary and specify its elements like this:

```
let [string: int] a = {"red": 0, "blue": 100,"green": 255}
```

or shorter:

```
b = {"red": 0, "blue": 100,"green": 255}
```

Dictionaries always use string-keys. When you specify the type of dictionary you must always include "string". In the future other types of keys may be supported.

```
struct test {
	[string: int] _my_dict
}
```

You can put any type of value into the dictionary (but not mix inside the same dictionary).

Use [] to look up elements using a key. It throws an exception is the key not found. If you want to avoid that. check with exists() first.

You copy dictionaries using = and all comparison expressions work, just like with strings and vectors.


##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one element of the dictionary and returns a new dictionary
- __size()__: returns the number of elements in the dictionary, as an integer.
- __exists()__: checks to see if the dictionary holds a specific key
- __erase()__: erase a specific key from the dictionary and returns a new dictionary



# STRUCT DATA TYPE

Structs are the central building block for composing data in Floyd. They are used in place of classes in other programming languages. Structs are always values and immutable. They are very fast and compact: behind the curtains copied structs shares state between them, even when partially modified.

##### AUTOMATIC FEATURES OF EVERY STRUCT

- constructor -- this is the only function that can create a value of the struct. It always requires every struct member, in the order they are listed in the struct definition. Usually you create some functions that makes instancing a struct convenient, like make_black_color(), make_empty() etc.
- destructor -- will destroy the value including member values, when no longer needed. There are no custom destructors.
- Comparison operators: == != < > <= >= (this allows sorting too).
- Reading member values.
- Modify member values.

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects caused by several clients modifying the same struct.

This all makes simple structs extremely simple to create and use.

##### NOT POSSIBLE

- You cannot make constructors. There is only *one* way to initialize the members, via the constructor, which always takes *all* members
- There is no way to directly initialize a member when defining the struct.
- There is no way to have several different constructors, instead create explicit functions like make_square().
- If you want a default constructor, implement one yourself: ```rect make_zero_rect(){ return rect(0, 0) }```.
- There is no aliasing of structs -- changing a struct is always invisible to all other code that has copies of that struct.


Example:

```
//	Make simple, ready-for use struct.
struct point {
	double x
	double y
}

//	Try the new struct:

let a = point(0, 3)
assert(a.x == 0)
assert(a.y == 3)

let b = point(0, 3)
let c = point(1, 3)

assert(a == a)
assert(a == b)
assert(a != c)
assert(c > a)
```

A simple struct works almost like a collection with fixed number of named elements. It is only possible to make new instances by specifying every member or copying / modifying an existing one.


##### UPDATE()

let b = update(a, member, value)

```
//	Make simple, ready-for use struct.
struct point {
	double x
	double y
}

let a = point(0, 3)

//	Nothing happens! Setting width to 100 returns us a new point but we we don't keep it.
update(a,"x", 100)
assert(a.x == 0)

//	Modifying a member creates a new instance, we assign it to b
let b = update(a,"x", 100)

//	Now we have the original, unmodified a and the new, updated b.
assert(a.x == 0)
assert(b.x == 100)
```

This works with nested values too:


```
//	Define an image-struct that holds some stuff, including a pixel struct.
struct image { string name; point size }

let a = image("Cat image.png", point(512, 256))

assert(a.size.x == 512)

//	Update the width-member inside the image's size-member. The result is a brand new image, b!
let b = update(a, "size.x", 100)
assert(a.size.x == 512)
assert(b.size.x == 100)
```



# PROTOCOL DATA TYPE

TODO 1.0

This defines a set of functions that serveral clients can implement differently. This introduced polymorphim into Floyd. Equivalent to inteface classes in other languages.

Protocol member functions can be tagged "impure" which allows it to be implemented so it saves or uses state, modifies the world. There is no way to do these things in the implementation of pure protocol function memembers.





# TYPEID DATA TYPE

A typeid is tells the type of a value.

When you reference one of the built in primitive types by name, you are accessing a variable of type typeid.

- bool
- int
- double
- string

```
assert(typeid("hello") == string)
assert(to_string(typeid([1,2,3])) == "[int]")
```

A typeid is a proper Floyd value: you can copy it, compare it, convert it to strings, store it in dictionaries or whatever. Since to_string() supports typeid, you can easily print out the exact layout of any type, including complex ones with nested structs and vectors etc.



# JSON_VALUE DATA TYPE

Why JSON? JSON is very central to Floyd. Floyd is based on values (simple ones or entire data models as one value) JSON is a simple and standard way to store composite values in a tree shape in a simple and standardized way. It also makes it easy to serializing any Floyd value to text and back. JSON is built directly into the language as the default serialized format for Floyd values.

It can be used for custom file format and protocol and to interface with other JSON-based systems. All structs also automatically are serializable to and from JSON automatically.

JSON format is also used by the compiler and language itself to store intermediate Floyd program code, for all logging and for debugging features.

- Floyd has built in support for JSON in the language. It has a JSON type called __json\_value__ and functions to pack & unpack strings / JSON files into the JSON type.

- Floyd has support for JSON literals: you can put JSON data directly into a Floyd file. Great for copy-pasting snippets for tests.

Read more about JSON here: www.json.org


This value can contain any of the 7 JSON-compatible types:

- string
- number
- object
- array
- true
- false
- null

Notice: This is the only situation were Floyd supports null. Floyd things null is a concept to avoid when possible.

Notice that Floyd stores true/false as a bool type with the values true / false, not as two types called "true" and "false".

__json\_value__: 	This is an immutable value containing any JSON. You can query it for its contents and alter it (you get new values).

Notice that json\_value can contain an entire huge JSON file, with a big tree of JSON objects and arrays and so on. A json\_value can also contain just a string or a number or a single JSON array of strings. The json\_value is used for every node in the json\_value tree.


## JSON LITERALS

You can directly embed JSON inside Floyd source code file. This is extremely simple - no escaping needed - just paste a snippet into the Floyd source code. Use this for test values. Round trip. Since the JSON code is not a string literal but actual Floyd syntax, there are not problems with escaping strings. The Floyd parser will create floyd strings, dictionaries and so on for the JSON data. Then it will create a json\_value from that data. This will validate that this indeed is correct JSON data or an exception is thrown.

This all means you can write Floyd code that at runtime creates all or parts of a composite JSON value. Also: you can nest JSONs in each other.

Example JSON:

```
let json_value a = 13
let json_value b = "Hello!"
let json_value c = { "hello": 1, "bye": 3 }
let json_value d = { "pigcount": 3, "pigcolor": "pink" }

assert(a == 13)
assert(b == "Hello!")
assert(c["hello"] == 1)
assert(c["bye"] == 3)
assert(size(c) == 2)

let test_json2 = json_value(
	{
		"one": 1,
		"two": 2,
		"three": "three",
		"four": [ 1, 2, 3, 4 ],
		"five": { "alpha": 1000, "beta": 2000 },
		"six": true,
		"seven": false,
	}
)
```

Notice that JSON objects are more lax than Floyd: you can mix different types of values in the same object or array. Floyd is stricter: a vector can only hold one type of element, same with dictionaries.



##### __get\_json\_type()__:

Returns the actual type of this value stores inside the json\_value. It can be one of the types supported by JSON.

	typeid get_json_type(json_value v)

This needs to be queried at runtime since JSON is dynamically typed.


##### CORE FUNCTIONS

Many of the core functions work with json\_value, but it often depends on the actual type of json\_value. Example: size() works for strings, arrays and object only.

- __get\_json\_type()__
- __size()__
- __jsonvalue_to_script()__
- __script\_to_jsonvalue()__
- __value\_to\_jsonvalue()__
- __jsonvalue\_to\_value()__



# COMMENTS AND DOCUMENTATION

Use comments to write documentation, notes or explanations in the code. Comments are not executed or compiled -- they are only for humans. You often use the comment features to disable / hide code from the compiler.

Two types of comments:


You can wrap many lines with "/\*" and "\*/" to make a big section of documentation or to disable many lines of code. You can nest comments, for example wrap a lot of code that already contains comments using /* ... */.

```
/*	This is a comment */
```


Everything between // and newline is a comment:

```
//	This is an end-of line comment
let a = "hello" //	This is an end of line comment.
```


# EXCEPTIONS

TODO 1.0
Throw exception. Built in types, free noun. Refine, final.



# AUTOMATIC SERIALIZATION

Serializing any Floyd value is a built in mechanism. It is always true-deep.

**This is very central to Floyd -- values are core and they can easily be passed around, sent as messages, stored in files, copy-pasted from log or debugger into the source code.**


##### JSON DATA SHAPES, ESCAPING

These are the different shapes a JSON can have in Floyd:

1. Floyd value: a normal Floyd value - struct or a vector or a number etc. Stored in RAM.

2. json\_value: data is JSON compatible, stored in RAM the 7 different value types supported by json\_value. It holds one JSON value or a JSON object or a JSON array, that in turn can hold other json\_value:s.

3. JSON-script string: JSON data encoded as a string of characters, as stored in a text file.

	Example string: {"name":"John", "age":31, "city":"New York"}

	It contains quotation characters, question mark characters, new lines etc.

4. Escaped string: the string is simplified to be stuffed as a string snippet into some restricted format, like inside a parameter in a URL, as a string-literal inside another JSON or inside a REST command.

	Example string: {\\"name\":\\"John\", \\"age\\":31, \\"city\\":\\"New York\\"}

Different destinations have different limitations and escape machanisms and will need different escape functions. This is not really a JSON-related issue, more a URL, REST question.


#### FUNCTIONS
Converting a floyd json\_value to a JSON string and back. The JSON-string can be directly read or written to a text file, sent via a protocol and so on.

```
string jsonvalue_to_script(json_value v)
json_value script_to_jsonvalue(string s)
```

Converts any Floyd value, (including any type of nesting of custom structs, collections and primitives) into a json\_value, storing enough info so the original Floyd value can be reconstructed at a later time from the json\_value, using jsonvalue_to_value().

```
json_value value_to_jsonvalue(any v)
any jsonvalue_to_value(json_value v)
```

- __jsonvalue\_to\_script()__
- __script\_to\_jsonvalue()__
- __value\_to\_jsonvalue()__
- __jsonvalue\_to\_value()__






# BUILT-IN FEATURES

These functions are built into the language itself and are always available to your code. They are all pure so can be used in pure functions.



## get\_json_type()

Returns the actual type of this value stores inside the json\_value. It can be one of the types supported by JSON.

```
typeid get_json_type(json_value v)
```

This is how you check the type of JSON value and reads their different values.

|INPUT						| RESULT 		| INT
|:---						|:---			|:---
| json_value({"a": 1})		| json_object	| 1
| json_value([1, 2, 3])		| json_array	| 2
| json_value("hi")			| json_string	| 3
| json_value(13)			| json_number	| 4
| json_value(true)			| json_true		| 5
| json_value(false)			| json_false	| 6
| 							| json_null		| 7


Demo snippet, that checks type of a json\_value:

```
func string get_name(json_value value){
	let t = get_json_type(value)
	if(t == json_object){
		return "json_object"
	}
	else if(t == json_array){
		return "json_array"
	}
	else if(t == json_string){
		return "json_string"
	}
	else if(t == json_number){
		return "json_number"
	}
	else if(t == json_true){
		return "json_true"
	}
	else if(t == json_false){
		return "json_false"
	}
	else if(t == json_null){
		return "json_null"
	}
	else {
		assert(false)
	}
}
	
assert(get_name(json_value({"a": 1, "b": 2})) == "json_object")
assert(get_name(json_value([1,2,3])) == "json_array")
assert(get_name(json_value("crash")) == "json_string")
assert(get_name(json_value(0.125)) == "json_number")
assert(get_name(json_value(true)) == "json_true")
assert(get_name(json_value(false)) == "json_false")
```



## update()

This is how you modify a field of a struct, an element in a vector or string or a dictionary. It replaces the value of the specified key and returns a completely new object. The original object (struct, vector etc) is unchanged.

	let obj_b = update(obj_a, key, new_value)


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| update("hello", 3, 120)		| "helxo"
| vector		| update([1,2,3,4], 2, 33)		| [1,2,33,4]
| dictionary	| update({"a": 1, "b": 2, "c": 3}, "a", 11) | {"a":11,"b":2,"c":3}
| struct		| update(pixel,"red", 123)		| pixel(123,---,---)
| json_value:array		| 
| json_value:object		| 

For dictionaries it can be used to add completely new elements too.

|TYPE		  	| EXAMPLE						| RESULT
|:---			|:---							|:---
| dictionary	| update({"a": 1}, "b", 2] | {"a":1,"b":2}



TODO 1.0 - Update nested collections and structs.



## size()

Returns the size of a collection -- the number of elements.

```
int size(obj)
```

|TYPE		  		| EXAMPLE					| RESULT
|:---				|:---						|:---
| string			| size("hello")				| 5
| vector			| size([1,2,3,4])			| 4
| dictionary		| size({"a": 1, "b": })		| 2
| struct			|							|
| json_value:array	| size(json_value([1,2,3])	| 3
| json_value:object	| size(json_value({"a": 9})	| 1




## find()

Searched for a value in a collection and returns its index or -1.

```
int find(obj, value)
```

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| find("hello", "o")			| 4
| string		| find("hello", "x")			| -1
| vector		| find([10,20,30,40], 30)		| 2
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|




## exists()

Checks if the dictionary has an element with this key. Returns true or false.

```
bool exists(dict, string key)
```

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| 								|
| vector		| 								|
| dictionary	| exists({"a":1,"b":2,"c":3), "b")	| true
| dictionary	| exists({"a":1,"b":2,"c":3), "f")	| false
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|




## erase()

Erase an element in a dictionary, as specified using its key.

```
dict erase(dict, string key)
```



## push_back()

Appends an element to the end of a collection. A new collection is returned, the original unaffected.

|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| push_back("hello", 120)		| hellox
| vector		| push_back([1,2,3], 7)			| [1,2,3,7]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## subset()

This returns a range of elements from the collection.

```
string subset(string a, int start, int end)
vector subset(vector a, int start, int end)
```

start: 0 or larger. If it is larger than the collection, it will be clipped to the size.
end: 0 or larger. If it is larger than the collection, it will be clipped to the size.


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		| subset("hello", 2, 4)			| ll
| vector		| subset([10,20,30,40, 1, 3)	| [20,30]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## replace()

Replaces a range of a collection with the contents of another collection.

```
string replace(string a, int start, int end, string new)
vector replace(vector a, int start, int end, vector new)
```


|TYPE		  	| EXAMPLE						| RESULT |
|:---			|:---							|:---
| string		|replace("hello", 0, 2, "bori")	| borillo
| vector		|replace([1,2,3,4,5], 1, 4, [8, 9])	| [1,8,9,45]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|


Notice: if you use an empty collection for *new*, you will actually erase the range.
Notice: by specifying the same index in *start* and *length* you will __insert__ the new collection into the existing collection.




## typeof()

Return the type of its input value. The returned typeid-value is a complete Floyd type and can be stored, compared and so on.

```
typeid typeof(any)
```




# FLOYD SYNTAX

Here is the DAG for the complete syntax of Floyd.

	IDENTIFIER_CHARS: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
	WHITESPACE: " \n\t"

	LITERAL:
		BOOL_LITERAL: "true" or "false"
		INTEGER_LITERAL: [0123456789]
		DOUBLE_LITERAL: [0123456789.]
		STRING_LITERAL: "?"
	IDENTIFIER: IDENTIFIER_CHARS*

	TYPE:
		NULL							"null"
		BOOL							"bool"
		INT							"int"
		DOUBLE						"double"
		STRING						"string"
		TYPEID						???
		VECTOR						"[" TYPE "]"
		DICTIONARY					"{" TYPE ":" TYPE" "}"
		FUNCTION-TYPE					TYPE "(" ARGUMENT ","* ")" "{" BODY "}"
			ARGUMENT					TYPE identifier
		STRUCT-DEF					"struct" IDENTIFIER "{" MEMBER* "}"
			MEMBER					 TYPE IDENTIFIER
		UNNAMED-STRUCT				"struct" "{" TYPE* "}"

		UNRESOLVED-TYPE				IDENTIFIER-CHARS, like "hello"

	EXPRESSION:
		EXPRESSION-COMMA-LIST			EXPRESSION | EXPRESSION "," EXPRESSION-COMMA-LIST
		NAME-EXPRESSION				IDENTIFIER ":" EXPRESSION
		NAME-COLON-EXPRESSION-LIST		NAME-EXPRESSION | NAME-EXPRESSION "," NAME-COLON-EXPRESSION-LIST

		TYPE-NAME						TYPE IDENTIFIER
		TYPE-NAME-COMMA-LIST			TYPE-NAME | TYPE-NAME "," TYPE-NAME-COMMA-LIST
		TYPE-NAME-SEMICOLON-LIST		TYPE-NAME ";" | TYPE-NAME ";" TYPE-NAME-SEMICOLON-LIST

		NUMERIC-LITERAL				"0123456789" +++
		RESOLVE-IDENTIFIER				IDENTIFIER +++

		CONSTRUCTOR-CALL				TYPE "(" EXPRESSION-COMMA-LIST ")" +++

		VECTOR-DEFINITION				"[" EXPRESSION-COMMA-LIST "]" +++
		DICT-DEFINITION				"[" NAME-COLON-EXPRESSION-LIST "]" +++
		ADD							EXPRESSION "+" EXPRESSION +++
		SUB							EXPRESSION "-" EXPRESSION +++
									EXPRESSION "&&" EXPRESSION +++
		RESOLVE-MEMBER				EXPRESSION "." EXPRESSION +++
		GROUP						"(" EXPRESSION ")"" +++
		LOOKUP						EXPRESSION "[" EXPRESSION "]"" +++
		CALL							EXPRESSION "(" EXPRESSION-COMMA-LIST ")" +++
		UNARY-MINUS					"-" EXPRESSION
		CONDITIONAL-OPERATOR			EXPRESSION ? EXPRESSION : EXPRESSION +++

	STATEMENT:
		BODY							"{" STATEMENT* "}"

		RETURN						"return" EXPRESSION
		DEFINE-STRUCT					"struct" IDENTIFIER "{" TYPE-NAME-SEMICOLON-LIST "}"
		DEFINE-FUNCTION				"func" TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" BODY
		DEFINE-IMPURE-FUNCTION			"func" TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" "impure" BODY
		IF							"if" "(" EXPRESSION ")" BODY "else" BODY
		IF-ELSE						"if" "(" EXPRESSION ")" BODY "else" "if"(EXPRESSION) BODY "else" BODY
		FOR							"for" "(" IDENTIFIER "in" EXPRESSION "..." EXPRESSION ")" BODY
		FOR							"for" "(" IDENTIFIER "in" EXPRESSION "..<" EXPRESSION ")" BODY
		WHILE 						"while" "(" EXPRESSION ")" BODY

 		BIND-IMMUTABLE-TYPED			"let" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-IMMUTABLE-DEDUCETYPE		"let" IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-TYPED				"mutable" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-DEDUCETYPE			"mutable" IDENTIFIER "=" EXPRESSION
		EXPRESSION-STATEMENT 			EXPRESSION
 		ASSIGNMENT	 				IDENTIFIER "=" EXPRESSION



## EXAMPLE BIND AND ASSIGNMENT STATEMENTS

|SOURCE		| MEANING
|:---		|:---	
| mutable int a = 10				| Allocate a mutable local int "a" and initialize it with 10
| let int b = 11				| Allocate an immutable local int "b" and initialize it with 11
| let c = 11				| Allocate an immutable local "b" and initialize it with 11. Type will be deduced to int.
| a = 12						| Assign 12 to local mutable "a".
| let d = 8.5				| Allocate an immutable local "d" and initialize it with 8.5. Type will be deduced to double.
| let e = "hello"				| Allocate an immutable local "e" and initialize it with "hello". Type will be deduced to string.
| let f = f(3) == 2		| Allocate an immutable local "f" and initialize it true/false. Type will be bool.

| let pixel x = 20 |
| let int x = {"a": 1, "b": 2} |
| let  int x = 10 |
| let int (string a) x = f(4 == 5) |
| mutable int x = 10 |



## EXAMPLE RETURN STATEMENTS

|SOURCE		| MEANING
|:---	|:---	
| return 3						|
| return myfunc(myfunc() + 3) |



## EXAMPLE FUNCTION DEFINITION STATEMENTS

|SOURCE		| MEANING
|:---	|:---	
| func int f(string name){ return 13 |
| func int print(float a) { ... }			|
| func int print (float a) { ... }			|
| func int f(string name)					|



## EXAMPLE IF STATEMENTS

|SOURCE		| MEANING
|:---	|:---	
| if(true){ return 1000 } else { return 1001 } [



## EXAMPLE OF STRUCT DEFINITION STATEMENTS

|SOURCE		| MEANING
|:---	|:---	
| struct mytype_t { float a float b } | 
| struct a {} 						|
| struct b { int a }					|
| struct c { int a = 13 }				|
| struct pixel { int red int green int blue }		|
| struct pixel { int red = 255 int green = 255 int blue = 255 }|



## EXAMPLE EXPRESSIONS

|SOURCE		| MEANING
|:---	|:---	
| 0											|
| 3											|
| 3.5										|
| (3)										|
| 3 + 4										|
| (1 + 2) * 3									|
| x											|
| x + y										|
| hello + 3									|
| "test"										|
| "test number"								|
| f()										|
| f(10, 122)									|
| print(3)									|
| print (3) 									|
| print ("Hello, World!")						|
| print("Hello, World!" + f(3) == 2)				|
| (my\_fun1("hello, 3) + 4) * my_fun2(10))		|	
| hello[\"troll\"].kitty[10].cat					|
| condition_expr ? yesexpr : noexpr				|
| condition_expr ? yesexpr : noexpr				|
| a == 1 ? "one" : ”some other number"			|



## EXAMPLE TYPE DECLARATIONS

|SOURCE		| MEANING
|:---	|:---	
| bool								| Bool type
| int								| Int type
| string								| String type
| [int]								| Vector of ints
| [[int]]								| Vector of int-vectors
| [string:int]						| Dictionary of ints
| int ()								| Function returning int, no arguments
| int (double a, string b)				| Function returning int, arguments are double and string
| [int (double a, string b)]			| vector of functions, were function returns int and takes double and string arg.
| int (double a) ()					| Function A with no arguments, that returns a function B. B returns int and has a double argument.
| my_global							| name of custom type
| [my_global]							| vector of custom type
| mything (mything a, mything b)			| function returning mything-type, with two mything arguments
| bool (int (double a) b)				| function returns bool and takes argument of type: function that returns in and take double-argument.

