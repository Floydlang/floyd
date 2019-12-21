//
//  expressions.h
//  Floyd
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef expressions_hpp
#define expressions_hpp

#include "ast_value.h"
#include "compiler_basics.h"
#include "types.h"

#include "quark.h"

#include <vector>
#include <string>
#include <variant>


namespace floyd {

struct struct_type_desc_t;
struct lexical_scope_t;



////////////////////////////////////////		expression_opcode_t


//	String keys used to specify statement type inside the AST JSON.

namespace expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_intrinsic = "intrinsic";
	const std::string k_load = "@";
	const std::string k_load2 = "@i";
	const std::string k_resolve_member = "->";
	const std::string k_update_member = "<-";
	const std::string k_unary_minus = "unary-minus";
	const std::string k_conditional_operator = "?:";
	const std::string k_struct_def = "struct-def";
	const std::string k_function_def = "function-def";
	const std::string k_value_constructor = "value-constructor";
	const std::string k_lookup_element = "[]";
	const std::string k_benchmark = "benchmark";


	const std::string k_arithmetic_add = "+";
	const std::string k_arithmetic_subtract = "-";
	const std::string k_arithmetic_multiply = "*";
	const std::string k_arithmetic_divide = "/";
	const std::string k_arithmetic_remainder = "%";

	const std::string k_logical_and = "&&";
	const std::string k_logical_or = "||";

	const std::string k_comparison_smaller_or_equal = "<=";
	const std::string k_comparison_smaller = "<";
	const std::string k_comparison_larger_or_equal = ">=";
	const std::string k_comparison_larger = ">";

	const std::string k_logical_equal = "==";
	const std::string k_logical_nonequal = "!=";
};



//////////////////////////////////////		expression_type



enum class expression_type {
	k_literal,
	k_call,
	k_intrinsic,
	k_load,
	k_load2,
	k_resolve_member,
	k_update_member,
	k_arithmetic_unary_minus,

	k_conditional_operator,
	k_struct_def,
	k_function_def,
	k_value_constructor,
	k_lookup_element,
	k_benchmark,

	k_arithmetic_add,
	k_arithmetic_subtract,
	k_arithmetic_multiply,
	k_arithmetic_divide,
	k_arithmetic_remainder,

	k_logical_and,
	k_logical_or,

	k_comparison_smaller_or_equal,
	k_comparison_smaller,
	k_comparison_larger_or_equal,
	k_comparison_larger,

	k_logical_equal,
	k_logical_nonequal

};


bool is_arithmetic_expression(expression_type op);
bool is_comparison_expression(expression_type op);

//	Opcode = as in the AST.
expression_type opcode_to_expression_type(const std::string& op);
std::string expression_type_to_opcode(const expression_type& op);


bool is_opcode_arithmetic_expression(const std::string& op);
bool is_opcode_comparison_expression(const std::string& op);



//////////////////////////////////////		symbol_pos_t


struct symbol_pos_t {
	/*
		0: current stack frame
		1: previous stack frame
		2: previous-previous stack frame
		-1: global stack frame
		-2: intrinsic

		Notice that the global frame has two integers: -1 and (frame_count - 1)
	*/
	enum scope_index_t {
		k_current_scope = 0,
		k_global_scope = -1,
		k_intrinsic = -2
	};


	public: symbol_pos_t() :
		_parent_steps(k_global_scope),
		_index(-1)
	{
	}

	public: bool is_empty() const {
		return _parent_steps == symbol_pos_t::k_global_scope && _index == -1;
	}
	public: bool check_invariant() const {
		QUARK_ASSERT(_parent_steps == symbol_pos_t::k_intrinsic || _parent_steps == symbol_pos_t::k_global_scope || _parent_steps >= 0);
		return true;
	}
	public: static symbol_pos_t make_stack_pos(int parent_steps, int index){
		return symbol_pos_t(parent_steps, index);
	}
	private: symbol_pos_t(int parent_steps, int index) :
		_parent_steps(parent_steps),
		_index(index)
	{
	}

