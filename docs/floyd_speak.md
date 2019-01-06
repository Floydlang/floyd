![](floyd_speak_logo.png)

# FLOYD SPEAK

Floyd Speak is the programming language part of Floyd. It's an alternative to Java or Javascript or C++. Using Floyd Speak you write functions and data structures.

![](floyd_speak_cheat_sheet.png)


# DATA TYPES

These are the primitive data types built into the language itself. The goal is that all the basics you need are already there in the language. This makes it easy to start making useful programs, you don't need to choose or build the basics. It allows composability since all libraries can rely on these types and communicate between themselves using them. Reduces need for custom types and glue code.

|Type		  	| Use
|---				|---	
|__bool__			|__true__ or __false__
|__int__			| Signed 64 bit integer
|__double__		| 64-bit floating point number
|__string__		| Built-in string type. 8-bit pure (supports embedded zeros). Use for machine strings, basic UI. Not localizable.
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


# TRUE DEEP

True-deep is a Floyd term that means that all values and sub-values are always considered in operations in any type of nesting of structs and values and collections. This includes equality checks or assignment, for example.

The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.


# CORE TYPE FEATURES

These are features built into every type: integer, string, struct, dictionary, etc. They are true-deep.

|Expression		| Explanation
|---				|---	
|__a = b__ 		| This true-deep copies the value b to the new name a.
|__a == b__		| a exactly the same as b
|__a != b__		| a different to b
|__a < b__		| a smaller than b
|__a <= b__ 		| a smaller or equal to b
|__a > b__ 		| a larger than b
|__a >= b__ 		| a larger or equal to b


# ABOUT SOURCE CODE FILES

Floyd Speak files are always utf-8 files with no BOM. Their extension is ".floyd".


# ABOUT VALUES, VARIABLES AND CONSTANTS

All "variables" aka values are by immutable. Local variables can be mutable if you specify it.

- Function arguments
- Function local variables
- Member variables of structs, etc.

Floyd is statically typed, which means every variable only supports a specific type of value.

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

# ABOUT GLOBAL SCOPE

Here you normally define functions, structs and global constants. The global scope can have almost any statement and they execute at program start, before main() is called. You don't even need a main function if you don't want it.


??? Not true
__main()__ This function is called by the host when program starts. You get the input arguments from the outside world (command line arguments etc) and you can return an integer to the outside.





# SOFTWARE-SYSTEM KEYWORD

This keyword is part of Floyd Systems -- a way to define how all the containers and components and processes are interfacting.

	software-system {
		"name": "My Arcade Game",
		"desc": "Space shooter for mobile devices, with connection to a server.",
		"containers": {}
	}

Read more about this in the Floyd Systems documentation


# ABOUT FUNCTIONS

Functions in Floyd are by default *pure*, or *referential transparent*. This means they can only read their input arguments and constants, never read or modify anything: not global variables, not by calling another, impure function. It's not possible to call a function with a set of arguments and later call it with the same argument and get a different result. A function like get_time() is impure.

While a function executes, it perceives the outside world to stand still.

Functions always return exactly one value. Use a struct to return more values.

A function without return value usually makes no sense since function cannot have side effects. Possible uses would be logging, asserting or throwing exceptions.

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


# ABOUT EXPRESSIONS

Reference: http://www.tutorialspoint.com/cprogramming/c_operators.htm
Comparisons are true-deep: they consider all members and also member structs and collections.

## Arithmetic Operators
How to add and combine values:

```
+	Addition - adds two operands: "a = b + c", "a = b + c + d"
−	Subtracts second operand from the first. "a = b - c", "a = b - c - d"
*	Multiplies both operands: "a = b * c", "a = b * c * d"
/	Divides numerator by de-numerator: "a = b / c", "a = b / c / d"
%	Modulus Operator and remainder of after an integer division: "a = b / c", "a = b / c / d"
```

## Relational Operators
Used to compare two values. The result is true or false:

```
	a == b					true if a and b have the same value
	a != b				true if a and b have different values
	a > b				true if the value of a is greater than the value of b
	a < b				true if the value of a is smaller than the value of b
	a >= b
	a <= b
```

## Logical Operators
Used to compare two values. The result is true or false:

```
	a && b
	a || b
```

## Conditional Operator
```
	condition ? a : b
```

When the condition is true, this entire expression has the value of a. Else it has the value of b. Condition, a and b can all be complex expressions, with function calls, etc.

```
	func bool is_polite(string x){
		return x == "hello" ? "polite" : "rude"
	}
	assert(is_polite("hiya!") == false)
	assert(is_polite("hello") == true)
```


# ABOUT IF - THEN - ELSE -- STATEMENTS

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



# ABOUT MATCH STATEMENT

TODO POC


# ABOUT FOR LOOPS

For-loops are used to execute a body of statements many times. The number of times is calculated while the program runs. The entire condition expression is evaluated *before* the first time the body is called. This means the program already decided the number of loops to run before running the first loop iteration.

Closed range that starts with 1 and ends with 5:

```
	for (index in 1...5) {
		print(index)
	}
```

Open range that starts with 1 and ends with 59:

```
	for (tickMark in 0..<60) {
	}
```

You can use expressions for range:

```
	for (tickMark in a..<string.size()) {
	}
```


Above snippet simulates the for loop of the C language but it works a little differently. There is always exactly ONE loop variable and it is defined and inited in the first section, checked in the condition section and incremented / updated in the third section. It must be the same symbol.
The result is the equivalent to

```
	let b = 3
	{ a = 0; print(a + b) }
	{ a = 1; print(a + b) }
	{ a = 2; print(a + b) }
```

The loop is expanded before the first time the body is called. There is no way to have any other kind of condition expression, that relies on the result of the body, etc.

- init: must be an assignment statement for variable X.
- condition: Must be a bool expression with only variable X. Is executed before each time body is executed. 
- advance-statement: must be an assignment statement to X.
- body: this is required and must have brackets. Brackets can be empty, like "{}".



