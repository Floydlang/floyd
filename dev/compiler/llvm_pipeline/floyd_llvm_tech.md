
# FLOYD

- Values live inside symbol-inits and expressions
- Function arguments are not part of its symbol list.
- Function still have nested blocks with their own symbols+statements

function_defs
	-- function definitions and struct definitions (inside function scopes) have been removed and collected in function_defs/globals
	#0	function_type, pure, arg-names, body OR host-function-id
		symbols
			0, SYMBOL-NAME, value-type, init: <value>. immutable/mutable
			1,
			2, 
		statements
			store2
			call
			block
				symbols
					0, SYMBOL-NAME, value-type, init: <value>. immutable/mutable
					1,
					2, 
				statements
					store2
					call
					block


	#1	function_type, pure, arg-names, body OR host-function-id
		symbols
			0, SYMBOL-NAME, value-type, init: <value>. immutable/mutable
			1,
			2, 
		statements
			store2
			call

globals
	-- function definitions and struct definitions have been removed and collected in function_defs/globals
	statements
		store2 []
		call[]

	symbols
		0, SYMBOL-NAME, value-type, init: <value>. immutable/mutable
		1,
		2, 


??? IDEA: Make a new tree-structure with data organised like LLVM wants it, then use that to record LLVM instructions in a small function. Less mess with LLVM code.


# LLVM MODULE

- All LLVM functions get an implicit first argument,called floyd_context_ptr, where we pass the host environment etc.

Function declarations (named prototypes)
	declare void @host_print(i64)

Function definitions (number of basic-blocks containing instructions)
	define i64 @host_print(i64) {
		entry:
		  %1 = load void (i64)*, void (i64)** @host_print
		  call void %1(i64 5)
		  ret i64 667
	}


Globals
	name, type, init