	public: int _parent_steps;
	public: int _index;
};

inline bool operator==(const symbol_pos_t& lhs, const symbol_pos_t& rhs){
	return lhs._parent_steps == rhs._parent_steps && lhs._index == rhs._index;
}

inline size_t symbol_pos_to_scope_index(const symbol_pos_t& pos, size_t scope_size){
	if(pos._parent_steps == symbol_pos_t::k_global_scope){
		return 0;
	}
	else if(pos._parent_steps == symbol_pos_t::k_intrinsic){
		return symbol_pos_t::k_intrinsic;
	}
	else{
		return scope_size - 1 - pos._parent_steps;
	}
}




//////////////////////////////////////////////////		function_definition_t


struct function_definition_t {
	static function_definition_t make_func(
		const location_t& location,
		const std::string& definition_name,
		const type_t& function_type,
		const std::vector<member_t>& named_args,
		const std::shared_ptr<lexical_scope_t>& scope
	){
		return {
			location,
			definition_name,
			function_type,
			named_args,
			scope
		};
	}
	static function_definition_t make_intrinsic(
		const location_t& location,
		const std::string& definition_name,
		const type_t& function_type,
		const std::vector<member_t>& named_args
	){
		return {
			location,
			definition_name,
			function_type,
			named_args,
			{}
		};
	}

	public: bool check_invariant() const;



	////////////////////////////////	STATE

	location_t _location;

	//	This is optional and may be different than the function name in the code.
	std::string _definition_name;
	type_t _function_type;

	//	Same types as in _function_type, but augumented with argument names.
	//??? Remove. Instead have vector of just the argument names. Or update type_t to contain the argument names of functions!?
	std::vector<member_t> _named_args;

	//	Contains scope if this function is implemented using Floyd code.
	//	Else the implementation needs to be linked in.
	std::shared_ptr<const lexical_scope_t> _optional_body;
};

bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);

const type_desc_t& get_function_type(const function_definition_t& f);

json_t function_def_to_ast_json(const types_t& types, const function_definition_t& v);
function_definition_t json_to_function_def(types_t& types, const json_t& p);

void trace_function_definition_t(const types_t& types, const function_definition_t& def);


struct struct_definition2_t {
	public: struct_definition2_t(const std::vector<member_t>& members) :
		_members(members)
	{
		QUARK_ASSERT(check_invariant());
	}
	public: bool check_invariant() const;
	public: bool operator==(const struct_definition2_t& other) const;


	////////////////////////////////////////		STATE
	public: std::vector<member_t> _members;
};




//////////////////////////////////////////////////		expression_t

/*
	Immutable. Value type.
*/
struct expression_t {

	////////////////////////////////		intrinsic_t

	struct intrinsic_t {
		std::string call_name;
		std::vector<expression_t> args;
	};

	public: static expression_t make_intrinsic(
		const std::string& call_name,
		const std::vector<expression_t>& args,
		const type_t& optional_type
	){
		return expression_t(
			{ intrinsic_t { call_name, args } },
			optional_type
		);
	}



	////////////////////////////////		literal_exp_t

	struct literal_exp_t {
		public: bool operator==(const literal_exp_t& other){ return value == other.value; }

		value_t value;
	};
	public: static expression_t make_literal(const value_t& value){
//		QUARK_ASSERT(is_floyd_literal(value.get_type()));

		return expression_t({ literal_exp_t{ value } }, value.get_type() );
	}
	public: static expression_t make_literal(const value_t& value, const type_t& optional_type){
//		QUARK_ASSERT(is_floyd_literal(value.get_type()));

		return expression_t({ literal_exp_t{ value } }, optional_type );
	}

	public: static expression_t make_literal_int(const int i){
		return make_literal(value_t::make_int(i));
	}
	public: static expression_t make_literal_bool(const bool i){
		return make_literal(value_t::make_bool(i));
	}
	public: static expression_t make_literal_double(const double i){
		return make_literal(value_t::make_double(i));
	}
	public: static expression_t make_literal_string(const std::string& s){
		return make_literal(value_t::make_string(s));
	}

