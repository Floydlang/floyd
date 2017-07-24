INVENtIONS

1) Take best part of imperativr and functional languages and make it extremely fast and simple.
2) Easy to learn for new programmers, easy to like for imperative programmers.

??? Why do you make quick scripts in python or shell script, not in C++? Fix this with Floyd


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
- **bool**				**true** or **false**
- **string**				built-in string type. 8bit pure (supports embedded nulls).
							Use for machine strings, basic UI. Not localizable.
- **float**				32-bit floating point number

# COMPOSITE TYPES
These are composites and collections of other types.
- **struct**		like C struct or class or tuple.
- **vector**		like C struct or class or tuple.


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


# STRUCTs - Simple structs
Structs are the central building blocks for composing data in Floyd. They are used in place of C-structs, classes and tuples in other languages. Structs are always values and immutable. Behind the curtains they share state between copies so they are fast and compact.


**??? make explicit list of all functions available.**

**??? Unify adressing nested dictionaries with structs and vectors. Functions too. Equivalent dict.member like dict["member"].**

Built-in features of every struct:

- You can create it using all its members, in order inside the strcuct definition
- You can destruct it
- You can copy it
- You can compare using ==, !=, < etc.
- You can sorts collections of your structs
- You can read all members
- You can modify any member (this keeps original struct and gives you a new, updated struct)

There is no concept of pointers or references or shared structs so there are no problems with aliasing or side effects because of several clients modifying the same struct.

This all makes simple structs extremely simple to create.

Structs are **true-deep**. True-deep is a Floyd term that means that all values and sub-values are always considered so any type of nesting of structs and values and collections. This includes equality checks or assignment, for example. The order of the members inside the struct (or collection) is important for sorting since those are done member by member from top to bottom.

- There is no way to directly initialize a member when defining the struct.
- There is no "default constructor" with no argument, only a constructor with *all* members. If you want one, implement one yourself: ```rect make_zero_rect(){ return rect(0, 0); }```.
- There is no way to have several different constructors, instead create explicit functions like make_square().

Insight: there are two different needs for structs: A) we want simple struct to be dead-simple -- not code needed just to collect data together. B) We want to be able to have explicit control over the member data invariant.

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

**??? Idea - use special operator for assigning to struct member: "b = a.red <- 10;"**


This works with nested values too:

	//	Define an image-struct that holds some stuff, including a pixel struct.
	struct image { string name; rect size; };

	a = image("Cat image.png", rect(512, 256));

	assert(a.size.width == 512);

	//	Update the width-member inside the image's size-member. The result is a brand new image, b!
	b = a.size.width = 100;
	assert(a.size.width == 512);
	assert(b.size.width == 100);
**??? Do some examples to prove this makes sense!**


### Struct runtime
Notice about optimizations, many accellerations are made behind the scenes:

- Causing a struct to be copied normally only bumps a reference counter and shares the data via a reference. This makes copy fast. This makes equality fast too -- the same objects is always equal.

- You cannot know that two structs with identical contents use the same storage -- if they are created independenlty from each other (not by copying) they are not necessarily deduplicated into the same object.



# Advanced STRUCTs --- TODO NEXT
**??? Maybe this isn't a struct but a class? **

There is a second, more advanced flavor of struct where the struct has an invariant that needs protecting. It is quick to upgrade a simple struct to the advanced variant and no clients needs to be affected.

An invariant is a dependency between the member variables that must always be correct. Every function that modifies or creates this struct must make sure the invariant is always correct.

With an advanced struct all automatic behavior is by default turned off and you need to enable / code each explicitly.

GOAL:

- as few functions as possible have access to the internals of the invariant, including rect's implementation
- clients should not need changing if we go from simple struct to advanced
- clients should not need changing if we make a member variable part of the invariant
- clients should not need changing if a function is a altered to have access / not access to privates.

Advice: Encapsule invariant into the smallest struct possible: big structs may want to nest many structs instead of having complex invariant.

**??? idea: only allow one invariant per struct? **


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
		//	Allow read & write of width & height.
		//	You can define a getter and setter or leave them out or use "default" which accesses the state.* with the same name.
		//	Allow init.
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


### Member and non-member access, syntactic sugar


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

Syntactic sugar for member variables:

	width(a) a.width

**??? This makes it possible to implement struct getter/setter like this:**

		rect {
			float width(rect this){ return this.state.width; }
			rect width(rect this, float width){ return this.state.width = width; }
			...
		}

Syntactic sugar for member functions:
**??? why needed? Let's be minimal.**

	is_empty(a) == a.is_empty()

Syntactic sugar for free functions:

	a = rect(6, 6);
	assert(is_square(a) == true)
	assert(a.is_square() == true)





# STRUCTs: paths
**??? Add paths to change deeply nested members?**
window_mgr_t m2 = window_mgr.window[3].root_view.child[0].title = "Welcome"

# STRUCTs: read-modify-write

**??? Add feature to read-modify-write value without listing path twice?**
window_mgr_t m2 = window_mgr.window[3].root_view.child[0].title = "Welcome: " + $0

Use <- to read & modify? A generic version of C's "a += 3".
		a <= $0 + 3
		a <= { $0 + 3 }
Works like map() but for any nested variable.


# STRUCTs: Invariant function -- SOMEDAY MAYBE

Advances structs requires an invariant() function. It performs checks that the value isn't corrupt. If it fails the code is defect. The invariant function will automatically be called before a new value is exposed to client code: when returning a value from a constructor, from a mutating member function etc.

	invariant(pixel_t this){
		assert(this.red >= 0 && this.red <= 255)
	}

# STRUCTs EXAMPLES-FEATURE --- SOMEDAY MAYBE
When defining a data type (composite) you need to list 4 example instances. Can use functions to build them or just fill-in manually or a mix. These are used in example docs, example code and for unit testing this data types and *other* data types. You cannot change examples without breaking client tests higher up physical dependency graph.


# STRUCTs: Unnamed struct members (tuples)
All members also have number in the order they are listed in the struct, starting with 0. A second name for the same member.

You can chose to not name members. An unnamed member can be accessed using its position in the struct: first member is "0", second is called "1" etc. Access like: "my_pixel.0".

my_pixel[0] // allow clients to enumerate struct members as elements. If all members are the same type result is that type. If struct members are different types, result is a dyn<int, float>

**??? How to block mutation of member variable? Some members makes no sense to write in isolation.**


# STRUCTs: Unnnamed structs (tuples)
You can access the members using indexes instead of their names, for use as a tuple. Also, there is no need to name a struct.

	a = struct { int, int, string }( 4, 5, "six");
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
	
	b = struct { 4, 5, "six" };
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
