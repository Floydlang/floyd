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

#include "quark.h"

#include <vector>
#include <string>
#include <variant>


namespace floyd {



////////////////////////////////////////		expression_opcode_t


//	String keys used to specify statement type inside the AST JSON.

namespace expression_opcode_t {
	const std::string k_literal = "k";
	const std::string k_call = "call";
	const std::string k_corecall = "corecall";
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
	k_arithmetic_add = 10,
	k_arithmetic_subtract,
	k_arithmetic_multiply,
	k_arithmetic_divide,
	k_arithmetic_remainder,

	k_comparison_smaller_or_equal,
	k_comparison_smaller,
	k_comparison_larger_or_equal,
	k_comparison_larger,

	k_logical_equal,
	k_logical_nonequal,

	k_logical_and,
	k_logical_or,

	k_literal,
	k_arithmetic_unary_minus,
	k_conditional_operator,
	k_call,
	k_corecall,
	k_load,
	k_load2,
	k_resolve_member,
	k_update_member,
	k_lookup_element,
	k_struct_def,
	k_function_def,
	k_value_constructor,
	k_benchmark
};


bool is_arithmetic_expression(expression_type op);
bool is_comparison_expression(expression_type op);

//	Opcode = as in the AST.
expression_type opcode_to_expression_type(const std::string& op);
std::string expression_type_to_opcode(const expression_type& op);


bool is_opcode_arithmetic_expression(const std::string& op);
bool is_opcode_comparison_expression(const std::string& op);



//////////////////////////////////////		variable_address_t


struct variable_address_t {
	public: variable_address_t() :
		_parent_steps(-1),
		_index(-1)
	{
	}

	public: bool is_empty() const {
		return _parent_steps == -1 && _index == -1;
	}
	public: bool check_invariant() const {
		return true;
	}
	public: static variable_address_t make_variable_address(int parent_steps, int index){
		return variable_address_t(parent_steps, index);
	}
	private: variable_address_t(int parent_steps, int index) :
		_parent_steps(parent_steps),
		_index(index)
	{
	}

	/*
		0: current stack frame
		1: previous stack frame
		2: previous-previous stack frame
		-1: global stack frame
	*/
	public: int _parent_steps;
	public: int _index;
};

inline bool operator==(const variable_address_t& lhs, const variable_address_t& rhs){
	return lhs._parent_steps == rhs._parent_steps && lhs._index == rhs._index;
}




//////////////////////////////////////////////////		function_definition_t


struct function_definition_t {
	struct empty_t {
		bool operator==(const empty_t& other) const{ return true; };

	};
	struct floyd_func_t {
		bool operator==(const floyd_func_t& other) const;

		std::shared_ptr<const body_t> _body;
	};
	struct host_func_t {
		bool operator==(const host_func_t& other) const { return _host_function_id == other._host_function_id; };

		function_id_t _host_function_id;
	};

	typedef std::variant<empty_t, floyd_func_t, host_func_t> function_def_variant_t;


	//	Placeholder.
	static function_definition_t make_empty(){
		return {
			k_no_location,
			"",
			typeid_t::make_function(typeid_t::make_void(), {}, epure::pure),
			{},
			empty_t{ }
		};
	}

	static function_definition_t make_floyd_func(const location_t& location, const std::string& definition_name, const typeid_t& function_type, const std::vector<member_t>& args, const std::shared_ptr<body_t>& body){
		return {
			location,
			definition_name,
			function_type,
			args,
			floyd_func_t{ body }
		};
	}
	static function_definition_t make_host_func(const location_t& location, const std::string& definition_name, const typeid_t& function_type, const std::vector<member_t>& args, function_id_t host_function_id){
		return {
			location,
			definition_name,
			function_type,
			args,
			host_func_t{ host_function_id }
		};
	}

	public: bool check_invariant() const;



	////////////////////////////////	STATE

	location_t _location;

	//	This is optional and may be different than the function name in the code. It's a hint only.
	std::string _definition_name;
	typeid_t _function_type;

	//	Same types as in _function_type, but augumented with argument names.
	std::vector<member_t> _args;

	function_def_variant_t _contents;
};

bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);