	const value_t& get_literal() const{
		return std::get<literal_exp_t>(_expression_variant).value;
	}


	////////////////////////////////		arithmetic_t

	struct arithmetic_t {
		expression_type op;
		std::shared_ptr<expression_t> lhs;
		std::shared_ptr<expression_t> rhs;
	};

	public: static expression_t make_arithmetic(expression_type op, const expression_t& lhs, const expression_t& rhs, const type_t& optional_type){
		QUARK_ASSERT(is_arithmetic_expression(op));

		return expression_t({ arithmetic_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, optional_type);
	}


	////////////////////////////////		comparison_t

	struct comparison_t {
		expression_type op;
		std::shared_ptr<expression_t> lhs;
		std::shared_ptr<expression_t> rhs;
	};

	public: static expression_t make_comparison(expression_type op, const expression_t& lhs, const expression_t& rhs, const type_t& optional_type){
		QUARK_ASSERT(is_comparison_expression(op));

		return expression_t({ comparison_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, optional_type);
	}


	////////////////////////////////		unary_minus_t

	struct unary_minus_t {
		std::shared_ptr<expression_t> expr;
	};

	public: static expression_t make_unary_minus(const expression_t expr, const type_t& optional_type){
		return expression_t({ unary_minus_t{ std::make_shared<expression_t>(expr) } }, optional_type);
	}


	////////////////////////////////		conditional_t

	struct conditional_t {
		std::shared_ptr<expression_t> condition;
		std::shared_ptr<expression_t> a;
		std::shared_ptr<expression_t> b;
	};

	public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b, const type_t& optional_type){
		return expression_t(
			{ conditional_t{ std::make_shared<expression_t>(condition), std::make_shared<expression_t>(a), std::make_shared<expression_t>(b) } },
			optional_type
		);
	}


	////////////////////////////////		call_t

	struct call_t {
		std::shared_ptr<expression_t> callee;
		std::vector<expression_t> args;
	};

	public: static expression_t make_call(
		const expression_t& callee,
		const std::vector<expression_t>& args,
		const type_t& optional_type
	)
	{
		return expression_t({ call_t{ std::make_shared<expression_t>(callee), args } }, optional_type);
	}


	////////////////////////////////		struct_definition_expr_t

	struct struct_definition_expr_t {
		std::string name;
		std::shared_ptr<const struct_type_desc_t> def;
	};

	public: static expression_t make_struct_definition(types_t& types, const std::string& name, const std::shared_ptr<const struct_type_desc_t>& def){
		return expression_t({ struct_definition_expr_t{ name, def } }, type_t::make_struct(types, *def));
	}


	////////////////////////////////		function_definition_expr_t

	struct function_definition_expr_t {
		function_definition_t def;
	};

	public: static expression_t make_function_definition(const function_definition_t& def){
		return expression_t({ function_definition_expr_t{ def } }, def._function_type);
	}


	////////////////////////////////		load_t

	struct load_t {
		std::string variable_name;
	};

	/*
		Specify free variables.
		It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
	*/
	public: static expression_t make_load(const std::string& variable, const type_t& optional_type){
		return expression_t({ load_t{ variable } }, optional_type);
	}


	////////////////////////////////		load2_t

	struct load2_t {
		symbol_pos_t address;
	};

	public: static expression_t make_load2(const symbol_pos_t& address, const type_t& optional_type){
		return expression_t({ load2_t{ address } }, optional_type);
	}


	////////////////////////////////		resolve_member_t

	struct resolve_member_t {
		std::shared_ptr<expression_t> parent_address;
		std::string member_name;
	};

	public: static expression_t make_resolve_member(
		const expression_t& parent_address,
		const std::string& member_name,
		const type_t& optional_type
	){
		return expression_t({ resolve_member_t{ std::make_shared<expression_t>(parent_address), member_name } }, optional_type);
	}


	////////////////////////////////		update_member_t

	struct update_member_t {
		std::shared_ptr<expression_t> parent_address;
		int member_index;
		std::shared_ptr<expression_t> new_value;
	};

	public: static expression_t make_update_member(
		const expression_t& parent_address,
		const int member_index,
		const expression_t& new_value,
		const type_t& optional_type
	){
		return expression_t({ update_member_t{ std::make_shared<expression_t>(parent_address), member_index, std::make_shared<expression_t>(new_value) } }, optional_type);
	}




	////////////////////////////////		lookup_t

	struct lookup_t {
		std::shared_ptr<expression_t> parent_address;
		std::shared_ptr<expression_t> lookup_key;
	};

	//	Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
	public: static expression_t make_lookup(
		const expression_t& parent_address,
		const expression_t& lookup_key,
		const type_t& optional_type
	)
	{
		return expression_t({ lookup_t{ std::make_shared<expression_t>(parent_address), std::make_shared<expression_t>(lookup_key) } }, optional_type);
	}



	////////////////////////////////		value_constructor_t

	struct value_constructor_t {
		type_t value_type;
		std::vector<expression_t> elements;
	};

	public: static expression_t make_construct_value_expr(
		const type_t& value_type,
		const std::vector<expression_t>& args
	)
	{
		return expression_t({ value_constructor_t{ value_type, args } }, value_type);
	}


	////////////////////////////////		benchmark_expr_t

	struct benchmark_expr_t {
		std::shared_ptr<lexical_scope_t> body;
	};

	public: static expression_t make_benchmark_expr(
		const lexical_scope_t& body
	)
	{
		return expression_t({ benchmark_expr_t{ std::make_shared<lexical_scope_t>(body) } }, type_t::make_int());
	}


	////////////////////////////////			expression_t-stuff



	public: bool check_invariant() const{
		//	QUARK_ASSERT(_debug.size() > 0);
		//	QUARK_ASSERT(_result_type._base_type != base_type::k_undefined && _result_type.check_invariant());
		QUARK_ASSERT(_output_type.check_invariant());
		return true;
	}

	public: bool operator==(const expression_t& other) const{
		QUARK_ASSERT(false);
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return true
#if DEBUG_DEEP
			&& _debug == other._debug
#endif
			&& location == other.location
//				&& _expression_variant == other._expression_variant
			&& _output_type == other._output_type
			;
	}

	public: type_t get_output_type() const {
		QUARK_ASSERT(check_invariant());

		return _output_type;
	}

	public: bool is_annotated_shallow() const{
		QUARK_ASSERT(check_invariant());

		return is_empty(_output_type) == false;
	}

	public: bool has_builtin_type() const {
		QUARK_ASSERT(check_invariant());

		return false
			|| std::holds_alternative<literal_exp_t>(_expression_variant)
			|| std::holds_alternative<struct_definition_expr_t>(_expression_variant)
			|| std::holds_alternative<function_definition_expr_t>(_expression_variant)
			|| std::holds_alternative<value_constructor_t>(_expression_variant)
			;
	}


	//////////////////////////		INTERNALS

	typedef std::variant<
		intrinsic_t,
		literal_exp_t,
		arithmetic_t,
		comparison_t,
		unary_minus_t,
		conditional_t,
		call_t,
		struct_definition_expr_t,
		function_definition_expr_t,
		load_t,
		load2_t,
		resolve_member_t,
		update_member_t,
		lookup_t,
		value_constructor_t,
		benchmark_expr_t
	> expression_variant_t;

	private: expression_t(const expression_variant_t& contents, const type_t& optional_type);


	//////////////////////////		STATE
#if DEBUG_DEEP
	public: std::string _debug;
#endif
	public: location_t location;
	public: expression_variant_t _expression_variant;
	public: type_t _output_type;
};




json_t expression_to_json(const types_t& types, const expression_t& e);
expression_t ast_json_to_expression(types_t& types, const json_t& e);

std::string expression_to_json_string(const types_t& types, const expression_t& e);

expression_type get_expression_type(const expression_t& e);


}	//	floyd


#endif /* expressions_hpp */
