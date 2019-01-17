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
#include "json_parser.h"
#include "host_functions.h"
#include "text_parser.h"

namespace floyd {

using namespace std;





//////////////////////////////////////		analyzer_imm_t

//	Immutable data used by analyser.

struct analyzer_imm_t {
	public: ast_t _ast;
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
	public: analyser_t(const ast_t& ast);
	public: analyser_t(const analyser_t& other);
	public: const analyser_t& operator=(const analyser_t& other);
#if DEBUG
	public: bool check_invariant() const;
#endif
	public: void swap(analyser_t& other) throw();



	////////////////////////		STATE

	public: std::shared_ptr<const analyzer_imm_t> _imm;


	//	Non-constant. Last scope is the current one. First scope is the root.
	public: std::vector<lexical_scope_t> _lexical_scope_stack;

	//	These are output functions, that have been fixed.
	public: std::vector<std::shared_ptr<const floyd::function_definition_t>> _function_defs;

	public: software_system_t _software_system;
};



//////////////////////////////////////		forward


std::pair<analyser_t, shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement);
floyd::semantic_ast_t analyse(const analyser_t& a);
typeid_t resolve_type(const analyser_t& a, const typeid_t& type);

/*
	Return value:
		null = statements were all executed through.
		value = return statement returned a value.
*/
std::pair<analyser_t, std::vector<std::shared_ptr<floyd::statement_t>> > analyse_statements(const analyser_t& a, const std::vector<std::shared_ptr<floyd::statement_t>>& statements);

floyd::symbol_t find_global_symbol(const analyser_t& a, const std::string& s);


/*
	analyses an expression as far as possible.
	return == _constant != nullptr:	the expression was completely analysed and resulted in a constant value.
	return == _constant == nullptr: the expression was partially analyse.
*/
std::pair<analyser_t, floyd::expression_t> analyse_expression_to_target(const analyser_t& a, const floyd::expression_t& e, const floyd::typeid_t& target_type);
std::pair<analyser_t, floyd::expression_t> analyse_expression_no_target(const analyser_t& a, const floyd::expression_t& e);





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

symbol_t find_global_symbol(const analyser_t& a, const string& s){
	const auto t = find_symbol_by_name(a, s);
	if(t.first == nullptr){
		throw std::runtime_error("Undefined indentifier \"" + s + "\"!");
	}
	return *t.first;
}

typeid_t resolve_type_internal(const analyser_t& a, const typeid_t& type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto basetype = type.get_base_type();

	if(basetype == base_type::k_internal_undefined){
		throw std::runtime_error("Cannot resolve type.");
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
			members2.push_back(member_t(resolve_type(a, e._type), e._name));
		}
		return typeid_t::make_struct2(members2);
	}
	else if(basetype == base_type::k_protocol){
		const auto& protocol_def = type.get_protocol();
		std::vector<member_t> members2;
		for(const auto& e: protocol_def._members){
			members2.push_back(member_t(resolve_type(a, e._type), e._name));
		}
		return typeid_t::make_protocol(members2);
	}
	else if(basetype == base_type::k_vector){
		return typeid_t::make_vector(resolve_type(a, type.get_vector_element_type()));
	}
	else if(basetype == base_type::k_dict){
		return typeid_t::make_dict(resolve_type(a, type.get_dict_value_type()));
	}
	else if(basetype == base_type::k_function){
		const auto ret = type.get_function_return();
		const auto args = type.get_function_args();
		const auto pure = type.get_function_pure();

		const auto ret2 = resolve_type(a, ret);
		vector<typeid_t> args2;
		for(const auto& e: args){
			args2.push_back(resolve_type(a, e));
		}
		return typeid_t::make_function(ret2, args2, pure);
	}

	else if(basetype == base_type::k_internal_unresolved_type_identifier){
		const auto found = find_symbol_by_name(a, type.get_unresolved_type_identifier());
		if(found.first != nullptr){
			if(found.first->_value_type.is_typeid()){
				return found.first->_const_value.get_typeid_value();
			}
			else{
				throw std::runtime_error("Cannot resolve type.");
			}
		}
		else{
			throw std::runtime_error("Cannot resolve type.");
		}
	}
	else {
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
typeid_t resolve_type(const analyser_t& a, const typeid_t& type){
	const auto result = resolve_type_internal(a, type);
	if(result.check_types_resolved() == false){
		throw std::runtime_error("Cannot resolve type.");
	}
	return result;
}



/////////////////////////////////////////			STATEMENTS



std::pair<analyser_t, vector<statement_t>> analyse_statements(const analyser_t& a, const vector<statement_t>& statements){
	QUARK_ASSERT(a.check_invariant());
	for(const auto& i: statements){ QUARK_ASSERT(i.check_invariant()); };

	auto a_acc = a;

	vector<statement_t> statements2;
	int statement_index = 0;
	while(statement_index < statements.size()){
		const auto statement = statements[statement_index];
		const auto& r = analyse_statement(a_acc, statement);
		a_acc = r.first;

		if(r.second){
			QUARK_ASSERT(r.second->check_types_resolved());
			statements2.push_back(*r.second);
		}
		statement_index++;
	}
	return { a_acc, statements2 };
}

std::pair<analyser_t, body_t > analyse_body(const analyser_t& a, const floyd::body_t& body, epure pure){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	auto new_environment = symbol_table_t{ body._symbols };
	const auto lexical_scope = lexical_scope_t{new_environment, pure};
	a_acc._lexical_scope_stack.push_back(lexical_scope);

	const auto result = analyse_statements(a_acc, body._statements);
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
			throw std::runtime_error("Cannot assign to immutable identifier.");
		}
		else{
			const auto lhs_type = existing_value_deep_ptr.first->get_type();
			QUARK_ASSERT(lhs_type.check_types_resolved());

			const auto rhs_expr2 = analyse_expression_to_target(a_acc, statement._expression, lhs_type);
			a_acc = rhs_expr2.first;
			const auto rhs_expr3 = rhs_expr2.second;

			if(lhs_type != rhs_expr3.get_output_type()){
				throw std::runtime_error("Types not compatible in bind.");
			}
			else{
				return { a_acc, statement_t::make__store2(existing_value_deep_ptr.second, rhs_expr3) };
			}
		}
	}

	//	Bind new value -- deduce type.
	else{
		const auto rhs_expr2 = analyse_expression_no_target(a_acc, statement._expression);
		a_acc = rhs_expr2.first;
		const auto rhs_expr2_type = rhs_expr2.second.get_output_type();

		a_acc._lexical_scope_stack.back().symbols._symbols.push_back({local_name, symbol_t::make_immutable_local(rhs_expr2_type)});
		int variable_index = (int)(a_acc._lexical_scope_stack.back().symbols._symbols.size() - 1);
		return { a_acc, statement_t::make__store2(floyd::variable_address_t::make_variable_address(0, variable_index), rhs_expr2.second) };
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
	const auto lhs_type = lhs_type0.check_types_resolved() == false && lhs_type0.is_undefined() == false ? resolve_type(a_acc, lhs_type0) : lhs_type0;

	const auto bind_statement_mutable_tag_flag = statement._locals_mutable_mode == statement_t::bind_local_t::k_mutable;

	const auto value_exists_in_env = does_symbol_exist_shallow(a_acc, new_local_name);
	if(value_exists_in_env){
		throw std::runtime_error("Local identifier already exists.");
	}

	//	Setup temporary simply so function definition can find itself = recursive.
	//	Notice: the final type may not be correct yet, but for function defintions it is.
	//	This logic should be available for deduced binds too, in analyse_store_statement().
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({
		new_local_name,
		bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type) : symbol_t::make_immutable_local(lhs_type)
	});
	const auto local_name_index = a_acc._lexical_scope_stack.back().symbols._symbols.size() - 1;

	try {
		const auto rhs_expr_pair = lhs_type.is_undefined()
			? analyse_expression_no_target(a_acc, statement._expression)
			: analyse_expression_to_target(a_acc, statement._expression, lhs_type);
		a_acc = rhs_expr_pair.first;

		//??? if expression is a k_define_struct, k_define_function -- make it a constant in symbol table and emit no store-statement!

		const auto rhs_type = rhs_expr_pair.second.get_output_type();
		const auto lhs_type2 = lhs_type.is_undefined() ? rhs_type : lhs_type;

		if(lhs_type2 != lhs_type2){
			throw std::runtime_error("Types not compatible in bind.");
		}
		else{
			//	Updated the symbol with the real function defintion.
			a_acc._lexical_scope_stack.back().symbols._symbols[local_name_index] = {new_local_name, bind_statement_mutable_tag_flag ? symbol_t::make_mutable_local(lhs_type2) : symbol_t::make_immutable_local(lhs_type2)};
			return {
				a_acc,
				statement_t::make__store2(floyd::variable_address_t::make_variable_address(0, (int)local_name_index), rhs_expr_pair.second)
			};
		}
	}
	catch(...){

		//	Erase temporary symbol.
		a_acc._lexical_scope_stack.back().symbols._symbols.pop_back();

		throw;
	}
}