# ABOUT WHILE LOOPS

```
	while (my_array[a] != 3){
	}
```

- condition: executed each time before body is executed. If the condition is false initially, then zero loops will run.



# STRING DATA TYPE

This is a pure 8-bit string type. It is immutable. You can compare with other strings, copy it using = and so on. There is a small kit of functions for changing and processing strings.

The encoding of the characters in the string is undefined. You can put 7-bit ASCII in them or UTF-8 or something else. You can also use them as fast arrays of bytes.

You can make string literals directly in the source code like this:

	let a = "Hello, world!"

Notice: You cannot use any escape characters, like in the C-language.

All comparison expressions work, like a == b, a < b, a >= b, a != b and so on.

You can access a random character in the string, using its integer position.

	let a = "Hello"[1]
	assert(a == "e")

Notice 1: You cannot modify the string using [], only read. Use update() to change a character.
Notice 2: Floyd returns the character as an int, which is 64 bit signed.

You can append two strings together using the + operation.

	let a = "Hello" + ", world!"
	assert(a == "Hello, world!")


##### CORE FUNCTIONS

- __print()__: prints a string to the default output of the app.
- __update()__: changes one character of the string and returns a new string.
- __size()__: returns the number of characters in the string, as an integer.
- __find()__: searches from left to right after a substring and returns its index or -1
- __push_back()__: appends a character or string to the right side of the string. The character is stored in an int.
- __subset__: extracts a range of characters from the string, as specified by start and end indexes. aka substr()
- __replace()__: replaces a range of a string with another string. Can also be used to erase or insert.



# VECTOR DATA TYPE

A vector is a collection of values where you lookup the values using an index between 0 and (vector size - 1). The positions in the vector are called "elements". The elements are ordered. Finding an element at a position uses constant time. In other languages vectors are called "arrays" or even "lists".


You can make a new vector and specify its elements directly, like this:

	let a = [ 1, 2, 3]

You can also calculate elements:

	let a = [ calc_pi(4), 2.1, calc_bounds() ]


You can put ANY type of value into a vector: integers, doubles, strings, structs, other vectors and so on. But all elements must be the same type inside a specific vector.

You can copy vectors using =. All comparison expressions work, like a == b, a < b, a >= b, a != b and similar. Comparisons will compare each element of the two vectors.

This lets you access a random element in the vector, using its integer position.

	let a = [10, 20, 30][1]
	assert(a == 20)

Notice: You cannot modify the vector using [], only read. Use update() to change an element.

You can append two vectors together using the + operation.

	let a = [ 10, 20, 30 ] + [ 40, 50 ]
	assert(a == [ 10, 20, 30, 40, 50 ])

##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one element of the vector and returns a new vector.
- __size()__: returns the number of elements in the vector, as an integer.
- __find()__: searches from left to right after a sub-vector and returns its index or -1
- __push_back()__: appends an element to the right side of the vector.
- __subset__: extracts a range of elements from the vector, as specified by start and end indexes.
- __replace()__: replaces a range of a vector with another vector. Can also be used to erase or insert.



# DICTIONARY DATA TYPE

A collection that maps a key to a value. It is not sorted. Like a C++ map. 

You make a new dictionary and specify its elements like this:

	let [string: int] a = {"red": 0, "blue": 100,"green": 255}

or shorter:

	b = {"red": 0, "blue": 100,"green": 255}

Dictionaries always use string-keys. When you specify the type of dictionary you must always include "string".

	struct test {
		{string: int} _my_dict
	}

You can put any type of value into the dictionary (but not mix inside the same dictionary).

Use [] to look up elements using a key. It throws an exception is the key not found. Use exists() first.

You copy dictionaries using = and all comparison expressions work.


##### CORE FUNCTIONS

- __print()__: prints a vector to the default output of the app.
- __update()__: changes one element of the dictionary and returns a new dictionary
- __size()__: returns the number of elements in the dictionary, as an integer.
- __exists()__: checks to see if the dictionary holds a specific key
- __erase()__: erase a specific key from the dictionary and returns a new dictionary



# STRUCT DATA TYPE

Structs are the central building block for composing data in Floyd. They are used in place of structs and classes in other programming languages. Structs are always values and immutable. They are still fast and compact: behind the curtains copied structs  shares state between them, even when partially modified.

##### Automatic features of every struct:

- constructor -- this is the only function that can create a value of the struct. It always requires every struct member, in the order they are listed in the struct definition. Make explicit function that makes making values more convenient.
- destructor -- will destroy the value including member values, when no longer needed. There are no custom destructors.
- Comparison operators: == != < > <= >= (this allows sorting too)
- Reading member values.
- Modify member values

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects caused by several clients modifying the same struct.

This all makes simple structs extremely simple to create and use.

##### Not possible:

- You cannot make constructors. There is only *one* way to initialize the members, via the constructor, which always takes *all* members
- There is no way to directly initialize a member when defining the struct.
- There is no way to have several different constructors, instead create explicit functions like make_square().
- If you want a default constructor, implement one yourself: ```rect make_zero_rect(){ return rect(0, 0) }```.
- There is no aliasing of structs -- changing a struct is always invisible to all other code that has copies of that struct.


Example:

```
	//	Make simple, ready-for use struct.
	struct rect {
		double width
		double height
	}

	//	Try the new struct:

	let a = rect(0, 3)
	assert(a.width == 0)
	assert(a.height == 3)

	let b = rect(0, 3)
	let c = rect(1, 3)

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
	struct rect {
		double width
		double height
	}

	let a = rect(0, 3)

	//	Nothing happens! Setting width to 100 returns us a new rect but we we don't keep it.
	update(a,"width", 100)
	assert(a.width == 0)

	//	Modifying a member creates a new instance, we assign it to b
	let b = update(a,"width", 100)

	//	Now we have the original, unmodified a and the new, updated b.
	assert(a.width == 0)
	assert(b.width == 100)
```

