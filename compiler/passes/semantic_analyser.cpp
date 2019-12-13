//
//  parse_tree_to_ast_conv.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "semantic_analyser.h"

#include "statement.h"
#include "ast_value.h"
#include "ast_helpers.h"
#include "utils.h"
#include "json_support.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "floyd_syntax.h"
#include "collect_used_types.h"
#include "semantic_ast.h"


namespace floyd {

static const bool k_trace_io = false;




//////////////////////////////////////		analyzer_imm_t

//	Immutable data used by analyser.

struct analyzer_imm_t {
	unchecked_ast_t _ast;

	intrinsic_signatures_t intrinsic_signatures;
};



//////////////////////////////////////		lexical_scope_t

/*
	Value object (MUTABLE!).
	Represents a node in the lexical scope tree.
*/

struct lexical_scope_t {
	symbol_table_t symbols;
	epure pure;
};



//////////////////////////////////////		analyser_t


struct analyser_t {
	public: analyser_t(const unchecked_ast_t& ast);
#if DEBUG
	public: bool check_invariant() const;
#endif


	////////////////////////		STATE

	public: std::shared_ptr<const analyzer_imm_t> _imm;

	//	Non-constant. Last scope is the current one. First scope is the root.
	//	This is ONE branch through the three of lexical scopes of the program.
	public: std::vector<lexical_scope_t> _lexical_scope_stack;

	//	These are output functions, that have been fixed.
	public: std::map<module_symbol_t, function_definition_t> _function_defs;
	public: types_t _types;

	public: software_system_t _software_system;
	public: container_t _container_def;

	public: std::vector<expression_t> benchmark_defs;
	public: std::vector<expression_t> test_defs;

	public: int scope_id_generator;
};




//////////////////////////////////////		forward


//semantic_ast_t analyse(const analyser_t& a);
static std::pair<analyser_t, std::shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement, const type_t& return_type);
static std::pair<analyser_t, std::vector<std::shared_ptr<statement_t>> > analyse_statements(const analyser_t& a, const std::vector<std::shared_ptr<statement_t>>& statements, const type_t& return_type);
static std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const expression_t& e, const type_t& target_type);
static std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const expression_t& e);

static std::pair<const symbol_t*, symbol_pos_t> find_symbol_by_name(const analyser_t& a, const std::string& s);




static void throw_local_identifier_already_exists(const location_t& loc, const std::string& local_identifier){
	std::stringstream what;
	what << "Local identifier \"" << local_identifier << "\" already exists.";
	throw_compiler_error(loc, what.str());
}



static void trace_analyser(const analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	QUARK_SCOPED_TRACE("ANALYSER");
	{
		QUARK_SCOPED_TRACE("CURRENT LEXICAL SCOPE PATH");

		int index = 0;
		for(const auto& scope: a._lexical_scope_stack){
			QUARK_SCOPED_TRACE(std::to_string(index));

			if(scope.pure == epure::pure){
				QUARK_TRACE("scope pure: PURE");
			}
			else{
				QUARK_TRACE("scope pure: IMPURE");
			}

			for(int i = 0 ; i < scope.symbols._symbols.size() ; i++){
				const auto& symbol = scope.symbols._symbols[i];
				QUARK_TRACE_SS(i << "\t " << symbol.first << ":\t" << symbol_to_string(a._types, symbol.second));
			}

			index++;
		}
	}
	{
		trace_types(a._types);
	}
}



/////////////////////////////////////////			FIND SYMBOL USING LEXICAL SCOPE PATH


static type_t get_symbol_named_type(const types_t& types, const symbol_t& symbol){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(symbol._symbol_type == symbol_t::symbol_type::named_type);

	const auto a = symbol._value_type;
	const auto b = refresh_type(types, a);
	return b;
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
static std::pair<const symbol_t*, symbol_pos_t> find_symbol_deep(const analyser_t& a, int depth, const std::string& s){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < a._lexical_scope_stack.size());
	QUARK_ASSERT(s.size() > 0);

    const auto it = std::find_if(
    	a._lexical_scope_stack[depth].symbols._symbols.begin(),
    	a._lexical_scope_stack[depth].symbols._symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);

	if(it != a._lexical_scope_stack[depth].symbols._symbols.end()){
		const auto parent_index = depth == 0 ? -1 : (int)(a._lexical_scope_stack.size() - depth - 1);
		const auto variable_index = (int)(it - a._lexical_scope_stack[depth].symbols._symbols.begin());
		return { &it->second, symbol_pos_t::make_stack_pos(parent_index, variable_index) };
	}
	else if(depth > 0){
		return find_symbol_deep(a, depth - 1, s);
	}
	else{
		return { nullptr, symbol_pos_t::make_stack_pos(0, 0) };
	}
}
//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
static std::pair<const symbol_t*, symbol_pos_t> find_symbol_by_name(const analyser_t& a, const std::string& s){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return find_symbol_deep(a, static_cast<int>(a._lexical_scope_stack.size() - 1), s);
}

static bool does_symbol_exist_shallow(const analyser_t& a, const std::string& s){
    const auto it = std::find_if(
    	a._lexical_scope_stack.back().symbols._symbols.begin(),
    	a._lexical_scope_stack.back().symbols._symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);
	return it != a._lexical_scope_stack.back().symbols._symbols.end();
}


static type_t resolve_type_symbols_internal(analyser_t& acc, const location_t& loc, const type_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		analyser_t& acc;
		const location_t& loc;
		const type_t& type;


		type_t operator()(const undefined_t& e) const{
			return type;
		}
		type_t operator()(const any_t& e) const{
			return type;
		}

		type_t operator()(const void_t& e) const{
			return type;
		}
		type_t operator()(const bool_t& e) const{
			return type;
		}
		type_t operator()(const int_t& e) const{
			return type;
		}
		type_t operator()(const double_t& e) const{
			return type;
		}
		type_t operator()(const string_t& e) const{
			return type;
		}

		type_t operator()(const json_type_t& e) const{
			return type;
		}
		type_t operator()(const typeid_type_t& e) const{
			return type;
		}

		type_t operator()(const struct_t& e) const{
			std::vector<member_t> members2;
			for(const auto& m: e.desc._members){
				members2.push_back(member_t { resolve_type_symbols_internal(acc, loc, m._type), m._name } );
			}
			return make_struct(acc._types, struct_type_desc_t(members2));
		}
		type_t operator()(const vector_t& e) const{
			return make_vector(acc._types, resolve_type_symbols_internal(acc, loc, type.get_vector_element_type(acc._types)));
		}
		type_t operator()(const dict_t& e) const{
			return make_dict(acc._types, resolve_type_symbols_internal(acc, loc, type.get_dict_value_type(acc._types)));
		}
		type_t operator()(const function_t& e) const{
			const auto ret = type.get_function_return(acc._types);
			const auto args = type.get_function_args(acc._types);
			const auto pure = type.get_function_pure(acc._types);
			const auto dyn_return_type = type.get_function_dyn_return_type(acc._types);

			const auto ret2 = resolve_type_symbols_internal(acc, loc, ret);
			std::vector<type_t> args2;
			for(const auto& m: args){
				args2.push_back(resolve_type_symbols_internal(acc, loc, m));
			}
			return make_function3(acc._types, ret2, args2, pure, dyn_return_type);
		}
		type_t operator()(const symbol_ref_t& e) const{
			QUARK_ASSERT(e.s != "");

#if DEBUG
			if(false) trace_analyser(acc);
#endif
			const auto existing_value_deep_ptr = find_symbol_by_name(acc, e.s);
			if(existing_value_deep_ptr.first == nullptr || existing_value_deep_ptr.first->_symbol_type != symbol_t::symbol_type::named_type){
				throw_compiler_error(loc, "Unknown type name '" + e.s + "'.");
			}
			const auto result = get_symbol_named_type(acc._types, *existing_value_deep_ptr.first);
#if DEBUG
			if(false) trace_analyser(acc);
#endif
			return result;
		}
		type_t operator()(const named_type_t& e) const{
			return type;
		}
	};
	const auto result = std::visit(visitor_t{ acc, loc, type }, get_type_variant(acc._types, type));
	return result;
}

//	Scan type deeply and resolve each symbol against the symbol table.
static type_t resolve_type_symbols(analyser_t& acc, const location_t& loc, const type_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	try {
#if DEBUG
		if(false) trace_analyser(acc);
#endif

		const auto resolved = resolve_type_symbols_internal(acc, loc, type);

#if DEBUG
		if(false) trace_analyser(acc);
#endif
		return resolved;
	}
	catch(const compiler_error& e){
		throw;
	}
	catch(const std::exception& e){
		throw_compiler_error(loc, e.what());
	}
}

static type_t analyze_expr_output_type(analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	return resolve_type_symbols(a, k_no_location, e.get_output_type());
}


//	When callee has "any" as return type, we need to figure out its return type using its algorithm and the actual types.
static const type_t figure_out_callee_return_type(analyser_t& a, const statement_t& parent, const type_t& callee_type, const std::vector<expression_t>& call_args){

	const auto callee_type_desc = dereference_type(a._types, callee_type);
	const auto callee_return_type = callee_type_desc.get_function_return(a._types);

	const auto ret_type_algo = callee_type_desc.get_function_dyn_return_type(a._types);
	switch(ret_type_algo){
		case return_dyn_type::none:
			{
				QUARK_ASSERT(dereference_type(a._types, callee_return_type).is_any() == false);
				return callee_return_type;
			}
			break;

		case return_dyn_type::arg0:
			{
				QUARK_ASSERT(call_args.size() > 0);

				return analyze_expr_output_type(a, call_args[0]);
			}
			break;
		case return_dyn_type::arg1:
			{
				QUARK_ASSERT(call_args.size() >= 2);

				return analyze_expr_output_type(a, call_args[1]);
			}
			break;
		case return_dyn_type::arg1_typeid_constant_type:
			{
				QUARK_ASSERT(call_args.size() >= 2);

				//???named-type, unify duplicated code for looking up variable addr in symbol tables
				const auto e = call_args[1];
				if(std::holds_alternative<expression_t::load2_t>(e._expression_variant) == false){
					std::stringstream what;
					what << "Argument 2 must be a typeid literal.";
					throw_compiler_error(parent.location, what.str());
				}
				const auto& load = std::get<expression_t::load2_t>(e._expression_variant);
				QUARK_ASSERT(load.address._parent_steps == -1);

				const auto& addr = load.address; 
				const size_t scope_index = addr._parent_steps == symbol_pos_t::k_global_scope ? 0 : a._lexical_scope_stack.size() - 1 - addr._parent_steps;
				const auto& symbol_kv = a._lexical_scope_stack[scope_index].symbols._symbols[addr._index];

				QUARK_ASSERT(symbol_kv.second._symbol_type == symbol_t::symbol_type::named_type);
				return get_symbol_named_type(a._types, symbol_kv.second);
			}
			break;
		case return_dyn_type::vector_of_arg1func_return:
			{
				//	x = make_vector(arg1.get_function_return());
				QUARK_ASSERT(call_args.size() >= 2);

				const auto arg1_type = analyze_expr_output_type(a, call_args[1]);
				const auto element_type = dereference_type(a._types, arg1_type).get_function_return(a._types);
				const auto ret = make_vector(a._types, element_type);
				return ret;
			}
			break;
		case return_dyn_type::vector_of_arg2func_return:
			{
				//	x = make_vector(arg2.get_function_return());
				QUARK_ASSERT(call_args.size() >= 3);

				const auto arg1_type = analyze_expr_output_type(a, call_args[2]);
				const auto element_type = dereference_type(a._types, arg1_type).get_function_return(a._types);
				const auto ret = make_vector(a._types, element_type);
				return ret;
			}
			break;


		default:
			QUARK_ASSERT(false);
			break;
	}
}

