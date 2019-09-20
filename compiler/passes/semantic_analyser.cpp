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

static const bool k_trace_io = true;

//???? remove!!
using namespace std;




//////////////////////////////////////		analyzer_imm_t

//	Immutable data used by analyser.

struct analyzer_imm_t {
	unchecked_ast_t _ast;

	std::vector<intrinsic_signature_t> intrinsic_signatures;
};




//////////////////////////////////////		analyser_t

/*
	Value object (MUTABLE!).
	Represents a node in the lexical scope tree.
*/

struct lexical_scope_t {
	symbol_table_t symbols;
	epure pure;
};

struct analyser_t : public i_resolve_identifer {
	public: analyser_t(const unchecked_ast_t& ast);
#if DEBUG
	public: bool check_invariant() const;
#endif



	itype_t i_resolve_identifer_resolve(const std::string& identifier) const override;


	////////////////////////		STATE

	public: std::shared_ptr<const analyzer_imm_t> _imm;

	//	Non-constant. Last scope is the current one. First scope is the root.
	//	This is ONE branch through the three of lexical scopes of the program.
	public: std::vector<lexical_scope_t> _lexical_scope_stack;

	//	These are output functions, that have been fixed.
	public: std::map<function_id_t, function_definition_t> _function_defs;
	public: type_interner_t _types;

	public: software_system_t _software_system;
	public: container_t _container_def;

	public: std::vector<expression_t> benchmark_defs;

	public: int scope_id_generator;
};





static void throw_local_identifier_already_exists(const location_t& loc, const std::string& local_identifier){
	std::stringstream what;
	what << "Local identifier \"" << local_identifier << "\" already exists.";
	throw_compiler_error(loc, what.str());
}






//////////////////////////////////////		forward


semantic_ast_t analyse(const analyser_t& a);
std::pair<analyser_t, shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement, const typeid_t& return_type);
std::pair<analyser_t, std::vector<std::shared_ptr<statement_t>> > analyse_statements(const analyser_t& a, const std::vector<std::shared_ptr<statement_t>>& statements, const typeid_t& return_type);
std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const expression_t& e, const typeid_t& target_type);
std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const expression_t& e);




//////////////////////////////////////		IMPLEMENTATION





static const function_definition_t& function_id_to_def(const analyser_t& a, const function_id_t& function_id){
//	QUARK_ASSERT(function_id >= 0 && function_id < a._function_defs.size());

	return a._function_defs.at(function_id);
}



/////////////////////////////////////////			RESOLVE SYMBOL USING LEXICAL SCOPE PATH



//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
static std::pair<const symbol_t*, variable_address_t> resolve_env_variable_deep(const analyser_t& a, int depth, const std::string& s){
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
		return { &it->second, variable_address_t::make_variable_address(parent_index, variable_index) };
	}
	else if(depth > 0){
		return resolve_env_variable_deep(a, depth - 1, s);
	}
	else{
		return { nullptr, variable_address_t::make_variable_address(0, 0) };
	}
}
//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
static std::pair<const symbol_t*, variable_address_t> find_symbol_by_name(const analyser_t& a, const std::string& s){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(a, static_cast<int>(a._lexical_scope_stack.size() - 1), s);
}

static bool does_symbol_exist_shallow(const analyser_t& a, const std::string& s){
    const auto it = std::find_if(
    	a._lexical_scope_stack.back().symbols._symbols.begin(),
    	a._lexical_scope_stack.back().symbols._symbols.end(),
    	[&s](const std::pair<std::string, symbol_t>& e) { return e.first == s; }
	);
	return it != a._lexical_scope_stack.back().symbols._symbols.end();
}

static itype_t get_named_symbol(const symbol_t& symbol){
	QUARK_ASSERT(symbol._symbol_type == symbol_t::symbol_type::named_type);

	return symbol._value_type;
}

static const std::pair<type_tag_t, typeid_t>& get_tagged_tag(const analyser_t& a, const std::string& identifier){
	const std::pair<const symbol_t*, variable_address_t> symbol_kv = find_symbol_by_name(a, identifier);
	QUARK_ASSERT(symbol_kv.first != nullptr && symbol_kv.first->_symbol_type == symbol_t::symbol_type::named_type);

	const auto itype = symbol_kv.first->get_value_type();
	const auto& info = lookup_typeinfo_from_itype(a._types, itype);
	QUARK_ASSERT((info.first == make_empty_type_tag()) == false);
	return info;
}




/////////////////////////////////////////			RESOLVE typeid_t::unresolved_t USING LEXICAL SCOPE PATH

static typeid_t record_type(analyser_t& acc, const location_t& loc, const typeid_t& type);

static typeid_t record_type_internal(analyser_t& acc, const location_t& loc, const typeid_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	//	NOTICE: Add parent type *first* so we can support recursive type definitions, like "struct node_t { node_t left, node_t right }
	//	NOTICE: All basic types are already interned.
	struct visitor_t {
		analyser_t& acc;
		const location_t& loc;
		const typeid_t& type;


		typeid_t operator()(const typeid_t::undefined_t& e) const{
			return type;
//			throw_compiler_error(loc, "Cannot resolve type");
		}
		typeid_t operator()(const typeid_t::any_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::void_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::bool_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::int_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::double_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::string_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::json_type_t& e) const{
			return type;
		}
		typeid_t operator()(const typeid_t::typeid_type_t& e) const{
			return type;
		}

		typeid_t operator()(const typeid_t::struct_t& e) const{
			const auto& struct_def = type.get_struct();
			std::vector<member_t> members2;
			for(const auto& m: struct_def._members){
				members2.push_back(member_t(record_type(acc, loc, m._type), m._name));
			}
			return typeid_t::make_struct2(members2);
		}
		typeid_t operator()(const typeid_t::vector_t& e) const{
			return typeid_t::make_vector(record_type(acc, loc, type.get_vector_element_type()));
		}
		typeid_t operator()(const typeid_t::dict_t& e) const{
			return typeid_t::make_dict(record_type(acc, loc, type.get_dict_value_type()));
		}
		typeid_t operator()(const typeid_t::function_t& e) const{
			const auto ret = type.get_function_return();
			const auto args = type.get_function_args();
			const auto pure = type.get_function_pure();
			const auto dyn_return_type = type.get_function_dyn_return_type();

			const auto ret2 = record_type(acc, loc, ret);
			vector<typeid_t> args2;
			for(const auto& m: args){
				args2.push_back(record_type(acc, loc, m));
			}
			return typeid_t::make_function3(ret2, args2, pure, dyn_return_type);
		}
		typeid_t operator()(const typeid_t::identifier_t& e) const{
			const auto identifier = type.get_identifier();
			QUARK_ASSERT(identifier != "");

			const auto existing_value_deep_ptr = find_symbol_by_name(acc, identifier);
			if(existing_value_deep_ptr.first == nullptr || existing_value_deep_ptr.first->_symbol_type != symbol_t::symbol_type::named_type){
				throw_compiler_error(loc, "Unknown type name '" + identifier + "'.");
			}
			const auto resolved = lookup_type_from_itype(acc._types, get_named_symbol(*existing_value_deep_ptr.first));
			return resolved;
		}
	};
	const auto resolved = std::visit(visitor_t{ acc, loc, type }, type._contents);
	return intern_anonymous_type(acc._types, nullptr, resolved).second;
}


itype_t analyser_t::i_resolve_identifer_resolve(const std::string& identifier) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(identifier != "");

	const auto existing_value_deep_ptr = find_symbol_by_name(*this, identifier);
	if(existing_value_deep_ptr.first == nullptr || existing_value_deep_ptr.first->_symbol_type != symbol_t::symbol_type::named_type){
		throw_compiler_error(k_no_location, "Unknown type name '" + identifier + "'.");
	}
	return get_named_symbol(*existing_value_deep_ptr.first);
}



static typeid_t record_type(analyser_t& acc, const location_t& loc, const typeid_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	try {
//		const auto resolved = record_type_internal(acc, loc, type);

		if(type.is_undefined()){
			return type;
		}
		else{
#if DEBUG
			if(false) trace_type_interner(acc._types);
#endif
			const auto resolved = intern_anonymous_type(acc._types, &acc, type).second;

/*
			if(check_types_resolved(acc._types, resolved) == false){
				throw_compiler_error(loc, "Cannot resolve type");
			}
*/

			lookup_itype_from_typeid(acc._types, type);

			return resolved;
		}
	}
	catch(const compiler_error& e){
		throw;
	}
	catch(const std::exception& e){
		throw_compiler_error(loc, e.what());
	}
}