This works with nested values too:


```
	//	Define an image-struct that holds some stuff, including a pixel struct.
	struct image { string name; rect size }

	let a = image("Cat image.png", rect(512, 256))

	assert(a.size.width == 512)

	//	Update the width-member inside the image's size-member. The result is a brand new image, b!
	let b = update(a, "size.width", 100)
	assert(a.size.width == 512)
	assert(b.size.width == 100)
```



# PROTOCOL DATA TYPE

TODO 1.0

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

A typeid is a proper Floyd value: you can copy it, compare it, convert it to strings, store it in dictionaries or whatever.



# JSON_VALUE DATA TYPE

JSON is very central to Floyd. JSON is a way to store composite values in a tree shape in a simple and standardized way. Since Floyd mainly works with values this is a perfect match for serializing any Floyd value to text and back. It is built directly into the language as the default serialized format for Floyd values. It can be used for custom file format and protocol and to interface with other JSON-based systems. All structs also automatically are serializable to and from JSON automatically.

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

This is the only situation were Floyd supports null.

Notice that Floyd stores true/false as a bool type with the values true / false, not as two types called "true" and "false".

__json\_value__: 	This is an immutable value containing any JSON. You can query it for its contents and alter it (you get new values).

Notice that json\_value can contain an entire huge JSON file, with a big tree of JSON objects and arrays and so on. A json\_value can also contain just a string or a number or a single JSON array of strings. The json\_value is used for every node in the json\_value tree.


##### JSON LITERALS

You can directly embed JSON inside source code file. Simple / no escaping needed. Just paste a snippet into the Floyd source code. Use this for test values. Round trip. Since the JSON code is not a string literal but actual Floyd syntax, there are not problems with escaping strings. The Floyd parser will create floyd strings, dictionaries and so on for the JSON data. Then it will create a json\_value from that data. This will validate that this indeed is correct JSON data or an exception is thrown.

This all means you can write Floyd code that at runtime creates all or parts of a composite JSON value. Also: you can nest JSONs in each other.

Example JSON:

	let json_value a = 13
	let json_value b = "Hello!"
	let json_value c = {"hello": 1, "bye": 3}
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

Notice that JSON objects are more lax than Floyd: you can mix different types of values in the same object or array. Floyd is stricter: a vector can only hold one type of element, same with dictionaries.



##### __get\_json\_type()__:

Returns the actual type of this value stores inside the json\_value. It can be one of the types supported by JSON.

	typeid get_json_type(json_value v)


##### CORE FUNCTIONS

Many of the core functions work with json\_value, but it often depends on the actual type of json\_value. Example: size() works for strings, arrays and object only.

- __get\_json\_type()__
- __pretty_string()__
- __size()__
- __encode_json()__
- __decode_json()__
- __flatten\_to\_json()__
- __unflatten\_from\_json()__



# COMMENTS AND DOCUMENTATION


Use comments to write documentation, notes or explanations in the code. Comments are not executed or compiled -- they are only for humans. You often use the comment features to disable / hide code from the compiler.

Two types of comments:


You can wrap many lines with "/*...*/" to make a big section of documentation or disable many lines of code. You can nest comments, for example wrap a lot of code with existing comments with /* ... */ to disable it.

	/*	This is a comment */


Everything between // and newline is a comment:

	//	This is an end-of line comment
	let a = "hello" //	This is an end of line comment.




# EXCEPTIONS

TODO 1.0
Throw exception. Built in types, free noun. Refine, final.



# AUTOMATIC SERIALIZATION

Serializing any Floyd value is a built in mechanism. It is always true-deep. The result is always a normalized JSON text file in a Floyd string.


Converting a floyd json\_value to a JSON string and back. The JSON-string can be directly read or written to a text file, sent via a protocol and so on.

	string encode_json(json_value v)
	json_value decode_json(string s)


Converts any Floyd value, (including any type of nesting of custom structs, collections and primitives) into a json\_value, storing enough info so the original Floyd value can be reconstructed at a later time from the json\_value, using unflatten_from_json().

	json_value flatten_to_json(any v)
	any unflatten_from_json(json_value v)


- __encode_json()__
- __decode_json()__
- __flatten\_to\_json()__
- __unflatten\_from\_json()__







# BUILT-IN FUNCTIONS

These functions are built into the language and are always available to your code. They are all pure so can be used in pure functions.



## assert()

Used this to check your code for programming errors, and check the inputs of your function for miss use by its callers.

	assert(bool)


If the expression evaluates to false, the program will log to the output, then be aborted via an exception.



## to_string()

Converts its input to a string. This works with any type of values. It also works with types, which is useful for debugging.

	string to_string(any)

You often use this function to convert numbers to strings.



## to\_pretty\_string()

Converts its input to a string of JSON data that is formatted nicely with indentations. It works with any Floyd value.



## get\_json_type()

Returns the actual type of this value stores inside the json\_value. It can be one of the types supported by JSON.

	typeid get_json_type(json_value v)

This is how you check the type of JSON value and reads their different values.

|Input						| Result 		| Int
|---						| ---			|---
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


|Type		  	| Example						| Result |
|---			|---							| ---
| string		| update("hello", 3, 120)		| "helxo"
| vector		| update([1,2,3,4], 2, 33)		| [1,2,33,4]
| dictionary	| update({"a": 1, "b": 2, "c": 3}, "a", 11) | {"a":11,"b":2,"c":3}
| struct		| update(pixel,"red", 123)		| pixel(123,---,---)
| json_value:array		| 
| json_value:object		| 

For dictionaries it can be used to add completely new elements too.

|Type		  	| Example						| Result
|---			|---							| ---
| dictionary	| update({"a": 1}, "b", 2] | {"a":1,"b":2}



TODO 1.0 - Update nested collections and structs.



## size()