std::pair<analyser_t, statement_t> analyse_block_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::block_statement_t>(s._contents);
	const auto e = analyse_body(a, statement._body, a._lexical_scope_stack.back().pure);
	return {e.first, statement_t::make__block_statement(e.second)};
}

std::pair<analyser_t, statement_t> analyse_return_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::return_statement_t>(s._contents);
	auto a_acc = a;
	const auto expr = statement._expression;
	const auto result = analyse_expression_no_target(a_acc, expr);
	a_acc = result.first;

	const auto result_value = result.second;

	//	Check that return value's type matches function's return type. Cannot be done here since we don't know who called us.
	//	Instead calling code must check.
	return { a_acc, statement_t::make__return_statement(result.second) };
}

analyser_t analyse_def_struct_statement(const analyser_t& a, const statement_t::define_struct_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto struct_name = statement._name;
	if(does_symbol_exist_shallow(a_acc, struct_name)){
		throw std::runtime_error("Name already used.");
	}

	const auto struct_typeid1 = typeid_t::make_struct2(statement._def->_members);
	const auto struct_typeid2 = resolve_type(a_acc, struct_typeid1);
	const auto struct_typeid_value = value_t::make_typeid_value(struct_typeid2);
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({struct_name, symbol_t::make_constant(struct_typeid_value)});

	return a_acc;
}

analyser_t analyse_def_protocol_statement(const analyser_t& a, const statement_t::define_protocol_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto protocol_name = statement._name;
	if(does_symbol_exist_shallow(a_acc, protocol_name)){
		throw std::runtime_error("Name already used.");
	}

	const auto protocol_typeid1 = typeid_t::make_protocol(statement._def->_members);
	const auto protocol_typeid2 = resolve_type(a_acc, protocol_typeid1);
	const auto protocol_typeid_value = value_t::make_typeid_value(protocol_typeid2);
	a_acc._lexical_scope_stack.back().symbols._symbols.push_back({protocol_name, symbol_t::make_constant(protocol_typeid_value)});

	return a_acc;
}

std::pair<analyser_t, statement_t> analyse_def_function_statement(const analyser_t& a, const statement_t& s){
	QUARK_ASSERT(a.check_invariant());

	const auto statement = std::get<statement_t::define_function_statement_t>(s._contents);

	auto a_acc = a;

	//	Translates into:  bind-local, "myfunc", function_definition_expr_t
	const auto function_def_expr = expression_t::make_function_definition(statement._def);
	const auto& s2 = statement_t::make__bind_local(
		statement._name,
		statement._def->_function_type,
		function_def_expr,
		statement_t::bind_local_t::k_immutable
	);
	const auto s3 = analyse_bind_local_statement(a_acc, s2);
	return s3;
}

