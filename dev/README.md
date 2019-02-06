#  OVERVIEW OF FLOYD'S INTERNALS

When running a program, this is what happens:

1. The parser reads the program text and makes a parse tree. Finds syntax errors, figures out statements and expressions.
2. The parse tree is converted to C++ objects.
3. Semantic analysis of the parse tree -- finds semantic errors, type errors, resolves types, figures out lexical scopes resolves variable names.
4. Code generation -- converts the AST to byte code, in Floyd's custom format.
5. Byte code interpretation -- executes the byte code.

Between each of the above steps there is *one* value that defines the entire program. This value is in JSON format or can be convert to/from JSON format. This makes it simple to insert tools along the way or skip or replace steps all together.


## PARSER - PROCESS PROGRAM TEXT

The parser reads a floyd program as a text file and outputs a parse tree encoded as JSON. It understands function definitions, complex expressions with parenthesis and brackets etc. It figures out evaluation order of expressions.

Source files sits in "floyd\_parser" directory.

- parse\_program() -- main function
- json\_t -- used for storing the output
- seq\_t -- clever type that allows simple text parsing in C++

Also uses typeid\_t indirectly to parse types.

The parser is designed so it can be used on its own. This makes it simple to create tools for Floyd. All information from the program text is preserved inside the parser output, except formatting and comments.


## PASS 2 - CONVERT JSON TO AST-TYPES

Turns the JSON into internal AST C++ types, see "floyd_ast" directory.

- json\_to\_ast() -- does the conversion
- ast\_t - holds the programs AST using C++ objects (not JSON).
- expression\_t
- statement\_t
- typeid\_t - use for all types
- value\_t - used for literals.

Pass2 code also support converting the AST types *back* to JSON for round-tripping the AST! This is great for debugging and tracing. It also makes tools simple. Pretty printer, code generator, optimizer, templates.


## PASS 3 - SEMANTIC ANALYSIS

Checks every detail of the AST for errors, resolves all names, infers all implicit types.

It does *not* perform any type of optimization, like constant folding. We want the purest representation of the programmer's intent at this point.

DETAILS

- Check for semantic errors.
- Create a lexical scope for globals, each struct, each function's contents and each nested block.
- Checks access too collections -- do types match up?
- Checks types for every function call and each argument.
- Match up and figure out all types in the program. Infered types may need detective work to figure out.
- Decide what each identifier referes too -- via lexical scopes.
- Converts string-identifiers to indexes into data structures
- Builds a symbol list for each lexical scope
- Makes sure there are no name collisions.
- Checks all arithmetic expression, that they make sense
- Has special handling for built in functions, like assert() and size().

After semantic analysis we have a statically checked program with no errors!

Output is a semantic\_ast\_t that holds all globals (symbols and statements) and a list of all functions (with their own local symbols and statements).

You can use json\_to\_pretty\_string(ast\_to\_json(ast.\_checked\_ast)._value)) to convert the entire program to a JSON-string.


## PASS 4 - BYTE CODE GENERATION

Floyd uses a custom register-based virtual machine. All instructions operate on registers. The registers are always allocated inside a stack frame. Temporary values are allocated as registers in the stack frame. At this time there is no reuse of stack frame entries -- all values and temps used in a stack frame have their own slot.

The values in the byte code don't know their own type. The type is embedded in the selection of byte code instructions, by the definitions of the stack frames and by the definitions of the structs.

Values can live in a stack frame (the global stack frame or another one) and within other values (like a vector or struct) or via a bc\_value\_t. The type bc\_value\_t wraps an interpreter value with propery value-semantics to make it safe for outside code to interact with the interpreter.

- interpreter\_t
- interpreter\_stack\_t
- bc\_pod\_value\_t
- bc\_pod64\_t
- bc\_value\_object\_t
- bc\_value\_t

Term: "ext" -- sometimes a bit is kept to know if a value is an "obj"-object or and "intern"-internal.
All obj-value are reference counted, usually by the instructions.

Function calls are pretty slow, they call the interpreter recursively. 