const typeid_t& get_function_type(const function_definition_t& f);

json_t function_def_to_ast_json(const function_definition_t& v);
function_definition_t json_to_function_def(const json_t& p);

json_t function_def_expression_to_ast_json(const function_definition_t& v);






//////////////////////////////////////////////////		expression_t

/*
	Immutable. Value type.
*/
struct expression_t {

	////////////////////////////////		corecall_t

	struct corecall_t {
		std::string call_name;
		std::vector<expression_t> args;
	};

	public: static expression_t make_corecall(
		const std::string& call_name,
		const std::vector<expression_t>& args,
		const std::shared_ptr<typeid_t>& annotated_type
	){
		return expression_t(
			{ corecall_t { call_name, args } },
			annotated_type
		);
	}
	public: static expression_t make_corecall(
		const std::string& call_name,
		const std::vector<expression_t>& args,
		const typeid_t& annotated_type
	){
		return expression_t(
			{ corecall_t { call_name, args } },
			std::make_shared<typeid_t>(annotated_type)
		);
	}



	////////////////////////////////		literal_exp_t

	struct literal_exp_t {
		public: bool operator==(const literal_exp_t& other){ return value == other.value; }

		value_t value;
	};
	public: static expression_t make_literal(const value_t& value){
		return expression_t({ literal_exp_t{ value } }, std::make_shared<typeid_t>(value.get_type()));
	}