struct fully_resolved_call_t {
	std::vector<expression_t> args;
	type_t function_type;
};

/*
	Generates code for computing arguments and figuring out each argument type.
	Checks the call vs the callee_type.
	It matches up if the callee uses ANY for argument(s) or return type.
	Returns the computed call arguments and the final, resolved function type which also matches the arguments.

	Throws errors on type mismatches.
*/
static std::pair<analyser_t, fully_resolved_call_t> analyze_resolve_call_type(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& call_args, const type_t& callee_type){
	auto a_acc = a;

	const auto callee_type_peek = dereference_type(a_acc._types, callee_type);
	const auto callee_arg_types = callee_type_peek.get_function_args(a_acc._types);

	//	arity
	if(call_args.size() != callee_arg_types.size()){
		std::stringstream what;
		what << "Wrong number of arguments in function call, got " << std::to_string(call_args.size()) << ", expected " << std::to_string(callee_arg_types.size()) << ".";
		throw_compiler_error(parent.location, what.str());
	}

	std::vector<expression_t> call_args2;
	for(int i = 0 ; i < callee_arg_types.size() ; i++){
		const auto call_arg_pair = analyse_expression_to_target(a_acc, parent, call_args[i], callee_arg_types[i]);
		a_acc = call_arg_pair.first;
		call_args2.push_back(call_arg_pair.second);
	}

	const auto callee_return_type = figure_out_callee_return_type(a_acc, parent, callee_type_peek, call_args2);


	std::vector<type_t> resolved_arg_types;
	for(const auto& e: call_args2){
		resolved_arg_types.push_back(analyze_expr_output_type(a_acc, e));
	}
	const auto resolved_function_type = make_function(
		a_acc._types,
		callee_return_type,
		resolved_arg_types,
		callee_type_peek.get_function_pure(a_acc._types)
	);

	if(false) trace_types(a_acc._types);

	return { a_acc, { call_args2, resolved_function_type } };
}





/////////////////////////////////////////			STATEMENTS



static std::pair<analyser_t, std::vector<statement_t>> analyse_statements(const analyser_t& a, const std::vector<statement_t>& statements, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());
	for(const auto& i: statements){ QUARK_ASSERT(i.check_invariant()); };

	auto a_acc = a;

	std::vector<statement_t> statements2;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		const auto& r = analyse_statement(a_acc, statement, return_type);
		a_acc = r.first;

		if(r.second){
			QUARK_ASSERT(floyd::check_types_resolved(a_acc._types, *r.second));
			statements2.push_back(*r.second);
		}
		statement_index++;
	}
	return { a_acc, statements2 };
}

std::pair<analyser_t, body_t> analyse_body(const analyser_t& a, const body_t& body, epure pure, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	auto new_environment = symbol_table_t{ body._symbol_table };
	const auto lexical_scope = lexical_scope_t{ new_environment, pure };
	a_acc._lexical_scope_stack.push_back(lexical_scope);

	const auto result = analyse_statements(a_acc, body._statements, return_type);
	a_acc = result.first;

	const auto body2 = body_t(result.second, result.first._lexical_scope_stack.back().symbols);

	a_acc._lexical_scope_stack.pop_back();
	return { a_acc, body2 };
}

/*
	Can update an existing local (if local is mutable).
*/
std::pair<analyser_t, statement_t> analyse_assign_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::assign_t>(s._contents);
	const auto local_name = statement._local_name;
	const auto existing_value_deep_ptr = find_symbol_by_name(a_acc, local_name);

	//	Attempt to mutate existing value!
	if(existing_value_deep_ptr.first != nullptr){
		bool mut = is_mutable(*existing_value_deep_ptr.first);
		if(mut == false){
			throw_compiler_error(s.location, "Cannot assign to immutable identifier \"" + local_name + "\".");
		}
		else{
			const auto lhs_type = existing_value_deep_ptr.first->get_value_type();
			const auto rhs_expr2 = analyse_expression_to_target(a_acc, s, statement._expression, lhs_type);
			a_acc = rhs_expr2.first;
			const auto rhs_expr3 = rhs_expr2.second;

			if(lhs_type != analyze_expr_output_type(a_acc, rhs_expr3)){
				std::stringstream what;
				what << "Types not compatible in assignment - cannot convert '"
				<< type_to_compact_string(a_acc._types, analyze_expr_output_type(a_acc, rhs_expr3))
				<< "' to '"
				<< type_to_compact_string(a_acc._types, lhs_type) << ".";
				throw_compiler_error(s.location, what.str());
			}
			else{
				return { a_acc, statement_t::make__assign2(s.location, existing_value_deep_ptr.second, rhs_expr3) };
			}
		}
	}

	//	Unknown identifier in lexical scope path.
	else{
		std::stringstream what;
		what << "Unknown identifier '" << local_name << "'.";
		throw_compiler_error(s.location, what.str());
	}
}

/*
Assign with target type info. Always creates a new local.

Ex:
	let int a = 10
	mutable a = 10
	mutable = 10
*/
std::pair<analyser_t, std::shared_ptr<statement_t>> analyse_bind_local_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::bind_local_t>(s._contents);

	const auto new_local_name = statement._new_local_name;

#if DEBUG
	if(false) trace_analyser(a_acc);
#endif

	//	If lhs may be
	//		(1) undefined, if input is "let a = 10" for example. Then we need to infer its type.
	//		(2) have a type, but it might not be fully resolved yet.
	const auto lhs_itype = resolve_type_symbols(a_acc, s.location, statement._bindtype);

	const auto mutable_flag = statement._locals_mutable_mode == statement_t::bind_local_t::k_mutable;

	const auto value_exists_in_env = does_symbol_exist_shallow(a_acc, new_local_name);
	if(value_exists_in_env){
		throw_local_identifier_already_exists(s.location, new_local_name);
	}

	//	Setup temporary symbol so function definition can find itself = recursive.
	//	Notice: the final type may not be correct yet, but for function definitions it is.
	//	This logic should be available for inferred binds too, in analyse_assign_statement().

	const auto temp_symbol = mutable_flag ? symbol_t::make_mutable(lhs_itype) : symbol_t::make_immutable_reserve(lhs_itype);
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({ new_local_name, temp_symbol });
	const auto local_name_index = a_acc._lexical_scope_stack.back().symbols._symbols.size() - 1;

	try {
		const auto rhs_expr_pair = lhs_itype.is_undefined()
			? analyse_expression_no_target(a_acc, s, statement._expression)
			: analyse_expression_to_target(a_acc, s, statement._expression, lhs_itype);
		a_acc = rhs_expr_pair.first;

		const auto rhs_itype = analyze_expr_output_type(a_acc, rhs_expr_pair.second);
		const auto lhs_itype2 = lhs_itype.is_undefined() ? rhs_itype : lhs_itype;

		//??? make test that checks I got this test + error right.
		if((lhs_itype2 == rhs_itype) == false){
			std::stringstream what;
			what << "Types not compatible in bind - cannot convert '"
			<< type_to_compact_string(a_acc._types, lhs_itype) << "' to '" << type_to_compact_string(a_acc._types, lhs_itype2) << ".";
			throw_compiler_error(s.location, what.str());
		}
		else{
			//	Replace the temporary symbol with the complete symbol.

			//	If symbol can be initialized directly, use make_immutable_precalc(). Else reserve it and create an init2-statement to set it up at runtime.
			//	??? Better to always initialise it, even if it's a complex value. Codegen then decides if to translate to a reserve + init. BUT PROBLEM: we lose info *when* to init the value.
			if(is_preinitliteral(dereference_type(a_acc._types, lhs_itype2)) && mutable_flag == false && get_expression_type(rhs_expr_pair.second) == expression_type::k_literal){
				const auto symbol2 = symbol_t::make_immutable_precalc(lhs_itype2, rhs_expr_pair.second.get_literal());
				a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = { new_local_name, symbol2 };
				analyze_expr_output_type(a_acc, rhs_expr_pair.second);
				return { a_acc, {} };
			}
			else{
				const auto symbol2 = mutable_flag ? symbol_t::make_mutable(lhs_itype2) : symbol_t::make_immutable_reserve(lhs_itype2);
				a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = { new_local_name, symbol2 };
				analyze_expr_output_type(a_acc, rhs_expr_pair.second);

				return {
					a_acc,
					std::make_shared<statement_t>(
						statement_t::make__init2(
							s.location,
							symbol_pos_t::make_stack_pos(0, (int)local_name_index),
							rhs_expr_pair.second
						)
					)
				};
			}
		}
	}
	catch(...){

		//	Erase temporary symbol.
		a_acc._lexical_scope_stack.back().symbols._symbols.pop_back();

		throw;
	}
}

std::pair<analyser_t, statement_t> analyse_block_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::block_statement_t>(s._contents);
	const auto e = analyse_body(a, statement._body, a._lexical_scope_stack.back().pure, return_type);
	return {e.first, statement_t::make__block_statement(s.location, e.second)};
}

std::pair<analyser_t, statement_t> analyse_return_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	if(dereference_type(a_acc._types, return_type).is_void()){
		std::stringstream what;
		what << "Cannot return value from function with void-return.";
		throw_compiler_error(s.location, what.str());
	}

	const auto statement = std::get<statement_t::return_statement_t>(s._contents);
	const auto expr = statement._expression;
	const auto result = analyse_expression_to_target(a_acc, s, expr, return_type);

	a_acc = result.first;

	const auto result_value = result.second;

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return { a_acc, statement_t::make__return_statement(s.location, result.second) };
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::ifelse_statement_t>(s._contents);

	const auto condition2 = analyse_expression_no_target(a_acc, s, statement._condition);
	a_acc = condition2.first;

	const auto condition_type = analyze_expr_output_type(a_acc, condition2.second);
	if(dereference_type(a_acc._types, condition_type).is_bool() == false){
		std::stringstream what;
		what << "Boolean condition required.";
		throw_compiler_error(s.location, what.str());
	}

	const auto then2 = analyse_body(a_acc, statement._then_body, a._lexical_scope_stack.back().pure, return_type);
	a_acc = then2.first;

	const auto else2 = analyse_body(a_acc, statement._else_body, a._lexical_scope_stack.back().pure, return_type);
	a_acc = else2.first;

	return { a_acc, statement_t::make__ifelse_statement(s.location, condition2.second, then2.second, else2.second) };
}

