# GOALS
1. Floyd preferes C-like / Java syntax to appeal to imperative programmers.
2. Avoid having to repeat things: no need to implement equal() etc manually. No need to initialize every member in each constructor etc.
3. One simple and clear way to do everthing, little opportunity to do crazy stuff.


### Built into every data structure: structs, string, collections etc.
* T = copy(T b)
* bool equal(T a, T b)
* bool less_than(T a, T b)
* hash hash(T a)
* string serialize(T a)
* T T.deserialize(string s)


# IDEAS



	### fixed_size_vector




### About structs
Structs are the central building blocks for data in Floyd. They are used for structs, classes, tuples. They are always value classes and immutable. Internally, instances are shared to conserve memoryt and performance.

The order of the members inside the struct or collection is important for equal() etc.

You can chose to name members or not. An unnamed member can be accessed using its position in the struct: first member is 0.

	struct song1 {
		float length = 0;
		float? last_pos = null;
		dyn<int, string> nav_info = "off";
		private bool selected = true;
		private float internal_play_pos = 0.0f;
		float play_pos = 0.0f;
	
		invariant(song a) {
			a.length > 0
			a.last_pos == null || a.last_pos <= length;
		}

		song(){
			return new song();
		}
	}

	struct song {
		float length = 0;
		float? last_pos = null;
		dyn<int, string> nav_info = "off";

		private bool selected = true;

		private float internal_play_pos = 0.0f;

		float play_pos(song original) {
			return internal_play_pos / 1000.0f;
		}
		float play_pos2(song original) {
			return internal_play_pos;
		}

		song scale_song(song original){
			mut temp = copy(original);
			temp.length = temp.length / 2.0f;
			temp.last_pos = temp.last_pos / 2.0f;
			temp.internal_play_pos = internal_play_pos / 2.0f;
		}

		invariant(song a) {
			a.length > 0
			a.last_pos == null || a.last_pos <= length;
		}

		song(){
			return new song();
		}
		song(float bars ( >=0 && @ <= 128.0f) ){
			mut temp = new song();
			temp.length = bars * 192;
			return temp;
		}
	}

* Users of the struct are not affected if you introduce a setter or getter to a property. They are also unaffected if you move a function from being a member function to a free function - they call them the same way.
* There is no implicit _this_.
* _invariant_ is required for structs that have dependencies between members (that have setters or mutating members) and is an expression that must always be true - or the song-struct is defect. The invariant function will automatically be called whenever before a new value is exposed to client code: when returning a value from a constructor, from a mutating member function etc.

### Member-data
* All members must have default value.
* All data is always read-only for clients. Member functions can mutate local instances of the struct.
* Clients can directly access all member variables. But you can explicitly hide member data by prefixing with "private".
* You can take control over reading members by replacing its default value with a function. 

### Mutation
* When client attempts to mutate a struct, it always gets a new copy of the struct, with the change applied. This is the default behaviour.
* If you want to control the mutation, you can add your own setter function as a member function.
* You can add any type of mutating functions, not just setters.
* It is often but not always possible to make an free function that mutates a value - if it can read all data from the original and can control all member data of the newly constructed value.

### Tuples (actually a struct)
You can access the members using indexes instead of their names, for use as a tuple.

	a = struct { int, int, string }( 4, 5, "six");
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");
	
	b = struct { 4, 5, "six" };
	assert(a.0 == 4);
	assert(a.1 == 5);
	assert(a.2 == "six");

### Anonymous structs
Unnamed structs / tuples with the same signature are the considered the same type and you can automatically assign between them.

NOTE: A function takes a struct/tuple as an argument and returns another struct/tuple or basic type.


### Variables and constants
All variables are by default constants / immutable.

	assert(floyd_verify(
		"int hello1(){
			a = "hello";
			a = "goodbye";	//	Compilation error - you cannot change variable a.
			return 3;
		}"
	) == -3);

Use _mut_ to define a local variable that can be mutated. Only local variables can be mut.

	int hello2(){
		mut a = "hello";
		a = "goodbye";	//	Changes variable a to "goodbye".
		return 3;
	}

You can postfix type with ? to make it optional.


### About collections
A fixed set of collections are built right into Floyd. They can store any Floyd value, inclusing structs, string, integers and even other collections. They are all immutable / peristent.

A unique feature in Floyd is that you cannot specify the exact implementation of the collection, only its basic type: vector or map. The precise data structure is selected by the runtime according to your recommendations and according the.


### Seq
You cannot make 
seq<V>

### Maps
map<K, V>

	struct {
		my_map = [string, int]({ "one", 1 }, { "two", 2 });
		my_map2 [string, int]({ "one", 1 }, { "two", 2 });
	}
	int function test_map([string, int] p1);
	a = {string, int}({ "one", 1 }, { "two", 2 }, { "three", 3 });

### Vectors
A vector is a collection where you lookup your values using an index between 0 and (vector_size - 1). The items are ordered. Finding the correct value is constant-time. There are many potential backends for a vector:

1. A C-array. Very fast to make and read, very compact. Slow and expensive to mutate (requires copying the entire array).
2. A HAMT-based persistent vector. Medium-fast to make, read an write. Uses more memory.
3. A function. Compact, potentially very fast / very slow. No read/write.

??? Explore functions vs vectors vs maps. Integer keys vs float keys. sin(float a)?

	b = [int](1, 2, 3);

### Functions

Function always have the same syntax, no matter if they are member functions of a struct, a constructor or a free function.

!!! The arguments for a function is a struct. Function internally only take one argument.

Functions always return exactly one value. Use a tuple to return more values. A function without return value makes no sense since function cannot have side effects.

#### FUNCTION CONTRACTS
Arguments and return values always specifies their contracts using an expression within []. You can use constants, function calls and all function arguments in the expressions.

	int [_ >=0 && _ <= a] function test1(int a, int b){
	}

	function1 = function {
		_: int [_ >= 0 && _ <= a]
		a: a >= 0 && a < 100, "The minor flabbergast"

	}

#### FUNCTION PROOFS
	proof(function1) {
		"Whenever flabbergast is 0, b don't matter) and result is 0
		function1(0, 0) == 0
		function1(0, 1) == 0
		function1(0, 10000) == 0

		function1(1, 0) == 0
		function1(1, 1) == 8
	}

??? Freeze complex inputs to function and serialzie them for later exploration, adding proofs and keeping as regression tests.

Example function definitions:

	int function(dyn<string,int> x){
	}

	int function a(int a, int b){
	}

	int function test(){	
	}

int function(string x, bool y)

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


# Value serialization
Serializing a value is a built in mechamism.
It is always deep.

The result is always a normalized JSON stream.

??? De-duplication vs references vs equality vs diffing.

//	Define serialised format of a type, usable as constant.


An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream. Runtime can chose to 