std::pair<analyser_t, statement_t> analyse_ifelse_statement(const analyser_t& a, const statement_t::ifelse_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto condition2 = analyse_expression_no_target(a_acc, statement._condition);
	a_acc = condition2.first;

	const auto condition_type = condition2.second.get_output_type();
	if(condition_type.is_bool() == false){
		throw std::runtime_error("Boolean condition required.");
	}

	const auto then2 = analyse_body(a_acc, statement._then_body, a._lexical_scope_stack.back().pure);
	const auto else2 = analyse_body(a_acc, statement._else_body, a._lexical_scope_stack.back().pure);
	return { a_acc, statement_t::make__ifelse_statement(condition2.second, then2.second, else2.second) };
}

std::pair<analyser_t, statement_t> analyse_for_statement(const analyser_t& a, const statement_t::for_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto start_expr2 = analyse_expression_no_target(a_acc, statement._start_expression);
	a_acc = start_expr2.first;

	if(start_expr2.second.get_output_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto end_expr2 = analyse_expression_no_target(a_acc, statement._end_expression);
	a_acc = end_expr2.first;

	if(end_expr2.second.get_output_type().is_int() == false){
		throw std::runtime_error("For-loop requires integer iterator.");
	}

	const auto iterator_symbol = symbol_t::make_immutable_local(typeid_t::make_int());

	//	Add the iterator as a symbol to the body of the for-loop.
	auto symbols = statement._body._symbols;
	symbols._symbols.push_back({ statement._iterator_name, iterator_symbol});
	const auto body_injected = body_t(statement._body._statements, symbols);
	const auto result = analyse_body(a_acc, body_injected, a._lexical_scope_stack.back().pure);
	a_acc = result.first;

	return { a_acc, statement_t::make__for_statement(statement._iterator_name, start_expr2.second, end_expr2.second, result.second, statement._range_type) };
}

std::pair<analyser_t, statement_t> analyse_while_statement(const analyser_t& a, const statement_t::while_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto condition2_expr = analyse_expression_no_target(a_acc, statement._condition);
	a_acc = condition2_expr.first;

	const auto result = analyse_body(a_acc, statement._body, a._lexical_scope_stack.back().pure);
	a_acc = result.first;

	return { a_acc, statement_t::make__while_statement(condition2_expr.second, result.second) };
}

std::pair<analyser_t, statement_t> analyse_expression_statement(const analyser_t& a, const statement_t::expression_statement_t& statement){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto expr2 = analyse_expression_no_target(a_acc, statement._expression);
	a_acc = expr2.first;

	return { a_acc, statement_t::make__expression_statement(expr2.second) };
}

//	Output is the RETURN VALUE of the analysed statement, if any.
std::pair<analyser_t, shared_ptr<statement_t>> analyse_statement(const analyser_t& a, const statement_t& statement){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(statement.check_invariant());



	typedef std::pair<analyser_t, shared_ptr<statement_t>> return_type;


	struct visitor_t {
		const analyser_t& a;
		const statement_t& statement;

		return_type operator()(const statement_t::return_statement_t& s) const{
			const auto e = analyse_return_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::define_struct_statement_t& s) const{
			const auto e = analyse_def_struct_statement(a, s);
			return { e, {} };
		}
		return_type operator()(const statement_t::define_protocol_statement_t& s) const{
			const auto e = analyse_def_protocol_statement(a, s);
			return { e, {} };
		}
		return_type operator()(const statement_t::define_function_statement_t& s) const{
			const auto e = analyse_def_function_statement(a, statement);
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		return_type operator()(const statement_t::bind_local_t& s) const{
			const auto e = analyse_bind_local_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::store_t& s) const{
			const auto e = analyse_store_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::store2_t& s) const{
			QUARK_ASSERT(false);
		}
		return_type operator()(const statement_t::block_statement_t& s) const{
			const auto e = analyse_block_statement(a, statement);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}

		return_type operator()(const statement_t::ifelse_statement_t& s) const{
			const auto e = analyse_ifelse_statement(a, s);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::for_statement_t& s) const{
			const auto e = analyse_for_statement(a, s);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::while_statement_t& s) const{
			const auto e = analyse_while_statement(a, s);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}


		return_type operator()(const statement_t::expression_statement_t& s) const{
			const auto e = analyse_expression_statement(a, s);
			QUARK_ASSERT(e.second.check_types_resolved());
			return { e.first, std::make_shared<statement_t>(e.second) };
		}
		return_type operator()(const statement_t::software_system_statement_t& s) const{
			analyser_t temp = a;
			temp._software_system = parse_software_system_json(s._json_data);
			return { temp, std::make_shared<statement_t>(statement
			) };
		}
	};

	return std::visit(visitor_t{a, statement}, statement._contents);
}



/////////////////////////////////////////			EXPRESSIONS



std::pair<analyser_t, expression_t> analyse_resolve_member_expression(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto parent_expr = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = parent_expr.first;

	const auto parent_type = parent_expr.second.get_output_type();

	if(parent_type.is_struct()){
		const auto struct_def = parent_type.get_struct();

		//??? store index in new expression
		int index = find_struct_member_index(struct_def, e._variable_name);
		if(index == -1){
			throw std::runtime_error("Unknown struct member \"" + e._variable_name + "\".");
		}
		const auto member_type = struct_def._members[index]._type;
		return { a_acc, expression_t::make_resolve_member(parent_expr.second, e._variable_name, make_shared<typeid_t>(member_type))};
	}
	else{
		throw std::runtime_error("Parent is not a struct.");
	}
}

std::pair<analyser_t, expression_t> analyse_lookup_element_expression(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto parent_expr = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = parent_expr.first;

	const auto key_expr = analyse_expression_no_target(a_acc, e._input_exprs[1]);
	a_acc = key_expr.first;

	const auto parent_type = parent_expr.second.get_output_type();
	const auto key_type = key_expr.second.get_output_type();

	if(parent_type.is_string()){
		if(key_type.is_int() == false){
			throw std::runtime_error("Lookup in string by index-only.");
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
			throw std::runtime_error("Lookup in vector by index-only.");
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_vector_element_type())) };
		}
	}
	else if(parent_type.is_dict()){
		if(key_type.is_string() == false){
			throw std::runtime_error("Lookup in dict by string-only.");
		}
		else{
			return { a_acc, expression_t::make_lookup(parent_expr.second, key_expr.second, make_shared<typeid_t>(parent_type.get_dict_value_type())) };
		}
	}
	else {
		throw std::runtime_error("Lookup using [] only works with strings, vectors, dicts and json_value.");
	}
}