std::pair<analyser_t, statement_t> analyse_for_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::for_statement_t>(s._contents);

	const auto start_expr2 = analyse_expression_no_target(a_acc, s, statement._start_expression);
	a_acc = start_expr2.first;

	const auto end_expr2 = analyse_expression_no_target(a_acc, s, statement._end_expression);
	a_acc = end_expr2.first;

	const auto start_type = analyze_expr_output_type(a_acc, start_expr2.second);
	const auto end_type = analyze_expr_output_type(a_acc, end_expr2.second);

	if(dereference_type(a_acc._types, start_type).is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, start type is " <<  type_to_compact_string(a_acc._types, start_type) << ".";
		throw_compiler_error(s.location, what.str());
	}

	if(dereference_type(a_acc._types, end_type).is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, end type is " <<  type_to_compact_string(a_acc._types, end_type) << ".";
		throw_compiler_error(s.location, what.str());
	}

	const auto iterator_symbol = symbol_t::make_immutable_reserve(type_t::make_int());

	//	Add the iterator as a symbol to the body of the for-loop.
	auto symbols = statement._body._symbol_table;
	symbols._symbols.push_back({ statement._iterator_name, iterator_symbol});
	const auto body_injected = body_t(statement._body._statements, symbols);
	const auto result = analyse_body(a_acc, body_injected, a._lexical_scope_stack.back().pure, return_type);
	a_acc = result.first;

	return { a_acc, statement_t::make__for_statement(s.location, statement._iterator_name, start_expr2.second, end_expr2.second, result.second, statement._range_type) };
}

std::pair<analyser_t, statement_t> analyse_while_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::while_statement_t>(s._contents);

	const auto condition2_expr = analyse_expression_to_target(a_acc, s, statement._condition, type_t::make_bool());
	a_acc = condition2_expr.first;

	const auto result = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure, return_type);
	a_acc = result.first;

	return { a_acc, statement_t::make__while_statement(s.location, condition2_expr.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_expression_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::expression_statement_t>(s._contents);
	const auto expr2 = analyse_expression_no_target(a_acc, s, statement._expression);
	a_acc = expr2.first;

	return { a_acc, statement_t::make__expression_statement(s.location, expr2.second) };
}

//	Make new global function containing the body of the benchmark-def.
//	Add the function as an entry in the global benchmark registry.
static analyser_t analyse_benchmark_def_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::benchmark_def_statement_t>(s._contents);

	const auto test_name = statement.name;
	const auto function_link_name = "benchmark__" + test_name;

	const auto benchmark_def_type = resolve_type_symbols(a_acc, k_no_location, make_symbol_ref(a_acc._types, "benchmark_def_t"));
	const auto f_type = resolve_type_symbols(a_acc, k_no_location, make_benchmark_function_t(a_acc._types));


	const auto function_id = module_symbol_t(function_link_name);

	//	Make a function def expression for the new benchmark function.

	const auto body_pair = analyse_body(a_acc, statement._body, epure::pure, dereference_type(a_acc._types, f_type).get_function_return(a_acc._types));
	a_acc = body_pair.first;


	const auto function_def2 = function_definition_t::make_func(
		k_no_location,
		function_link_name,
		dereference_type(a_acc._types, f_type),
		{},
		std::make_shared<body_t>(body_pair.second)
	);
	QUARK_ASSERT(check_types_resolved(a_acc._types, function_def2));

	a_acc._function_defs.insert({ function_id, function_definition_t(function_def2) });

	const auto f = value_t::make_function_value(a_acc._types, f_type, function_id);


	//	Add benchmark-def record to benchmark_defs.
	{
		const auto new_record_expr = expression_t::make_construct_value_expr(
			benchmark_def_type,
			{
				expression_t::make_literal_string(test_name),
				expression_t::make_literal(f)
			}
		);
		const auto new_record_expr3_pair = analyse_expression_to_target(a_acc, s, new_record_expr, benchmark_def_type);
		a_acc = new_record_expr3_pair.first;
		a_acc.benchmark_defs.push_back(new_record_expr3_pair.second);
	}

	const auto body2 = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure, dereference_type(a_acc._types, f_type).get_function_return(a_acc._types));
	a_acc = body2.first;
	return a_acc;
}

//??? Change this to find the symbol instead of using make_test_def_t().
static analyser_t analyse_test_def_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::test_def_statement_t>(s._contents);

	const auto test_name = statement.function_name + ":" + statement.scenario;
	const auto function_link_name = "test__" + test_name;

	const auto test_def_itype = resolve_type_symbols(a_acc, k_no_location, make_symbol_ref(a_acc._types, "test_def_t"));
	const auto f_itype = resolve_type_symbols(a_acc, k_no_location, make_test_function_t(a_acc._types));


	const auto function_id = module_symbol_t(function_link_name);

	//	Make a function def expression for the new test function.

	const auto body_pair = analyse_body(a_acc, statement._body, epure::pure, dereference_type(a_acc._types, f_itype).get_function_return(a_acc._types));
	a_acc = body_pair.first;


	const auto function_def2 = function_definition_t::make_func(
		k_no_location,
		function_link_name,
		dereference_type(a_acc._types, f_itype),
		{},
		std::make_shared<body_t>(body_pair.second)
	);
	QUARK_ASSERT(check_types_resolved(a_acc._types, function_def2));

	a_acc._function_defs.insert({ function_id, function_definition_t(function_def2) });

	const auto f = value_t::make_function_value(a_acc._types, f_itype, function_id);


	//	Add test-def record to test_defs.
	{
		const auto new_record_expr = expression_t::make_construct_value_expr(
			test_def_itype,
			{
				expression_t::make_literal_string(statement.function_name),
				expression_t::make_literal_string(statement.scenario),
				expression_t::make_literal(f)
			}
		);
		const auto new_record_expr3_pair = analyse_expression_to_target(a_acc, s, new_record_expr, test_def_itype);
		a_acc = new_record_expr3_pair.first;
		a_acc.test_defs.push_back(new_record_expr3_pair.second);
	}

	const auto body2 = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure, dereference_type(a_acc._types, f_itype).get_function_return(a_acc._types));
	a_acc = body2.first;
	return a_acc;
}


static analyser_t analyse_container_def_statement(const analyser_t& a, const statement_t& s, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::container_def_statement_t>(s._contents);
	auto a_acc = a;
	a_acc._container_def = statement._container;

	return a_acc;
}




//	Output is the RETURN VALUE of the analysed statement, if any.
static std::pair<analyser_t, std::shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement, const type_t& return_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	typedef std::pair<analyser_t, std::shared_ptr<statement_t>> return_type_t;

	struct visitor_t {
		const analyser_t& a;
		const statement_t& statement;
		const type_t return_type;


		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::return_statement_t& s) const{
			const auto e = analyse_return_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::bind_local_t& s) const{
			const auto e = analyse_bind_local_statement(a, statement);
			if(e.second){
				QUARK_ASSERT(check_types_resolved(e.first._types, *e.second));
			}
			return { e.first, e.second };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::assign_t& s) const{
			const auto e = analyse_assign_statement(a, statement);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::assign2_t& s) const{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::init2_t& s) const{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::block_statement_t& s) const{
			const auto e = analyse_block_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::ifelse_statement_t& s) const{
			const auto e = analyse_ifelse_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::for_statement_t& s) const{
			const auto e = analyse_for_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::while_statement_t& s) const{
			const auto e = analyse_while_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}


		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::expression_statement_t& s) const{
			const auto e = analyse_expression_statement(a, statement);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::software_system_statement_t& s) const{
			analyser_t temp = a;
			temp._software_system = s._system;
			return { temp, {} };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::container_def_statement_t& s) const{
			const auto e = analyse_container_def_statement(a, statement, return_type);
			return { e, {} };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::benchmark_def_statement_t& s) const{
			const auto e = analyse_benchmark_def_statement(a, statement, return_type);
			return { e, {} };
		}
		std::pair<analyser_t, std::shared_ptr<statement_t>> operator()(const statement_t::test_def_statement_t& s) const{
			const auto e = analyse_test_def_statement(a, statement, return_type);
			return { e, {} };
		}

	};

	return std::visit(visitor_t{ a, statement, return_type }, statement._contents);
}



/////////////////////////////////////////			EXPRESSIONS



std::pair<analyser_t, expression_t> analyse_resolve_member_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::resolve_member_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto parent_expr = analyse_expression_no_target(a_acc, parent, *details.parent_address);
	a_acc = parent_expr.first;

	const auto parent_type0 = analyze_expr_output_type(a_acc, parent_expr.second);
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type0);
	if(parent_type_peek.is_struct()){
		const auto struct_def = parent_type_peek.get_struct(a_acc._types);

		//??? store index in new expression
		int index = find_struct_member_index(struct_def, details.member_name);
		if(index == -1){
			std::stringstream what;
			what << "Unknown struct member \"" + details.member_name + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		const auto member_type = struct_def._members[index]._type;
		return { a_acc, expression_t::make_resolve_member(parent_expr.second, details.member_name, member_type) };
	}
	else{
		std::stringstream what;
		what << "Left hand side is not a struct value, it's of type \"" + type_to_compact_string(a_acc._types, parent_type0) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_intrinsic_update_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const std::vector<expression_t>& call_args){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto sign = make_update_signature(a_acc._types);

	//	IMPORTANT: For structs we manipulate the key-expression. We can't run normal analyse on key-expression - it's encoded as a variable resolve.
	//	Explore arg0: this is the collection type. We need to detect if it's a struct quickly.
	const auto collection_expr_kv = analyse_expression_no_target(a_acc, parent, call_args[0]);
	a_acc = collection_expr_kv.first;

	const auto collection_expr = collection_expr_kv.second;
	const auto collection_type0 = analyze_expr_output_type(a_acc, collection_expr);
	const auto collection_type_peek = dereference_type(a_acc._types, collection_type0);

	if(collection_type_peek.is_struct()){
		const auto& struct_def = collection_type_peek.get_struct(a_acc._types);

		const auto callee_itype = sign._function_type;

		const auto callee_type_peek = dereference_type(a_acc._types, callee_itype);
		const auto callee_arg_types = callee_type_peek.get_function_args(a_acc._types);


		const auto new_value_expr_pair = analyse_expression_to_target(a_acc, parent, call_args[2], callee_arg_types[2]);
		a_acc = new_value_expr_pair.first;
		const auto new_value_type = analyze_expr_output_type(a_acc, new_value_expr_pair.second);

		const auto new_value_expr = new_value_expr_pair.second;

		//	The key needs to be the name of an symbol. It's a compile-time constant.
		//	It's encoded as a load which is confusing.
		//	??? Idea: convert the symbol to an integer and call analyse_intrinsic_update_expression() again. Also support accessing struct members by index.


		//	arity
		if(call_args.size() != callee_arg_types.size()){
			std::stringstream what;
			what << "Wrong number of arguments in function call, got " << std::to_string(call_args.size()) << ", expected " << std::to_string(callee_arg_types.size()) << ".";
			throw_compiler_error(parent.location, what.str());
		}

		if(get_expression_type(call_args[1]) == expression_type::k_load){
			const auto member_name = std::get<expression_t::load_t>(call_args[1]._expression_variant).variable_name;
			int member_index = find_struct_member_index(struct_def, member_name);
			if(member_index == -1){
				std::stringstream what;
				what << "Unknown struct member \"" + member_name + "\".";
				throw_compiler_error(parent.location, what.str());
			}
			const auto member_type = struct_def._members[member_index]._type;
			if(new_value_type != member_type){
				throw_compiler_error(parent.location, "New value's type does not match struct member's type.");
			}


			const auto callee_return_type = collection_type0;

			const auto resolved_arg_types = { collection_type0, type_t::make_int(), new_value_type };


			//	Force generating the function-type into types.
			const std::vector<floyd::type_t> resolved_type_vec(resolved_arg_types.begin(), resolved_arg_types.end());
			const auto resolved_function_type = make_function(
				a_acc._types,
				callee_return_type,
				resolved_type_vec,
				callee_type_peek.get_function_pure(a_acc._types)
			);
			(void)resolved_function_type;


			if(false) trace_types(a_acc._types);

			return {
				a_acc,
				expression_t::make_update_member(collection_expr, member_index, new_value_expr, collection_type0)
			};
		}
		else{
			std::stringstream what;
			what << "Struct member needs to be a string literal.";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else {
		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, call_args, sign._function_type);
		a_acc = resolved_call.first;

		const auto key_expr = resolved_call.second.args[1];
		const auto key_type = key_expr.get_output_type();
		const auto key_peek = dereference_type(a_acc._types, key_type);

		const auto new_value_expr = resolved_call.second.args[2];

		const auto new_value_type = analyze_expr_output_type(a_acc, new_value_expr);

		if(collection_type_peek.is_string()){
			if(key_peek.is_int() == false){
				std::stringstream what;
				what << "Updating string needs an integer index, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
				throw_compiler_error(parent.location, what.str());
			}

			if(dereference_type(a_acc._types, new_value_type).is_int() == false){
				std::stringstream what;
				what << "Updating string needs an integer value, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
				throw_compiler_error(parent.location, what.str());
			}

			return {
				a_acc,
				expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr, key_expr, new_value_expr }, collection_type0)
			};
		}
		else if(collection_type_peek.is_vector()){
			if(key_peek.is_int() == false){
				std::stringstream what;
				what << "Updating vector needs and integer index, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
				throw_compiler_error(parent.location, what.str());
			}

			const auto element_type = collection_type_peek.get_vector_element_type(a_acc._types);
			if(element_type != new_value_type){
				throw_compiler_error(parent.location, "New value's type must match vector's element type.");
			}

			return {
				a_acc,
				expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr, key_expr, new_value_expr }, collection_type0)
			};
		}
		else if(collection_type_peek.is_dict()){
			if(key_peek.is_string() == false){
				std::stringstream what;
				what << "Updating dictionary requires string key, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
				throw_compiler_error(parent.location, what.str());
			}

			const auto element_type = collection_type_peek.get_dict_value_type(a_acc._types);
			if(element_type != new_value_type){
				throw_compiler_error(parent.location, "New value's type must match dict's value type.");
			}

			return {
				a_acc,
				expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr, key_expr, new_value_expr }, collection_type0)
			};
		}

		else{
			std::stringstream what;
			what << "Left hand side does not support update() - it's of type \"" + type_to_compact_string(a_acc._types, collection_type0) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
}