Returns the size of a collection -- the number of elements.

	int size(obj)

|Type		  		| Example					| Result
|---				|---						| ---
| string			| size("hello")				| 5
| vector			| size([1,2,3,4])			| 4
| dictionary		| size({"a": 1, "b": })		| 2
| struct			|							|
| json_value:array	| size(json_value([1,2,3])	| 3
| json_value:object	| size(json_value({"a": 9})	| 1



## find()

Searched for a value in a collection and returns its index or -1.

	int find(obj, value)

|Type		  	| Example						| Result |
|---			|---							| ---
| string		| find("hello", "o")			| 4
| string		| find("hello", "x")			| -1
| vector		| find([10,20,30,40], 30)		| 2
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## exists()

Checks if the dictionary has an element with this key. Returns true or false.

	bool exists(dict, string key)

|Type		  	| Example						| Result |
|---			|---							| ---
| string		| 								|
| vector		| 								|
| dictionary	| exists({"a":1,"b":2,"c":3), "b")	| true
| dictionary	| exists({"a":1,"b":2,"c":3), "f")	| false
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## erase()

Erase an element in a dictionary, as specified using its key.


	dict erase(dict, string key)


## push_back()

Appends an element to the end of a collection. A new collection is returned, the original unaffected.

|Type		  	| Example						| Result |
|---			|---							| ---
| string		| push_back("hello", 120)		| hellox
| vector		| push_back([1,2,3], 7)			| [1,2,3,7]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## subset()

This returns a range of elements from the collection.

	string subset(string a, int start, int end)
	vector subset(vector a, int start, int end)

start: 0 or larger. If it is larger than the collection, it will be clipped to the size.
end: 0 or larger. If it is larger than the collection, it will be clipped to the size.


|Type		  	| Example						| Result |
|---			|---							| ---
| string		| subset("hello", 2, 4)			| ll
| vector		| subset([10,20,30,40, 1, 3)	| [20,30]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|



## replace()

Replaces a range of a collection with the contents of another collection.

	string replace(string a, int start, int end, string new)
	vector replace(vector a, int start, int end, vector new)


|Type		  	| Example						| Result |
|---			|---							| ---
| string		|replace("hello", 0, 2, "bori")	| borillo
| vector		|replace([1,2,3,4,5], 1, 4, [8, 9])	| [1,8,9,45]
| dictionary	| 								|
| struct		|								|
| json_value:array	|							|
| json_value:object	| 							|


Notice: if you use an empty collection for *new*, you will actually erase the range.
Notice: by specifying the same index in *start* and *length* you will __insert__ the new collection into the existing collection.



## map(), filter(), reduce()

TODO POC: Implement

Expose possible parallelism of pure function, like shaders, at the code level (not declarative). The supplied function must be pure.



## supermap()

TODO 1.0

	[int:R] supermap(tasks: [T, [int], f: R (T, [R]))

This function runs a bunch of tasks with dependencies between them. When supermap() returns, all tasks have been executed.

- Tasks can call blocking functions or impure functions. This makes the supermap() call impure too.
- Tasks cannot generate new tasks.
- A task *can* call supermap.
- Task will not start until all its dependencies have been finished.
- There is no way for any code to observe partially executed supermap(). It's done or not.

- **tasks**: a vector of tasks and their dependencies. A task is a value of type T. T can be an enum to allow mixing different types of tasks. Each task also has a vector of integers tell which other tasks it depends upon. The indexes are the indexes into the tasks-vector.

- **f**: this is your function that processes one T and returns a result R. The function must not depend on the order in which tasks execute. When f is called, all the tasks dependency tasks have already been executed and you get their results in [R].

- **result**: a vector with one element for each element in the tasks-argument. The order of the elements are undefined. The int specifies which task, the R is its result.



Notice: your function f can send messages to a clock — this means another clock can start consuming results while supermap() is still running.

Notice: using this function exposes potential for parallelism.


[//]: # (???)

IDEA: Make this a two-step process. First analyze the tasks into an execution description. Then use that description to run the tasks. This allows grouping small tasks into lumps. Allow you to reuse the dependency graph but tag some tasks NOP. This lets you keep the execution description for next time, if tasks are the same. Also lets you inspect the execution description and improve it or create one for scratch.



## typeof()
Return the type of its input value. The returned typeid-value is a complete Floyd type and can be stored, compared and so on.

	typeid typeof(any)



## encode_json()

Pack a JSON value to a JSON script string, ready to write to a file, send via protocol etc.

	string encode_json(json_value v)

The result is a valid JSON script string that can be handed to another system to be unpacked.

##### JSON data shapes, escaping

Notice that the string is *unescaped* - it contains the quotation character an so on. This is fine for storing it in a file or other use that supports the full 8 bit characters, but you cannot use it directly inside a URL or store it as a value in another JSON or paste it into the source code of most programming languages. Floyd has special support for unescaped JSON literals.

These are the different shapes a JSON can have in Floyd:

- Floyd value: a normal Floyd value - struct or a vector or a number etc.

- json\_value: data is JSON compatible, stored in the 7 different value types supported by json\_value. It holds one JSON value or a JSON object or a JSON array, that in turn can hold other json\_value:s.

- JSON-script string: JSON data encoded as a string of characters, as stored in a text file.

	Example string: {"name":"John", "age":31, "city":"New York"}

- Escaped string: the string is prepared to be stuffed in some more restricted format, like inside a parameter in a URL, as a string-literal inside another JSON or inside a REST command.

	Example string: {\"name\":\"John\", \"age\":31, \"city\":\"New York\"}

Different destinations have different limitations on characters and may need different escaping -- this is not really a JSON-related issue.



## decode_json()

Make a new Floyd JSON value from a JSON-script string. If the string is malformed, exceptions will be thrown.
 
	json_value decode_json(string s)



## flatten\_to_json()

	json_value flatten_to_json(any v)



## unflatten\_from_json()

	any unflatten_from_json(json_value v)






# FUTURE -- MORE BUILT-IN TYPES


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


## inbox\_tag\_t

This is used to tag message put in a green process inbox. It allows easy detection.

	struct inbox_tag_t {
		string tag_string
	}


## uuid_t

A universally unique identifier (UUID) is a 128-bit number used to identify information in computer systems. The term globaly unique identifier (GUID) is also used.

	struct uuid_t {
		int high64
		int low64
	}


## url_t

Internet URL.

	struct url_t {
		string absolute_url
	}


## quick\_hash\_t

64-bit hash value used to speed up lookups and comparisons.

	struct quick_hash_t {
		int hash
	}


## key_t

Efficent keying using 64-bit hash instead of a string. Hash can often be computed from string-key at compile time.

	struct key_t {
		quick_hash_t hash
	}


## date_t

Stores a UDT.

	struct date_t {
		string utd_date
	}


## sha1_t

128bit SHA1 hash number.

	struct sha1_t {
		string ascii40
	}


## relative\_path\_t 

File path, relative to some other path.

	struct relative_path_t {
		string fRelativePath
	}


## absolute\_path\_t

Absolute file path, from root of file system. Can specify the root, a directory or a file.

	struct absolute_path_t {
		string fAbsolutePath
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
	};


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







# FUTURE -- BUILT-IN COLLECTION FUNCTIONS


```
sort()

DT diff(T v0, T v1)
T merge(T v0, T v1)
T update(T v0, DT changes)
```



# FUTURE -- BUILT-IN STRING FUNCTIONS

```
std::vector<string> split_on_chars(seq_t s, string match_chars)
/*
	Concatunates vector of strings. Adds div between every pair of strings. There is never a div at end.
*/
string concat_strings_with_divider([string] v, string div)


float parse_float(string pos)
double parse_double(string pos)
```


# FUTURE -- BUILT-IN TEXT AND TEXT RESOURCES FUNTIONS

Unicode is used to support text in any human language. It's an extremely complex format and it's difficult to write code to manipulate or interpret it. It contains graphene clusters, code points, characters, compression etc.

Floyd's solution:

- a dedicated data type for holding Unicode text - text_t, separate from the string data type.

- use an opaque encoding of the text data -- always use the text-functions only.

Think of text_t more like PDF:s or JPEGs than strings of characters.


## lookup\_text()

Reads text from resource file (cached in RAM).
This is a pure function, it expected to give the same result every time.

	text_t lookup_text(text_resource_id id)

## text\_from\_ascii()

Makes a text from a normal string. Only 7bit ASCII is supported.

	text_t text_from_ascii(string s)


## populate\_text\_variables()

Let's you safely assemble a Unicode text from a template that has insert-tags where to insert variable texts, in a way that is localizable. Localizing the template_text can change order of variables.

	text_t populate_text_variables(text_t template_text, [string: text_t])


Example 1:

```
populate_text_variables(
	"Press ^1 button with ^2!",
	{ "^1": text_from_ascii("RED"), "^2": text_from_ascii("pinkie")}
)
```
**Output: "Press RED button with pinkie!"**

Example 2 -- reorder:

```
populate_text_variables(
	"Använd ^2 för att trycka på den ^1 knappen!",
	{ "^1": text_from_ascii("röda"), "^2": text_from_ascii("lillfingret")}
)
```
**Output: "Använd lillfingret för att trycka på den röda knappen!"**


## read\_unicode\_file()

Reads a text file into a text_t. It supports several encodings of the text file:

file_encoding:

-	0 = utf8
-	1 = Windows latin 1
-	2 = ASCII 7bit
-	3 = UTF16 BIG ENDIAN
-	4 = UTF16 LITTLE ENDIAN
-	5 = UTF32 BIG ENDIAN
-	6 = UTF32 LITTLE ENDIAN

```
	text_t read_unicode_file(absolute_path_t path, int file_encoding)
```

## write\_unicode\_file()

Writes text file in unicode format.

	void write_unicode_file(absolute_path_t path, text_t t, int file_encoding, bool write_bom)


## utf8\_to\_text()

Converts a string with UTF8-text into a text_t.

	text_t utf8_to_text(string s)


## text\_to\_utf8()

Converts Unicode in text_t value to an UTF8 string.

	string text_to_utf8(text_t t, bool add_bom)


# FUTURE -- BUILT-IN URL FUNTIONS


## is\_valid\_url()

	bool is_valid_url(const std::string& s);


## make\_url()

	url_t make_url(const std::string& s);


## url\_parts\_t {}

This struct contains an URL separate to its components.

	struct url_parts_t {
		std::string protocol;
		std::string domain;
		std::string path;
		[string:string] query_strings;
	};

Example 1:

	url_parts_t("http", "example.com", "path/to/page", {"name": "ferret", "color": "purple"})

	Output: "http://example.com/path/to/page?name=ferret&color=purple"



## split_url()

Splits a URL into its parts. This makes it easy to get to specific query strings or change a parts and reassemble the URL.

	url_parts_t split_url(const url_t& url);

## make_urls()

Makes a url_t from parts.

	url_t make_urls(const url_parts_t& parts);


## escape\_for\_url()

Prepares a string to be stored inside an URL. This requries some characters to be translated to special escapes.

| Character						| Result |
|---								| ---
| space							| "%20"
| !								| "%21"
| "								| "%22"

etc.

	std::string escape_for_url(const std::string& s);

## unescape\_from\_url()

This is to opposite of escape_for_url() - it takes a string from a URL and turns %20 into a space character and so on.

	std::string unescape_string_from_url(const std::string& s);



# FUTURE -- BUILT-IN SHA1 FUNTIONS
```

sha1_t calc_sha1(string s)
sha1_t calc_sha1(binary_t data)

//	SHA1 is 20 bytes.
//	String representation uses hex, so is 40 characters long.
//	"1234567890123456789012345678901234567890"
let sha1_bytes = 20
let sha1_chars = 40

string sha1_to_string(sha1_t s)
sha1_t sha1_from_string(string s)

```


# FUTURE - BUILT-IN RANDOM FUNTIONS

```
struct random_t {
	int value
}

random_t make_random(int seed)
int get_random(random_t& r)
random_t next(random_t& r)
```



# FUTURE - BUILT-IN MATH FUNTIONS

```
let pi_constant = 3.141592653589793238462

//	Convert an angle in degrees to radians.
radians (x)

//	Convert an angle in radians to degrees.
degrees(x)

//	Returns absolute value. If value is negative it becomes positive.
double abs(double value)

//	Returns the smaller of a and b
double min(double a, double b)

//	Returns the larger of a and b
double max(double a, double b)


//	Returns value but clipped to the span min -- max.
double truncate(double value, double min, double max)


//	Returns the arc cosine of x in radians.
double acos(double x)

//	Returns the arc sine of x in radians.
double asin(double x)

//	Returns the arc tangent of x in radians.
double atan(double x)

//	Returns the arc tangent in radians of y/x based on the signs of
//	both values to determine the correct quadrant.
double atan2(double y, double x)

//	Returns the cosine of a radian angle x.
double cos(double x)

//	Returns the hyperbolic cosine of x.
double cosh(double x)

//	Returns the sine of a radian angle x.
double sin(double x)

//	Returns the hyperbolic sine of x.
double sinh(double x)

//	Returns the hyperbolic tangent of x.
double tanh(double x)

//	Returns the value of e raised to the xth power.
double exp(double x)

//	The returned value is the mantissa and the integer pointed to by
//	exponent is the exponent. The resultant value is x = mantissa * 2 ^ exponent.
double frexp(double x, int *exponent)

//	Returns x multiplied by 2 raised to the power of exponent.
double ldexp(double x, int exponent)

//	Returns the natural logarithm (base-e logarithm) of x.
double log(double x)

//	Returns the common logarithm (base-10 logarithm) of x.
double log10(double x)

//	The returned value is the fraction component (part after the decimal),
//	and sets integer to the integer component.
double modf(double x, double *integer)

//	Returns x raised to the power of y.
double pow(double x, double y)

//	Returns the square root of x.
double sqrt(double x)

//	Returns the smallest integer value greater than or equal to x.
double ceil(double x)

//	Returns the absolute value of x.
double fabs(double x)

//	Returns the largest integer value less than or equal to x.
double floor(double x)

//	Returns the remainder of x divided by y.
double fmod(double x, double y)

```


# FUTURE - BUILT-IN FILE PATH FUNTIONS


```
struct fs_environment_t {
	//	A writable directory were we can store files for next run time.
	absolute_path_t persistence_dir

	absolute_path_t app_resource_dir
	absolute_path_t executable_dir

	absolute_path_t test_read_dir
	absolute_path_t test_write_dir

	absolute_path_t preference_dir
	absolute_path_t desktop_dir
};

fs_environment_t get_environment();

absolute_path_t get_parent_directory(absolute_path_t path)

struct path_parts_t {
	//	"/Volumes/MyHD/SomeDir/"
	string fPath

	//	"MyFileName"
	string fName

	//	Returns "" when there is no extension.
	//	Returned extension includes the leading ".".
	//	Examples:
	//		".wav"
	//		""
	//		".doc"
	//		".AIFF"
	string fExtension
}

path_parts_t split_path(absolute_path_t path)


/*
	base								relativePath
	"/users/marcus/"					"resources/hello.jpg"			"/users/marcus/resources/hello.jpg"
*/
absolute_path_t make_path(path_parts_t parts)
```


# FUTURE - BUILT-IN STRING FUNCTIONS

```
struct seq_out {
	string s
	seq_t seq
}

seq_t make_seq(string s)
bool check_invariant(seq_t s)


//	Peeks at first char (you get a string). Throws if there is no more char to read.
string first1(seq_t s)

//	Skips 1 char.
//	You get empty if rest_size() == 0.
seq_t rest1(seq_t s)


//	Returned string can be "" or shorter than chars if there aren't enough chars.
//	Won't throw.
string first(seq_t s, size_t chars)

//	Skips n characters.
//	Limited to rest_size().
seq_t rest(seq_t s, size_t count)


//	Returns entire string. Equivalent to x.rest(x.size()).
//	Notice: these returns what's left to consume of the original string, not the full original string.
string str(seq_t s)
size_t size(seq_t s)

//	If true, there are no more characters.
bool empty(seq_t s)



//	Skip any leading occurrances of these chars.
seq_t skip(seq_t p1, string chars)

seq_out read_while(seq_t p1, string chars)
seq_out read_until(seq_t p1, string chars)

seq_out split_at(seq_t p1, string str)

//	If p starts with wanted_string, return true and consume those chars. Else return false and the same seq_t.
std::pair<bool, seq_t> if_first(seq_t p, string wanted_string)

bool is_first(seq_t p, string wanted_string)


seq_out read_char(seq_t s)

/*
	Returns "rest" if s is found, else throws exceptions.
*/
seq_t read_required(seq_t s, string req)

std::pair<bool, seq_t> read_optional_char(seq_t s, char ch)
```



# CORE BUILT-IN WORLD FUNCTIONS

These are built in primitives you can always rely on being available.

They are used to interact with the world around your program and communicate with other Floyd green-processes.

**They are not pure.**

These functions can only be called at the container level, not in pure Floyd code.



## print()

This outputs one line of text to the default output of the application. It can print any type of value. If you want to compose output of many parts you need to convert them to strings and add them. Also works with types, like a struct-type.

	print(any)


| Example										| Result |
|---											| ---
| print(3)										| 3
| print("shark")								| shark
| print("Number four: " + to_string(4))			| Number four: 4
| print(int)									| int
| print([int])									| [int]
| print({string: double})						| {string:double}
| print([7, 8, 9])								| [7, 8, 9]
| print({"a": 1})								| {"a": 1}
| print(json_value("b"))						| b
| print(json_value(5.3))						| 5.3
| print(json_value({"x": 0, "y": -1}))			| {"a": 0, "b": -1}
| print(json_value(["q", "u", "v"]))			| ["q", "u", "v"]
| print(json_value(true))						| true
| print(json_value(false))						| false
| print(json_value(null))						| null



## get\_time\_of\_day()

Returns the computer's realtime clock, expressed in the number of milliseconds since system start. Useful to measure program execution. Sample get_time_of_day() before and after execution and compare them to see duration.

	int get_time_of_day()



## get\_env\_path()

Returns user's home directory, like "/Volumes/Bob".

	string get_env_path()



## read\_text\_file()

Reads a text file from the file system and returns it as a string.

	string read_text_file(string path)

Throws exception if file cannot be found or read.


## write\_text\_file()

Write a string to the file system as a text file. Will create any missing directories in the path.

	void write_text_file(string path, string data)



## send()

Sends a message to the inbox of a Floyd green process, possibly your own process.

The process may run on a different OS thread but send() is thread safe.

	send(string process_key, json_value message)



# FUTURE -- BUILT-IN WORLD FUNCTIONS

## probe()

TODO POC

	probe(value, description_string, probe_tag)

In your code you write probe(my_temp, "My intermediate value", "key-1") to let clients log my_temp. The probe will appear as a hook in tools and you can chose to log the value and make stats and so on. Argument 2 is a descriptive name, argument 3 is a string-key that is scoped to the function and used to know if several probe()-statements log to the same signal or not.



## select()

TBD POC

Called from a process function to read its inbox. It will block until a message is received or it times out.






# FUTURE -- WORLD FILE SYSTEM FUNCTIONS

These functions allow you to access the OS file system. They are all impure. Temporary files are sometimes used to make the functions revertable on errors.

Floyd uses unix-style paths in all its APIs. It will convert these to native paths with accessing the OS.


??? Keep file open, read/write part by part.
??? Paths could use [string] instead.


## load\_binary\_file()

	binary_t load_file(absolute_path_t path)

## save\_binary\_file()

Will _create_ any needed directories in the save-path.

	void save_file(absolute_path_t path, binary_t data)

## does\_entry\_exist()

	bool does_entry_exist(absolute_path_t path)


## create\_directories\_deep()

	void create_directories_deep(absolute_path_t path)

## delete\_fs\_entry\_deep()

Deletes a file or directory. If the entry has children those are deleted too = delete folder also deletes is contents.

	void delete_fs_entry_deep(absolute_path_t path)

## rename\_entry()

	void rename_entry(absolute_path_t path, string n)


## get\_entry\_info()

	struct directory_entry_info_t {
		enum EType {
			kFile,
			kDir
		}
	
		date_t creation_date
		date_t modification_date
		EType type
		file_pos_t file_size
		relative_path_t path
		string name
	}
	
	directory_entry_info_t get_entry_info(absolute_path_t path)


## get\_directory\_entries(), get\_directory\_entries\_deep()

Returns a vector of all the files and directories found at the path.

	struct directory_entry_t {
		enum EType {
			kFile,
			kDir
		}
		string fName
		EType fType
	}
	
	std::vector<directory_entry_t> get_directory_entries(absolute_path_t path)

get_directory_entries_deep() works the same way, but will also traverse each found directory. Contents of sub-directories will be also be prefixed by the sub-directory names. All path names are relative to the input directory - not absolute to file system.

	std::vector<directory_entry_t> get_directory_entries_deep(absolute_path_t path)


## NATIVE PATHS

Converts all forward slashes "/" to the path separator of the current operating system:
	Windows is "\"
	Unix is "/"
	Mac OS 9 is ":"

string to_native_path(absolute_path_t path)
absolute_path_t get_native_path(string path)




# FUTURE -- WORLD TCP COMMUNICATION


FAQ:

- ??? How can server code handle many clients in parallell? Use one green thread per request?
- How can a client have many pending requests it waits for. It queues them all and calls select() on inbox.


Network calls are normally IO-bound - it takes a long time from sending a message to getting the result -- often billions and billions of CPU clock cycles. When performing a call that is IO-bound you have two choices to handle this fact:

1. Make a synchronous call -- your code will stop until the IO operation is completed. Other processes and tasks can still run.
	- the message has been sent via sockets
	- destination devices has processed or failed to handle the message
	- the destination has transmitted a reply back.


2. Make an async call and privide a tag. Floyd will queue up your request then return immediately so your code can continue executing.

These functions are called "queue()". At a future time when there is a reply, your green-process will receive a special message in its INBOX.



```
struct tcp_server_t {
	int id;
};

struct tcp_request_t {
	tcp_server_t server;
	binary_t payload;
};

struct tcp_reply_t {
	binary_t payload;
};

struct tcp_server_settings_t {
	url_t url;
	int port;

	//	domain: 1 = AF_INET (IPv4 protocol), 2 = AF_INET6 (IPv6 protocol).
	//	Requests arrive in INBOX as tcp_request_t.
	int domain;
	inbox_tag_t inbox_tag;
};

tcp_server_t open_tcp_server(const tcp_server_settings_t& s);
tcp_server_settings_t get_settings(const tcp_server_t& s);
void close_tcp_server(const tcp_server_t& s);

void reply(const tcp_request_t& request, const tcp_reply_t& reply);



struct tcp_client_t {
	int id;
};

tcp_client_t open_tcp_client_socket(const url_t& url, int port);
void close_tcp_client_socket(const tcp_client_t& s);

//	Blocks until IO is complete.
tcp_reply_t send(const tcp_client_t& s, const binary_t& payload);

Returns at once. When later a reply is received, you will get a message

with a tcp_reply_t in your green-process INBOX.

void queue(const tcp_client_t& s, const binary_t& payload, const inbox_tag_t& inbox_tag);

```


# FUTURE -- WORLD REST COMMUNICATION

REST uses HTTP commands to communicate between client and server and these is no open session -- each command is its own session.

```
struct rest_request_t {
	//	"GET", "POST", "PUT"
	std::string operation_type;

	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> params;
	binary_t raw_data;

	//	 If not "", then will be inserted in HTTP request header as XYZ.
	std::string optional_session_token;
};

struct rest_reply_t {
	rest_request_t request;
	int html_status_code;
	std::string response;
	std::string internal_exception;
};

//	Blocks until IO is complete.
rest_reply_t send(const rest_request_t& request);

//	Returns at once. When later a reply is received, you will get a message with a rest_reply_t in your green-process INBOX.
void queue_rest(const rest_request_t& request, const inbox_tag_t& inbox_tag);

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
		INT								"int"
		DOUBLE							"double"
		STRING							"string"
		TYPEID							???
		VECTOR							"[" TYPE "]"
		DICTIONARY						"{" TYPE ":" TYPE" "}"
		FUNCTION-TYPE					TYPE "(" ARGUMENT ","* ")" "{" BODY "}"
			ARGUMENT					TYPE identifier
		STRUCT-DEF						"struct" IDENTIFIER "{" MEMBER* "}"
			MEMBER						 TYPE IDENTIFIER
		UNNAMED-STRUCT					"struct" "{" TYPE* "}"

		UNRESOLVED-TYPE					IDENTIFIER-CHARS, like "hello"

	EXPRESSION:
		EXPRESSION-COMMA-LIST			EXPRESSION | EXPRESSION "," EXPRESSION-COMMA-LIST
		NAME-EXPRESSION					IDENTIFIER ":" EXPRESSION
		NAME-COLON-EXPRESSION-LIST		NAME-EXPRESSION | NAME-EXPRESSION "," NAME-COLON-EXPRESSION-LIST

		TYPE-NAME						TYPE IDENTIFIER
		TYPE-NAME-COMMA-LIST			TYPE-NAME | TYPE-NAME "," TYPE-NAME-COMMA-LIST
		TYPE-NAME-SEMICOLON-LIST		TYPE-NAME ";" | TYPE-NAME ";" TYPE-NAME-SEMICOLON-LIST

		NUMERIC-LITERAL					"0123456789" +++
		RESOLVE-IDENTIFIER				IDENTIFIER +++

		CONSTRUCTOR-CALL				TYPE "(" EXPRESSION-COMMA-LIST ")" +++

		VECTOR-DEFINITION				"[" EXPRESSION-COMMA-LIST "]" +++
		DICT-DEFINITION					"[" NAME-COLON-EXPRESSION-LIST "]" +++
		ADD								EXPRESSION "+" EXPRESSION +++
		SUB								EXPRESSION "-" EXPRESSION +++
										EXPRESSION "&&" EXPRESSION +++
		RESOLVE-MEMBER					EXPRESSION "." EXPRESSION +++
		GROUP							"(" EXPRESSION ")"" +++
		LOOKUP							EXPRESSION "[" EXPRESSION "]"" +++
		CALL							EXPRESSION "(" EXPRESSION-COMMA-LIST ")" +++
		UNARY-MINUS						"-" EXPRESSION
		CONDITIONAL-OPERATOR			EXPRESSION ? EXPRESSION : EXPRESSION +++

	STATEMENT:
		BODY							"{" STATEMENT* "}"

		RETURN							"return" EXPRESSION
		DEFINE-STRUCT					"struct" IDENTIFIER "{" TYPE-NAME-SEMICOLON-LIST "}"
		DEFINE-FUNCTION				 	"func" TYPE IDENTIFIER "(" TYPE-NAME-COMMA-LIST ")" BODY
		IF								"if" "(" EXPRESSION ")" BODY "else" BODY
		IF-ELSE							"if" "(" EXPRESSION ")" BODY "else" "if"(EXPRESSION) BODY "else" BODY
		FOR								"for" "(" IDENTIFIER "in" EXPRESSION "..." EXPRESSION ")" BODY
		FOR								"for" "(" IDENTIFIER "in" EXPRESSION "..<" EXPRESSION ")" BODY
		WHILE 							"while" "(" EXPRESSION ")" BODY

 		BIND-IMMUTABLE-TYPED			"let" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-IMMUTABLE-DEDUCETYPE		"let" IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-TYPED				"mutable" TYPE IDENTIFIER "=" EXPRESSION
 		BIND-MUTABLE-DEDUCETYPE			"mutable" IDENTIFIER "=" EXPRESSION
		EXPRESSION-STATEMENT 			EXPRESSION
 		ASSIGNMENT	 					IDENTIFIER "=" EXPRESSION



## EXAMPLE BIND AND ASSIGNMENT STATEMENTS

|Source		| Meaning
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

|Source		| Meaning
|:---	|:---	
| return 3						|
| return myfunc(myfunc() + 3) |



## EXAMPLE FUNCTION DEFINITION STATEMENTS

|Source		| Meaning
|:---	|:---	
| func int f(string name){ return 13 |
| func int print(float a) { ... }			|
| func int print (float a) { ... }			|
| func int f(string name)					|



## EXAMPLE IF STATEMENTS

|Source		| Meaning
|:---	|:---	
| if(true){ return 1000 } else { return 1001 } [



## EXAMPLE OF STRUCT DEFINITION STATEMENTS

|Source		| Meaning
|:---	|:---	
| struct mytype_t { float a float b } | 
| struct a {} 						|
| struct b { int a }					|
| struct c { int a = 13 }				|
| struct pixel { int red int green int blue }		|
| struct pixel { int red = 255 int green = 255 int blue = 255 }|



## EXAMPLE EXPRESSIONS

|Source		| Meaning
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

|Source		| Meaning
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