static typeid_t record_type(analyser_t& acc, const location_t& loc, const ast_type_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return record_type(acc, loc, get_typeid(type));
}

static itype_t record_type_itype(analyser_t& acc, const location_t& loc, const typeid_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto t = record_type(acc, loc, type);
	return lookup_itype_from_typeid(acc._types, t);
}

static itype_t record_type_itype(analyser_t& acc, const location_t& loc, const ast_type_t& type){
	QUARK_ASSERT(acc.check_invariant());
	QUARK_ASSERT(loc.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	return record_type_itype(acc, loc,get_typeid(type));
}


//	Pushes the expression type through recording, resolving and interning.
static itype_t analyze_expr_output_itype(analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	return record_type_itype(a, k_no_location, e.get_output_type());
}

//	Pushes the expression type through recording, resolving and interning.
static typeid_t analyze_expr_output_type(analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto itype = analyze_expr_output_itype(a, e);
	return lookup_type_from_itype(a._types, itype);
}




//	When callee has "any" as return type, we need to figure out its return type using its algorithm and the actual types
static const typeid_t figure_out_callee_return_type(analyser_t& a, const statement_t& parent, const typeid_t& callee_type, const std::vector<expression_t>& call_args){
	const auto callee_return_type = callee_type.get_function_return();

	const auto ret_type_algo = callee_type.get_function_dyn_return_type();
	switch(ret_type_algo){
		case typeid_t::return_dyn_type::none:
			{
				QUARK_ASSERT(callee_return_type.is_any() == false);
				return callee_return_type;
			}
			break;

		case typeid_t::return_dyn_type::arg0:
			{
				QUARK_ASSERT(call_args.size() > 0);

				return analyze_expr_output_type(a, call_args[0]);
			}
			break;
		case typeid_t::return_dyn_type::arg1:
			{
				QUARK_ASSERT(call_args.size() >= 2);

				return analyze_expr_output_type(a, call_args[1]);
			}
			break;
		case typeid_t::return_dyn_type::arg1_typeid_constant_type:
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
				const size_t scope_index = addr._parent_steps == variable_address_t::k_global_scope ? 0 : a._lexical_scope_stack.size() - 1 - addr._parent_steps;
				const auto& symbol_kv = a._lexical_scope_stack[scope_index].symbols._symbols[addr._index];

				QUARK_ASSERT(symbol_kv.second._symbol_type == symbol_t::symbol_type::named_type);
				return lookup_type_from_itype(a._types, get_named_symbol(symbol_kv.second));
			}
			break;
		case typeid_t::return_dyn_type::vector_of_arg1func_return:
			{
		//	x = make_vector(arg1.get_function_return());
				QUARK_ASSERT(call_args.size() >= 2);

				const auto f = analyze_expr_output_type(a, call_args[1]).get_function_return();
				const auto ret = typeid_t::make_vector(f);
				return ret;
			}
			break;
		case typeid_t::return_dyn_type::vector_of_arg2func_return:
			{
		//	x = make_vector(arg2.get_function_return());
			QUARK_ASSERT(call_args.size() >= 3);
			const auto f = analyze_expr_output_type(a, call_args[2]).get_function_return();
			const auto ret = typeid_t::make_vector(f);
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
	typeid_t function_type;
};

/*
	Generates code for computing arguments and figuring out each argument type.
	Checks the call vs the callee_type.
	It matches up if the callee uses ANY for argument(s) or return type.
	Returns the computed call arguments and the final, resolved function type which also matches the arguments.

	Throws errors on type mismatches.
*/
static std::pair<analyser_t, fully_resolved_call_t> analyze_resolve_call_type(const analyser_t& a, const statement_t& parent, const vector<expression_t>& call_args, const typeid_t& callee_type){
	const std::vector<typeid_t> callee_arg_types = callee_type.get_function_args();

	auto a_acc = a;

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

	const auto callee_return_type = figure_out_callee_return_type(a_acc, parent, callee_type, call_args2);
	std::vector<typeid_t> resolved_arg_types;
	for(const auto& e: call_args2){
		resolved_arg_types.push_back(analyze_expr_output_type(a_acc, e));
	}
	const auto resolved_function_type = typeid_t::make_function(callee_return_type, resolved_arg_types, callee_type.get_function_pure());
	return { a_acc, { call_args2, resolved_function_type } };
}





/////////////////////////////////////////			STATEMENTS



std::pair<analyser_t, vector<statement_t>> analyse_statements(const analyser_t& a, const vector<statement_t>& statements, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());
	for(const auto& i: statements){ QUARK_ASSERT(i.check_invariant()); };

	auto a_acc = a;

	vector<statement_t> statements2;
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

std::pair<analyser_t, body_t> analyse_body(const analyser_t& a, const body_t& body, epure pure, const typeid_t& return_type){
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
			const auto lhs_type = lookup_type_from_itype(a_acc._types, existing_value_deep_ptr.first->get_value_type());
			QUARK_ASSERT(check_types_resolved(a_acc._types, lhs_type));

			const auto rhs_expr2 = analyse_expression_to_target(a_acc, s, statement._expression, lhs_type);
			a_acc = rhs_expr2.first;
			const auto rhs_expr3 = rhs_expr2.second;

			if(lhs_type != analyze_expr_output_type(a_acc, rhs_expr3)){
				std::stringstream what;
				what << "Types not compatible in assignment - cannot convert '"
				<< typeid_to_compact_string(analyze_expr_output_type(a_acc, rhs_expr3)) << "' to '" << typeid_to_compact_string(lhs_type) << ".";
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

	const auto statement = std::get<statement_t::bind_local_t>(s._contents);
	auto a_acc = a;

	const auto new_local_name = statement._new_local_name;

#if DEBUG
	if(true) trace_type_interner(a_acc._types);
#endif

	//	If lhs may be
	//		(1) undefined, if input is "let a = 10" for example. Then we need to infer its type.
	//		(2) have a type, but it might not be fully resolved yet.
	const auto lhs_itype = record_type_itype(a_acc, s.location, statement._bindtype);

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
			: analyse_expression_to_target(a_acc, s, statement._expression, lookup_type_from_itype(a_acc._types, lhs_itype));
		a_acc = rhs_expr_pair.first;

		const auto rhs_itype = analyze_expr_output_itype(a_acc, rhs_expr_pair.second);
		const auto lhs_itype2 = lhs_itype.is_undefined() ? rhs_itype : lhs_itype;

		//??? make test that checks I got this test + error right.
		if((lhs_itype2 == rhs_itype) == false){
			std::stringstream what;
			what << "Types not compatible in bind - cannot convert '"
			<< typeid_to_compact_string(lookup_type_from_itype(a_acc._types, lhs_itype)) << "' to '" << typeid_to_compact_string(lookup_type_from_itype(a_acc._types, lhs_itype2)) << ".";
			throw_compiler_error(s.location, what.str());
		}
		else{
			//	Replace the temporary symbol with the complete symbol.

			//	If symbol can be initialized directly, use make_immutable_precalc(). Else reserve it and create an init2-statement to set it up at runtime.
			//	??? Better to always initialise it, even if it's a complex value. Codegen then decides if to translate to a reserve + init. BUT PROBLEM: we lose info *when* to init the value.
			if(is_preinitliteral(lookup_type_from_itype(a_acc._types, lhs_itype2)) && mutable_flag == false && get_expression_type(rhs_expr_pair.second) == expression_type::k_literal){
				const auto symbol2 = symbol_t::make_immutable_precalc(lhs_itype2, rhs_expr_pair.second.get_literal());
				a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = { new_local_name, symbol2 };
				record_type(a_acc, s.location, analyze_expr_output_type(a_acc, rhs_expr_pair.second));
				return { a_acc, {} };
			}
			else{
				const auto symbol2 = mutable_flag ? symbol_t::make_mutable(lhs_itype2) : symbol_t::make_immutable_reserve(lhs_itype2);
				a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = { new_local_name, symbol2 };
				record_type(a_acc, s.location, analyze_expr_output_type(a_acc, rhs_expr_pair.second));

				return {
					a_acc,
					std::make_shared<statement_t>(
						statement_t::make__init2(
							s.location,
							variable_address_t::make_variable_address(0, (int)local_name_index),
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

std::pair<analyser_t, statement_t> analyse_block_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::block_statement_t>(s._contents);
	const auto e = analyse_body(a, statement._body, a._lexical_scope_stack.back().pure, return_type);
	return {e.first, statement_t::make__block_statement(s.location, e.second)};
}

std::pair<analyser_t, statement_t> analyse_return_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	if(return_type.is_void()){
		std::stringstream what;
		what << "Cannot return value from function with void-return.";
		throw_compiler_error(s.location, what.str());
	}

	const auto statement = std::get<statement_t::return_statement_t>(s._contents);
	auto a_acc = a;
	const auto expr = statement._expression;
	const auto result = analyse_expression_to_target(a_acc, s, expr, return_type);

	a_acc = result.first;

	const auto result_value = result.second;

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return { a_acc, statement_t::make__return_statement(s.location, result.second) };
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::ifelse_statement_t>(s._contents);
	auto a_acc = a;

	const auto condition2 = analyse_expression_no_target(a_acc, s, statement._condition);
	a_acc = condition2.first;

	const auto condition_type = analyze_expr_output_type(a_acc, condition2.second);
	if(condition_type.is_bool() == false){
		std::stringstream what;
		what << "Boolean condition required.";
		throw_compiler_error(s.location, what.str());
	}

	const auto then2 = analyse_body(a_acc, statement._then_body, a._lexical_scope_stack.back().pure, return_type);
	const auto else2 = analyse_body(a_acc, statement._else_body, a._lexical_scope_stack.back().pure, return_type);
	return { a_acc, statement_t::make__ifelse_statement(s.location, condition2.second, then2.second, else2.second) };
}

std::pair<analyser_t, statement_t> analyse_for_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::for_statement_t>(s._contents);
	auto a_acc = a;

	const auto start_expr2 = analyse_expression_no_target(a_acc, s, statement._start_expression);
	a_acc = start_expr2.first;

	if(analyze_expr_output_type(a_acc, start_expr2.second).is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, start type is " <<  typeid_to_compact_string(analyze_expr_output_type(a_acc, start_expr2.second)) << ".";
		throw_compiler_error(s.location, what.str());
	}

	const auto end_expr2 = analyse_expression_no_target(a_acc, s, statement._end_expression);
	a_acc = end_expr2.first;

	if(analyze_expr_output_type(a_acc, end_expr2.second).is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, end type is " <<  typeid_to_compact_string(analyze_expr_output_type(a_acc, end_expr2.second)) << ".";
		throw_compiler_error(s.location, what.str());
	}

	const auto iterator_symbol = symbol_t::make_immutable_reserve(itype_t::make_int());

	//	Add the iterator as a symbol to the body of the for-loop.
	auto symbols = statement._body._symbol_table;
	symbols._symbols.push_back({ statement._iterator_name, iterator_symbol});
	const auto body_injected = body_t(statement._body._statements, symbols);
	const auto result = analyse_body(a_acc, body_injected, a._lexical_scope_stack.back().pure, return_type);
	a_acc = result.first;

	return { a_acc, statement_t::make__for_statement(s.location, statement._iterator_name, start_expr2.second, end_expr2.second, result.second, statement._range_type) };
}

std::pair<analyser_t, statement_t> analyse_while_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::while_statement_t>(s._contents);
	auto a_acc = a;

	const auto condition2_expr = analyse_expression_to_target(a_acc, s, statement._condition, typeid_t::make_bool());
	a_acc = condition2_expr.first;

	const auto result = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure, return_type);
	a_acc = result.first;

	return { a_acc, statement_t::make__while_statement(s.location, condition2_expr.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_expression_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::expression_statement_t>(s._contents);
	auto a_acc = a;
	const auto expr2 = analyse_expression_no_target(a_acc, s, statement._expression);
	a_acc = expr2.first;

	return { a_acc, statement_t::make__expression_statement(s.location, expr2.second) };
}


//	Make new global function containing the body of the benchmark-def.
//	Add the function as an entry in the global benchmark registry.
static analyser_t analyse_benchmark_def_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::benchmark_def_statement_t>(s._contents);
	auto a_acc = a;

	const auto test_name = statement.name;
	const auto function_link_name = "benchmark__" + test_name;

	const auto benchmark_def_t_struct = make_benchmark_def_t();
	const auto benchmark_function_t = make_benchmark_function_t();
	const auto function_id = function_id_t { function_link_name };

	//	Make a function def expression for the new benchmark function.

	const auto body_pair = analyse_body(a_acc, statement._body, epure::pure, benchmark_function_t.get_function_return());
	a_acc = body_pair.first;


	const auto function_def2 = function_definition_t::make_func(k_no_location, function_link_name, benchmark_function_t, {}, std::make_shared<body_t>(body_pair.second));
	QUARK_ASSERT(check_types_resolved(a_acc._types, function_def2));

	a_acc._function_defs.insert({ function_id, function_definition_t(function_def2) });

	const auto f = value_t::make_function_value(benchmark_function_t, function_id);


	//	Add benchmark-def record to benchmark_defs.

	{//???named-type -- name this type
		const auto new_record_expr = expression_t::make_construct_value_expr(
			make_type_name_from_typeid(benchmark_def_t_struct),
			{
				expression_t::make_literal_string(test_name),
				expression_t::make_literal(f)
			}
		);
		const auto new_record_expr3_pair = analyse_expression_to_target(a_acc, s, new_record_expr, benchmark_def_t_struct);
		a_acc = new_record_expr3_pair.first;
		a_acc.benchmark_defs.push_back(new_record_expr3_pair.second);
	}

	const auto body2 = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure, benchmark_function_t.get_function_return());
	a_acc = body2.first;
	return a_acc;
}

//	Output is the RETURN VALUE of the analysed statement, if any.
std::pair<analyser_t, shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(statement.check_invariant());

	typedef std::pair<analyser_t, shared_ptr<statement_t>> return_type_t;

	struct visitor_t {
		const analyser_t& a;
		const statement_t& statement;
		const typeid_t return_type;


		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::return_statement_t& s) const{
			const auto e = analyse_return_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::bind_local_t& s) const{
			const auto e = analyse_bind_local_statement(a, statement);
			if(e.second){
				QUARK_ASSERT(check_types_resolved(e.first._types, *e.second));
			}
			return { e.first, e.second };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::assign_t& s) const{
			const auto e = analyse_assign_statement(a, statement);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::assign2_t& s) const{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::init2_t& s) const{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::block_statement_t& s) const{
			const auto e = analyse_block_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::ifelse_statement_t& s) const{
			const auto e = analyse_ifelse_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::for_statement_t& s) const{
			const auto e = analyse_for_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::while_statement_t& s) const{
			const auto e = analyse_while_statement(a, statement, return_type);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}


		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::expression_statement_t& s) const{
			const auto e = analyse_expression_statement(a, statement);
			QUARK_ASSERT(check_types_resolved(e.first._types, e.second));
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::software_system_statement_t& s) const{
			analyser_t temp = a;
			temp._software_system = parse_software_system_json(s._json_data);
			return { temp, {} };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::container_def_statement_t& s) const{
			analyser_t temp = a;
			temp._container_def = parse_container_def_json(s._json_data);
			return { temp, {} };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::benchmark_def_statement_t& s) const{
			const auto e = analyse_benchmark_def_statement(a, statement, return_type);
//			QUARK_ASSERT(check_types_resolved(e.second));
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

	const auto parent_itype = analyze_expr_output_itype(a_acc, parent_expr.second);

	const auto parent_type = expand_type_description(a_acc._types, parent_itype);
	if(parent_type.is_struct()){
		const auto struct_def = parent_type.get_struct();

		//??? store index in new expression
		int index = find_struct_member_index(struct_def, details.member_name);
		if(index == -1){
			std::stringstream what;
			what << "Unknown struct member \"" + details.member_name + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		const auto member_type = struct_def._members[index]._type;
		return { a_acc, expression_t::make_resolve_member(parent_expr.second, details.member_name, make_type_name_from_typeid(member_type))};
	}
	else{
		std::stringstream what;
		what << "Left hand side is not a struct value, it's of type \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_intrinsic_update_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());

	const auto sign = make_update_signature();
	auto a_acc = a;
	const auto collection_expr = analyse_expression_no_target(a_acc, parent, args[0]);
	a_acc = collection_expr.first;

	//	IMPORTANT: For structs we manipulate the key-expression. We can't analyse key expression - it's encoded as a variable resolve.

	const auto new_value_expr = analyse_expression_no_target(a_acc, parent, args[2]);
	a_acc = new_value_expr.first;
	const auto collection_type = analyze_expr_output_type(a_acc, collection_expr.second);


	const auto& key = args[1];
	const auto new_value_type = analyze_expr_output_type(a_acc, new_value_expr.second);

	if(collection_type.is_struct()){
		const auto struct_def = collection_type.get_struct();

		//	The key needs to be the name of an identifier. It's a compile-time constant.
		//	It's encoded as a load which is confusing.

		if(get_expression_type(key) == expression_type::k_load){
			const auto member_name = std::get<expression_t::load_t>(key._expression_variant).variable_name;
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

			return {
				a_acc,
				expression_t::make_update_member(collection_expr.second, member_index, new_value_expr.second, make_type_name_from_typeid(collection_type))
			};
		}
		else{
			std::stringstream what;
			what << "Struct member needs to be a string literal.";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else if(collection_type.is_string()){
		const auto key_expr = analyse_expression_no_target(a_acc, parent, key);
		a_acc = key_expr.first;
		const auto key_type = analyze_expr_output_type(a_acc, key_expr.second);

		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Updating string needs an integer index, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}

		if(new_value_type.is_int() == false){
			std::stringstream what;
			what << "Updating string needs an integer value, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}

		return {
			a_acc,
			expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr.second, key_expr.second, new_value_expr.second }, make_type_name_from_typeid(collection_type))
		};
	}
	else if(collection_type.is_vector()){
		const auto key_expr = analyse_expression_no_target(a_acc, parent, key);
		a_acc = key_expr.first;
		const auto key_type = analyze_expr_output_type(a_acc, key_expr.second);

		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Updating vector needs and integer index, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}

		const auto element_type = collection_type.get_vector_element_type();
		if(element_type != new_value_type){
			throw_compiler_error(parent.location, "New value's type must match vector's element type.");
		}

		return {
			a_acc,
			expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr.second, key_expr.second, new_value_expr.second }, make_type_name_from_typeid(collection_type))
		};
	}
	else if(collection_type.is_dict()){
		const auto key_expr = analyse_expression_no_target(a_acc, parent, key);
		a_acc = key_expr.first;
		const auto key_type = analyze_expr_output_type(a_acc, key_expr.second);

		if(key_type.is_string() == false){
			std::stringstream what;
			what << "Updating dictionary requires string key, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}

		const auto element_type = collection_type.get_dict_value_type();
		if(element_type != new_value_type){
			throw_compiler_error(parent.location, "New value's type must match dict's value type.");
		}

		return {
			a_acc,
			expression_t::make_intrinsic(get_intrinsic_opcode(sign), { collection_expr.second, key_expr.second, new_value_expr.second }, make_type_name_from_typeid(collection_type))
		};
	}

	else{
		std::stringstream what;
		what << "Left hand side does not support update() - it's of type \"" + typeid_to_compact_string(collection_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_intrinsic_push_back_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());

	const auto sign = make_push_back_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto value_type = resolved_call.second.function_type.get_function_args()[1];

	if(parent_type.is_string()){
		if(value_type.is_int() == false){
			std::stringstream what;
			what << "string push_back() needs an integer element, not a \"" + typeid_to_compact_string(value_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else if(parent_type.is_vector()){
		if(value_type != parent_type.get_vector_element_type()){
			std::stringstream what;
			what << "Vector push_back() has mismatching element type vs supplies a \"" + typeid_to_compact_string(value_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
	else{
		std::stringstream what;
		what << "Left hand side does not support push_back() - it's of type \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(parent_type))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_size_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_size_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	if(parent_type.is_string() || parent_type.is_json() || parent_type.is_vector() || parent_type.is_dict()){
	}
	else{
		std::stringstream what;
		what << "Left hand side does not support size() - it's of type \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_find_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_find_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto wanted_type = resolved_call.second.function_type.get_function_args()[1];

	if(parent_type.is_string()){
		if(wanted_type.is_string() == false){
			throw_compiler_error(parent.location, "find() requires argument 2 to be a string.");
		}
	}
	else if(parent_type.is_vector()){
		if(wanted_type != parent_type.get_vector_element_type()){
			throw_compiler_error(parent.location, "find([]) requires argument 2 to be of vector's element type.");
		}
	}
	else{
		std::stringstream what;
		what << "Function find() doesn not work on type \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_exists_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_exists_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto wanted_type = resolved_call.second.function_type.get_function_args()[1];

	if(parent_type.is_dict() == false){
		throw_compiler_error(parent.location, "exists() requires a dictionary.");
	}
	if(wanted_type.is_string() == false){
		throw_compiler_error(parent.location, "exists() requires a string key.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_erase_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_erase_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto key_type = resolved_call.second.function_type.get_function_args()[1];

	if(parent_type.is_dict() == false){
		throw_compiler_error(parent.location, "erase() requires a dictionary.");
	}
	if(key_type.is_string() == false){
		throw_compiler_error(parent.location, "erase() requires a string key.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

//??? const statement_t& parent == very confusing. Rename parent => statement!

std::pair<analyser_t, expression_t> analyse_intrinsic_get_keys_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_get_keys_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];

	if(parent_type.is_dict() == false){
		throw_compiler_error(parent.location, "get_keys() requires a dictionary.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_subset_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_subset_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto key_type = resolved_call.second.function_type.get_function_args()[1];

	if(parent_type.is_string() == false && parent_type.is_vector() == false){
		throw_compiler_error(parent.location, "subset([]) requires a string or a vector.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_replace_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_replace_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto parent_type = resolved_call.second.function_type.get_function_args()[0];
	const auto replace_with_type = resolved_call.second.function_type.get_function_args()[3];

	if(parent_type != replace_with_type){
		throw_compiler_error(parent.location, "replace() requires argument 4 to be same type of collection.");
	}

	if(parent_type.is_string() == false && parent_type.is_vector() == false){
		throw_compiler_error(parent.location, "replace([]) requires a string or a vector.");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}
//??? pass around location_t instead of statement_t& parent!



//	[R] map([E] elements, func R (E e, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_map_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_map_func_type(resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", needs to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

//	string map_string(string s, func string(string e, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_string_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_map_string_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_map_string_func_type(resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map_string() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", needs to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_map_dag_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_map_dag_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_map_dag_func_type(resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to map_dag() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", needs to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

std::pair<analyser_t, expression_t> analyse_intrinsic_filter_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_filter_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_filter_func_type(resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to filter() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", expected to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_reduce_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	auto a_acc = a;

	const auto sign = make_reduce_signature();
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_reduce_func_type(resolved_call.second.function_type);
	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to reduce() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", expected to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
	};
}

//	[T] stable_sort([T] elements, bool less(T left, T right, C context), C context)
std::pair<analyser_t, expression_t> analyse_intrinsic_stable_sort_expression(const analyser_t& a, const statement_t& parent, const std::vector<expression_t>& args){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());

	const auto sign = make_stable_sort_signature();
	auto a_acc = a;
	const auto resolved_call = analyze_resolve_call_type(a_acc, parent, args, sign._function_type);
	a_acc = resolved_call.first;

	const auto expected = harden_stable_sort_func_type(resolved_call.second.function_type);

	if(resolved_call.second.function_type != expected){
		throw_compiler_error(parent.location, "Call to stable_sort() uses signature \"" + typeid_to_compact_string(resolved_call.second.function_type) + "\", needs to be \"" + typeid_to_compact_string(expected) + "\".");
	}

	return {
		a_acc,
		expression_t::make_intrinsic(get_intrinsic_opcode(sign), resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return()))
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
	const auto key_type = analyze_expr_output_type(a_acc, key_expr.second);

	if(parent_type.is_string()){
		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Strings can only be indexed by integers, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_type_name_from_typeid(typeid_t::make_int())) };
		}
	}
	else if(parent_type.is_json()){
		return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_type_name_from_typeid(typeid_t::make_json())) };
	}
	else if(parent_type.is_vector()){
		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Vector can only be indexed by integers, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_type_name_from_typeid(parent_type.get_vector_element_type())) };
		}
	}
	else if(parent_type.is_dict()){
		if(key_type.is_string() == false){
			std::stringstream what;
			what << "Dictionary can only be looked up using string keys, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_type_name_from_typeid(parent_type.get_dict_value_type())) };
		}
	}
	else {
		std::stringstream what;
		what << "Lookup using [] only works with strings, vectors, dicts and json - not a \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_load(const analyser_t& a, const statement_t& parent,const expression_t& e, const expression_t::load_t& details){
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
				expression_t::make_load2(found.second, make_type_name_from_typeid(typeid_t::make_typeid()))
			};
		}
		else{
			return {
				a_acc,
				expression_t::make_load2(found.second, make_type_name_from_typeid(lookup_type_from_itype(a._types, found.first->_value_type)))
			};
		}
	}
	else{
		const std::vector<intrinsic_signature_t>& intrinsic_signatures = a._imm->intrinsic_signatures;

		const auto it = std::find_if(
			intrinsic_signatures.begin(),
			intrinsic_signatures.end(),
			[&](const intrinsic_signature_t& e) { return e.name == details.variable_name; }
		);
		if(it != intrinsic_signatures.end()){
			const auto index = it - intrinsic_signatures.begin();
			const auto addr = variable_address_t::make_variable_address(variable_address_t::k_intrinsic, static_cast<int32_t>(index));
			const auto e2 = expression_t::make_load2(addr, make_type_name_from_typeid(it->_function_type));
			return { a_acc, e2 };
		}
		else{
			std::stringstream what;
			what << "Undefined variable \"" << details.variable_name << "\".";
			throw_compiler_error(parent.location, what.str());
		}
	}
}

std::pair<analyser_t, expression_t> analyse_load2(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	return { a, e };
}


static typeid_t select_inferred_type(const type_interner_t& interner, const typeid_t& target_type_or_any, const typeid_t& rhs_guess_type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(target_type_or_any.check_invariant());
	QUARK_ASSERT(rhs_guess_type.check_invariant());

	const bool rhs_guess_type_valid = check_types_resolved(interner, rhs_guess_type);
	const bool have_target_type = target_type_or_any.is_any() == false;

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
std::pair<analyser_t, expression_t> analyse_construct_value_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::value_constructor_t& details, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto current_type = analyze_expr_output_type(a_acc, e);

	if(current_type.is_vector()){
		//	JSON constants supports mixed element types: convert each element into a json.
		//	Encode as [json]
		if(target_type.is_json()){
			const auto element_type = typeid_t::make_json();

			std::vector<expression_t> elements2;
			for(const auto& m: details.elements){
				const auto element_expr = analyse_expression_to_target(a_acc, parent, m, element_type);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}
			const auto result_type = typeid_t::make_vector(typeid_t::make_json());
			if(check_types_resolved(a_acc._types, result_type) == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}
			return {
				a_acc,
				expression_t::make_construct_value_expr(
					make_type_name_from_typeid(typeid_t::make_json()),
					{ expression_t::make_construct_value_expr(make_type_name_from_typeid(typeid_t::make_vector(typeid_t::make_json())), elements2) }
				)
			};
		}
		else {
			const auto element_type = current_type.get_vector_element_type();
			std::vector<expression_t> elements2;
			for(const auto& m: details.elements){
				const auto element_expr = analyse_expression_no_target(a_acc, parent, m);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}

			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? analyze_expr_output_type(a_acc, elements2[0]) : element_type;
			const auto rhs_guess_type = typeid_t::make_vector(element_type2);
			const auto selected_type = select_inferred_type(a_acc._types, target_type, rhs_guess_type);

			if(check_types_resolved(a_acc._types, selected_type) == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			const auto final_type = record_type(a_acc, parent.location, selected_type);

			for(const auto& m: elements2){
				if(analyze_expr_output_type(a_acc, m) != element_type2){
					std::stringstream what;
					what << "Vector of type " << typeid_to_compact_string(final_type) << " cannot hold an element of type " << typeid_to_compact_string(analyze_expr_output_type(a_acc, m)) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			QUARK_ASSERT(check_types_resolved(a_acc._types, final_type));
			return { a_acc, expression_t::make_construct_value_expr(make_type_name_from_typeid(final_type), elements2) };
		}
	}

	//	Dicts uses pairs of (string,value). This is stored in _args as interleaved expression: string0, value0, string1, value1.
	else if(current_type.is_dict()){
		//	JSON constants supports mixed element types: convert each element into a json.
		//	Encode as [string:json]
		if(target_type.is_json()){
			const auto element_type = typeid_t::make_json();

			std::vector<expression_t> elements2;
			for(int i = 0 ; i < details.elements.size() / 2 ; i++){
				const auto& key = details.elements[i * 2 + 0].get_literal().get_string_value();
				const auto& value = details.elements[i * 2 + 1];
				const auto element_expr = analyse_expression_to_target(a_acc, parent, value, element_type);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			const auto rhs_guess_type = typeid_t::make_dict(typeid_t::make_json());
			const auto selected_type = select_inferred_type(a_acc._types, target_type, rhs_guess_type);

			if(check_types_resolved(a_acc._types, selected_type) == false){
				std::stringstream what;
				what << "Cannot infer dictionary element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			const auto final_type = record_type(a_acc, parent.location, selected_type);

			return {
				a_acc,
				expression_t::make_construct_value_expr(
					make_type_name_from_typeid(typeid_t::make_json()),
					{ expression_t::make_construct_value_expr(make_type_name_from_typeid(typeid_t::make_dict(typeid_t::make_json())), elements2) }
				)
			};
		}
		else {
			QUARK_ASSERT(details.elements.size() % 2 == 0);

			const auto element_type = current_type.get_dict_value_type();

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
			const auto rhs_guess_type = typeid_t::make_dict(element_type2);
			const auto selected_type = select_inferred_type(a_acc._types, target_type, rhs_guess_type);

			if(check_types_resolved(a_acc._types, selected_type) == false){
				std::stringstream what;
				what << "Cannot infer dictionary element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			const auto final_type = record_type(a_acc, parent.location, selected_type);

			//	Make sure all elements have the correct type.
			for(int i = 0 ; i < elements2.size() / 2 ; i++){
				const auto element_type0 = analyze_expr_output_type(a_acc, elements2[i * 2 + 1]);
				if(element_type0 != element_type2){
					std::stringstream what;
					what << "Dictionary of type " << typeid_to_compact_string(final_type) << " cannot hold an element of type " << typeid_to_compact_string(element_type0) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			QUARK_ASSERT(check_types_resolved(a_acc._types, final_type));
			return {a_acc, expression_t::make_construct_value_expr(make_type_name_from_typeid(final_type), elements2)};
		}
	}
	else if(current_type.is_struct()){
		const auto construct_value_type0 = details.value_type;
		const auto construct_value_type = lookup_type_from_asttype(a_acc._types, construct_value_type0);
		const auto& def = construct_value_type.get_struct();
		const auto struct_constructor_callee_type = typeid_t::make_function(construct_value_type, get_member_types(def._members), epure::pure);
		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, details.elements, struct_constructor_callee_type);
		a_acc = resolved_call.first;
		return { a_acc, expression_t::make_construct_value_expr(construct_value_type0, resolved_call.second.args) };
	}
	else{
		if(details.elements.size() != 1){
			std::stringstream what;
			what << "Construct value of primitive type requires exactly 1 argument.";
			throw_compiler_error(parent.location, what.str());
		}
		const auto construct_value_type0 = details.value_type;
		const auto construct_value_type = lookup_type_from_asttype(a_acc._types, construct_value_type0);
		const auto struct_constructor_callee_type = typeid_t::make_function(construct_value_type, { construct_value_type }, epure::pure);
		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, details.elements, struct_constructor_callee_type);
		a_acc = resolved_call.first;
		return { a_acc, expression_t::make_construct_value_expr(construct_value_type0, resolved_call.second.args) };
	}
}

std::pair<analyser_t, expression_t> analyse_benchmark_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::benchmark_expr_t& details, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());

	auto acc = a;
	const auto body_pair = analyse_body(acc, *details.body, epure::impure, typeid_t::make_void());
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
	if(type.is_int() || type.is_double()){
		return {a_acc, expression_t::make_unary_minus(expr2.second, make_type_name_from_typeid(type))  };
	}
	else{
		std::stringstream what;
		what << "Unary minus don't work on expressions of type \"" << typeid_to_compact_string(type) << "\"" << ", only int and double.";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_conditional_operator_expression(const analyser_t& analyser, const statement_t& parent, const expression_t& e, const expression_t::conditional_t& details){
	QUARK_ASSERT(analyser.check_invariant());

	auto analyser_acc = analyser;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = analyse_expression_no_target(analyser_acc, parent, *details.condition);
	analyser_acc = cond_result.first;

	const auto a = analyse_expression_no_target(analyser_acc, parent, *details.a);
	analyser_acc = a.first;

	const auto b = analyse_expression_no_target(analyser_acc, parent, *details.b);
	analyser_acc = b.first;

	const auto type = analyze_expr_output_type(analyser_acc, cond_result.second);
	if(type.is_bool() == false){
		std::stringstream what;
		what << "Conditional expression needs to be a bool, not a " << typeid_to_compact_string(type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else if(analyze_expr_output_type(analyser_acc, a.second) != analyze_expr_output_type(analyser_acc, b.second)){
		std::stringstream what;
		what << "Conditional expression requires true/false expressions to have the same type, currently " << typeid_to_compact_string(analyze_expr_output_type(analyser_acc, a.second)) << " : " << typeid_to_compact_string(analyze_expr_output_type(analyser_acc, b.second)) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto final_expression_type = analyze_expr_output_type(analyser_acc, a.second);
		return {
			analyser_acc,
			expression_t::make_conditional_operator(
				cond_result.second,
				a.second,
				b.second,
				make_type_name_from_typeid(final_expression_type)
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
		what << "Left and right expressions must be same type in comparison, " << typeid_to_compact_string(lhs_type) << " " << expression_type_to_opcode(op) << typeid_to_compact_string(rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto shared_type = lhs_type;

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
				make_type_name_from_typeid(typeid_t::make_bool())
			)
		};
	}
}



//??? What is difference between literal_exp_t and value_constructor_t?
// Literals: Only a few built-in types can be initialized as immediates in the code and via code segment, the rest needs to be reserved and explicitly initialised at runtime.

std::pair<analyser_t, expression_t> analyse_literal_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::literal_exp_t& details){
	QUARK_ASSERT(a.check_invariant());

	return { a,e };
}




std::pair<analyser_t, expression_t> analyse_arithmetic_expression(const analyser_t& a, const statement_t& parent, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse both inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, parent, *details.lhs);
	a_acc = left_expr.first;

	const auto lhs_type = analyze_expr_output_type(a_acc, left_expr.second);

	//	Make rhs match lhs if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, parent, *details.rhs, lhs_type);
	a_acc = right_expr.first;

	const auto rhs_type = analyze_expr_output_type(a_acc, right_expr.second);


	if(lhs_type != rhs_type){
		std::stringstream what;
		what << "Artithmetics: Left and right expressions must be same type, currently " << typeid_to_compact_string(lhs_type) << " : " << typeid_to_compact_string(rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto shared_type = lhs_type;

		//	bool
		if(shared_type.is_bool()){
			if(
				op == expression_type::k_arithmetic_add
				|| op == expression_type::k_logical_and
				|| op == expression_type::k_logical_or
			){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
			}
			else {
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}
		}

		//	int
		else if(shared_type.is_int()){
			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
		}

		//	double
		else if(shared_type.is_double()){
			if(op == expression_type::k_arithmetic_remainder){
				throw_compiler_error(parent.location, "Modulo operation on double not supported.");
			}
			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
		}

		//	string
		else if(shared_type.is_string()){
			if(op == expression_type::k_arithmetic_add){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
			}
			else{
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
		}

		//	struct
		else if(shared_type.is_struct()){
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

			return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
		}

		//	vector
		else if(shared_type.is_vector()){
			const auto element_type = shared_type.get_vector_element_type();
			if(op == expression_type::k_arithmetic_add){
				return { a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_type_name_from_typeid(shared_type)) };
			}
			else{
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
		}

		//	function
		else if(shared_type.is_function()){
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
	return {
		a_acc,
		expression_t::make_intrinsic(
			get_intrinsic_opcode(sign),
			resolved_call.second.args,
			make_type_name_from_typeid(resolved_call.second.function_type.get_function_return())
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
	const auto callee_type = analyze_expr_output_type(a_acc, callee_expr);
	if(callee_type.is_function()){
		const auto callee_pure = callee_type.get_function_pure();

		if(callsite_pure == epure::pure && callee_pure == epure::impure){
			throw_compiler_error(parent.location, "Cannot call impure function from a pure function.");
		}

		//	Detect use of intrinsics.
		if(callee_expr_load2){
			if(callee_expr_load2->address._parent_steps == variable_address_t::k_intrinsic){

				const std::vector<intrinsic_signature_t>& intrinsic_signatures = a0._imm->intrinsic_signatures;
				const auto index = callee_expr_load2->address._index;
				QUARK_ASSERT(index >= 0 && index < intrinsic_signatures.size());
				const auto& sign = intrinsic_signatures[index];
				const auto& s = sign.name;

				if(s == make_assert_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_assert_signature());
				}
				else if(s == make_to_string_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_to_string_signature());
				}
				else if(s == make_to_pretty_string_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_to_pretty_string_signature());
				}

				else if(s == make_typeof_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_typeof_signature());
				}

				else if(s == make_update_signature().name){
					return analyse_intrinsic_update_expression(a_acc, parent, e, details.args);
				}
				else if(s == make_size_signature().name){
					return analyse_intrinsic_size_expression(a_acc, parent, details.args);
				}
				else if(s == make_find_signature().name){
					return analyse_intrinsic_find_expression(a_acc, parent, details.args);
				}
				else if(s == make_exists_signature().name){
					return analyse_intrinsic_exists_expression(a_acc, parent, details.args);
				}
				else if(s == make_erase_signature().name){
					return analyse_intrinsic_erase_expression(a_acc, parent, details.args);
				}
				else if(s == make_get_keys_signature().name){
					return analyse_intrinsic_get_keys_expression(a_acc, parent, details.args);
				}
				else if(s == make_push_back_signature().name){
					return analyse_intrinsic_push_back_expression(a_acc, parent, details.args);
				}
				else if(s == make_subset_signature().name){
					return analyse_intrinsic_subset_expression(a_acc, parent, details.args);
				}
				else if(s == make_replace_signature().name){
					return analyse_intrinsic_replace_expression(a_acc, parent, details.args);
				}


				else if(s == make_parse_json_script_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_parse_json_script_signature());
				}
				else if(s == make_generate_json_script_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_generate_json_script_signature());
				}
				else if(s == make_to_json_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_to_json_signature());
				}
				else if(s == make_from_json_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_from_json_signature());
				}

				else if(s == make_get_json_type_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_get_json_type_signature());
				}




				else if(s == make_map_signature().name){
					return analyse_intrinsic_map_expression(a_acc, parent, details.args);
				}
				else if(s == make_map_string_signature().name){
					return analyse_intrinsic_map_string_expression(a_acc, parent, details.args);
				}
				else if(s == make_map_dag_signature().name){
					return analyse_intrinsic_map_dag_expression(a_acc, parent, details.args);
				}
				else if(s == make_filter_signature().name){
					return analyse_intrinsic_filter_expression(a_acc, parent, details.args);
				}
				else if(s == make_reduce_signature().name){
					return analyse_intrinsic_reduce_expression(a_acc, parent, details.args);
				}
				else if(s == make_stable_sort_signature().name){
					return analyse_intrinsic_stable_sort_expression(a_acc, parent, details.args);
				}




				else if(s == make_print_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_print_signature());
				}
				else if(s == make_send_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_send_signature());
				}



				else if(s == make_bw_not_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_not_signature());
				}
				else if(s == make_bw_and_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_and_signature());
				}
				else if(s == make_bw_or_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_or_signature());
				}
				else if(s == make_bw_xor_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_xor_signature());
				}
				else if(s == make_bw_shift_left_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_shift_left_signature());
				}
				else if(s == make_bw_shift_right_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_shift_right_signature());
				}
				else if(s == make_bw_shift_right_arithmetic_signature().name){
					return analyse_intrinsic_fallthrough_expression(a_acc, parent, details.args, make_bw_shift_right_arithmetic_signature());
				}

				else{
				}
			}
		}

		const auto resolved_call = analyze_resolve_call_type(a_acc, parent, call_args, callee_type);
		a_acc = resolved_call.first;
		return { a_acc, expression_t::make_call(callee_expr, resolved_call.second.args, make_type_name_from_typeid(resolved_call.second.function_type.get_function_return())) };
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	//	Desugar!
	//	Converts these calls to construct-value-expressions.
	else if(/*callee_type.is_typeid() &&*/ callee_expr_load2){
		const auto& addr = callee_expr_load2->address; 
		const size_t scope_index = addr._parent_steps == variable_address_t::k_global_scope ? 0 : a_acc._lexical_scope_stack.size() - 1 - addr._parent_steps;
		const auto& callee_symbol = a_acc._lexical_scope_stack[scope_index].symbols._symbols[addr._index];

		if(callee_symbol.second._symbol_type == symbol_t::symbol_type::named_type){
			const auto x = get_named_symbol(callee_symbol.second);
			const auto construct_value_type = lookup_type_from_itype(a_acc._types, x);
			const auto construct_value_type2 = construct_value_type;

			//	Convert calls to struct-type into construct-value expression.
			if(construct_value_type2.is_struct()){
				const auto construct_value_expr = expression_t::make_construct_value_expr(make_type_name_from_typeid(construct_value_type), details.args);
				const auto result_pair = analyse_expression_to_target(a_acc, parent, construct_value_expr, construct_value_type2);
				return { result_pair.first, result_pair.second };
			}

			//	One argument for primitive types.
			else{
				const auto construct_value_expr = expression_t::make_construct_value_expr(make_type_name_from_typeid(construct_value_type), details.args);
				const auto result_pair = analyse_expression_to_target(a_acc, parent, construct_value_expr, construct_value_type2);
				return { result_pair.first, result_pair.second };
			}
		}
		else{
		}
	}

	{
		std::stringstream what;
		what << "Cannot call non-function, its type is " << typeid_to_compact_string(callee_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
}

static type_tag_t make_type_tag(analyser_t& a, const std::string& identifier){
	const auto id = a.scope_id_generator++;

	//??? TOOD: user proper hiearchy of lexical scopes, not a flat list of generated ID:s. This requires a name--string in each lexical_scope_t.

	const auto b = std::string() + "lexical_scope" + std::to_string(id);

	return type_tag_t { { b, identifier } };
}

static std::pair<analyser_t, expression_t> analyse_struct_definition_expression(const analyser_t& a, const statement_t& parent, const expression_t& e0, const expression_t::struct_definition_expr_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const std::string identifier = details.name;

	// Add symbol for the new, with undefined. Later store resolved type.
	if(does_symbol_exist_shallow(a_acc, identifier)){
		throw_local_identifier_already_exists(parent.location, identifier);
	}

	const auto name_tag = make_type_tag(a_acc, identifier);
	const auto named_itype = new_tagged_type(a_acc._types, nullptr, name_tag, typeid_t::make_undefined());

	const auto type_name_symbol = symbol_t::make_named_type(named_itype);
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({ identifier, type_name_symbol });

	const auto struct_typeid1 = typeid_t::make_struct2(details.def->_members);
	const auto struct_typeid2 = struct_typeid1;

	//	Update our temporary. Notice that we need to find it again since other types might have been inserted since.
//	const auto symbol2 = symbol_t::make_named_type(lookup_itype_from_typeid(a_acc._types, struct_typeid2));
//	a_acc._lexical_scope_stack.back().symbols._symbols[symbol_index].second = symbol2;

	const auto struct_typeid_value = value_t::make_typeid_value(struct_typeid2);

	update_tagged_type(a_acc._types, name_tag, struct_typeid2);

	const auto r = expression_t::make_literal(struct_typeid_value);

#if DEBUG
	if(true) trace_type_interner(a_acc._types);
#endif

	return { a_acc, r };
}


// ??? Check that function returns always returns a value, if it has a return-type.
std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& analyser, const statement_t& parent, const expression_t& e, const expression_t::function_definition_expr_t& details){
	QUARK_ASSERT(analyser.check_invariant());

	auto a_acc = analyser;

	const auto function_def = details.def;
	const auto function_type2 = record_type(a_acc, parent.location, function_def._function_type);
	const auto function_pure = function_type2.get_function_pure();

	vector<member_t> args2;
	for(const auto& arg: function_def._named_args){
		const auto arg_type2 = record_type(a_acc, parent.location, arg._type);
		args2.push_back(member_t(arg_type2, arg._name));
	}

	//??? Can there be a pure function inside an impure lexical scope?
	const auto pure = function_pure;

	std::shared_ptr<body_t> body_result;
	if(function_def._optional_body){
		//	Make function body with arguments injected FIRST in body as local symbols.
		auto symbol_vec = function_def._optional_body->_symbol_table;
		for(const auto& arg: args2){
			symbol_vec._symbols.push_back({ arg._name , symbol_t::make_immutable_arg(lookup_itype_from_typeid(a_acc._types, arg._type)) });
		}
		const auto function_body2 = body_t(function_def._optional_body->_statements, symbol_vec);

		const auto body_pair = analyse_body(a_acc, function_body2, pure, function_type2.get_function_return());
		a_acc = body_pair.first;
		const auto function_body3 = body_pair.second;
		body_result = std::make_shared<body_t>(function_body3);
	}
	else{
	}

	const auto definition_name = function_def._definition_name;
	const auto function_id = function_id_t { definition_name };

	const auto function_def2 = function_definition_t::make_func(k_no_location, definition_name, function_type2, args2, body_result);
	QUARK_ASSERT(check_types_resolved(a_acc._types, function_def2));

	a_acc._function_defs.insert({ function_id, function_def2 });

	const auto r = expression_t::make_literal(value_t::make_function_value(function_type2, function_id));

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

std::pair<analyser_t, expression_t> analyse_expression__operation_specific(const analyser_t& a, const statement_t& parent, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		const analyser_t& a;
		const statement_t& parent;
		const expression_t& e;
		const typeid_t& target_type;

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

	//	Record all output type, if there is one.
	auto a_acc = result.first;
	if(check_types_resolved(a_acc._types, result.second)){
		record_type(a_acc, parent.location, analyze_expr_output_type(a_acc, result.second));
	}
	return { a_acc, result.second };
}


/*
	- Insert automatic type-conversions from string -> json etc.
*/
expression_t auto_cast_expression_type(analyser_t& a, const expression_t& e, const typeid_t& wanted_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(wanted_type.check_invariant());
	QUARK_ASSERT(wanted_type.is_undefined() == false);

	const auto current_type = analyze_expr_output_type(a, e);

	if(wanted_type.is_any()){
		return e;
	}
	else if(wanted_type.is_string()){
		if(current_type.is_json()){
			return expression_t::make_construct_value_expr(make_type_name_from_typeid(wanted_type), { e });
		}
		else{
			return e;
		}
	}
	else if(wanted_type.is_json()){
		if(current_type.is_int() || current_type.is_double() || current_type.is_string() || current_type.is_bool()){
			return expression_t::make_construct_value_expr(make_type_name_from_typeid(wanted_type), { e });
		}
		else if(current_type.is_vector()){
			return expression_t::make_construct_value_expr(make_type_name_from_typeid(wanted_type), { e });
		}
		else if(current_type.is_dict()){
			return expression_t::make_construct_value_expr(make_type_name_from_typeid(wanted_type), { e });
		}
		else{
			return e;
		}
	}
	else{
		return e;
	}
}

std::string get_expression_name(const expression_t& e){
	const expression_type op = get_expression_type(e);
	return expression_type_to_opcode(op);
}


//	Return new expression where all types have been resolved. The target-type is used as a hint for type inference.
//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(target_type.is_void() == false && target_type.is_undefined() == false);
	QUARK_ASSERT(check_types_resolved(a._types, target_type));

	auto a_acc = a;
	const auto e2_pair = analyse_expression__operation_specific(a_acc, parent, e, target_type);
	a_acc = e2_pair.first;
	const auto e3 = e2_pair.second;
	if(floyd::check_types_resolved(a_acc._types, e3) == false){
		std::stringstream what;
		what << "Cannot infer type in " << get_expression_name(e3) << "-expression.";
		throw_compiler_error(parent.location, what.str());
	}

	const auto e4 = auto_cast_expression_type(a_acc, e3, target_type);

	if(target_type.is_any()){
	}
	else if(analyze_expr_output_type(a_acc, e4) == target_type){
	}
	else if(analyze_expr_output_type(a_acc, e4).is_undefined()){
		QUARK_ASSERT(false);
		throw_compiler_error(parent.location, "Expression type mismatch.");
	}
	else{
		std::stringstream what;
		what << "Expression type mismatch - cannot convert '"
		//??? missing a trailing '
		<< typeid_to_compact_string(analyze_expr_output_type(a_acc, e4)) << "' to '" << typeid_to_compact_string(target_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}

	if(floyd::check_types_resolved(a_acc._types, e4) == false){
		throw_compiler_error(parent.location, "Cannot resolve type.");
	}
	QUARK_ASSERT(floyd::check_types_resolved(a_acc._types, e4));
	return { a_acc, e4 };
}

//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const expression_t& e){
	return analyse_expression_to_target(a, parent, e, typeid_t::make_any());
}

void test__analyse_expression(const statement_t& parent, const expression_t& e, const expression_t& expected){
	const unchecked_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter, parent, e);

	ut_verify(QUARK_POS, expression_to_json(e3.second), expression_to_json(expected));
}


QUARK_TEST("analyse_expression_no_target()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		statement_t::make__bind_local(k_no_location, "xyz", ast_type_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_TEST("analyse_expression_no_target()", "1 + 2 == 3", "", "") {
	const unchecked_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(
		interpreter,
		statement_t::make__bind_local(k_no_location, "xyz", ast_type_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_arithmetic(
			expression_type::k_arithmetic_add,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			make_no_type_name()
		)
	);

	ut_verify(QUARK_POS,
		expression_to_json(e3.second),
		parse_json(seq_t(R"(   ["+", ["k", 1, "^int"], ["k", 2, "^int"], "^int"]   )")).first
	);
}





static std::pair<std::string, symbol_t> make_builtin_type(type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto bt = type.get_base_type();
	const auto opcode = base_type_to_opcode(bt);
	return { opcode, symbol_t::make_named_type(lookup_itype_from_typeid(interner, type)) };
}


static std::vector<std::pair<std::string, symbol_t>> generate_builtins(analyser_t& a, const analyzer_imm_t& input){
	/*
		Create built-in global symbol map: built in data types, built-in functions (intrinsics).
	*/
	auto& symbol_map = a._lexical_scope_stack.back().symbols._symbols;
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_void()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_bool()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_int()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_double()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_string()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_typeid()) );
	symbol_map.push_back( make_builtin_type(a._types, typeid_t::make_json()) );

	//	"null" is equivalent to json::null
	symbol_map.push_back( { "null", symbol_t::make_immutable_precalc(itype_t::make_json(), value_t::make_json(json_t())) });

	symbol_map.push_back( { "json_object", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(1)) });
	symbol_map.push_back( { "json_array", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(2)) });
	symbol_map.push_back( { "json_string", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(3)) });
	symbol_map.push_back( { "json_number", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(4)) });
	symbol_map.push_back( { "json_true", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(5)) });
	symbol_map.push_back( { "json_false", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(6)) });
	symbol_map.push_back( { "json_null", symbol_t::make_immutable_precalc(itype_t::make_int(), value_t::make_int(7)) });



	symbol_map.push_back( {
		"benchmark_result_t",
		symbol_t::make_named_type(
			new_tagged_type(a._types, &a, make_type_tag(a, "benchmark_result_t"), make_benchmark_result_t())
		)
	} );

	symbol_map.push_back( {
		"benchmark_def_t",
		symbol_t::make_named_type(
			new_tagged_type(a._types, &a, make_type_tag(a, "benchmark_def_t"), make_benchmark_def_t())
		)
	} );



	//	Reserve a symbol table entry for benchmark_registry instance.
	{
		const auto benchmark_registry_type = typeid_t::make_vector(typeid_t::make_identifier("benchmark_def_t"));
		symbol_map.push_back( {
			k_global_benchmark_registry,
			symbol_t::make_immutable_reserve(
				record_type_itype(a, k_no_location, benchmark_registry_type)
			)
		} );
	}

	return symbol_map;
}