std::pair<analyser_t, expression_t> analyse_intrinsic_push_back_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto sign = make_push_back_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto value_type = function_type.get_function_args(a_acc._types)[1];

	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type_peek.is_string()){
		if(dereference_type(a_acc._types, value_type).is_int() == false){
			std::stringstream what;
			what << "string push_back() needs an integer element, not a \"" + type_to_compact_string(a_acc._types, value_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else if(parent_type_peek.is_vector()){
		if(value_type != parent_type_peek.get_vector_element_type(a_acc._types)){
			std::stringstream what;
			what << "Vector push_back() has mismatching element type vs supplies a \"" + type_to_compact_string(a_acc._types, value_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else{
		std::stringstream what;
		what << "Left hand side does not support push_back() - it's of type \"" + type_to_compact_string(a_acc._types, parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, parent_type)
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_size_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_size_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);
	if(parent_type_peek.is_string() || parent_type_peek.is_json() || parent_type_peek.is_vector() || parent_type_peek.is_dict()){
	}
	else{
		std::stringstream what;
		what << "Left hand side does not support size() - it's of type \"" + type_to_compact_string(a_acc._types, parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_find_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_find_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto wanted_type = function_type.get_function_args(a_acc._types)[1];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type_peek.is_string()){
		if(dereference_type(a_acc._types, wanted_type).is_string() == false){
			throw_compiler_error(parent.location, "find() requires argument 2 to be a string.");
		}
	}
	else if(parent_type_peek.is_vector()){
		if(wanted_type != parent_type_peek.get_vector_element_type(a_acc._types)){
			throw_compiler_error(parent.location, "find([]) requires argument 2 to be of vector's element type.");
		}
	}
	else{
		std::stringstream what;
		what << "Function find() doesn not work on type \"" + type_to_compact_string(a_acc._types, parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_exists_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_exists_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto wanted_type = function_type.get_function_args(a_acc._types)[1];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type_peek.is_dict() == false){
		throw_compiler_error(parent.location, "exists() requires a dictionary.");
	}
	if(dereference_type(a_acc._types, wanted_type).is_string() == false){
		throw_compiler_error(parent.location, "exists() requires a string key.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_erase_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_erase_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto key_type = function_type.get_function_args(a_acc._types)[1];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type_peek.is_dict() == false){
		throw_compiler_error(parent.location, "erase() requires a dictionary.");
	}
	if(dereference_type(a_acc._types, key_type).is_string() == false){
		throw_compiler_error(parent.location, "erase() requires a string key.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

//??? const statement_t& parent == very confusing. Rename parent => statement!

std::pair<analyser_t, expression_t> analyse_intrinsic_get_keys_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_get_keys_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];

	if(dereference_type(a_acc._types, parent_type).is_dict() == false){
		throw_compiler_error(parent.location, "get_keys() requires a dictionary.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_subset_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_subset_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
//	const auto key_type = function_type.get_function_args(a_acc._types)[1];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type_peek.is_string() == false && parent_type_peek.is_vector() == false){
		throw_compiler_error(parent.location, "subset([]) requires a string or a vector.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_replace_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_replace_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
	const auto parent_type = function_type.get_function_args(a_acc._types)[0];
	const auto replace_with_type = function_type.get_function_args(a_acc._types)[3];
	const auto parent_type_peek = dereference_type(a_acc._types, parent_type);

	if(parent_type != replace_with_type){
		throw_compiler_error(parent.location, "replace() requires argument 4 to be same type of collection.");
	}

	if(parent_type_peek.is_string() == false && parent_type_peek.is_vector() == false){
		throw_compiler_error(parent.location, "replace([]) requires a string or a vector.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}


//??? pass around location_t instead of statement_t& parent!


//	[R] map([E] elements, func R (E e, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_map_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", needs to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

//	string map_string(string s, func string(string e, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_string_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_map_string_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map_string() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", needs to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_dag_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_map_dag_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map_dag() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", needs to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_filter_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_filter_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to filter() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", expected to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_reduce_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_reduce_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to reduce() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", expected to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_stable_sort_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;
	const auto sign = make_stable_sort_signature(a_acc._types);
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	//??? Fix this signature check!
	const auto expected = resolved_call.second.function_type;
	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to stable_sort() uses signature \"" + type_to_compact_string(a_acc._types, resolved_call.second.function_type) + "\", needs to be \"" + type_to_compact_string(a_acc._types, expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, function_type.get_function_return(a_acc._types))
	};
}




	


std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto parent_expr = analyse_expression_no_target(a_acc, parent, *details.parent_address);
	a_acc = parent_expr.first;

	const auto key_expr = analyse_expression_no_target(a_acc, parent, *details.lookup_key);
	a_acc = key_expr.first;

	const auto parent_type = analyze_expr_output_type(a_acc, parent_expr.second);
	const auto parent_peek = dereference_type(a_acc._types, parent_type);
	const auto key_type = analyze_expr_output_type(a_acc, key_expr.second);
	const auto key_peek = dereference_type(a_acc._types, key_type);

	if(parent_peek.is_string()){
		if(key_peek.is_int() == false){
			std::stringstream what;
			what << "Strings can only be indexed by integers, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, type_t::make_int()) };
		}
	}
	else if(parent_peek.is_json()){
		return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, type_t::make_json()) };
	}
	else if(parent_peek.is_vector()){
		if(key_peek.is_int() == false){
			std::stringstream what;
			what << "Vector can only be indexed by integers, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return {
				a_acc,
				expression_t::make_lookup(
					parent_expr.second,
					key_expr.second,
					parent_peek.get_vector_element_type(a_acc._types)
				)
			};
		}
	}
	else if(parent_peek.is_dict()){
		if(key_peek.is_string() == false){
			std::stringstream what;
			what << "Dictionary can only be looked up using string keys, not a \"" + type_to_compact_string(a_acc._types, key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, parent_peek.get_dict_value_type(a_acc._types)) };
		}
	}
	else {
		std::stringstream what;
		what << "Lookup using [] only works with strings, vectors, dicts and json - not a \"" + type_to_compact_string(a_acc._types, parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_load(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::load_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto found = find_symbol_by_name(a_acc, details.variable_name);
	if(found.first != nullptr){
/*
		immutable_reserve,
		immutable_arg,
		immutable_precalc,
		named_type,
		mutable_reserve
*/
		if(found.first->_symbol_type == symbol_t::symbol_type::named_type){
			return {
				a_acc,
				expression_t::make_load2(found.second, type_desc_t::make_typeid())
			};
		}
		else{
			return {
				a_acc,
				expression_t::make_load2(found.second, found.first->_value_type)
			};
		}
	}
	else{
		const intrinsic_signatures_t& intrinsic_signatures = a._imm->intrinsic_signatures;

		const auto it = std::find_if(
			intrinsic_signatures.vec.begin(),
			intrinsic_signatures.vec.end(),
			[&](const intrinsic_signature_t& e) { return e.name == details.variable_name; }
		);
		if(it != intrinsic_signatures.vec.end()){
			const auto index = it - intrinsic_signatures.vec.begin();
			const auto addr = symbol_pos_t::make_stack_pos(symbol_pos_t::k_intrinsic, static_cast<int32_t>(index));
			const auto e2 = expression_t::make_load2(addr, it->_function_type);
			return { a_acc, e2 };
		}
		else{
			std::stringstream what;
			what << "Undefined identifier \"" << details.variable_name << "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
}

std::pair<analyser_t, expression_t> analyse_load2(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	return { a, e };
}


static type_t select_inferred_type(const types_t& types, const type_t& target_type_or_any, const type_t& rhs_guess_type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(target_type_or_any.check_invariant());
	QUARK_ASSERT(rhs_guess_type.check_invariant());

	const bool rhs_guess_type_valid = check_types_resolved(types, rhs_guess_type);
	const bool have_target_type = dereference_type(types, target_type_or_any).is_any() == false;

	if(have_target_type && rhs_guess_type_valid == false){
	 	return target_type_or_any;
	}
	else{
		return rhs_guess_type;
	}
}


/*
	Here is trickery with JSON support.

	Case A:
		A = [ "one", 2, 3, "four" ];
	rhs is invalid floyd vector -- mixes value types -- but it's a valid JSON array.

	Case B:
		b = { "one": 1, "two": "zwei" };

	rhs is an invalid dict construction -- you can't mix string/int values in a floyd dict. BUT: it's a valid JSON!
*/
std::pair<analyser_t, expression_t> analyse_construct_value_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::value_constructor_t& details, const type_t& target_type0){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto target_type_peek = dereference_type(a_acc._types, target_type0);

	const auto type0 = analyze_expr_output_type(a_acc, e);
	QUARK_ASSERT(type0 == details.value_type);

	const auto type_peek = dereference_type(a_acc._types, type0);

	if(type_peek.is_vector()){
		//	JSON constants supports mixed element types: convert each element into a json.
		//	Encode as [json]
		if(target_type_peek.is_json()){
			const auto element_type = type_t::make_json();

			std::vector<expression_t> elements2;
			for(const auto& m: details.elements){
				const auto element_expr = analyse_expression_to_target(a_acc, parent, m, element_type);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}
			const auto result_type = make_vector(a_acc._types, type_t::make_json());
			if(check_types_resolved(a_acc._types, result_type) == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}
			return {
				a_acc,
				expression_t::make_construct_value_expr(
					type_t::make_json(),
					{ expression_t::make_construct_value_expr(make_vector(a_acc._types, type_t::make_json()), elements2) }
				)
			};
		}
		else {
			const auto element_type = type_peek.get_vector_element_type(a_acc._types);
			std::vector<expression_t> elements2;
			for(const auto& m: details.elements){
				const auto element_expr = analyse_expression_no_target(a_acc, parent, m);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}

			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? analyze_expr_output_type(a_acc, elements2[0]) : element_type;
			const auto rhs_guess_type = resolve_type_symbols(a_acc, parent.location, make_vector(a_acc._types, element_type2));
			const auto final_type = select_inferred_type(a_acc._types, target_type_peek, rhs_guess_type);

			if(check_types_resolved(a_acc._types, final_type) == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

//			const auto final_type = resolve_type_symbols(a_acc, parent.location, selected_type);

			for(const auto& m: elements2){
				if(analyze_expr_output_type(a_acc, m) != element_type2){
					std::stringstream what;
					what << "Vector of type " << type_to_compact_string(a_acc._types, final_type) << " cannot hold an element of type " << type_to_compact_string(a_acc._types, analyze_expr_output_type(a_acc, m)) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			QUARK_ASSERT(check_types_resolved(a_acc._types, final_type));
			return { a_acc, expression_t::make_construct_value_expr(final_type, elements2) };
		}
	}

	//	Dicts uses pairs of (string,value). This is stored in _args as interleaved expression: string0, value0, string1, value1.
	else if(type_peek.is_dict()){
		//	JSON constants supports mixed element types: convert each element into a json.
		//	Encode as [string:json]
		if(target_type_peek.is_json()){
			const auto element_type = type_t::make_json();

			std::vector<expression_t> elements2;
			for(int i = 0 ; i < details.elements.size() / 2 ; i++){
				const auto& key = details.elements[i * 2 + 0].get_literal().get_string_value();
				const auto& value = details.elements[i * 2 + 1];
				const auto element_expr = analyse_expression_to_target(a_acc, parent, value, element_type);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			const auto rhs_guess_type = resolve_type_symbols(a_acc, parent.location, make_dict(a_acc._types, type_t::make_json()));
			 auto final_type = select_inferred_type(a_acc._types, target_type0, rhs_guess_type);

			if(check_types_resolved(a_acc._types, final_type) == false){
				std::stringstream what;
				what << "Cannot infer dictionary element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			return {
				a_acc,
				expression_t::make_construct_value_expr(
					type_t::make_json(),
					{ expression_t::make_construct_value_expr(make_dict(a_acc._types, type_t::make_json()), elements2) }
				)
			};
		}
		else {
			QUARK_ASSERT(details.elements.size() % 2 == 0);

			const auto element_type = type_peek.get_dict_value_type(a_acc._types);

			std::vector<expression_t> elements2;
			for(int i = 0 ; i < details.elements.size() / 2 ; i++){
				const auto& key = details.elements[i * 2 + 0].get_literal().get_string_value();
				const auto& value = details.elements[i * 2 + 1];
				const auto element_expr = analyse_expression_no_target(a_acc, parent, value);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			//	Infer type of dictionary based on first value.
			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? analyze_expr_output_type(a_acc, elements2[0 * 2 + 1]) : element_type;
			const auto rhs_guess_type = resolve_type_symbols(a_acc, parent.location, make_dict(a_acc._types, element_type2));
			const auto final_type = select_inferred_type(a_acc._types, target_type_peek, rhs_guess_type);

			if(check_types_resolved(a_acc._types, final_type) == false){
				std::stringstream what;
				what << "Cannot infer dictionary element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			//	Make sure all elements have the correct type.
			for(int i = 0 ; i < elements2.size() / 2 ; i++){
				const auto element_type0 = analyze_expr_output_type(a_acc, elements2[i * 2 + 1]);
				if(element_type0 != element_type2){
					std::stringstream what;
					what << "Dictionary of type " << type_to_compact_string(a_acc._types, final_type) << " cannot hold an element of type " << type_to_compact_string(a_acc._types, element_type0) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			QUARK_ASSERT(check_types_resolved(a_acc._types, final_type));
			return {a_acc, expression_t::make_construct_value_expr(final_type, elements2)};
		}
	}
	else if(type_peek.is_struct()){
		const auto& def = type_peek.get_struct(a_acc._types);
		const auto f_type = make_function(a_acc._types, type0, get_member_types(def._members), epure::pure);
		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, details.elements, f_type);
		a_acc = resolved_call.first;
		return { a_acc, expression_t::make_construct_value_expr(type0, resolved_call.second.args) };
	}
	else{
		if(details.elements.size() != 1){
			std::stringstream what;
			what << "Construct value of primitive type requires exactly 1 argument.";
			throw_compiler_error(parent.location, what.str());
		}
		const auto struct_constructor_callee_type = make_function(a_acc._types, type0, { type0 }, epure::pure);
		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, details.elements, struct_constructor_callee_type);
		a_acc = resolved_call.first;
		return { a_acc, expression_t::make_construct_value_expr(type0, resolved_call.second.args) };
	}
}

std::pair<analyser_t, expression_t> analyse_benchmark_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::benchmark_expr_t& details, const type_t& target_type){
	QUARK_ASSERT(a.check_invariant());

	auto acc = a;
	const auto body_pair = analyse_body(acc, *details.body, epure::impure, type_t::make_void());
	acc = body_pair.first;

	const auto e2 = expression_t::make_benchmark_expr(body_pair.second);
	return { acc, e2 };
}


std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto& expr2 = analyse_expression_no_target(a_acc, parent, *details.expr);
	a_acc = expr2.first;

	//??? We could simplify here and return [ "-", 0, expr]
	const auto type = analyze_expr_output_type(a_acc, expr2.second);
	const auto peek_type = dereference_type(a_acc._types, type);
	if(peek_type.is_int() || peek_type.is_double()){
		return {a_acc, expression_t::make_unary_minus(expr2.second, type)  };
	}
	else{
		std::stringstream what;
		what << "Unary minus don't work on expressions of type \"" << type_to_compact_string(a_acc._types, type) << "\"" << ", only int and double.";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_conditional_operator_expression(const analyser_t& analyser, const statement_t& parent, const expression_t& e, const expression_t::conditional_t& details){
	QUARK_ASSERT(analyser.check_invariant());

	auto a_acc = analyser;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = analyse_expression_no_target(a_acc, parent, *details.condition);
	a_acc = cond_result.first;

	const auto a = analyse_expression_no_target(a_acc, parent, *details.a);
	a_acc = a.first;

	const auto b = analyse_expression_no_target(a_acc, parent, *details.b);
	a_acc = b.first;

	const auto type = analyze_expr_output_type(a_acc, cond_result.second);
	const auto peek_type = dereference_type(a_acc._types, type);
	if(peek_type.is_bool() == false){
		std::stringstream what;
		what << "Conditional expression needs to be a bool, not a " << type_to_compact_string(a_acc._types, type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else if(analyze_expr_output_type(a_acc, a.second) != analyze_expr_output_type(a_acc, b.second)){
		std::stringstream what;
		what << "Conditional expression requires true/false expressions to have the same type, currently "
		<< type_to_compact_string(a_acc._types, analyze_expr_output_type(a_acc, a.second))
		<< " : "
		<< type_to_compact_string(a_acc._types, analyze_expr_output_type(a_acc, b.second))
		<< ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto final_expression_type = analyze_expr_output_type(a_acc, a.second);
		return {
			a_acc,
			expression_t::make_conditional_operator(
				cond_result.second,
				a.second,
				b.second,
				final_expression_type
			)
		};
	}
}


std::pair<analyser_t, expression_t> analyse_comparison_expression(const analyser_t& a, const statement_t& parent, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse all inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, parent, *details.lhs);
	a_acc = left_expr.first;

	const auto lhs_type = analyze_expr_output_type(a_acc, left_expr.second);

	//	Make rhs match left if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, parent, *details.rhs, lhs_type);
	a_acc = right_expr.first;
	const auto rhs_type = analyze_expr_output_type(a_acc, right_expr.second);

	if(lhs_type != rhs_type || (lhs_type.is_undefined() == true || rhs_type.is_undefined() == true)){
		std::stringstream what;
		what << "Left and right expressions must be same type in comparison, " << type_to_compact_string(a_acc._types, lhs_type) << " " << expression_type_to_opcode(op) << type_to_compact_string(a_acc._types, rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		if(op == expression_type::k_comparison_smaller_or_equal){
		}
		else if(op == expression_type::k_comparison_smaller){
		}
		else if(op == expression_type::k_comparison_larger_or_equal){
		}
		else if(op == expression_type::k_comparison_larger){
		}

		else if(op == expression_type::k_logical_equal){
		}
		else if(op == expression_type::k_logical_nonequal){
		}
		else{
			quark::throw_exception();
		}
		return {
			a_acc,
			expression_t::make_comparison(
				op,
				left_expr.second,
				right_expr.second,
				type_t::make_bool()
			)
		};
	}
}



//??? What is difference between literal_exp_t and value_constructor_t?
// Literals: Only a few built-in types can be initialized as immediates in the code and via code segment, the rest needs to be reserved and explicitly initialised at runtime.

std::pair<analyser_t, expression_t> analyse_literal_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::literal_exp_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto e2 = analyze_expr_output_type(a_acc, e);
	const auto r = expression_t::make_literal(details.value, e2);
	return { a_acc, r };
}




std::pair<analyser_t, expression_t> analyse_arithmetic_expression(const analyser_t& a, const statement_t& parent, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse both inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, parent, *details.lhs);
	a_acc = left_expr.first;

	const auto lhs_type = analyze_expr_output_type(a_acc, left_expr.second);
	if(dereference_type(a_acc._types, lhs_type).is_void()){
		std::stringstream what;
		what << "Artithmetics: Left hand side of expression cannot be of type void.";
		throw_compiler_error(parent.location, what.str());
	}

	//	Make rhs match lhs if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, parent, *details.rhs, lhs_type);
	a_acc = right_expr.first;

	const auto rhs_type = analyze_expr_output_type(a_acc, right_expr.second);


	if(lhs_type != rhs_type){
		std::stringstream what;
		what << "Artithmetics: Left and right expressions must be same type, currently " << type_to_compact_string(a_acc._types, lhs_type) << " : " << type_to_compact_string(a_acc._types, rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto shared_type = lhs_type;
		const auto shared_type_peek = dereference_type(a_acc._types, shared_type);


		//	bool
		if(shared_type_peek.is_bool()){
			if(
				op == expression_type::k_arithmetic_add
				|| op == expression_type::k_logical_and
				|| op == expression_type::k_logical_or
			){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
			}
			else {
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}
		}

		//	int
		else if(shared_type_peek.is_int()){
			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
		}

		//	double
		else if(shared_type_peek.is_double()){
			if(op == expression_type::k_arithmetic_remainder){
				throw_compiler_error(parent.location, "Modulo operation on double not supported.");
			}
			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
		}

		//	string
		else if(shared_type_peek.is_string()){
			if(op == expression_type::k_arithmetic_add){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
			}
			else{
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
		}

		//	struct
		else if(shared_type_peek.is_struct()){
			//	Structs must be exactly the same type to match.

			if(op == expression_type::k_arithmetic_add){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_subtract){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_multiply){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_divide){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_remainder){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}

			else if(op == expression_type::k_logical_and){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_logical_or){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}

			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
		}

		//	vector
		else if(shared_type_peek.is_vector()){
			if(op == expression_type::k_arithmetic_add){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, shared_type) };
			}
			else{
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
		}

		//	function
		else if(shared_type_peek.is_function()){
			throw_compiler_error(parent.location, "Cannot perform operations on two function values.");
		}
		else{
			throw_compiler_error(parent.location, "Arithmetics failed.");
		}
	}
}



/*
	FUNCTION CALLS, INTRINSICS

	TERMINOLOGY

	"call" = the expression that wants to make a call. It needs to tell will callee (the target function value) it wants to call. It includes arguments to pass to the call.

	"callee" (plural callees)
	"As a noun callee is  (telephony) the person who is called by the caller (on the telephone).". The function value to call. The callee value has a function-type.

	Since Floyd uses static typing, the call's argument types and number (arity) needs to match excactly.
	BUT: Function values can have arguments of type ANY and return ANY.


	The call will either match 100% or, if the callee has any-type as return and/or arguments, then the call will have *more* type info than callee.

	After semantic analysis has run (or any of the analys_*() functions returns an expression), then the actual types (arguments and return) of all call expressions are 100% known.

	callee:						void print(any)
	BEFORE call expression:		print(13)
	AFTER call expression:		print(int 13)
*/

static std::pair<analyser_t, expression_t> analyse_intrinsic_fallthrough_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& call_args, const intrinsic_signature_t& sign){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;

	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, call_args, sign._function_type);
	a_acc = resolved_call.first;

	const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);

	return {
		a_acc,
		expression_t::make_intrinsic(
			get_intrinsic_opcode(sign),
			resolved_call.second.args,
			function_type.get_function_return(a_acc._types)
		)
	};
}

/*
	There are several types of calls in the input AST.

	1. Normal function call: via a function value. Supports any-type.

	2. intrinsics: like assert() and update(). Those calls are intercepted and converted from function calls to
		intrinsic-expressions. Each intrinsic has its own special type checking. Supports any-type in its callee function type.

	3. Callee is a type: Example: my_color_t(0, 0, 255). This call is converted to a construct-value expression.
*/
std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& a0, const statement_t& parent, const expression_t& e, const expression_t::call_t& details){
	QUARK_ASSERT(a0.check_invariant());

	auto a_acc = a0;

	const auto callee_expr0 = analyse_expression_no_target(a_acc, parent, *details.callee);
	a_acc = callee_expr0.first;
	const auto callee_expr = callee_expr0.second;

	const auto call_args = details.args;

	const auto callsite_pure = a_acc._lexical_scope_stack.back().pure;

	auto callee_expr_load2 = std::get_if<expression_t::load2_t>(&callee_expr._expression_variant);

	//	This is a call to a function-value. Callee is a function-type.
	const auto callee_type0 = analyze_expr_output_type(a_acc, callee_expr);
	const auto callee_type_peek = dereference_type(a_acc._types, callee_type0);

	if(callee_type_peek.is_function()){
		const auto callee_pure = callee_type_peek.get_function_pure(a_acc._types);

		if(callsite_pure == epure::pure && callee_pure == epure::impure){
			throw_compiler_error(parent.location, "Cannot call impure function from a pure function.");
		}

		//	Detect use of intrinsics.
		if(callee_expr_load2){
			if(callee_expr_load2->address._parent_steps == symbol_pos_t::k_intrinsic){
				const intrinsic_signatures_t& intrinsic_signatures = a0._imm->intrinsic_signatures;
				const auto index = callee_expr_load2->address._index;
				QUARK_ASSERT(index >= 0 && index < intrinsic_signatures.vec.size());
				const auto& sign = intrinsic_signatures.vec[index];
				const auto& s = sign.name;

				if(s == intrinsic_signatures.assert.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.assert);
				}
				else if(s == intrinsic_signatures.to_string.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.to_string);
				}
				else if(s == intrinsic_signatures.to_pretty_string.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.to_pretty_string);
				}

				else if(s == intrinsic_signatures.typeof_sign.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.typeof_sign);
				}

				else if(s == intrinsic_signatures.update.name){
					return analyse_intrinsic_update_expression(a_acc, parent, e, details.args);
				}
				else if(s == intrinsic_signatures.size.name){
					return analyse_intrinsic_size_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.find.name){
					return analyse_intrinsic_find_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.exists.name){
					return analyse_intrinsic_exists_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.erase.name){
					return analyse_intrinsic_erase_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.get_keys.name){
					return analyse_intrinsic_get_keys_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.push_back.name){
					return analyse_intrinsic_push_back_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.subset.name){
					return analyse_intrinsic_subset_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.replace.name){
					return analyse_intrinsic_replace_expression(a_acc, parent, details.args);
				}


				else if(s == intrinsic_signatures.parse_json_script.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.parse_json_script);
				}
				else if(s == intrinsic_signatures.generate_json_script.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.generate_json_script);
				}
				else if(s == intrinsic_signatures.to_json.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.to_json);
				}
				else if(s == intrinsic_signatures.from_json.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.from_json);
				}

				else if(s == intrinsic_signatures.get_json_type.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.get_json_type);
				}




				else if(s == intrinsic_signatures.map.name){
					return analyse_intrinsic_map_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.map_dag.name){
					return analyse_intrinsic_map_dag_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.filter.name){
					return analyse_intrinsic_filter_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.reduce.name){
					return analyse_intrinsic_reduce_expression(a_acc, parent, details.args);
				}
				else if(s == intrinsic_signatures.stable_sort.name){
					return analyse_intrinsic_stable_sort_expression(a_acc, parent, details.args);
				}




				else if(s == intrinsic_signatures.print.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.print);
				}
				else if(s == intrinsic_signatures.send.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.send);
				}
				else if(s == intrinsic_signatures.exit.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.exit);
				}



				else if(s == intrinsic_signatures.bw_not.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_not);
				}
				else if(s == intrinsic_signatures.bw_and.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_and);
				}
				else if(s == intrinsic_signatures.bw_or.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_or);
				}
				else if(s == intrinsic_signatures.bw_xor.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_xor);
				}
				else if(s == intrinsic_signatures.bw_shift_left.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_shift_left);
				}
				else if(s == intrinsic_signatures.bw_shift_right.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_shift_right);
				}
				else if(s == intrinsic_signatures.bw_shift_right_arithmetic.name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, intrinsic_signatures.bw_shift_right_arithmetic);
				}

				else{
				}
			}
		}

		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, call_args, callee_type_peek);
		a_acc = resolved_call.first;

		const auto function_type = dereference_type(a_acc._types, resolved_call.second.function_type);
		return { a_acc, expression_t::make_call(callee_expr, resolved_call.second.args, function_type.get_function_return(a_acc._types)) };
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	//	Desugar!
	//	Converts these calls to construct-value-expressions.
	else if(callee_expr_load2){
		const auto& addr = callee_expr_load2->address; 
		const size_t scope_index = addr._parent_steps == symbol_pos_t::k_global_scope ? 0 : a_acc._lexical_scope_stack.size() - 1 - addr._parent_steps;
		const auto& callee_symbol = a_acc._lexical_scope_stack[scope_index].symbols._symbols[addr._index];

		if(callee_symbol.second._symbol_type == symbol_t::symbol_type::named_type){
			const auto construct_value_type = get_symbol_named_type(a_acc._types, callee_symbol.second);

			//	Convert calls to struct-type into construct-value expression.
			if(dereference_type(a_acc._types, construct_value_type).is_struct()){
				const auto construct_value_expr = expression_t::make_construct_value_expr(construct_value_type, details.args);
				const auto result_pair = analyse_expression_to_target(a_acc, parent, construct_value_expr, construct_value_type);
				return { result_pair.first, result_pair.second };
			}

			//	One argument for primitive types.
			else{
				const auto construct_value_expr = expression_t::make_construct_value_expr(construct_value_type, details.args);
				const auto result_pair = analyse_expression_to_target(a_acc, parent, construct_value_expr, construct_value_type);
				return { result_pair.first, result_pair.second };
			}
		}
		else{
		}
	}

	else if(callee_type_peek.is_typeid()){
		auto callee_expr_struct_def = std::get_if<expression_t::struct_definition_expr_t>(&details.callee->_expression_variant);
		const auto construct_value_type = details.callee->get_output_type();

		//	Convert calls to struct-type into construct-value expression.
		if(callee_expr_struct_def){
			const auto construct_value_expr = expression_t::make_construct_value_expr(construct_value_type, details.args);
			const auto result_pair = analyse_expression_to_target(a_acc, parent, construct_value_expr, construct_value_type);
			return { result_pair.first, result_pair.second };
		}
	}



	{
		std::stringstream what;
		what << "Cannot call non-function, its type is " << type_to_compact_string(a_acc._types, callee_type0) << ".";
		throw_compiler_error(parent.location, what.str());
	}
}

