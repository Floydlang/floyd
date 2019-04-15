	//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass3.h"

#include "statement.h"
#include "ast_value.h"
#include "ast_typeid_helpers.h"
#include "ast_basics.h"
#include "utils.h"
#include "json_support.h"
#include "host_functions.h"
#include "text_parser.h"
#include "pass2.h"

namespace floyd {

using namespace std;

bool check_types_resolved(const general_purpose_ast_t& ast);



//////////////////////////////////////		semantic_ast_t

semantic_ast_t::semantic_ast_t(const general_purpose_ast_t& tree){
	QUARK_ASSERT(tree.check_invariant());
	QUARK_ASSERT(check_types_resolved(tree));

	_tree = tree;
}

#if DEBUG
bool semantic_ast_t::check_invariant() const{
	QUARK_ASSERT(_tree.check_invariant());
	QUARK_ASSERT(check_types_resolved(_tree));
	return true;
}
#endif



//////////////////////////////////////		analyzer_imm_t

//	Immutable data used by analyser.

struct analyzer_imm_t {
	public: pass2_ast_t _ast;
	public: std::map<std::string, floyd::host_function_signature_t> _host_functions;
};




//////////////////////////////////////		analyser_t

/*
	Value object (MUTABLE!) that represents one step of the semantic analysis. It is passed around.
*/

struct lexical_scope_t {
	symbol_table_t symbols;
	epure pure;
};

struct analyser_t {
	public: analyser_t(const pass2_ast_t& ast);
#if DEBUG
	public: bool check_invariant() const;
#endif


	////////////////////////		STATE

	public: std::shared_ptr<const analyzer_imm_t> _imm;

	//	Non-constant. Last scope is the current one. First scope is the root.
	public: std::vector<lexical_scope_t> _lexical_scope_stack;

	//	These are output functions, that have been fixed.
	public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;
	public: type_interner_t _types;

	public: software_system_t _software_system;
	public: container_t _container_def;
};



//////////////////////////////////////		forward


std::pair<analyser_t, shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement, const typeid_t& return_type);
floyd::semantic_ast_t analyse(const analyser_t& a);

/*
	Return value:
		null = statements were all executed through.
		value = return statement returned a value.
*/
std::pair<analyser_t, std::vector<std::shared_ptr<floyd::statement_t>> > analyse_statements(const analyser_t& a, const std::vector<std::shared_ptr<floyd::statement_t>>& statements, const typeid_t& return_type);



/*
	analyses an expression as far as possible.
	return == _constant != nullptr:	the expression was completely analysed and resulted in a constant value.
	return == _constant == nullptr: the expression was partially analyse.
*/
std::pair<analyser_t, floyd::expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const floyd::expression_t& e, const floyd::typeid_t& target_type);
std::pair<analyser_t, floyd::expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const floyd::expression_t& e);




//////////////////////////////////////		IMPLEMENTATION



const function_definition_t& function_id_to_def(const analyser_t& a, int function_id){
	QUARK_ASSERT(function_id >= 0 && function_id < a._function_defs.size());

	return *a._function_defs[function_id];
}



/////////////////////////////////////////			TYPES AND SYMBOLS



//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<const symbol_t*, floyd::variable_address_t> resolve_env_variable_deep(const analyser_t& a, int depth, const std::string& s){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(depth >= 0 && depth < a._lexical_scope_stack.size());
	QUARK_ASSERT(s.size() > 0);

    const auto it = std::find_if(
    	a._lexical_scope_stack[depth].symbols._symbols.begin(),
    	a._lexical_scope_stack[depth].symbols._symbols.end(),
    	[&s](const std::pair<std::string, floyd::symbol_t>& e) { return e.first == s; }
	);

	if(it != a._lexical_scope_stack[depth].symbols._symbols.end()){
		const auto parent_index = depth == 0 ? -1 : (int)(a._lexical_scope_stack.size() - depth - 1);
		const auto variable_index = (int)(it - a._lexical_scope_stack[depth].symbols._symbols.begin());
		return { &it->second, floyd::variable_address_t::make_variable_address(parent_index, variable_index) };
	}
	else if(depth > 0){
		return resolve_env_variable_deep(a, depth - 1, s);
	}
	else{
		return { nullptr, floyd::variable_address_t::make_variable_address(0, 0) };
	}
}
//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
std::pair<const symbol_t*, floyd::variable_address_t> find_symbol_by_name(const analyser_t& a, const std::string& s){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	return resolve_env_variable_deep(a, static_cast<int>(a._lexical_scope_stack.size() - 1), s);
}

bool does_symbol_exist_shallow(const analyser_t& a, const std::string& s){
    const auto it = std::find_if(a._lexical_scope_stack.back().symbols._symbols.begin(), a._lexical_scope_stack.back().symbols._symbols.end(),  [&s](const std::pair<std::string, floyd::symbol_t>& e) { return e.first == s; });
	return it != a._lexical_scope_stack.back().symbols._symbols.end();
}

//	Warning: returns reference to the found value-entry -- this could be in any environment in the call stack.
const symbol_t* resolve_symbol_by_address(const analyser_t& a, const floyd::variable_address_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto env_index = s._parent_steps == -1 ? 0 : a._lexical_scope_stack.size() - s._parent_steps - 1;
	auto& env = a._lexical_scope_stack[env_index];
	return &env.symbols._symbols[s._index].second;
}



typeid_t resolve_type_internal2(const analyser_t& a, const location_t& loc, const typeid_t& type);