	public: static expression_t make_literal_undefined(){
		return make_literal(value_t::make_undefined());
	}
	public: static expression_t make_literal_internal_dynamic(){
		return make_literal(value_t::make_any());
	}
	public: static expression_t make_literal_void(){
		return make_literal(value_t::make_void());
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

	public: static expression_t make_arithmetic(expression_type op, const expression_t& lhs, const expression_t& rhs, const std::shared_ptr<typeid_t>& annotated_type){
		QUARK_ASSERT(is_arithmetic_expression(op));

		return expression_t({ arithmetic_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, annotated_type);
	}


	////////////////////////////////		comparison_t

	struct comparison_t {
		expression_type op;
		std::shared_ptr<expression_t> lhs;
		std::shared_ptr<expression_t> rhs;
	};

	public: static expression_t make_comparison(expression_type op, const expression_t& lhs, const expression_t& rhs, const std::shared_ptr<typeid_t>& annotated_type){
		QUARK_ASSERT(is_comparison_expression(op));

		return expression_t({ comparison_t{ op, std::make_shared<expression_t>(lhs), std::make_shared<expression_t>(rhs) } }, annotated_type);
	}


	////////////////////////////////		unary_minus_t

	struct unary_minus_t {
		std::shared_ptr<expression_t> expr;
	};

	public: static expression_t make_unary_minus(const expression_t expr, const std::shared_ptr<typeid_t>& annotated_type){
		return expression_t({ unary_minus_t{ std::make_shared<expression_t>(expr) } }, annotated_type);
	}


	////////////////////////////////		conditional_t

	struct conditional_t {
		std::shared_ptr<expression_t> condition;
		std::shared_ptr<expression_t> a;
		std::shared_ptr<expression_t> b;
	};

	public: static expression_t make_conditional_operator(const expression_t& condition, const expression_t& a, const expression_t& b, const std::shared_ptr<typeid_t>& annotated_type){
		return expression_t(
			{ conditional_t{ std::make_shared<expression_t>(condition), std::make_shared<expression_t>(a), std::make_shared<expression_t>(b) } },
			annotated_type
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
		const std::shared_ptr<typeid_t>& annotated_type
	)
	{
		return expression_t({ call_t{ std::make_shared<expression_t>(callee), args } }, annotated_type);
	}


	////////////////////////////////		struct_definition_expr_t

	struct struct_definition_expr_t {
		std::shared_ptr<const struct_definition_t> def;
	};

	public: static expression_t make_struct_definition(const std::shared_ptr<const struct_definition_t>& def){
		return expression_t({ struct_definition_expr_t{ def } }, std::make_shared<typeid_t>(typeid_t::make_struct1(def)));
	}


	////////////////////////////////		function_definition_expr_t

	struct function_definition_expr_t {
		std::shared_ptr<const function_definition_t> def;
	};

	public: static expression_t make_function_definition(const std::shared_ptr<const function_definition_t>& def){
		return expression_t({ function_definition_expr_t{ def } }, std::make_shared<typeid_t>(def->_function_type));
	}


	////////////////////////////////		load_t

	struct load_t {
		std::string variable_name;
	};

	/*
		Specify free variables.
		It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
	*/
	public: static expression_t make_load(const std::string& variable, const std::shared_ptr<typeid_t>& annotated_type){
		return expression_t({ load_t{ variable } }, annotated_type);
	}


	////////////////////////////////		load2_t

	struct load2_t {
		variable_address_t address;
	};

	public: static expression_t make_load2(const variable_address_t& address, const std::shared_ptr<typeid_t>& annotated_type){
		return expression_t({ load2_t{ address } }, annotated_type);
	}


	////////////////////////////////		resolve_member_t

	struct resolve_member_t {
		std::shared_ptr<expression_t> parent_address;
		std::string member_name;
	};

	public: static expression_t make_resolve_member(
		const expression_t& parent_address,
		const std::string& member_name,
		const std::shared_ptr<typeid_t>& annotated_type
	){
		return expression_t({ resolve_member_t{ std::make_shared<expression_t>(parent_address), member_name } }, annotated_type);
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
		const std::shared_ptr<typeid_t>& annotated_type
	){
		return expression_t({ update_member_t{ std::make_shared<expression_t>(parent_address), member_index, std::make_shared<expression_t>(new_value) } }, annotated_type);
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
		const std::shared_ptr<typeid_t>& annotated_type
	)
	{
		return expression_t({ lookup_t{ std::make_shared<expression_t>(parent_address), std::make_shared<expression_t>(lookup_key) } }, annotated_type);
	}



	////////////////////////////////		value_constructor_t

	struct value_constructor_t {
		typeid_t value_type;
		std::vector<expression_t> elements;
	};

	public: static expression_t make_construct_value_expr(
		const typeid_t& value_type,
		const std::vector<expression_t>& args
	)
	{
		return expression_t({ value_constructor_t{ value_type, args } }, std::make_shared<typeid_t>(value_type));
	}


	////////////////////////////////		benchmark_expr_t

	struct benchmark_expr_t {
		std::shared_ptr<body_t> body;
	};

	public: static expression_t make_benchmark_expr(
		const body_t& body
	)
	{
		return expression_t({ benchmark_expr_t{ std::make_shared<body_t>(body) } }, std::make_shared<typeid_t>(typeid_t::make_int()));
	}


	////////////////////////////////			expression_t-stuff



	public: bool check_invariant() const{
		//	QUARK_ASSERT(_debug.size() > 0);
		//	QUARK_ASSERT(_result_type._base_type != base_type::k_undefined && _result_type.check_invariant());
		QUARK_ASSERT(_output_type == nullptr || _output_type->check_invariant());
		return true;
	}

	public: bool operator==(const expression_t& other) const{
		QUARK_ASSERT(false);
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return true
#if DEBUG
			&& _debug == other._debug
#endif
			&& location == other.location
//				&& _expression_variant == other._expression_variant
			&& compare_shared_values(_output_type, other._output_type)
			;
	}

	public: typeid_t get_output_type() const {
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(_output_type != nullptr);

		return *_output_type;
	}

	public: bool is_annotated_shallow() const{
		QUARK_ASSERT(check_invariant());

		return _output_type != nullptr;
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
		corecall_t,
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

	private: expression_t(const expression_variant_t& contents, const std::shared_ptr<typeid_t>& output_type);


	//////////////////////////		STATE
#if DEBUG
	public: std::string _debug;
#endif
	public: location_t location;
	public: expression_variant_t _expression_variant;
	public: std::shared_ptr<typeid_t> _output_type;
};


json_t expression_to_json(const expression_t& e);
expression_t ast_json_to_expression(const json_t& e);

std::string expression_to_json_string(const expression_t& e);

expression_type get_expression_type(const expression_t& e);


}	//	floyd


#endif /* expressions_hpp */