/*
	Notice: the symbol table mechanism makes sure all clients access the proper types via their lexical scopes.
	The name of the type itself needs only be program-unique, not encode the actual lexical scope it's defined in.

	??? WRONG: TOOD: user proper hiearchy of lexical scopes, not a flat list of generated ID:s. This requires a name--string in each lexical_scope_t.
*/
static type_name_t generate_type_name(analyser_t& a, const std::string& identifier){
	const auto id = a.scope_id_generator++;

//	const auto b = std::string() + "lexical_scope" + std::to_string(id);
	const auto b = a._lexical_scope_stack.size() == 1 ? "global_scope" : std::to_string(id);

	return type_name_t { { b, identifier } };
}

static std::pair<analyser_t, expression_t> analyse_struct_definition_expression(const analyser_t& a, const statement_t& parent, const expression_t& e0, const expression_t::struct_definition_expr_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const std::string identifier = details.name;
	if(identifier != ""){
		// Add symbol for the new struct, with undefined type. Later update with final type.
		if(does_symbol_exist_shallow(a_acc, identifier)){
			throw_local_identifier_already_exists(parent.location, identifier);
		}

		const auto name = generate_type_name(a_acc, identifier);
		const auto named_type = make_named_type(a_acc._types, name, make_undefined());

		const auto type_name_symbol = symbol_t::make_named_type(named_type);
		a_acc._lexical_scope_stack.back().symbols._symbols.push_back({ identifier, type_name_symbol });


		std::vector<member_t> members2;
		for(const auto& m: details.def->_members){
			members2.push_back(member_t{ resolve_type_symbols(a_acc, parent.location, m._type), m._name } );
		}
		const auto struct_type1 = make_struct(a_acc._types, struct_type_desc_t{ members2 } );

		//	Update our temporary.
		const auto named_type2 = update_named_type(a_acc._types, named_type, struct_type1);

		const auto typeid_value = value_t::make_typeid_value(named_type2);
		const auto r = expression_t::make_literal(typeid_value, type_desc_t::make_typeid());

#if DEBUG
		if(false) trace_analyser(a_acc);
#endif

		return { a_acc, r };
	}
	else{
		std::vector<member_t> members2;
		for(const auto& m: details.def->_members){
			members2.push_back(member_t{ resolve_type_symbols(a_acc, parent.location, m._type), m._name } );
		}
		const auto struct_type1 = make_struct(a_acc._types, struct_type_desc_t{ members2 } );

		const auto typeid_value = value_t::make_typeid_value(struct_type1);
		const auto r = expression_t::make_literal(typeid_value, type_desc_t::make_typeid());

#if DEBUG
		if(false) trace_analyser(a_acc);
#endif

		return { a_acc, r };
	}
}