typeid_t resolve_type_internal(const analyser_t& a, const location_t& loc, const typeid_t& type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto basetype = type.get_base_type();

	if(basetype == base_type::k_internal_undefined){
		throw_compiler_error(loc, "Cannot resolve type");
	}
	else if(basetype == base_type::k_internal_dynamic){
		return type;
	}

	else if(basetype == base_type::k_void){
		return type;
	}
	else if(basetype == base_type::k_bool){
		return type;
	}
	else if(basetype == base_type::k_int){
		return type;
	}
	else if(basetype == base_type::k_double){
		return type;
	}
	else if(basetype == base_type::k_string){
		return type;
	}
	else if(basetype == base_type::k_json_value){
		return type;
	}

	else if(basetype == base_type::k_typeid){
		return type;
	}

	else if(basetype == base_type::k_struct){
		const auto& struct_def = type.get_struct();
		std::vector<member_t> members2;
		for(const auto& e: struct_def._members){
			members2.push_back(member_t(resolve_type_internal2(a, loc, e._type), e._name));
		}
		return typeid_t::make_struct2(members2);
	}
	else if(basetype == base_type::k_vector){
		return typeid_t::make_vector(resolve_type_internal2(a, loc, type.get_vector_element_type()));
	}
	else if(basetype == base_type::k_dict){
		return typeid_t::make_dict(resolve_type_internal2(a, loc, type.get_dict_value_type()));
	}
	else if(basetype == base_type::k_function){
		const auto ret = type.get_function_return();
		const auto args = type.get_function_args();
		const auto pure = type.get_function_pure();
		const auto dyn_return_type = type.get_function_dyn_return_type();

		const auto ret2 = resolve_type_internal2(a, loc, ret);
		vector<typeid_t> args2;
		for(const auto& e: args){
			args2.push_back(resolve_type_internal2(a, loc, e));
		}
		return typeid_t::make_function3(ret2, args2, pure, dyn_return_type);
	}

	else if(basetype == base_type::k_internal_unresolved_type_identifier){
		const auto found = find_symbol_by_name(a, type.get_unresolved_type_identifier());
		if(found.first != nullptr){
			if(found.first->_value_type.is_typeid()){
				return found.first->_init.get_typeid_value();
			}
			else{
				throw_compiler_error(loc, "Cannot resolve type");
			}
		}
		else{
			throw_compiler_error(loc, "Cannot resolve type");
		}
	}
	else {
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

typeid_t resolve_type_internal2(const analyser_t& a, const location_t& loc, const typeid_t& type){
	const auto result = resolve_type_internal(a, loc, type);
	if(result.check_types_resolved() == false){
		throw_compiler_error(loc, "Cannot resolve type");
	}
	return result;
}





typeid_t resolve_type(analyser_t& a, const location_t& loc, const typeid_t& type){
	const auto type2 = resolve_type_internal2(a, loc, type);
	intern_type(a._types, type2);
	return type2;
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
			QUARK_ASSERT(r.second->check_types_resolved());
			statements2.push_back(*r.second);
		}
		statement_index++;
	}
	return { a_acc, statements2 };
}

std::pair<analyser_t, body_t > analyse_body(const analyser_t& a, const floyd::body_t& body, epure pure, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	auto new_environment = symbol_table_t{ body._symbol_table };
	const auto lexical_scope = lexical_scope_t{new_environment, pure};
	a_acc._lexical_scope_stack.push_back(lexical_scope);

	const auto result = analyse_statements(a_acc, body._statements, return_type);
	a_acc = result.first;

	const auto body2 = body_t(result.second, result.first._lexical_scope_stack.back().symbols);

	a_acc._lexical_scope_stack.pop_back();
	return { a_acc, body2 };
}

/*
	- Can update an existing local (if local is mutable).
	- Can implicitly create a new local
*/
std::pair<analyser_t, statement_t> analyse_store_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto statement = std::get<statement_t::store_t>(s._contents);
	const auto local_name = statement._local_name;
	const auto existing_value_deep_ptr = find_symbol_by_name(a_acc, local_name);

	//	Attempt to mutate existing value!
	if(existing_value_deep_ptr.first != nullptr){
		if(existing_value_deep_ptr.first->_symbol_type != symbol_t::mutable_local){
			throw_compiler_error(s.location, "Cannot assign to immutable identifier \"" + local_name + "\".");
		}
		else{
			const auto lhs_type = existing_value_deep_ptr.first->get_type();
			QUARK_ASSERT(lhs_type.check_types_resolved());

			const auto rhs_expr2 = analyse_expression_to_target(a_acc, s, statement._expression, lhs_type);
			a_acc = rhs_expr2.first;
			const auto rhs_expr3 = rhs_expr2.second;

			if(lhs_type != rhs_expr3.get_output_type()){
				std::stringstream what;
				what << "Types not compatible in bind - cannot convert '"
				<< typeid_to_compact_string(rhs_expr3.get_output_type()) << "' to '" << typeid_to_compact_string(lhs_type) << ".";
				throw_compiler_error(s.location, what.str());
			}
			else{
				return { a_acc, statement_t::make__store2(s.location, existing_value_deep_ptr.second, rhs_expr3) };
			}
		}
	}

	//	Bind new value -- infer type.
	else{
		const auto rhs_expr2 = analyse_expression_no_target(a_acc, s, statement._expression);
		a_acc = rhs_expr2.first;
		const auto rhs_expr2_type = rhs_expr2.second.get_output_type();

		a_acc._lexical_scope_stack.back().symbols._symbols.push_back({local_name, symbol_t::make_immutable_local(rhs_expr2_type)});
		int variable_index = (int)(a_acc._lexical_scope_stack.back().symbols._symbols.size() - 1);
		return { a_acc, statement_t::make__store2(s.location, floyd::variable_address_t::make_variable_address(0, variable_index), rhs_expr2.second) };
	}
}

