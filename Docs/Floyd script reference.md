GOALS

1) Take best part of imperative and functional languages and make it extremely fast and simple.
2) Easy to learn for new programmers, easy to like for imperative programmers.

??? Why do you make quick scripts in python or shell script, not in C++? Fix this with Floyd


# Floyd Script Reference

Floyd Script is a fast and modern C-like program language that tries to make writing correct programs simpler and faster than any other programming language.

It's focus is composability, minimalism and proven programming techniques, like pure functions and immutability.

It as a unique concept to handle mutation of data, control of the outside world (files, network, UI) and controlling time.

Functions and classes are pure. A pure function can only call pure functions. This delegates all mutation / time to the top levels of your program. Here you need to call non-pure functions to affect the world around the program. You must tag your functions with "nonpure" to be able to call other nonpure functions.

This makes those functions a risk. Have as little nonpure code as possible. Try to not have any conditional code inside nonpure code - this makes testing easier. Put all logic into the pure functions.

Mutable data can exist locally inside a function, but never leak out of the function. 


# BASIC TYPES
These are the primitive data types built into the language itself. The goals is that all the basics you need are already there in the language. This makes it easy to start making useful programs, you don't need to chose or build the basics. It allows composability since all libraries can rely on these types and communicate bewteen themselves using them. Reduces need for custom types and glue code.

- **int**			Same as int64
- **bool**			**true** or **false**
- **string**		built-in string type. 8bit pure (supports embedded nulls).
					Use for machine strings, basic UI. Not localizable.
- **float**			32-bit floating point number

- **function**		A function value. Functions can be Floyd functions or C functions. They are callable.


# COMPOSITE TYPES
These are composites and collections of other types.
- **struct**		like C struct or class or tuple.
- **vector**		like C struct or class or tuple.


# CORE TYPE FEATURES
These are features built into every type: integer, string, struct, collections etc.

- **a = b** 							This true-deep copies the value b to the new name a.
- **a != b**										derivated of a = b.
- **a < b**								tests all member data in the order they appear in the struct.
- **a <= b**, **a > b**, **a >= b**	these are derivated of a < b


# VALUES, VARIABLES AND CONSTANTS
All "variables" aka values are by default constants / immutable.

- Function arguments
- Local function variables
- Member variables of structs etc.

	assert(
		floyd_verify(
			"int hello1(){
				a = "hello";
				a = "goodbye";	//	Compilation error - you cannot change variable a.
				return 3;
			}"
		)
		== -3
	);


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

Function types:

	bool (string, float)


Details:

	bool f(string a, string b){
		return a == b;
	}

This creates:

1. a new type, bool (string, string)
2. a new function object (containing the code) with type: bool (string, string).
3. a new variable "f" assigned to the new function value.

This is a function that takes a function value as argument:

	int f5(bool (string, string))

This is a function that returns a function value:

	bool (string, string) f5(int x)


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


# IF - THEN - ELSE -- STATEMENT
This is a normal if-elseif-else feature, like in most languages. Brackets are required always.

		if (s == "one"){
			return 1;
		}

You can add an else body like this:

		if(s == "one"){
			return 1;
		}
		else{
			return -1;
		}

Else-if lets you avoid big nested if-else statements and do this:

		if(s == "one"){
			return 1;
		}
		else if(s == "two"){
			return 2;
		}
		else{
			return -1;
		}

In each body you can write any statements. There is no "break" keyword.


# LOOPS: FOR AND WHILE

There are two loop feature, the for-loop and the while-loop. They are simpler and stricter than in most other languages, like the C programming language.


# FOR

For-loops are used to evaluate its body many times, with a range of input values. The entire condition expression is evaluated *before* the first time the body is called.

Closed range that starts with 1 and ends with 5.:

	for (index in 1...5) {
		print(index)
	}

Open range that starts with 1 and ends with 59:

	for (tickMark in 0..<60) {
	}

You can use expressions for range:

	for (tickMark in a..<string.size()) {
	}



Above snippet simulates the for loop of the C language but it works a little differently. There is always exactly ONE loop variable and it is defined and inited in the first section, checked in the condition section and incremented / updated in the third section. It must be the same symbol.
The result is the equivalent to

	b = 3
	{ a = 0; print(a + b); }
	{ a = 1; print(a + b); }
	{ a = 2; print(a + b); }

The loop is expanded before the first time the body is called. There is no way to have any other kind of condition expression, that relies on the result of the body etc.

- init: must be an assignment statement for variable X.
- condition: Must be a bool expression with only variable X. Is executed before each time body is executed. 
- advance-statement: must be an assignement statement to X.
- body: this is required and must have brackets. Brackets can be empty, like "{}".


# WHILE

	while (my_array[a] != 3){
	}

- condition: executed before body is executed.


# STRUCTs - Simple structs
Structs are the central building blocks for composing data in Floyd. They are used in place of C-structs, classes and tuples in other languages. Structs are always values and immutable. Behind the curtains they share state between copies so they are fast and compact.

Built-in features of every struct:

- init(...) by supplying EVERY member in order
- destructor
- Comparison operators: == != < > <= >= (this allows sorting too)
- read member
- modify member (this keeps original struct and gives you a new, updated struct)

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects caused by several clients modifying the same struct.

This all makes simple structs extremely simple to create.

Structs are **true-deep**. True-deep is a Floyd term that means that all values and sub-values are always considered so any type of nesting of structs and values and collections. This includes equality checks or assignment, for example. The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

- There is only *one* way to initialize the members, via the constructor - which always takes *all* members
- There is no way to directly initialize a member when defining the struct.
- There is no way to have several different constructors, instead create explicit functions like make_square().
- If you want a default constructor, implement one yourself: ```rect make_zero_rect(){ return rect(0, 0); }```.

Insight: there are two different needs for structs: A) we want simple struct to be dead-simple -- whenyou just want to collect data together. B) We want to be able to have explicit control over the member data invariant.

	//	Make simple, ready-for use struct.
	struct rect {
		float width;
		float height;
	};

	a = rect(0, 3);
	assert(a.width == 0);
	assert(a.height == 3);

	b = rect(0, 3);
	c = rect(1, 3);

	asset(a == a)
	asset(a == b)
	asset(a != c)
	asset(c > a)

A simple struct works almost like a collection with fixed number of named elements. It is only possible to make new instances by specifying every member or copying / modifying an existing one.

Changing member variable of a struct:

	//	Make simple, ready-for use struct.
	struct rect {
		float width;
		float height;
	};

	a = rect(0, 3);
	//	Nothing happens! Setting width to 100 returns us a new rect but we we don't keep it.
	a.width = 100
	assert(a.width == 0)

	//	Modifying a member creates a new instance, we assign it to b
	b = a.width = 100

	//	Now we have the original, unmodified a and the new, updated b.
	assert(a.width == 0)
	assert(b.width == 100)

This works with nested values too:

	//	Define an image-struct that holds some stuff, including a pixel struct.
	struct image { string name; rect size; };

	a = image("Cat image.png", rect(512, 256));

	assert(a.size.width == 512);

	//	Update the width-member inside the image's size-member. The result is a brand new image, b!
	b = a.size.width = 100;
	assert(a.size.width == 512);
	assert(b.size.width == 100);


### Struct runtime
Notice about optimizations, many accellerations are made behind the scenes:

- Causing a struct to be copied normally only bumps a reference counter and shares the data via a reference. This makes copy fast. This makes equality fast too -- the same objects is always equal.

- You cannot know that two structs with identical contents use the same storage -- if they are created independenlty from each other (not by copying) they are not necessarily deduplicated into the same object.