// ??? Check that function returns always returns a value, if it has a return-type.
std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& analyser, const statement_t& parent, const expression_t& e, const expression_t::function_definition_expr_t& details){
	QUARK_ASSERT(analyser.check_invariant());

	auto a_acc = analyser;

	const auto function_def = details.def;
	const auto function_type0 = resolve_type_symbols(a_acc, parent.location, function_def._function_type);
	const auto function_type_peek = dereference_type(a_acc._types, function_type0);
	const auto function_pure = function_type_peek.get_function_pure(a_acc._types);

	std::vector<member_t> args2;
	for(const auto& arg: function_def._named_args){
		const auto arg_type2 = resolve_type_symbols(a_acc, parent.location, arg._type);
		args2.push_back(member_t { arg_type2, arg._name } );
	}

	//??? Can there be a pure function inside an impure lexical scope? I think yes.
	const auto pure = function_pure;

	std::shared_ptr<body_t> body_result;
	if(function_def._optional_body){
		//	Make function body with arguments injected FIRST in body as local symbols.
		auto symbol_vec = function_def._optional_body->_symbol_table;
		for(const auto& arg: args2){
			symbol_vec._symbols.push_back({ arg._name , symbol_t::make_immutable_arg(arg._type) });
		}
		const auto function_body2 = body_t(function_def._optional_body->_statements, symbol_vec);

		const auto body_pair = analyse_body(a_acc, function_body2, pure, function_type_peek.get_function_return(a_acc._types));
		a_acc = body_pair.first;
		const auto function_body3 = body_pair.second;
		body_result = std::make_shared<body_t>(function_body3);
	}
	else{
	}

	const auto definition_name = function_def._definition_name;
	const auto function_id = module_symbol_t(definition_name);

	const auto function_def2 = function_definition_t::make_func(k_no_location, definition_name, function_type_peek, args2, body_result);
	QUARK_ASSERT(check_types_resolved(a_acc._types, function_def2));

	a_acc._function_defs.insert({ function_id, function_def2 });

	const auto r = expression_t::make_literal(value_t::make_function_value(a_acc._types, function_type0, function_id), function_type0);

	return { a_acc, r };
}