/*
Assign with target type info.
- Always creates a new local.

Ex:
	int a = 10
	mutable a = 10
	mutable = 10
*/
std::pair<analyser_t, statement_t> analyse_bind_local_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::bind_local_t>(s._contents);
	auto a_acc = a;

	const auto new_local_name = statement._new_local_name;
	const auto lhs_type0 = statement._bindtype;
	const auto lhs_type = lhs_type0.check_types_resolved() == false && lhs_type0.is_undefined() == false ? resolve_type(a_acc, s.location, lhs_type0) : lhs_type0;

	const auto bind_statement_mutable_tag_flag = statement._locals_mutable_mode == statement_t::bind_local_t::k_mutable;

	const auto value_exists_in_env = does_symbol_exist_shallow(a_acc, new_local_name);
	if(value_exists_in_env){
		std::stringstream what;
		what << "Local identifier \"" << new_local_name << "\" already exists.";
		throw_compiler_error(s.location, what.str());
	}

	//	Setup temporary simply so function definition can find itself = recursive.
	//	Notice: the final type may not be correct yet, but for function defintions it is.
	//	This logic should be available for infered binds too, in analyse_store_statement().
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({
		new_local_name,
		bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type) : symbol_t::make_immutable_local(lhs_type)
	});
	const auto local_name_index = a_acc._lexical_scope_stack.back().symbols._symbols.size() - 1;

	try {
		const auto rhs_expr_pair = lhs_type.is_undefined()
			? analyse_expression_no_target(a_acc, s, statement._expression)
			: analyse_expression_to_target(a_acc, s, statement._expression, lhs_type);
		a_acc = rhs_expr_pair.first;

		//??? if expression is a k_define_struct, k_define_function -- make it a constant in symbol table and emit no store-statement!

		const auto rhs_type = rhs_expr_pair.second.get_output_type();
		const auto lhs_type2 = lhs_type.is_undefined() ? rhs_type : lhs_type;

		//??? always true?
		if(lhs_type2 != lhs_type2){
			std::stringstream what;
			what << "Types not compatible in bind - cannot convert '"
			<< typeid_to_compact_string(lhs_type2) << "' to '" << typeid_to_compact_string(lhs_type2) << ".";
			throw_compiler_error(s.location, what.str());
		}
		else{
			//	Updated the symbol with the real function defintion.
			a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = {new_local_name, bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type2) : symbol_t::make_immutable_local(lhs_type2)};
			return {
				a_acc,
				statement_t::make__store2(s.location, floyd::variable_address_t::make_variable_address(0, (int)local_name_index), rhs_expr_pair.second)
			};
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

analyser_t analyse_def_struct_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::define_struct_statement_t>(s._contents);
	auto a_acc = a;
	const auto struct_name = statement._name;
	if(does_symbol_exist_shallow(a_acc, struct_name)){
		std::stringstream what;
		what << "Name \"" << struct_name << "\" already used in current lexical scope.";
		throw_compiler_error(s.location, what.str());
	}

	const auto struct_typeid1 = typeid_t::make_struct2(statement._def->_members);
	const auto struct_typeid2 = resolve_type(a_acc, s.location, struct_typeid1);
	const auto struct_typeid_value = value_t::make_typeid_value(struct_typeid2);
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({struct_name, symbol_t::make_constant(struct_typeid_value)});

	return a_acc;
}