semantic_ast_t analyse(analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	////////////////////////////////	Create built-in global symbol map: built in data types and intrinsics.
	////////////////////////////////	Analyze global namespace, including all Floyd functions defined there.




	auto global_body = body_t(a._imm->_ast._tree._globals._statements, symbol_table_t{ });

	auto new_environment = symbol_table_t{ global_body._symbol_table };
	const auto lexical_scope = lexical_scope_t{ new_environment, epure::pure };
	a._lexical_scope_stack.push_back(lexical_scope);

	const auto builtin_symbols = generate_builtins(a, *a._imm);
	for(const auto& e: a._imm->intrinsic_signatures){
		record_type(a, k_no_location, e._function_type);
	}

	if(false) trace_type_interner(a._types);

	const auto result = analyse_statements(a, global_body._statements, typeid_t::make_void());
	a = result.first;

	const auto body2 = body_t(result.second, result.first._lexical_scope_stack.back().symbols);

	a._lexical_scope_stack.pop_back();

	auto global_body3 = body2;


	//	Add Init benchmark_registry.
	{
		const auto it = std::find_if(
			global_body3._symbol_table._symbols.begin(),
			global_body3._symbol_table._symbols.end(),
			[&](const std::pair<std::string, symbol_t>& e) { return e.first == k_global_benchmark_registry; }
		);
		QUARK_ASSERT(it != global_body3._symbol_table._symbols.end());

		const auto index = static_cast<int>(it - global_body3._symbol_table._symbols.begin());

		const auto benchmark_registry_type = typeid_t::make_vector(typeid_t::make_identifier("benchmark_def_t"));

		//??? remove most use of make_type_name_from_typeid()
		const auto s = statement_t::make__init2(
			k_no_location,
			variable_address_t::make_variable_address(variable_address_t::k_global_scope, index),
			expression_t::make_construct_value_expr(make_type_name_from_typeid(benchmark_registry_type), a.benchmark_defs)
		);


		global_body3._statements.insert(global_body3._statements.begin(), s);
	}



	////////////////////////////////	Make AST

	std::vector<floyd::function_definition_t> function_defs_vec;
	for(const auto& e: a._function_defs){
		function_defs_vec.push_back(e.second);
	}

	auto gp1 = general_purpose_ast_t{
		._globals = global_body3,
		._function_defs = function_defs_vec,
		._interned_types = a._types,
		._software_system = a._software_system,
		._container_def = a._container_def
	};

	type_interner_t types3 = a._types;
//	collect_used_types(types3, gp1);
	gp1._interned_types = types3;


	const auto result_ast0 = unchecked_ast_t{ gp1 };

	if(false){
		{
			QUARK_SCOPED_TRACE("OUTPUT AST");
			QUARK_TRACE(json_to_pretty_string(gp_ast_to_json(result_ast0._tree)));
		}
		{
			QUARK_SCOPED_TRACE("OUTPUT TYPES");
			trace_type_interner(result_ast0._tree._interned_types);
		}
	}

//	QUARK_ASSERT(check_types_resolved(result_ast0._tree));
	const auto result_ast1 = semantic_ast_t(result_ast0._tree);

	QUARK_ASSERT(result_ast1._tree.check_invariant());
//	QUARK_ASSERT(check_types_resolved(result_ast1._tree));
	return result_ast1;
}




//////////////////////////////////////		analyser_t



analyser_t::analyser_t(const unchecked_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	const auto intrinsics = get_intrinsic_signatures();
	_imm = make_shared<analyzer_imm_t>(analyzer_imm_t{ ast, intrinsics });
	scope_id_generator = 1000;
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast.check_invariant());
	QUARK_ASSERT(_types.check_invariant());
	return true;
}
#endif


//////////////////////////////////////		run_semantic_analysis()


static semantic_ast_t run_semantic_analysis0(const unchecked_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	analyser_t a(ast);
	const auto result = analyse(a);
	return result;
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
					trace_type_interner(result._tree._interned_types);
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
