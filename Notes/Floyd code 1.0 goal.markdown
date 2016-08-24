# BASIC TYPES
- **float**					Same as float32
- **float32**				32 bit floating point
- **float80**				80 bit floating point
- **int8**					8 bit integer
- **int16**
- **int32**
- **int64**
- **function**				function pointer. Functions can be Floyd functions or C functions.


### MORE TYPES

- **map**			look up values from a key. Localizable.
- **vector**		look up values from a 0-based continous range of integer indexes.
- **enum**		same as struct with only static constant data members


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

- Global variables
- Contents of collections


Use _mutable_ to define a local variable that can be mutated. Only _local variables_ can be mutable.

	int hello2(){
		mutable a = "hello";
		a = "goodbye";	//	Changes variable a to "goodbye".
		return 3;
	}


# FUNCTIONS
Function always have the same syntax, no matter if they are member functions of a struct, a constructor or a free function.

NOTE: A function is a thing that takes a structas an argument and returns another struct/or basic type.


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


# EXPRESSIONS
	%		Modulus Operator and remainder of after an integer division.				"B % A = 0"

### Relational Operators
	a == b				true if a and b have the same value
	a != b				true if a and b have different values
	a > b				true if the value of a is greater than the value of b
	a < b				true if the value of a is smaller than the value of b
	a >= b
	a <= b

### Logical Operators
a && b
a ||b
!a

### Trinary Operators
condition ? a : b

	bool is_polite(string x){
		return x == "hello" ? "polite" : "rude"
	}
	assert(is_polity("hiya!") == false);
	assert(is_polity("hello") == true);

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



# STRUCTs
You can chose to name members or not. An unnamed member can be accessed using its position in the struct: first member is "0", second is called "1" etc. Access like: "my_pixel.0". Named members are also numbered - a parallell name.

??? How to block mutation of member variable? Some members makes no sense to write in isolation.

## Struct Signature

* Users of the struct are not affected if you introduce a setter or getter to a property. They are also unaffected if you move a function from being a member function to a free function - they call them the same way.
* There is no implicit _this_.

Clients to a struct should not care if:
1) A struct property is a data member or a function, ever. Always the same syntax.
2) a function has private access or not to the struct. Always the same syntax.
3) a function is part of the struct or provided some other way. Always the same syntax.

This makes it easier to refactor code: you can change a function from non-member to member without affecting any client code!

??? private member data


## Constructors

Two constructors are automatically generated:
	The first one takes no argument and initializes all members to their defaults
	The second one has one argument for each member, in the order they appear in the struct.

You cannot stop these two contructors, but you can reimplement them if you wish.??? yes you want to be able to block them. Some struct have no meaningful default.






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

	c = [string];	//	Empty vector of strings.
	
There are many potential backends for a vector - the runtime choses for each vector instances (not each type!):

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read an write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No read/write.

Vector Reference:

	vector(seq<t> in);
	vector [T][ 1, 2, 3, ... };
	vector<T> make(vector<T> other, int start, int end);
	vector<T> subset(vector<T> in, int start, int end);
	a = b + c;


# MAP
A collection that maps a key to a value.Unsorted. Like a dictionary. 
	//	Create a string -> int map with three entries.
	a = [string: int]["red": 0, "blue": 100,"green": 255];

	//	Make map where key is a string and value is an int. Initialize it.
	a = [string: int]("one": 1, "two": 2, "three": 3);

	//	Make a string->int map where collection type is deducted from the initialization values.
	b = ["one": 1, "two": 2, "three", 3];

	c = [string: int]; //	Empty map of string,int.
	

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

An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream.


# IMPORT / DEFINE LIBRARIES

# RUNTIME ERRORS / EXCEPTIONS
Principles and policies.
Functions.
	??? Only support this in mutating parts and externals?


# C GLUE