std::pair<analyser_t, statement_t> analyse_def_function_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::define_function_statement_t>(s._contents);

	auto a_acc = a;

	//	Translates into:  bind-local, "myfunc", function_definition_expr_t
	const auto function_def_expr = expression_t::make_function_definition(statement._def);
	const auto& s2 = statement_t::make__bind_local(
		s.location,
		statement._name,
		statement._def->_function_type,
		function_def_expr,
		statement_t::bind_local_t::k_immutable
	);
	const auto s3 = analyse_bind_local_statement(a_acc, s2);
	return s3;
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& a, const statement_t& s, const typeid_t& return_type){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::ifelse_statement_t>(s._contents);
	auto a_acc = a;

	const auto condition2 = analyse_expression_no_target(a_acc, s, statement._condition);
	a_acc = condition2.first;

	const auto condition_type = condition2.second.get_output_type();
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

	if(start_expr2.second.get_output_type().is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, start type is " <<  typeid_to_compact_string(start_expr2.second.get_output_type()) << ".";
		throw_compiler_error(s.location, what.str());
	}

	const auto end_expr2 = analyse_expression_no_target(a_acc, s, statement._end_expression);
	a_acc = end_expr2.first;

	if(end_expr2.second.get_output_type().is_int() == false){
		std::stringstream what;
		what << "For-loop requires integer iterator, end type is " <<  typeid_to_compact_string(end_expr2.second.get_output_type()) << ".";
		throw_compiler_error(s.location, what.str());
	}

	const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());

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
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::define_struct_statement_t& s) const{
			const auto e = analyse_def_struct_statement(a, statement);
			return { e, {} };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::define_function_statement_t& s) const{
			const auto e = analyse_def_function_statement(a, statement);
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::bind_local_t& s) const{
			const auto e = analyse_bind_local_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::store_t& s) const{
			const auto e = analyse_store_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::store2_t& s) const{
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::block_statement_t& s) const{
			const auto e = analyse_block_statement(a, statement, return_type);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::ifelse_statement_t& s) const{
			const auto e = analyse_ifelse_statement(a, statement, return_type);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::for_statement_t& s) const{
			const auto e = analyse_for_statement(a, statement, return_type);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::while_statement_t& s) const{
			const auto e = analyse_while_statement(a, statement, return_type);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}


		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::expression_statement_t& s) const{
			const auto e = analyse_expression_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::software_system_statement_t& s) const{
			analyser_t temp = a;
			temp._software_system = parse_software_system_json(s._json_data);
			return { temp, std::make_shared<statement_t>(statement) };
		}
		std::pair<analyser_t, shared_ptr<statement_t>> operator()(const statement_t::container_def_statement_t& s) const{
			analyser_t temp = a;
			temp._container_def = parse_container_def_json(s._json_data);
			return { temp, std::make_shared<statement_t>(statement) };
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

	const auto parent_type = parent_expr.second.get_output_type();

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
		return { a_acc, expression_t::make_resolve_member(parent_expr.second, details.member_name, make_shared<typeid_t>(member_type))};
	}
	else{
		std::stringstream what;
		what << "Left hand side is not a struct value, it's of type \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::lookup_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto parent_expr = analyse_expression_no_target(a_acc, parent, *details.parent_address);
	a_acc = parent_expr.first;

	const auto key_expr = analyse_expression_no_target(a_acc, parent, *details.lookup_key);
	a_acc = key_expr.first;

	const auto parent_type = parent_expr.second.get_output_type();
	const auto key_type = key_expr.second.get_output_type();

	if(parent_type.is_string()){
		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Strings can only be indexed by integers, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(typeid_t::make_int())) };
		}
	}
	else if(parent_type.is_json_value()){
		return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(typeid_t::make_json_value())) };
	}
	else if(parent_type.is_vector()){
		if(key_type.is_int() == false){
			std::stringstream what;
			what << "Vector can only be indexed by integers, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_vector_element_type())) };
		}
	}
	else if(parent_type.is_dict()){
		if(key_type.is_string() == false){
			std::stringstream what;
			what << "Dictionary can only be looked up using string keys, not a \"" + typeid_to_compact_string(key_type) + "\".";
			throw_compiler_error(parent.location, what.str());
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_dict_value_type())) };
		}
	}
	else {
		std::stringstream what;
		what << "Lookup using [] only works with strings, vectors, dicts and json_value - not a \"" + typeid_to_compact_string(parent_type) + "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_load(const analyser_t& a, const statement_t& parent,const expression_t& e, const expression_t::load_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto found = find_symbol_by_name(a_acc, details.variable_name);
	if(found.first != nullptr){
		return {a_acc, expression_t::make_load2(found.second, make_shared<typeid_t>(found.first->_value_type)) };
	}
	else{
		std::stringstream what;
		what << "Undefined variable \"" << details.variable_name << "\".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_load2(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	return { a, e };
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

	const auto current_type = *e._output_type;
	if(current_type.is_vector()){
		//	JSON constants supports mixed element types: convert each element into a json_value.
		//	Encode as [json_value]
		if(target_type.is_json_value()){
			const auto element_type = typeid_t::make_json_value();

			std::vector<expression_t> elements2;
			for(const auto& m: details.elements){
				const auto element_expr = analyse_expression_to_target(a_acc, parent, m, element_type);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}
			const auto result_type = typeid_t::make_vector(typeid_t::make_json_value());
			if(result_type.check_types_resolved() == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}
			return {
				a_acc,
				expression_t::make_construct_value_expr(
					typeid_t::make_json_value(),
					{ expression_t::make_construct_value_expr(typeid_t::make_vector(typeid_t::make_json_value()), elements2) }
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

			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? elements2[0].get_output_type() : element_type;
			const auto result_type0 = typeid_t::make_vector(element_type2);
			const auto result_type = result_type0.check_types_resolved() == false && target_type.is_internal_dynamic() == false ? target_type : result_type0;

			if(result_type.check_types_resolved() == false){
				std::stringstream what;
				what << "Cannot infer vector element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}

			for(const auto& m: elements2){
				if(m.get_output_type() != element_type2){
					std::stringstream what;
					what << "Vector of type " << typeid_to_compact_string(result_type) << " cannot hold an element of type " << typeid_to_compact_string(m.get_output_type()) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			QUARK_ASSERT(result_type.check_types_resolved());
			return { a_acc, expression_t::make_construct_value_expr(result_type, elements2) };
		}
	}

	//	Dicts uses pairs of (string,value). This is stored in _args as interleaved expression: string0, value0, string1, value1.
	else if(current_type.is_dict()){
		//	JSON constants supports mixed element types: convert each element into a json_value.
		//	Encode as [string:json_value]
		if(target_type.is_json_value()){
			const auto element_type = typeid_t::make_json_value();

			std::vector<expression_t> elements2;
			for(int i = 0 ; i < details.elements.size() / 2 ; i++){
				const auto& key = details.elements[i * 2 + 0].get_literal().get_string_value();
				const auto& value = details.elements[i * 2 + 1];
				const auto element_expr = analyse_expression_to_target(a_acc, parent, value, element_type);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			const auto result_type = typeid_t::make_dict(typeid_t::make_json_value());
			if(result_type.check_types_resolved() == false){
				std::stringstream what;
				what << "Cannot infer dictionary element type, add explicit type.";
				throw_compiler_error(parent.location, what.str());
			}
			return {
				a_acc,
				expression_t::make_construct_value_expr(
					typeid_t::make_json_value(),
					{ expression_t::make_construct_value_expr(typeid_t::make_dict(typeid_t::make_json_value()), elements2) }
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
			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? elements2[0 * 2 + 1].get_output_type() : element_type;
			const auto result_type0 = typeid_t::make_dict(element_type2);
			const auto result_type = result_type0.check_types_resolved() == false && target_type.is_internal_dynamic() == false ? target_type : result_type0;

			//	Make sure all elements have the correct type.
			for(int i = 0 ; i < elements2.size() / 2 ; i++){
				const auto element_type0 = elements2[i * 2 + 1].get_output_type();
				if(element_type0 != element_type2){
					std::stringstream what;
					what << "Dictionary of type " << typeid_to_compact_string(result_type) << " cannot hold an element of type " << typeid_to_compact_string(element_type0) << ".";
					throw_compiler_error(parent.location, what.str());
				}
			}
			return {a_acc, expression_t::make_construct_value_expr(result_type, elements2)};
		}
	}
	else{
		QUARK_ASSERT(false);
	}
	quark::throw_exception();
}

std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::unary_minus_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto& expr2 = analyse_expression_no_target(a_acc, parent, *details.expr);
	a_acc = expr2.first;

	//??? We could simplify here and return [ "-", 0, expr]
	const auto type = expr2.second.get_output_type();
	if(type.is_int() || type.is_double()){
		return {a_acc, expression_t::make_unary_minus(expr2.second, make_shared<typeid_t>(type))  };
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

	const auto type = cond_result.second.get_output_type();
	if(type.is_bool() == false){
		std::stringstream what;
		what << "Conditional expression needs to be a bool, not a " << typeid_to_compact_string(type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else if(a.second.get_output_type() != b.second.get_output_type()){
		std::stringstream what;
		what << "Conditional expression requires true/false expressions to have the same type, currently " << typeid_to_compact_string(a.second.get_output_type()) << " : " << typeid_to_compact_string(b.second.get_output_type()) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto final_expression_type = a.second.get_output_type();
		return {
			analyser_acc,
			expression_t::make_conditional_operator(
				cond_result.second,
				a.second,
				b.second,
				make_shared<typeid_t>(final_expression_type)
			)
		};
	}
}

//	Term: Type inference

std::pair<analyser_t, expression_t> analyse_comparison_expression(const analyser_t& a, const statement_t& parent, expression_type op, const expression_t& e, const expression_t::comparison_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse all inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, parent, *details.lhs);
	a_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_output_type();

	//	Make rhs match left if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, parent, *details.rhs, lhs_type);
	a_acc = right_expr.first;
	const auto rhs_type = right_expr.second.get_output_type();

	if(lhs_type != rhs_type || (lhs_type.is_undefined() == true || rhs_type.is_undefined() == true)){
		std::stringstream what;
		what << "Left and right expressions must be same type in comparison, " << typeid_to_compact_string(lhs_type) << " " << expression_type_to_token(op) << typeid_to_compact_string(rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto shared_type = lhs_type;

		if(op == expression_type::k_comparison_smaller_or_equal__2){
		}
		else if(op == expression_type::k_comparison_smaller__2){
		}
		else if(op == expression_type::k_comparison_larger_or_equal__2){
		}
		else if(op == expression_type::k_comparison_larger__2){
		}

		else if(op == expression_type::k_logical_equal__2){
		}
		else if(op == expression_type::k_logical_nonequal__2){
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
				make_shared<typeid_t>(typeid_t::make_bool())
			)
		};
	}
}

std::pair<analyser_t, expression_t> analyse_arithmetic_expression(const analyser_t& a, const statement_t& parent, expression_type op, const expression_t& e, const expression_t::arithmetic_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse both inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, parent, *details.lhs);
	a_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_output_type();

	//	Make rhs match lhs if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, parent, *details.rhs, lhs_type);
	a_acc = right_expr.first;

	const auto rhs_type = right_expr.second.get_output_type();


	if(lhs_type != rhs_type){
		std::stringstream what;
		what << "Artithmetics: Left and right expressions must be same type, currently " << typeid_to_compact_string(lhs_type) << " : " << typeid_to_compact_string(rhs_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
	else{
		const auto shared_type = lhs_type;

		//	bool
		if(shared_type.is_bool()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw_compiler_error(parent.location, "Operation not allowed on bool.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}

			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	int
		else if(shared_type.is_int()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
			}
			else if(op == expression_type::k_arithmetic_divide__2){
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}

			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	double
		else if(shared_type.is_double()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
			}
			else if(op == expression_type::k_arithmetic_divide__2){
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw_compiler_error(parent.location, "Modulo operation on double not supported.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}

			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	string
		else if(shared_type.is_string()){
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw_compiler_error(parent.location, "Operation not allowed on string.");
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}

			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	struct
		else if(shared_type.is_struct()){
			//	Structs must be exactly the same type to match.

			if(op == expression_type::k_arithmetic_add__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw_compiler_error(parent.location, "Operation not allowed on structs.");
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}

			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	vector
		else if(shared_type.is_vector()){
			const auto element_type = shared_type.get_vector_element_type();
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}


			else if(op == expression_type::k_logical_and__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw_compiler_error(parent.location, "Operation not allowed on vectors.");
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			return {a_acc, expression_t::make_arithmetic(op, left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
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


//	callee (plural callees)
//	(telephony) The person who is called by the caller (on the telephone).
std::pair<analyser_t, vector<expression_t>> analyze_call_args(const analyser_t& a, const statement_t& parent, const vector<expression_t>& call_args, const std::vector<typeid_t>& callee_args){
	//	arity
	if(call_args.size() != callee_args.size()){
		std::stringstream what;
		what << "Wrong number of arguments in function call, got " << std::to_string(call_args.size()) << ", expected " << std::to_string(callee_args.size()) << ".";
		throw_compiler_error(parent.location, what.str());
	}

	auto a_acc = a;
	vector<expression_t> call_args2;
	for(int i = 0 ; i < callee_args.size() ; i++){
		const auto call_arg_pair = analyse_expression_to_target(a_acc, parent, call_args[i], callee_args[i]);
		a_acc = call_arg_pair.first;
		call_args2.push_back(call_arg_pair.second);
	}
	return { a_acc, call_args2 };
}




/*
//	??? This only works when directly calling one of the host functions, via its global name.
bool is_host_function_call(const analyser_t& a, const expression_t& callee_expr){
	QUARK_ASSERT(std::get_if<expression_t::load_t>(&callee_expr._contents) == nullptr);

	if(auto load2 = std::get_if<expression_t::load2_t>(&callee_expr._contents)){
		const auto callee = resolve_symbol_by_address(a, load2->address);
		QUARK_ASSERT(callee != nullptr);

		if(callee->_init.is_function()){
			const auto& function_def = function_id_to_def(a, callee->_init.get_function_value());
			return std::get<function_definition_t::host_func_t>(function_def._contents)._host_function_id != k_no_host_function_id;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

typeid_t get_host_function_return_type(const analyser_t& a, const statement_t& parent, const expression_t& callee_expr, const vector<expression_t>& args){
	auto load2 = std::get_if<expression_t::load2_t>(&callee_expr._contents);
	QUARK_ASSERT(load2 != nullptr);

	const auto callee = resolve_symbol_by_address(a, load2->address);
	QUARK_ASSERT(callee != nullptr);

	const auto& function_def = function_id_to_def(a, callee->_init.get_function_value());

	const auto host_function_id = std::get<function_definition_t::host_func_t>(function_def._contents)._host_function_id;

	const auto& host_functions = a._imm->_host_functions;
	const auto found_it = find_if(host_functions.begin(), host_functions.end(), [&](const std::pair<std::string, floyd::host_function_signature_t>& kv){ return kv.second._function_id == host_function_id; });
	QUARK_ASSERT(found_it != host_functions.end());

	const auto calc_dyn_type = found_it->second._dynamic_return_type;
	if(calc_dyn_type){
		std::vector<typeid_t> arg_types;
		for(const auto& e: args){
			arg_types.push_back(e.get_output_type());
		}
		return calc_dyn_type(arg_types);
	}
	else{
		const auto function_name = found_it->first;

		//	jsonvalue_to_value() supports argument 2 that is a compile-time type name.
		if(found_it->second._function_id == static_cast<int>(host_function_id::jsonvalue_to_value)){
			QUARK_ASSERT(args.size() == 2);

			const auto arg0 = args[0];
			const auto arg1 = args[1];

			if(auto arg1_load2 = std::get_if<expression_t::load2_t>(&arg1._contents)){
				const auto symbol = resolve_symbol_by_address(a, arg1_load2->address);
				if(symbol != nullptr && symbol->_init.is_undefined() == false){
					return symbol->_init.get_typeid_value();
				}
				else{
					throw_compiler_error(parent.location, "Cannot resolve type for jsonvalue_to_value().");
				}
			}
			else{
				throw_compiler_error(parent.location, "Cannot resolve type for jsonvalue_to_value().");
			}
		}
		else{
			return callee_expr.get_output_type().get_function_return();
		}
	}
}

const typeid_t figure_out_return_type_via_hf(const analyser_t& a, const statement_t& parent, const expression_t& callee_expr, const std::vector<expression_t>& call_args){
	const auto callee_type = callee_expr.get_output_type();
	const auto callee_return_value = callee_type.get_function_return();

	if(is_host_function_call(a, callee_expr)){
		const auto return_type = get_host_function_return_type(a, parent, callee_expr, call_args);
		return return_type;
	}
	else{
		return callee_return_value;
	}
}
*/

const typeid_t figure_out_return_type(const analyser_t& a, const statement_t& parent, const expression_t& callee_expr, const std::vector<expression_t>& call_args){
	const auto callee_type = callee_expr.get_output_type();
	const auto callee_return_value = callee_type.get_function_return();

	const auto ret_type = callee_type.get_function_dyn_return_type();
	switch(ret_type){
		case typeid_t::return_dyn_type::none:
			{
				return callee_return_value;
			}
			break;

		case typeid_t::return_dyn_type::arg0:
			{
				QUARK_ASSERT(call_args.size() > 0);

				return call_args[0].get_output_type();
			}
			break;
		case typeid_t::return_dyn_type::arg1:
			{
				QUARK_ASSERT(call_args.size() >= 2);

				return call_args[1].get_output_type();
			}
			break;
		case typeid_t::return_dyn_type::vector_of_arg1func_return:
			{
		//	x = make_vector(arg1.get_function_return());
				QUARK_ASSERT(call_args.size() >= 2);

				const auto f = call_args[1].get_output_type().get_function_return();
				const auto ret = typeid_t::make_vector(f);
				return ret;
			}
			break;
		case typeid_t::return_dyn_type::vector_of_arg2func_return:
			{
		//	x = make_vector(arg2.get_function_return());
			QUARK_ASSERT(call_args.size() >= 3);
			const auto f = call_args[2].get_output_type().get_function_return();
			const auto ret = typeid_t::make_vector(f);
			return ret;
			}
			break;


		default:
			QUARK_ASSERT(false);
			break;
	}
}

std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::call_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto callee_expr0 = analyse_expression_no_target(a_acc, parent, *details.callee);
	a_acc = callee_expr0.first;
	const auto callee_expr = callee_expr0.second;

	const auto call_args = details.args;

	const auto callsite_pure = a_acc._lexical_scope_stack.back().pure;

	auto callee_expr_load2 = std::get_if<expression_t::load2_t>(&callee_expr._contents);

	//	This is a call to a function-value. Callee is a function-type.
	const auto callee_type = callee_expr.get_output_type();
	if(callee_type.is_function()){
		const auto callee_args = callee_type.get_function_args();
		const auto callee_return_value = callee_type.get_function_return();
		const auto callee_pure = callee_type.get_function_pure();

		if(callsite_pure == epure::pure && callee_pure == epure::impure){
			throw_compiler_error(parent.location, "Cannot call impure function from a pure function.");
		}

		const auto call_args_pair = analyze_call_args(a_acc, parent, call_args, callee_args);
		a_acc = call_args_pair.first;

		const auto call_return_type = figure_out_return_type(a, parent, callee_expr, call_args_pair.second);
		return { a_acc, expression_t::make_call(callee_expr, call_args_pair.second, make_shared<typeid_t>(call_return_type)) };
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	//	Converts these calls to construct-value-expressions.
	else if(callee_type.is_typeid() && callee_expr_load2){
		const auto found_symbol_ptr = resolve_symbol_by_address(a_acc, callee_expr_load2->address);
		QUARK_ASSERT(found_symbol_ptr != nullptr);

		if(found_symbol_ptr->_init.is_undefined()){
			throw_compiler_error(parent.location, "Cannot resolve callee.");
		}
		else{
			const auto callee_type2 = found_symbol_ptr->_init.get_typeid_value();

			//	Convert calls to struct-type into construct-value expression.
			if(callee_type2.is_struct()){
				const auto& def = callee_type2.get_struct();
				const auto callee_args = get_member_types(def._members);
				const auto call_args_pair = analyze_call_args(a_acc, parent, call_args, callee_args);
				a_acc = call_args_pair.first;

				return { a_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}

			//	One argument for primitive types.
			else{
				const auto callee_args = vector<typeid_t>{ callee_type2 };
				QUARK_ASSERT(callee_args.size() == 1);
				const auto call_args_pair = analyze_call_args(a_acc, parent, call_args, callee_args);
				a_acc = call_args_pair.first;
				return { a_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}
		}
	}
	else{
		std::stringstream what;
		what << "Cannot call non-function, its type is " << typeid_to_compact_string(callee_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}
}

std::pair<analyser_t, expression_t> analyse_struct_definition_expression(const analyser_t& a, const statement_t& parent, const expression_t& e0, const expression_t::struct_definition_expr_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto& struct_def = *details.def;

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto& e: struct_def._members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = resolve_type(a_acc, parent.location, type);
		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}
	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	return {a_acc, expression_t::make_struct_definition(resolved_struct_def) };
}

// ??? Check that function returns a value, if so specified.
std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& a, const statement_t& parent, const expression_t& e, const expression_t::function_definition_expr_t& details){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto function_def = details.def;
	const auto function_type2 = resolve_type(a_acc, parent.location, function_def->_function_type);
	const auto function_pure = function_type2.get_function_pure();

	vector<member_t> args2;
	for(const auto& arg: function_def->_args){
		const auto arg_type2 = resolve_type(a_acc, parent.location, arg._type);
		args2.push_back(member_t(arg_type2, arg._name));
	}

	//??? Can there be a pure function inside an impure lexical scope?
	const auto pure = function_pure;


	const function_definition_t::floyd_func_t floyd_func = std::get<function_definition_t::floyd_func_t>(function_def->_contents);


	//	Make function body with arguments injected FIRST in body as local symbols.
	auto symbol_vec = floyd_func._body->_symbol_table;
	for(const auto& arg: args2){
		symbol_vec._symbols.push_back({arg._name , symbol_t::make_immutable_local(arg._type)});
	}
	const auto function_body2 = body_t(floyd_func._body->_statements, symbol_vec);

	const auto function_body_pair = analyse_body(a_acc, function_body2, pure, function_type2.get_function_return());
	a_acc = function_body_pair.first;
	const auto function_body3 = function_body_pair.second;

	const auto function_def2 = function_definition_t::make_floyd_func(k_no_location, function_def->_definition_name, function_type2, args2, make_shared<body_t>(function_body3));
	QUARK_ASSERT(function_def2.check_types_resolved());

	a_acc._function_defs.push_back(make_shared<function_definition_t>(function_def2));

	const int function_id = static_cast<int>(a_acc._function_defs.size() - 1);
	const auto r = expression_t::make_literal(value_t::make_function_value(function_type2, function_id));

	return {a_acc, r };
}


std::pair<analyser_t, expression_t> analyse_expression__operation_specific(const analyser_t& a, const statement_t& parent, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		const analyser_t& a;
		const statement_t& parent;
		const expression_t& e;
		const typeid_t& target_type;

		std::pair<analyser_t, expression_t> operator()(const expression_t::literal_exp_t& expr) const{
			return { a, e };
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
		std::pair<analyser_t, expression_t> operator()(const expression_t::lookup_t& expr) const{
			return analyse_lookup_element_expression(a, parent, e, expr);
		}
		std::pair<analyser_t, expression_t> operator()(const expression_t::value_constructor_t& expr) const{
			return analyse_construct_value_expression(a, parent, e, expr, target_type);
		}
	};

	const auto result = std::visit(visitor_t{ a, parent, e, target_type }, e._contents);
	return result;
}


/*
	- Insert automatic type-conversions from string -> json_value etc.
*/
expression_t auto_cast_expression_type(const expression_t& e, const floyd::typeid_t& wanted_type){
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(wanted_type.check_invariant());
	QUARK_ASSERT(wanted_type.is_undefined() == false);

	const auto current_type = e.get_output_type();

	if(wanted_type.is_internal_dynamic()){
		return e;
	}
	else if(wanted_type.is_string()){
		if(current_type.is_json_value()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else{
			return e;
		}
	}
	else if(wanted_type.is_json_value()){
		if(current_type.is_int() || current_type.is_double() || current_type.is_string() || current_type.is_bool()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else if(current_type.is_vector()){
			return expression_t::make_construct_value_expr(wanted_type, { e });
		}
		else if(current_type.is_dict()){
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

std::string get_expression_name(const expression_t& e){
	const expression_type op = get_opcode(e);
	return expression_type_to_token(op);
}


//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const statement_t& parent, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(parent.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(target_type.is_void() == false && target_type.is_undefined() == false);
	QUARK_ASSERT(target_type.check_types_resolved());

	auto a_acc = a;
	const auto e2_pair = analyse_expression__operation_specific(a_acc, parent, e, target_type);
	a_acc = e2_pair.first;
	const auto e2b = e2_pair.second;
	if(e2b.check_types_resolved() == false){
		std::stringstream what;
		what << "Cannot infer type in " << get_expression_name(e2b) << "-expression.";
		throw_compiler_error(parent.location, what.str());
	}

	const auto e3 = auto_cast_expression_type(e2b, target_type);

	if(target_type.is_internal_dynamic()){
	}
	else if(e3.get_output_type() == target_type){
	}
	else if(e3.get_output_type().is_undefined()){
		QUARK_ASSERT(false);
		quark::throw_runtime_error("Expression type mismatch.");
	}
	else{
		std::stringstream what;
		what << "Expression type mismatch - cannot convert '"
		<< typeid_to_compact_string(e3.get_output_type()) << "' to '" << typeid_to_compact_string(target_type) << ".";
		throw_compiler_error(parent.location, what.str());
	}

	if(e3.check_types_resolved() == false){
		throw_compiler_error(parent.location, "Cannot resolve type.");
	}
	QUARK_ASSERT(e3.check_types_resolved());
	return { a_acc, e3 };
}

//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const statement_t& parent, const expression_t& e){
	return analyse_expression_to_target(a, parent, e, typeid_t::make_internal_dynamic());
}

void test__analyse_expression(const statement_t& parent, const expression_t& e, const expression_t& expected){
	const pass2_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter, parent, e);

	ut_verify(QUARK_POS, expression_to_json(e3.second), expression_to_json(expected));
}


QUARK_UNIT_TEST("analyse_expression_no_target()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		statement_t::make__bind_local(k_no_location, "xyz", typeid_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_UNIT_TEST("analyse_expression_no_target()", "1 + 2 == 3", "", "") {
	const pass2_ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(
		interpreter,
		statement_t::make__bind_local(k_no_location, "xyz", typeid_t::make_string(), expression_t::make_literal_string("abc"), statement_t::bind_local_t::mutable_mode::k_immutable),
		expression_t::make_arithmetic(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			nullptr
		)
	);

	ut_verify(QUARK_POS,
		expression_to_json(e3.second),
		parse_json(seq_t(R"(   ["+", ["k", 1, "^int"], ["k", 2, "^int"], "^int"]   )")).first
	);
}


bool check_types_resolved(const general_purpose_ast_t& ast){
	if(ast._globals.check_types_resolved() == false){
		return false;
	}
	for(const auto& e: ast._function_defs){
		const auto result = e->check_types_resolved();
		if(result == false){
			return false;
		}
	}
	return true;
}


struct builtins_t {
	std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs;
	std::vector<std::pair<std::string, symbol_t>> symbol_map;
};

builtins_t insert_builtin_functions(analyser_t& a, const std::map<std::string, floyd::host_function_signature_t>& host_functions, int function_def_start_id){
	std::vector<std::shared_ptr<const floyd::function_definition_t>> function_defs;
	std::vector<std::pair<std::string, symbol_t>> symbol_map;

	int function_id = function_def_start_id;
	for(auto hf_kv: host_functions){
		const auto& function_name = hf_kv.first;
		const auto& signature = hf_kv.second;

		resolve_type(a, k_no_location, signature._function_type);

		const auto args = [&](){
			vector<member_t> result;
			for(const auto& e: signature._function_type.get_function_args()){
				result.push_back(member_t(e, "dummy"));
			}
			return result;
		}();

		const auto def = make_shared<function_definition_t>(function_definition_t::make_host_func(k_no_location, function_name, signature._function_type, args, signature._function_id));

		function_defs.push_back(def);

		const auto function_value = value_t::make_function_value(signature._function_type, function_id);
		function_id++;

		symbol_map.push_back({function_name, symbol_t::make_constant(function_value)});
	}
	return builtins_t{ function_defs, symbol_map };
}

semantic_ast_t analyse(analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	/*
		Create built-in globla symbol map: built in data types, built-in functions (host functions).
	*/
	std::vector<std::pair<std::string, symbol_t>> symbol_map;

	auto function_defs = a._imm->_ast._tree._function_defs;

	const auto builtins = insert_builtin_functions(a, a._imm->_host_functions, static_cast<int>(function_defs.size()));
	function_defs.insert(function_defs.end(), builtins.function_defs.begin(), builtins.function_defs.end());
	symbol_map.insert(symbol_map.end(), builtins.symbol_map.begin(), builtins.symbol_map.end());

	//	"null" is equivalent to json_value::null
	symbol_map.push_back({"null", symbol_t::make_constant(value_t::make_json_value(json_t()))});

	symbol_map.push_back({keyword_t::k_internal_undefined, symbol_t::make_constant(value_t::make_undefined())});
	symbol_map.push_back({keyword_t::k_internal_dynamic, symbol_t::make_constant(value_t::make_internal_dynamic())});

	symbol_map.push_back({keyword_t::k_void, symbol_t::make_type(typeid_t::make_void())});
	symbol_map.push_back({keyword_t::k_bool, symbol_t::make_type(typeid_t::make_bool())});
	symbol_map.push_back({keyword_t::k_int, symbol_t::make_type(typeid_t::make_int())});
	symbol_map.push_back({keyword_t::k_double, symbol_t::make_type(typeid_t::make_double())});
	symbol_map.push_back({keyword_t::k_string, symbol_t::make_type(typeid_t::make_string())});
	symbol_map.push_back({keyword_t::k_typeid, symbol_t::make_type(typeid_t::make_typeid())});
	symbol_map.push_back({keyword_t::k_json_value, symbol_t::make_type(typeid_t::make_json_value())});

	symbol_map.push_back({keyword_t::k_json_object, symbol_t::make_constant(value_t::make_int(1))});
	symbol_map.push_back({keyword_t::k_json_array, symbol_t::make_constant(value_t::make_int(2))});
	symbol_map.push_back({keyword_t::k_json_string, symbol_t::make_constant(value_t::make_int(3))});
	symbol_map.push_back({keyword_t::k_json_number, symbol_t::make_constant(value_t::make_int(4))});
	symbol_map.push_back({keyword_t::k_json_true, symbol_t::make_constant(value_t::make_int(5))});
	symbol_map.push_back({keyword_t::k_json_false, symbol_t::make_constant(value_t::make_int(6))});
	symbol_map.push_back({keyword_t::k_json_null, symbol_t::make_constant(value_t::make_int(7))});

	a._function_defs.swap(function_defs);

	const auto body = body_t(a._imm->_ast._tree._globals._statements, symbol_table_t{symbol_map});
	const auto result = analyse_body(a, body, epure::impure, typeid_t::make_undefined());

for(const auto& e: a._types.interned){
	QUARK_TRACE_SS(typeid_to_compact_string(e.second));
}

	const auto result_ast0 = pass2_ast_t{
		general_purpose_ast_t{
			._globals = result. second,
			._function_defs = result.first._function_defs,
			._interned_types = a._types,
			._software_system = result.first._software_system,
			._container_def = result.first._container_def
		}
	};

	QUARK_ASSERT(check_types_resolved(result_ast0._tree));
	const auto result_ast1 = semantic_ast_t(result_ast0._tree);

	QUARK_ASSERT(result_ast1._tree.check_invariant());
	QUARK_ASSERT(check_types_resolved(result_ast1._tree));
	return result_ast1;
}



//////////////////////////////////////		analyser_t



analyser_t::analyser_t(const pass2_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	const auto host_functions = floyd::get_host_function_signatures();
	_imm = make_shared<analyzer_imm_t>(analyzer_imm_t{ast, host_functions});
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast.check_invariant());
	return true;
}
#endif


//////////////////////////////////////		run_semantic_analysis()


semantic_ast_t run_semantic_analysis(const pass2_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	analyser_t a(ast);

	const auto result = analyse(a);
	return result;
}




ast_json_t semantic_ast_to_json(const semantic_ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	return ast_json_t::make(gp_ast_to_json(ast._tree));
}


semantic_ast_t json_to_semantic_ast(const ast_json_t& json){
	const auto gp_ast = json_to_gp_ast(json._value);
	bool resolved = check_types_resolved(gp_ast);
	if(resolved == false){
		throw std::exception();
	}
	return semantic_ast_t{ gp_ast };
}


}	//	floyd
