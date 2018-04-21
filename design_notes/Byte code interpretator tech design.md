#TERMINOLOGY & REFS

A basic block is a maximal sequence of
instructions with:
– no labels (except at the first instruction), and
– no jumps (except in the last instruction)


Control-Flow Graphs: Example



For languages like C there are three granularities
of optimizations
(1) Local optimizations
• Apply to a basic block in isolation
(2) Global optimizations
• Apply to a control-flow graph (function body) in isolation
(3) Inter-procedural optimizations
• Apply across method boundaries



The three-address code (3AC), in its simplest form, is a language in which an instruction is an assignment and has at most 3 operands.

source -> PARSER -> AST
AST -> Semantic Analysis -> SYMBOL TABLE + annotated AST
	Type inference
	Type checking
	Symbol management


# HOW TO MAKE SUM-TYPE IN C++

expression_t
	type
	expr-baseclass


statement_t
	shared_ptr<return_statement>
	shared_ptr<return_statement>
	shared_ptr<return_statement>




typeid_t {
	type
	expressions[]
	string variable_name
}

value_t {
	type
	shared_ptr<return_statement>
	shared_ptr<return_statement>
	shared_ptr<return_statement>
}


test_t {
	type
	union {
	}
}

# BYTE CODE

	struct symbol_t {
		string _name;
	}
	struct env_symbol_table_t {
		vector<symbol_t> _elements;
	}


	//??? Have a block-type operand that holds a list of instructions.

	struct bc_statement {
		enum {

			//	return operand1
			k_return = 1,

			//	operand1 = operand2
			k_mutate,

			//	operand1 = operand2
			k_bind_new_mutable_variable,

			//	operand1 = operand2
			k_bind_new_immutable_variable,


			//	operand1: value == 0, operand2 = 
			k_branch_if_zero,
			k_branch_if_nonzero,

			k_call,
		}

		long _opcode;

		//	Index into variable set.
		long _operand1_index;
		long _operand1_type;

		long _operand2_index;
		long _operand2_type;
	};