/*
//	Create a global function-def record, return a function-value.
static std::pair<analyser_t, expression_t> analyse_function_definition_expression2(const analyser_t& analyser, const statement_t& parent, const expression_t& e, const expression_t::function_definition_expr_t& details){
	QUARK_ASSERT(analyser.check_invariant());

	auto a_acc = analyser;

	const auto func_def_expr = analyse_function_definition_expression(a_acc, parent, e, details);
	const auto func_def_expr2 = std::get_if<expression_t::function_definition_expr_t>(&func_def_expr.second._expression_variant);
	QUARK_ASSERT(func_def_expr2 != nullptr);

	struct visitor_t {
		analyser_t& a_acc;
		const expression_t& expr;
		const expression_t::function_definition_expr_t& details;

		std::pair<analyser_t, expression_t> operator()(const function_definition_t::empty_t& e) const{
			const auto function_type = expr.get_output_type();
			const auto r = expression_t::make_literal(value_t::make_function_value(function_type, k_no_function_id));
			return { a_acc, r };
		}
		std::pair<analyser_t, expression_t> operator()(const function_definition_t::floyd_func_t& floyd_func) const{
			const auto function_type = expr.get_output_type();
			const auto r = expression_t::make_literal(value_t::make_function_value(function_type, function_id_t { details.def._definition_name } ));
			return { a_acc, r };
		}
		std::pair<analyser_t, expression_t> operator()(const function_definition_t::host_func_t& e) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	};
	const auto result = std::visit(visitor_t{ a_acc, func_def_expr.second, *func_def_expr2 }, func_def_expr2->def._contents);
	return result;
}
*/

std::pair<analyser_t, expression_t> analyse_expression__operation_specific(const analyser_t& a, const statement_t& parent, const expression_t& e, const type_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		const analyser_t& a;
		const statement_t& parent;
		const expression_t& e;
		const type_t& target_type;

		std::pair<analyser_t, expression_t> operator()(const expression_t::literal_exp_t& expr) const{
			return analyse_literal_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::arithmetic_t& expr) const{
			return analyse_arithmetic_expression(a, parent, expr.op, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::comparison_t& expr) const{
			return analyse_comparison_expression(a, parent, expr.op, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::unary_minus_t& expr) const{
			return analyse_arithmetic_unary_minus_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::conditional_t& expr) const{
			return analyse_conditional_operator_expression(a, parent, e, expr);
		}

		std::pair<analyser_t, expression_t> operator()(const expression_t::call_t& expr) const{
			return analyse_call_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::intrinsic_t& expr) const{
			QUARK_ASSERT(false);
			throw std::exception();
		}


		std::pair<analyser_t, expression_t> operator()(const expression_t::struct_definition_expr_t& expr) const{
			return analyse_struct_definition_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::function_definition_expr_t& expr) const{
			return analyse_function_definition_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::load_t& expr) const{
			return analyse_load(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::load2_t& expr) const{
			return analyse_load2(a, e);
		}

		std::pair<analyser_t, expression_t> operator()(const expression_t::resolve_member_t& expr) const{
			return analyse_resolve_member_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::update_member_t& expr) const{
			QUARK_ASSERT(false);
//			return analyse_resolve_member_expression(a, parent, e, expr);
			return { a, e };
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::lookup_t& expr) const{
			return analyse_lookup_element_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::value_constructor_t& expr) const{
			return analyse_construct_value_expression(a, parent, e, expr, target_type);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::benchmark_expr_t& expr) const{
			return analyse_benchmark_expression(a, parent, e, expr, target_type);
		}
	};

	const auto result = std::visit(visitor_t{ a, parent, e, target_type }, e._expression_variant);

	auto a_acc = result.first;

	const auto output_type = result.second.get_output_type();
	if(is_empty(output_type) == false){
		QUARK_ASSERT(check_types_resolved(a_acc._types, result.second));
	}

	return { a_acc, result.second };
}


/*
	- Insert automatic type-conversions from string -> json etc.
*/
expression_t auto_cast_expression_type(analyser_t& a, const expression_t& e, const type_t& wanted_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(wanted_type.check_invariant());
	QUARK_ASSERT(wanted_type.is_undefined() == false);

	const auto current_type = analyze_expr_output_type(a, e);
	const auto current_peek = dereference_type(a._types, current_type);

	const auto wanted_type_peek = dereference_type(a._types, wanted_type);
	if(wanted_type_peek.is_any()){
		return e;
	}
	else if(wanted_type_peek.is_string()){
		if(current_peek.is_json()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else{
			return e;
		}
	}
	else if(wanted_type_peek.is_json()){
		if(current_peek.is_int() || current_peek.is_double() || current_peek.is_string() || current_peek.is_bool()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else if(current_peek.is_vector()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else if(current_peek.is_dict()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else{
			return e;
		}
	}
	else{
		return e;
	}
}

static std::string get_expression_name(const expression_t& e){
	const expression_type op = get_expression_type(e);
	return expression_type_to_opcode(op);
}


//	Return new expression where all types have been resolved. The target-type is used as a hint for type inference.
//	Returned expression is guaranteed to be deep-resolved.
static std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const expression_t& e, const type_t& target_type0){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	auto a_acc = a;

	if(false) trace_types(a_acc._types);

	const auto target_type_peek = dereference_type(a_acc._types, target_type0);

	QUARK_ASSERT(target_type_peek.is_void() == false && target_type_peek.is_undefined() == false);
	QUARK_ASSERT(check_types_resolved(a._types, target_type0));

	const auto e2_pair = analyse_expression__operation_specific(a_acc, parent, e, target_type0);
	a_acc = e2_pair.first;
	const auto e3 = e2_pair.second;
	if(floyd::check_types_resolved(a_acc._types, e3) == false){
		std::stringstream what;
		what << "Cannot infer type in " << get_expression_name(e3) << "-expression.";
		throw_compiler_error(parent.location, what.str());
	}

	const auto e4 = auto_cast_expression_type(a_acc, e3, target_type0);
	const auto e4_output_type = e4.get_output_type();
	if(target_type_peek.is_any()){
	}
	else if(e4_output_type == target_type0){
	}
	else if(e4_output_type.is_undefined()){
		QUARK_ASSERT(false);
		throw_compiler_error(parent.location, "Expression type mismatch.");
	}
	else{
		std::stringstream what;
		what << "Expression type mismatch - cannot convert '"
		//??? missing a trailing '
		<< type_to_compact_string(a_acc._types, e4_output_type) << "' to '" << type_to_compact_string(a_acc._types, target_type0) << ".";
		throw_compiler_error(parent.location, what.str());
	}

	if(floyd::check_types_resolved(a_acc._types, e4) == false){
		throw_compiler_error(parent.location, "Cannot resolve type.");
	}
//	QUARK_ASSERT(floyd::check_types_resolved(a_acc._types, e4));
	return { a_acc, e4 };
}

//	Returned expression is guaranteed to be deep-resolved.
static std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const expression_t& e){
	return analyse_expression_to_target(a, parent, e, type_t::make_any());
}

void test__analyse_expression(const statement_t& parent, const expression_t& e, const expression_t& expected){
	const unchecked_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter, parent, e);

	const types_t temp;
	ut_verify_json(QUARK_POS, expression_to_json(temp, e3.second), expression_to_json(temp, expected));
}


QUARK_TEST("analyse_expression_no_target()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		statement_t::make__bind_local(k_no_location, "xyz", type_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_TEST("analyse_expression_no_target()", "1 + 2 == 3", "", "") {
	const types_t temp;
	const unchecked_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(
		interpreter,
		statement_t::make__bind_local(k_no_location, "xyz", type_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_arithmetic(
			expression_type::k_arithmetic_add,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			make_undefined()
		)
	);

	ut_verify_json(QUARK_POS,
		expression_to_json(temp, e3.second),
		parse_json(seq_t(R"(   ["+", ["k", 1, "int"], ["k", 2, "int"], "int"]   )")).first
	);
}





static std::pair<std::string, symbol_t> make_builtin_type(types_t& types, const type_t& type){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto bt = type.get_base_type();
	const auto opcode = base_type_to_opcode(bt);
	return { opcode, symbol_t::make_named_type(type) };
}

void register_named_type(analyser_t& a, std::vector<std::pair<std::string, symbol_t>>& symbol_map, const std::string& name, const type_t& type){
	const auto type0 = resolve_type_symbols(a, k_no_location, type);
	const auto type2 = make_named_type(a._types, generate_type_name(a, name), type0);
	symbol_map.push_back( { name, symbol_t::make_named_type(type2) } );
}

static std::vector<std::pair<std::string, symbol_t>> generate_builtin_symbols(analyser_t& a, const analyzer_imm_t& input){
	/*
		Create built-in global symbol map: built in data types, built-in functions (intrinsics).
	*/
	auto& symbol_map = a._lexical_scope_stack.back().symbols._symbols;
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_void()) );
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_bool()) );
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_int()) );
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_double()) );
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_string()) );
	symbol_map.push_back( make_builtin_type(a._types, type_desc_t::make_typeid()) );
	symbol_map.push_back( make_builtin_type(a._types, type_t::make_json()) );

	//	"null" is equivalent to json::null
	symbol_map.push_back( { "null", symbol_t::make_immutable_precalc(type_t::make_json(), value_t::make_json(json_t())) });

	symbol_map.push_back( { "json_object", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(1)) });
	symbol_map.push_back( { "json_array", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(2)) });
	symbol_map.push_back( { "json_string", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(3)) });
	symbol_map.push_back( { "json_number", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(4)) });
	symbol_map.push_back( { "json_true", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(5)) });
	symbol_map.push_back( { "json_false", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(6)) });
	symbol_map.push_back( { "json_null", symbol_t::make_immutable_precalc(type_t::make_int(), value_t::make_int(7)) });



	//	Insert the types that are built-into the compiler itself = not part of standard library.

	register_named_type(a, symbol_map, "benchmark_result_t", make_benchmark_result_t(a._types));
	register_named_type(a, symbol_map, "benchmark_def_t", make_benchmark_def_t(a._types));

	//	Reserve a symbol table entry for benchmark_registry **instance**.
	{
		const auto benchmark_registry_type = make_vector(a._types, make_symbol_ref(a._types, "benchmark_def_t"));
		symbol_map.push_back( {
			k_global_benchmark_registry,
			symbol_t::make_immutable_reserve(
				resolve_type_symbols(a, k_no_location, benchmark_registry_type)
			)
		} );
	}

	register_named_type(a, symbol_map, "test_def_t", make_test_def_t(a._types));


	//	Reserve a symbol table entry for test_registry **instance**.
	{
		const auto test_registry_type = make_vector(a._types, make_symbol_ref(a._types, "test_def_t"));
		symbol_map.push_back( {
			k_global_test_registry,
			symbol_t::make_immutable_reserve(
				resolve_type_symbols(a, k_no_location, test_registry_type)
			)
		} );
	}

	return symbol_map;
}



