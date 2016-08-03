


//////////////////////////////////////////////////		NODES



#if 0
enum node1_type {

	////////////////////////////////		STATEMENTS

	/*
		Example source code:
			{
				int a = 10;
				int b = random();
				if(b > 2){
					return a;
				}
				return a + b;
			}

		[body_node]
			[statement]
			[statement 2]
			[statement 3]
	*/
	body_node = 10,

	/*
		Example source code: return x * y;

		[return_value_statement]
			[expression]
	*/
	return_value_statement,

	/*
		Example source code:
			if(a == 1){
				a = a + 2;
			}
			else if (b == a){
				a = a + 1;
			}
			else {
				return 100;
			}

		[if__else_if__else_statement]
				[expression]
				[body_node]
			[else if]
				[expression]
				[body_node]
			[else if]
				[expression]
				[body_node]
			[else]
				[body_node]
	*/
//	if__else_if__else_statement,

	/*
		[for]
			[define_mutable N]
				[expression]
			[define_mutable N2]
				[expression]
			[expression]
			[update_mutable M]
			[update_mutable M2]
			[body_node]
	*/
//	for_statement,

//	foreach_statement,

	/*
		Example source code:
			while(x < 3 && y == 2){
				x++;
				y++;
			}

		[while_statement]
			[expression]
			[body_node]
	*/
//	while_statement,

	/*
		Define constant using expression and bind it to an identifier.

		let a = 3 + 4 + 5;

		[define_local_constant] [value_type] [name] [expression]
	*/
	define_local_constant_statement,

	/*
		mutable a = 3 + 4 + 5;

		Create a mutable local variable and name it. Assign it an initial value.
		[define_mutable]
			[mutable_name]
			[expression]
	*/
//	define_mutable_local_statement,

	/*
		a = 6 * 4;

		Rebind a local mutable variable to a new value.
		[update_mutable]
			[mutable_name]
			[expression]
	*/
//	update_mutable_local_statement,

	/*
		[assert]
			[expression]
	*/
//	assert_statement,

	define_global_constant_statement,



	////////////////////////////////		EXPRESSIONS



	/*
		Named function:

		int myfunc(string a, int b){
			...
			return b + 1;
		}

		Lambda:

		int myfunc(string a){
			() => {
			}
		}

		Just defines an unnamed function - it has no name.

		[make_function_expression]
			[value_type]
			[arg_list]
				[arg0]
					[value_type]
					[name]
				[arg1]
					[value_type]
					[name]
			[body_node]
	*/
	make_function_expression,

	/*
		( 14 + my_mutable_y + some_function("hi", 100) )
	*/
	expression,

	/*
		14
		"hello"
	*/
//	inline_constant,

	/*
		my_constant_x
	*/
//	local_constant,

	/*
		my_mutable_y
	*/
//	mutable_variable_expression,

	/*
		some_function("hello", 4000)
	*/
	function_call_expression,

	/*
		(a + b)
		(a - b)
		(a * b)
		(a / b)
		(a % b)
	*/
//	arithmetic_expression
};
#endif