std::pair<analyser_t, expression_t> analyse_load(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto found = find_symbol_by_name(a_acc, e._variable_name);
	if(found.first != nullptr){
		return {a_acc, expression_t::make_load2(found.second, make_shared<typeid_t>(found.first->_value_type)) };
	}
	else{
		throw std::runtime_error("Undefined variable \"" + e._variable_name + "\".");
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
std::pair<analyser_t, expression_t> analyse_construct_value_expression(const analyser_t& a, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto current_type = *e._output_type;
	if(current_type.is_vector()){
		//	JSON constants supports mixed element types: convert each element into a json_value.
		//	Encode as [json_value]
		if(target_type.is_json_value()){
			const auto element_type = typeid_t::make_json_value();

			std::vector<expression_t> elements2;
			for(const auto& m: e._input_exprs){
				const auto element_expr = analyse_expression_to_target(a_acc, m, element_type);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}
			const auto result_type = typeid_t::make_vector(typeid_t::make_json_value());
			if(result_type.check_types_resolved() == false){
				throw std::runtime_error("Cannot resolve type.");
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
			for(const auto& m: e._input_exprs){
				const auto element_expr = analyse_expression_no_target(a_acc, m);
				a_acc = element_expr.first;
				elements2.push_back(element_expr.second);
			}

			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? elements2[0].get_output_type() : element_type;
			const auto result_type0 = typeid_t::make_vector(element_type2);
			const auto result_type = result_type0.check_types_resolved() == false && target_type.is_internal_dynamic() == false ? target_type : result_type0;

			if(result_type.check_types_resolved() == false){
				throw std::runtime_error("Cannot resolve type.");
			}

			for(const auto& m: elements2){
				if(m.get_output_type() != element_type2){
					throw std::runtime_error("Vector can not hold elements of different types.");
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
			for(int i = 0 ; i < e._input_exprs.size() / 2 ; i++){
				const auto& key = e._input_exprs[i * 2 + 0].get_literal().get_string_value();
				const auto& value = e._input_exprs[i * 2 + 1];
				const auto element_expr = analyse_expression_to_target(a_acc, value, element_type);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			const auto result_type = typeid_t::make_dict(typeid_t::make_json_value());
			if(result_type.check_types_resolved() == false){
				throw std::runtime_error("Cannot resolve type.");
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
			QUARK_ASSERT(e._input_exprs.size() % 2 == 0);

			const auto element_type = current_type.get_dict_value_type();

			std::vector<expression_t> elements2;
			for(int i = 0 ; i < e._input_exprs.size() / 2 ; i++){
				const auto& key = e._input_exprs[i * 2 + 0].get_literal().get_string_value();
				const auto& value = e._input_exprs[i * 2 + 1];
				const auto element_expr = analyse_expression_no_target(a_acc, value);
				a_acc = element_expr.first;
				elements2.push_back(expression_t::make_literal_string(key));
				elements2.push_back(element_expr.second);
			}

			//	Deduce type of dictionary based on first value.
			const auto element_type2 = element_type.is_undefined() && elements2.size() > 0 ? elements2[0 * 2 + 1].get_output_type() : element_type;
			const auto result_type0 = typeid_t::make_dict(element_type2);
			const auto result_type = result_type0.check_types_resolved() == false && target_type.is_internal_dynamic() == false ? target_type : result_type0;

			//	Make sure all elements have the correct type.
			for(int i = 0 ; i < elements2.size() / 2 ; i++){
				const auto element_type0 = elements2[i * 2 + 1].get_output_type();
				if(element_type0 != element_type2){
					throw std::runtime_error("Dict can not hold elements of different type!");
				}
			}
			return {a_acc, expression_t::make_construct_value_expr(result_type, elements2)};
		}
	}
	else{
		QUARK_ASSERT(false);
	}
	throw std::exception();
}

std::pair<analyser_t, expression_t> analyse_arithmetic_unary_minus_expression(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto& expr2 = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = expr2.first;

	//??? We could simplify here and return [ "-", 0, expr]
	const auto type = expr2.second.get_output_type();
	if(type.is_int() || type.is_double()){
		return {a_acc, expression_t::make_unary_minus(expr2.second, make_shared<typeid_t>(type))  };
	}
	else{
		throw std::runtime_error("Unary minus won't work on expressions of type \"" + json_to_compact_string(typeid_to_ast_json(type, floyd::json_tags::k_plain)._value) + "\".");
	}
}

std::pair<analyser_t, expression_t> analyse_conditional_operator_expression(const analyser_t& analyser, const expression_t& e){
	QUARK_ASSERT(analyser.check_invariant());

	auto analyser_acc = analyser;

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	const auto cond_result = analyse_expression_no_target(analyser_acc, e._input_exprs[0]);
	analyser_acc = cond_result.first;

	const auto a = analyse_expression_no_target(analyser_acc, e._input_exprs[1]);
	analyser_acc = a.first;

	const auto b = analyse_expression_no_target(analyser_acc, e._input_exprs[2]);
	analyser_acc = b.first;

	const auto type = cond_result.second.get_output_type();
	if(type.is_bool() == false){
		throw std::runtime_error("Could not analyse condition in conditional expression.");
	}
	else if(a.second.get_output_type() != b.second.get_output_type()){
		throw std::runtime_error("Conditional expression requires both true/false conditions to have the same type.");
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

std::pair<analyser_t, expression_t> analyse_comparison_expression(const analyser_t& a, expression_type op, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse all inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_output_type();

	//	Make rhs match left if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, e._input_exprs[1], lhs_type);
	a_acc = right_expr.first;
	const auto rhs_type = right_expr.second.get_output_type();

	if(lhs_type != rhs_type && lhs_type.is_undefined() == false && rhs_type.is_undefined() == false){
		throw std::runtime_error("Comparison: Left and right expressions must be same type!");
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
			throw std::exception();
		}
		return {
			a_acc,
			expression_t::make_simple_expression__2(
				e.get_operation(),
				left_expr.second,
				right_expr.second,
				make_shared<typeid_t>(typeid_t::make_bool())
			)
		};
	}
}

std::pair<analyser_t, expression_t> analyse_arithmetic_expression(const analyser_t& a, expression_type op, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	//	First analyse both inputs to our operation.
	const auto left_expr = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = left_expr.first;

	const auto lhs_type = left_expr.second.get_output_type();

	//	Make rhs match lhs if needed/possible.
	const auto right_expr = analyse_expression_to_target(a_acc, e._input_exprs[1], lhs_type);
	a_acc = right_expr.first;

	const auto rhs_type = right_expr.second.get_output_type();


	if(lhs_type != rhs_type){
		throw std::runtime_error("Artithmetics: Left and right expressions must be same type!");
	}
	else{
		const auto shared_type = lhs_type;

		//	bool
		if(shared_type.is_bool()){
			if(op == expression_type::k_arithmetic_add__2){
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on bool.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
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
				throw std::exception();
			}

			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
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
				throw std::runtime_error("Modulo operation on double not allowed.");
			}

			else if(op == expression_type::k_logical_and__2){
			}
			else if(op == expression_type::k_logical_or__2){
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	string
		else if(shared_type.is_string()){
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on string.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on string.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	struct
		else if(shared_type.is_struct()){
			//	Structs must be exactly the same type to match.

			if(op == expression_type::k_arithmetic_add__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}

			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on struct.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}

			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	vector
		else if(shared_type.is_vector()){
			const auto element_type = shared_type.get_vector_element_type();
			if(op == expression_type::k_arithmetic_add__2){
			}

			else if(op == expression_type::k_arithmetic_subtract__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_multiply__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_divide__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_arithmetic_remainder__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}


			else if(op == expression_type::k_logical_and__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else if(op == expression_type::k_logical_or__2){
				throw std::runtime_error("Operation not allowed on vectors.");
			}
			else{
				QUARK_ASSERT(false);
				throw std::exception();
			}
			return {a_acc, expression_t::make_simple_expression__2(e.get_operation(), left_expr.second, right_expr.second, make_shared<typeid_t>(shared_type)) };
		}

		//	function
		else if(shared_type.is_function()){
			throw std::runtime_error("Cannot perform operations on two function values.");
		}
		else{
			throw std::runtime_error("Arithmetics failed.");
		}
	}
}

/*
	Magic support variable argument functions ,like c-lang (...). Use a function taking ONE argument of type internal_dynamic.
*/
std::pair<analyser_t, vector<expression_t>> analyze_call_args(const analyser_t& a, const vector<expression_t>& call_args, const std::vector<typeid_t>& callee_args){
	if(callee_args.size() == 1 && callee_args[0].is_internal_dynamic()){
		auto a_acc = a;
		vector<expression_t> call_args2;
		for(int i = 0 ; i < call_args.size() ; i++){
			const auto call_arg_pair = analyse_expression_no_target(a_acc, call_args[i]);
			a_acc = call_arg_pair.first;
			call_args2.push_back(call_arg_pair.second);
		}
		return { a_acc, call_args2 };
	}
	else{
		//	arity
		if(call_args.size() != callee_args.size()){
			throw std::runtime_error("Wrong number of arguments in function call.");
		}

		auto a_acc = a;
		vector<expression_t> call_args2;
		for(int i = 0 ; i < callee_args.size() ; i++){
			const auto callee_arg = callee_args[i];

			const auto call_arg_pair = analyse_expression_to_target(a_acc, call_args[i], callee_arg);
			a_acc = call_arg_pair.first;
			call_args2.push_back(call_arg_pair.second);
		}
		return { a_acc, call_args2 };
	}
}

bool is_host_function_call(const analyser_t& a, const expression_t& callee_expr){
	if(callee_expr.get_operation() == expression_type::k_load){
		const auto function_name = callee_expr._variable_name;

		const auto find_it = a._imm->_host_functions.find(function_name);
		return find_it != a._imm->_host_functions.end();
	}
	else if(callee_expr.get_operation() == expression_type::k_load2){
		const auto callee = resolve_symbol_by_address(a, callee_expr._address);
		QUARK_ASSERT(callee != nullptr);

		if(callee->_const_value.is_function()){
			const auto& function_def = function_id_to_def(a, callee->_const_value.get_function_value());
			return function_def._host_function_id != 0;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

typeid_t get_host_function_return_type(const analyser_t& a, const expression_t& callee_expr, const vector<expression_t>& args){
	QUARK_ASSERT(is_host_function_call(a, callee_expr));
	QUARK_ASSERT(callee_expr.get_operation() == expression_type::k_load2);

	const auto callee = resolve_symbol_by_address(a, callee_expr._address);
	QUARK_ASSERT(callee != nullptr);

	const auto& function_def = function_id_to_def(a, callee->_const_value.get_function_value());

	const auto host_function_id = function_def._host_function_id;

	const auto& host_functions = a._imm->_host_functions;
	const auto found_it = find_if(host_functions.begin(), host_functions.end(), [&](const std::pair<std::string, floyd::host_function_signature_t>& kv){ return kv.second._function_id == host_function_id; });
	QUARK_ASSERT(found_it != host_functions.end());

	const auto function_name = found_it->first;

	if(function_name == "update"){
		return args[0].get_output_type();
	}
	else if(function_name == "erase"){
		return args[0].get_output_type();
	}
	else if(function_name == "push_back"){
		return args[0].get_output_type();
	}
	else if(function_name == "subset"){
		return args[0].get_output_type();
	}
	else if(function_name == "replace"){
		return args[0].get_output_type();
	}
	else if(function_name == "map"){
		const auto f = args[1].get_output_type().get_function_return();
		//	[R] map([E], R f(E e))

		const auto ret = typeid_t::make_vector(f);
		return ret;
	}
	else if(function_name == "reduce"){
		const auto f = args[1].get_output_type();
		return f;
	}
/*
	else if(function_name == "instantiate_from_typeid"){
		if(args[0].get_operation() == expression_type::k_load2){
			const auto symbol = resolve_symbol_by_address(a, args[0]._address);
			if(symbol != nullptr && symbol->_const_value.is_undefined() == false){
				return symbol->_const_value.get_typeid_value();
			}
			else{
				throw std::runtime_error("Cannot resolve type for instantiate_from_typeid().");
			}
		}
		else{
			throw std::runtime_error("Cannot resolve type for instantiate_from_typeid().");
		}
	}
*/
	else if(function_name == "jsonvalue_to_value"){
		QUARK_ASSERT(args.size() == 2);

		const auto arg0 = args[0];
		const auto arg1 = args[1];
		if(arg1.get_operation() == expression_type::k_load2){
			const auto symbol = resolve_symbol_by_address(a, arg1._address);
			if(symbol != nullptr && symbol->_const_value.is_undefined() == false){
				return symbol->_const_value.get_typeid_value();
			}
			else{
				throw std::runtime_error("Cannot resolve type for jsonvalue_to_value().");
			}
		}
		else{
			throw std::runtime_error("Cannot resolve type for jsonvalue_to_value().");
		}
	}
	else{
		return callee_expr.get_output_type().get_function_return();
	}
}

/*
	Notice: e._input_expr[0] is callee, the remaining are arguments.
*/
std::pair<analyser_t, expression_t> analyse_call_expression(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto callee_expr0 = analyse_expression_no_target(a_acc, e._input_exprs[0]);
	a_acc = callee_expr0.first;
	const auto callee_expr = callee_expr0.second;

	const auto args0 = vector<expression_t>(e._input_exprs.begin() + 1, e._input_exprs.end());

	const auto callsite_pure = a_acc._lexical_scope_stack.back().pure;

	//	This is a call to a function-value. Callee is a function-type.
	const auto callee_type = callee_expr.get_output_type();
	if(callee_type.is_function()){
		const auto callee_args = callee_type.get_function_args();
		const auto callee_return_value = callee_type.get_function_return();
		const auto callee_pure = callee_type.get_function_pure();

		if(callsite_pure == epure::pure && callee_pure == epure::impure){
			throw std::runtime_error(std::string() + "Cannot call impure function pure function.");
		}


		const auto call_args_pair = analyze_call_args(a_acc, args0, callee_args);
		a_acc = call_args_pair.first;
		if(is_host_function_call(a, callee_expr)){
			const auto return_type = get_host_function_return_type(a, callee_expr, call_args_pair.second);
			return { a_acc, expression_t::make_call(callee_expr, call_args_pair.second, make_shared<typeid_t>(return_type)) };
		}
		else{
			return { a_acc, expression_t::make_call(callee_expr, call_args_pair.second, make_shared<typeid_t>(callee_return_value)) };
		}
	}

	//	Attempting to call a TYPE? Then this may be a constructor call.
	//	Converts these calls to construct-value-expressions.
	else if(callee_type.is_typeid() && callee_expr.get_operation() == expression_type::k_load2){
		const auto found_symbol_ptr = resolve_symbol_by_address(a_acc, callee_expr._address);
		QUARK_ASSERT(found_symbol_ptr != nullptr);

		if(found_symbol_ptr->_const_value.is_undefined()){
			throw std::runtime_error("Cannot resolve callee.");
		}
		else{
			const auto callee_type2 = found_symbol_ptr->_const_value.get_typeid_value();

			//	Convert calls to struct-type into construct-value expression.
			if(callee_type2.is_struct()){
				const auto& def = callee_type2.get_struct();
				const auto callee_args = get_member_types(def._members);
				const auto call_args_pair = analyze_call_args(a_acc, args0, callee_args);
				a_acc = call_args_pair.first;

				return { a_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}

			//	One argument for primitive types.
			else{
				const auto callee_args = vector<typeid_t>{ callee_type2 };
				QUARK_ASSERT(callee_args.size() == 1);
				const auto call_args_pair = analyze_call_args(a_acc, args0, callee_args);
				a_acc = call_args_pair.first;
				return { a_acc, expression_t::make_construct_value_expr(callee_type2, call_args_pair.second) };
			}
		}
	}
	else{
		throw std::runtime_error("Cannot call non-function.");
	}
}

std::pair<analyser_t, expression_t> analyse_struct_definition_expression(const analyser_t& a, const expression_t& e0){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;
	const auto& struct_def = *e0._struct_def;

	//	Resolve member types in this scope.
	std::vector<member_t> members2;
	for(const auto& e: struct_def._members){
		const auto name = e._name;
		const auto type = e._type;
		const auto type2 = resolve_type(a_acc, type);
		const auto e2 = member_t(type2, name);
		members2.push_back(e2);
	}
	const auto resolved_struct_def = std::make_shared<struct_definition_t>(struct_definition_t(members2));
	return {a_acc, expression_t::make_struct_definition(resolved_struct_def) };
}

// ??? Check that function returns a value, if so specified.
std::pair<analyser_t, expression_t> analyse_function_definition_expression(const analyser_t& a, const expression_t& e){
	QUARK_ASSERT(a.check_invariant());

	auto a_acc = a;

	const auto function_def = e._function_def;
	const auto function_type2 = resolve_type(a_acc, function_def->_function_type);
	const auto function_pure = function_type2.get_function_pure();

	vector<member_t> args2;
	for(const auto& arg: function_def->_args){
		const auto arg_type2 = resolve_type(a_acc, arg._type);
		args2.push_back(member_t(arg_type2, arg._name));
	}

	//	Make function body with arguments injected FIRST in body as local symbols.
	auto symbol_vec = function_def->_body->_symbols;
	for(const auto& arg: args2){
		symbol_vec._symbols.push_back({arg._name , symbol_t::make_immutable_local(arg._type)});
	}
	const auto function_body2 = body_t(function_def->_body->_statements, symbol_vec);


	//??? Can there be a pure function inside an impure lexical scope?
	const auto pure = function_pure;

	const auto function_body_pair = analyse_body(a_acc, function_body2, pure);
	a_acc = function_body_pair.first;
	const auto function_body3 = function_body_pair.second;

	const auto function_def2 = function_definition_t{ function_type2, args2, make_shared<body_t>(function_body3), 0 };

	QUARK_ASSERT(function_def2.check_types_resolved());

	a_acc._function_defs.push_back(make_shared<function_definition_t>(function_def2));

	const int function_id = static_cast<int>(a_acc._function_defs.size() - 1);
	const auto r = expression_t::make_literal(value_t::make_function_value(function_type2, function_id));

	return {a_acc, r };
}


std::pair<analyser_t, expression_t> analyse_expression__operation_specific(const analyser_t& a, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto op = e.get_operation();

	if(op == expression_type::k_literal){
		return { a, e };
	}
	else if(op == expression_type::k_resolve_member){
		return analyse_resolve_member_expression(a, e);
	}
	else if(op == expression_type::k_lookup_element){
		return analyse_lookup_element_expression(a, e);
	}
	else if(op == expression_type::k_load){
		return analyse_load(a, e);
	}
	else if(op == expression_type::k_load2){
		return analyse_load2(a, e);
	}

	else if(op == expression_type::k_call){
		return analyse_call_expression(a, e);
	}

	else if(op == expression_type::k_define_struct){
		return analyse_struct_definition_expression(a, e);
	}

	else if(op == expression_type::k_define_function){
		return analyse_function_definition_expression(a, e);
	}

	else if(op == expression_type::k_construct_value){
		return analyse_construct_value_expression(a, e, target_type);
	}

	else if(op == expression_type::k_arithmetic_unary_minus__1){
		return analyse_arithmetic_unary_minus_expression(a, e);
	}

	//	Special-case since it uses 3 expressions & uses shortcut evaluation.
	else if(op == expression_type::k_conditional_operator3){
		return analyse_conditional_operator_expression(a, e);
	}
	else if(is_comparison_expression(op)){
		return analyse_comparison_expression(a, op, e);
	}
	else if(is_arithmetic_expression(op)){
		return analyse_arithmetic_expression(a, op, e);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
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

//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_to_target(const analyser_t& a, const expression_t& e, const typeid_t& target_type){
	QUARK_ASSERT(a.check_invariant());
	QUARK_ASSERT(e.check_invariant());
	QUARK_ASSERT(target_type.is_void() == false && target_type.is_undefined() == false);
	QUARK_ASSERT(target_type.check_types_resolved());

	auto a_acc = a;
	const auto e2_pair = analyse_expression__operation_specific(a_acc, e, target_type);
	a_acc = e2_pair.first;
	const auto e2b = e2_pair.second;
	if(e2b.check_types_resolved() == false){
		throw std::runtime_error("Cannot resolve type.");
	}

	const auto e3 = auto_cast_expression_type(e2b, target_type);

	if(target_type.is_internal_dynamic()){
	}
	else if(e3.get_output_type() == target_type){
	}
	else if(e3.get_output_type().is_undefined()){
		QUARK_ASSERT(false);
		throw std::runtime_error("Expression type mismatch.");
	}
	else{
		const auto err =
			std::string()
			+ "Expression type mismatch. Cannot convert '"
			+ typeid_to_compact_string( e3.get_output_type())
			+ "' to '" + typeid_to_compact_string(target_type)
			+ ".";
		throw std::runtime_error(err);
	}

	if(e3.check_types_resolved() == false){
		throw std::runtime_error("Cannot resolve type.");
	}
	QUARK_ASSERT(e3.check_types_resolved());
	return { a_acc, e3 };
}

//	Returned expression is guaranteed to be deep-resolved.
std::pair<analyser_t, expression_t> analyse_expression_no_target(const analyser_t& a, const expression_t& e){
	return analyse_expression_to_target(a, e, typeid_t::make_internal_dynamic());
}

/*
json_t analyser_to_json(const analyser_t& a){
	vector<json_t> callstack;
	for(const auto& e: a._lexical_scope_stack){
		std::map<string, json_t> values;
		for(const auto& symbol_kv: e.symbols._symbols){
			const auto b = json_t::make_array({
				json_t((int)symbol_kv.second._symbol_type),
				typeid_to_ast_json(symbol_kv.second._value_type, floyd::json_tags::k_tag_resolve_state)._value,
				value_to_ast_json(symbol_kv.second._const_value, floyd::json_tags::k_tag_resolve_state)._value
			});
			values[symbol_kv.first] = b;
		}

		const auto& env = json_t::make_object({
//			{ "parent_env", e->_parent_env ? e->_parent_env->_object_id : json_t() },
//			{ "object_id", json_t(double(e->_object_id)) },
			{ "values", values }
		});
		callstack.push_back(env);
	}

	return json_t::make_object({
		{ "ast", ast_to_json(a._imm->_ast)._value },
		{ "host_functions", json_t("dummy1234") },
		{ "callstack", json_t::make_array(callstack) }
	});
}
*/

void test__analyse_expression(const expression_t& e, const expression_t& expected){
	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter, e);

	ut_compare_jsons(expression_to_json(e3.second)._value, expression_to_json(expected)._value);
}


QUARK_UNIT_TEST("analyse_expression_no_target()", "literal 1234 == 1234", "", "") {
	test__analyse_expression(
		expression_t::make_literal_int(1234),
		expression_t::make_literal_int(1234)
	);
}

QUARK_UNIT_TESTQ("analyse_expression_no_target()", "1 + 2 == 3") {
	const ast_t ast;
	const analyser_t interpreter(ast);
	const auto e3 = analyse_expression_no_target(interpreter,
		expression_t::make_simple_expression__2(
			expression_type::k_arithmetic_add__2,
			expression_t::make_literal_int(1),
			expression_t::make_literal_int(2),
			nullptr
		)
	);

	ut_compare_jsons(
		expression_to_json(e3.second)._value,
		parse_json(seq_t(R"(   ["+", ["k", 1, "^int"], ["k", 2, "^int"], "^int"]   )")).first
	);
}


bool check_types_resolved(const ast_t& ast){
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

semantic_ast_t analyse(const analyser_t& a){
	QUARK_ASSERT(a.check_invariant());

	/*
		Create built-in globla symbol map: built in data types, built-in functions (host functions).
	*/
	std::vector<std::pair<std::string, symbol_t>> symbol_map;

	auto function_defs = a._imm->_ast._function_defs;

	//	Insert built-in functions.
	for(auto hf_kv: a._imm->_host_functions){
		const auto& function_name = hf_kv.first;
		const auto& signature = hf_kv.second;

		const auto args = [&](){
			vector<member_t> result;
			for(const auto& e: signature._function_type.get_function_args()){
				result.push_back(member_t(e, "dummy"));
			}
			return result;
		}();

		const auto def = make_shared<function_definition_t>(function_definition_t{signature._function_type, args, {}, signature._function_id});

		const auto function_id = static_cast<int>(function_defs.size());
		function_defs.push_back(def);

		const auto function_value = value_t::make_function_value(signature._function_type, function_id);

		symbol_map.push_back({function_name, symbol_t::make_constant(function_value)});
	}

	//	"null" is equivalent to json_value::null
	symbol_map.push_back({"null", symbol_t::make_constant(value_t::make_json_value(json_t()))});

	symbol_map.push_back({keyword_t::k_internal_undefined, symbol_t::make_constant(value_t::make_undefined())});
	symbol_map.push_back({keyword_t::k_internal_dynamic, symbol_t::make_constant(value_t::make_internal_dynamic())});

	symbol_map.push_back({keyword_t::k_void, symbol_t::make_constant(value_t::make_void())});
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

	auto analyser2 = a;
	analyser2._function_defs.swap(function_defs);

	const auto body = body_t(analyser2._imm->_ast._globals._statements, symbol_table_t{symbol_map});
	const auto result = analyse_body(analyser2, body, epure::impure);
	const auto result_ast0 = ast_t{
		._globals = result. second,
		._function_defs = result.first._function_defs,
		._software_system = result.first._software_system
	};

	const auto result_ast1 = semantic_ast_t(result_ast0);
	QUARK_ASSERT(result_ast1._checked_ast.check_invariant());
	QUARK_ASSERT(check_types_resolved(result_ast1._checked_ast));
	return result_ast1;
}




//////////////////////////////////////		analyser_t



analyser_t::analyser_t(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	const auto host_functions = floyd::get_host_function_signatures();
	_imm = make_shared<analyzer_imm_t>(analyzer_imm_t{ast, host_functions});
}

analyser_t::analyser_t(const analyser_t& other) :
	_imm(other._imm),
	_lexical_scope_stack(other._lexical_scope_stack),
	_function_defs(other._function_defs),
	_software_system(other._software_system)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

void analyser_t::swap(analyser_t& other) throw() {
	_imm.swap(other._imm);
	_lexical_scope_stack.swap(other._lexical_scope_stack);
	_function_defs.swap(other._function_defs);
	std::swap(_software_system, other._software_system);
}

const analyser_t& analyser_t::operator=(const analyser_t& other){
	auto temp = other;
	swap(temp);
	return *this;
}

#if DEBUG
bool analyser_t::check_invariant() const {
	QUARK_ASSERT(_imm->_ast.check_invariant());
	return true;
}
#endif



//////////////////////////////////////		run_semantic_analysis()


semantic_ast_t run_semantic_analysis(const ast_t& ast){
	QUARK_ASSERT(ast.check_invariant());

	analyser_t a(ast);
	const auto result = analyse(a);
	return result;
}


}	//	floyd