TODO: Confusing naming: ext, obj, object, pod64, intern, bc\_pod\_value\_t, bc\_pod64\_t, bc\_value\_object\_t. RC for obj.

Example instructions and their encoding:

```
/*
	A: IMMEDIATE: global index
	B: Register: source reg
	C: ---
*/
k_store_global_obj,
```

```
/*
	A: Register: where to put result
	B: Register: struct object
	C: IMMEDIATE: member-index
*/
k_get_struct_member,
```

```
/*
	A: Register: where to put result: integer
	B: Register: object
	C: Register: value
*/
k_pushback_vector_obj,
k_pushback_vector_pod64,
k_pushback_string,
```

```

/*
	A: Register: where to put result
	B: Register: lhs
	C: Register: rhs
*/
k_add_bool,
k_add_int,
```


```
	struct bc_instruction_t {
		bc_opcode _opcode;
		uint8_t _pad8;
		int16_t _a;
		int16_t _b;
		int16_t _c;
	};
```


This is the static information the interpreter knows about a stack frame:

```
struct bc_frame_t {
	std::vector<bc_instruction_t> _instrs2;
	std::vector<std::pair<std::string, bc_symbol_t>> _symbols;
	std::vector<typeid_t> _args;
	std::vector<bool> _exts;
	std::vector<bool> _locals_exts;
	std::vector<bc_value_t> _locals;
};
```




# PASS 5 - EXECUTING PROGRAM

This is an example how the interpreter executes a simple register based instruction:

```
case bc_opcode::k_add_int: {
	ASSERT(stack.check_reg_int(i._a));
	ASSERT(stack.check_reg_int(i._b));
	ASSERT(stack.check_reg_int(i._c));

	regs[i._a]._pod64._int64 = regs[i._b]._pod64._int64 + regs[i._c]._pod64._int64;
	break;
}
```



# IMPORTANT INTERNAL TYPES

## BASICS

- json\_t -- a JSON value that can be a string or an object or array of nested JSON values.

JSON is central to Floyd. It's used internally between the compiler passes to store the Floyd program itself. This allows for easy processing of Floyd code by the compiler but also for making new tools.

JSON is also a built-in data type in the Floyd language - a default built in way to get data values in and out of your programs.

- seq\_t -- wraps a potentially huge string and an offset in an immutable value. You get new seq\_t:s when you read from the seq\_t but the internal string is shared.
 

## PARSER AND COMPILER

- typeid\_t -- this value describes a floyd type. It can be "int" or "string" or struct { int red int blue int green } or [string] etc. It can be converted to/from JSON.

- value\_t -- this can represent any value that a floyd program can hold. It can also represent some internal types. This type is used inside the compiler for literals.

- ast\_t -- the complete program encoded using C++ objects.

- expression\_t -- one of several types of expressions, like k\_lookup\_element, k\_literal, k\_struct\_def, k\_arithmetic\_add__2

- statement\_t -- one of several types of statements, like bind, return, if.

- symbol\_table\_t -- list of all symbols inside a lexical scope
 

# BYTE CODE INTERPRETER

- bc\_pod\_value\_t -- a 64-bit value that holds any value. It doesn't know its type.

- bc\_pod64\_t -- an "internal" value that can be completely stored inside the 64 bits. Like int, bool, double.

- bc\_value\_object\_t -- an extra block of data that holds what can't be fit inside the bc\_pod\_value\_t.

- interpreter\_stack\_t -- represents the virtual machine's stack. It has a current-position and an array of bc\_pod\_value\_t:s.

- interpreter\_t -- the virtual machine itself.

- bc\_value\_t - This is a standalone representation of a value, as a real C++ value class, with value semantics. It knows its type and handles reference counting properly, can be copied around etc. Using bc\_value\_t it's simple to interact with the interpreter but slighly inefficent compared to manipulating the interpreter directly.


# MORE

- floyd\_main.cpp -- The main application, that creates a command line application. It can compile and run script files or run its internal tests.

- quark: small library that supports simple unit tests, tracing (logging), exceptions and asserts.