//	Create built-in global symbol map: built in data types and intrinsics.
//	Analyze global namespace, including all Floyd functions defined there.
const body_t make_global_body(analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	auto global_body = body_t(a._imm->_ast._tree._globals._statements, symbol_table_t{ });

	auto new_environment = symbol_table_t{ global_body._symbol_table };
	const auto lexical_scope = lexical_scope_t{ new_environment, epure::impure };
	a._lexical_scope_stack.push_back(lexical_scope);

	const auto builtin_symbols = generate_builtin_symbols(a, *a._imm);
	for(const auto& e: a._imm->intrinsic_signatures.vec){
		resolve_type_symbols(a, k_no_location, e._function_type);
	}

	if(false) trace_analyser(a);

	const auto result = analyse_statements(a, global_body._statements, type_t::make_void());
	a = result.first;

	const auto body2 = body_t(result.second, result.first._lexical_scope_stack.back().symbols);

	if(false) trace_analyser(a);

	auto global_body3 = body2;


	//	Add Init benchmark_registry.
	{
		auto symbol_ptr = find_symbol_by_name(a, k_global_benchmark_registry);
		QUARK_ASSERT(symbol_ptr.first != nullptr);

		const auto s = statement_t::make__init2(
			k_no_location,
			symbol_ptr.second,
			expression_t::make_construct_value_expr(
				symbol_ptr.first->_value_type,
				a.benchmark_defs
			)
		);

		global_body3._statements.insert(global_body3._statements.begin(), s);
	}

	//	Add Init test_registry.
	{
		auto symbol_ptr = find_symbol_by_name(a, k_global_test_registry);
		QUARK_ASSERT(symbol_ptr.first != nullptr);

		const auto s = statement_t::make__init2(
			k_no_location,
			symbol_ptr.second,
			expression_t::make_construct_value_expr(
				symbol_ptr.first->_value_type,
				a.test_defs
			)
		);

		global_body3._statements.insert(global_body3._statements.begin(), s);
	}



	//	Setup container and Floyd processes.
	/*
		STATE_TYPE base__init()
		STATE_TYPE base__msg(STATE_TYPE, MSG_TYPE msg)

	 	- setup state_type and msg_type.
	 	- Make sure functions exists
	 	- Check types of functions.
	 	- Make sure send() works with any msg type.

		??? make_process_init_type(types, process_state_type), make_process_message_handler_type(types, process_state_type)
	*/
	{
		auto container_def = a._container_def;

		for(auto& bus: container_def._clock_busses){
			for(auto& process: bus.second._processes){

				const auto init_func_name = process.second.name_key + "__init";
				auto init_func_symbol = find_symbol_by_name(a, init_func_name);
				if(init_func_symbol.first == nullptr){
					std::stringstream what;
					what << "Missing init function \"" << init_func_name << "\" needed by process \"" << process.second.name_key << "\".";

					//	Cannot throw compiler error since we have no code location.
					throw std::runtime_error(what.str());
				}

				const auto init_func_type = init_func_symbol.first->get_value_type();
				const auto init_func_peek = dereference_type(a._types, init_func_type);


				if(
					(init_func_peek.is_function() == false)
					|| (
						init_func_peek.get_function_args(a._types).size() != 0
						|| dereference_type(a._types, init_func_peek.get_function_return(a._types)).is_void()
					)
				){
					std::stringstream what;
					what << "Incorrect init function \"" << init_func_name << "\" needed by process \"" << process.second.name_key << "\".";

					//	Cannot throw compiler error since we have no code location.
					throw std::runtime_error(what.str());
				}
				const auto state_type = init_func_peek.get_function_return(a._types);


				const auto msg_func_name = process.second.name_key + "__msg";
				auto msg_func_symbol = find_symbol_by_name(a, msg_func_name);
				if(msg_func_symbol.first == nullptr){
					std::stringstream what;
					what << "Missing message function \"" << msg_func_name << "\" needed by process \"" << process.second.name_key << "\".";

					//	Cannot throw compiler error since we have no code location.
					throw std::runtime_error(what.str());
				}

				const auto msg_func_type = msg_func_symbol.first->get_value_type();
				const auto msg_func_peek = dereference_type(a._types, msg_func_type);

				if(
					(msg_func_peek.is_function() == false)
					|| (msg_func_peek.get_function_args(a._types).size() != 2
						|| msg_func_peek.get_function_return(a._types) != state_type
						|| msg_func_peek.get_function_args(a._types)[0] != state_type
					)
				){
					std::stringstream what;
					what << "Incorrect message function \"" << msg_func_name << "\" needed by process \"" << process.second.name_key << "\".";

					//	Cannot throw compiler error since we have no code location.
					throw std::runtime_error(what.str());
				}
				const auto msg_type = msg_func_peek.get_function_args(a._types)[1];


				process.second.init_func_linkname = init_func_symbol.first->_init.get_function_value();
				process.second.msg_func_linkname = msg_func_symbol.first->_init.get_function_value();

				process.second.state_type = state_type;
				process.second.msg_type = msg_type;
			}
		}

		a._container_def = container_def;
	}








	if(false) trace_analyser(a);

	a._lexical_scope_stack.pop_back();

	return global_body3;
}




//////////////////////////////////////		analyser_t



analyser_t::analyser_t(const unchecked_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	_types = ast._tree._types;
	const auto intrinsic_signatures = make_intrinsic_signatures(_types);
	_imm = std::make_shared<analyzer_imm_t>(analyzer_imm_t{ ast, intrinsic_signatures });
	scope_id_generator = 1000;

	QUARK_ASSERT(check_invariant());
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_imm != nullptr);
	QUARK_ASSERT(_imm->_ast.check_invariant());
	QUARK_ASSERT(_types.check_invariant());
	return true;
}
#endif


//////////////////////////////////////		run_semantic_analysis()


static semantic_ast_t run_semantic_analysis0(const unchecked_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	analyser_t a(ast);
	const auto global_body3 = make_global_body(a);

	std::vector<floyd::function_definition_t> function_defs_vec;
	for(const auto& e: a._function_defs){
		function_defs_vec.push_back(e.second);
	}

	auto ast2 = general_purpose_ast_t{
		._globals = global_body3,
		._function_defs = function_defs_vec,
		._types = a._types,
		._software_system = a._software_system,
		._container_def = a._container_def
	};
	const auto ast3 = semantic_ast_t(ast2, a._imm->intrinsic_signatures);

	if(false){
		{
			QUARK_SCOPED_TRACE("OUTPUT AST");
			QUARK_TRACE(json_to_pretty_string(gp_ast_to_json(ast3._tree)));
		}
		{
			QUARK_SCOPED_TRACE("OUTPUT TYPES");
			trace_analyser(a);
		}
	}

//	QUARK_ASSERT(check_types_resolved(ast3._tree));

	QUARK_ASSERT(ast3._tree.check_invariant());
//	QUARK_ASSERT(check_types_resolved(result._tree));
	return ast3;
}

semantic_ast_t run_semantic_analysis(const unchecked_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	try {
		if(k_trace_io){
			QUARK_SCOPED_TRACE("SEMANTIC ANALYSIS PASS");

			{
				QUARK_SCOPED_TRACE("INPUT AST");
				QUARK_TRACE(json_to_pretty_string(gp_ast_to_json(ast._tree)));
			}

			const auto result = run_semantic_analysis0(ast);

			{
				{
					QUARK_SCOPED_TRACE("OUTPUT AST");
					QUARK_TRACE(json_to_pretty_string(gp_ast_to_json(result._tree)));
				}
				{
					QUARK_SCOPED_TRACE("OUTPUT TYPES");
					trace_types(result._tree._types);
				}
			}

			return result;
		}
		else {
			return run_semantic_analysis0(ast);
		}
	}
	catch(const compiler_error& e){
		const auto what = e.what();
		const auto what2 = std::string("[Semantics] ") + what;
		throw compiler_error(e.location, e.location2, what2);
	}
}




}	//	floyd
