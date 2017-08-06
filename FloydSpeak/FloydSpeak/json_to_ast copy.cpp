
//??? Simplify now that expression_t doesn't have classes for each expression type.
expression_t expression_from_json(const json_t& e, const map<string, symbol_t>& types){
	QUARK_ASSERT(e.is_array() && e.get_array_size() >= 1);

	const string op = e.get_array_n(0).get_string();
	if(op == "k"){
//???
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto value = e.get_array_n(1);
		const auto type = e.get_array_n(2);

		//	#Basic-types
		if(type == "$null"){
			return expression_t::make_constant(value_t());
		}
		else if(type == "$bool"){
			return expression_t::make_constant(value.is_true());
		}
		else if(type == "$int"){
			return expression_t::make_constant((int)value.get_number());
		}
		else if(type == "$float"){
			return expression_t::make_constant((float)value.get_number());
		}
		else if(type == "$string"){
			return expression_t::make_constant(value.get_string());
		}
		else{
			const auto it = types.find(type.get_string());
			if(it == types.end()){
				QUARK_ASSERT(false);
			}
			const auto base_type = it->second._typeid.get_base_type();
			if(base_type == base_type::k_struct){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(false);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(false);
			}
			else{
				QUARK_ASSERT(false);
			}
		}
	}
	else if(is_math2_op(op)){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto lhs_expr = expression_from_json(e.get_array_n(1), types);
		const auto rhs_expr = expression_from_json(e.get_array_n(2), types);
		const auto op2 = string_to_math2_op(op);
		return expression_t::make_math2_operation(op2, lhs_expr, rhs_expr);
	}
	else if(op == "?:"){
		QUARK_ASSERT(e.get_array_size() == 5);
		const auto condition_expr = expression_from_json(e.get_array_n(1), types);
		const auto a_expr = expression_from_json(e.get_array_n(2), types);
		const auto b_expr = expression_from_json(e.get_array_n(3), types);
		const auto expr_type = resolve_type123(e.get_array_n(4).get_string(), types);
		return expression_t::make_conditional_operator(condition_expr, a_expr, b_expr);
	}
	else if(op == "call"){
		QUARK_ASSERT(e.get_array_size() == 4);

		const auto f_address = expression_from_json(e.get_array_n(1), types);

		//??? Hack - we should have real expressions for function names.
		//??? Also: we should resolve all names 100% at this point.
		QUARK_ASSERT(f_address.get_operation() == expression_t::operation::k_resolve_variable);
		const string func_name = f_address.get_symbol();


//		const auto function_expr = expression_from_json(e.get_array_n(1), types);

		const auto args = e.get_array_n(2);
		vector<expression_t> args2;
		for(const auto& arg: args.get_array()){
			const auto arg2 = expression_from_json(arg, types);
			args2.push_back(arg2);
		}

		const auto return_type = resolve_type123(e.get_array_n(3).get_string(), types);
		return expression_t::make_function_call(type_identifier_t::make(func_name), args2, return_type);
//		return expression_t::make_function_call(function_expr, args2, return_type);
	}
	else if(op == "->"){
		QUARK_ASSERT(e.get_array_size() == 4);
		const auto base_expr = expression_from_json(e.get_array_n(1), types);
		const auto member_name = e.get_array_n(2).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(3).get_string(), types);
		return expression_t::make_resolve_member(base_expr, member_name, expr_type);
	}
	else if(op == "@"){
		QUARK_ASSERT(e.get_array_size() == 3);
		const auto variable_name = e.get_array_n(1).get_string();
		const auto expr_type = resolve_type123(e.get_array_n(2).get_string(), types);

//??? Bound to variable?
		return expression_t::make_resolve_variable(variable_name, expr_type);
	}
	else{
		QUARK_ASSERT(false);
	}
}

}

/*
	{
		"args": [],
		"locals": [],
		"members": [],
		"name": "main",
		"return_type": "$1001",
		"statements": [["return", ["k", 3, "$1001"]]],
		"type": "function",
		"types": {}
	}
*/
std::shared_ptr<const scope_def_t> conv_scope_def__no_expressions(const json_t& scope_def, const map<string, symbol_t>& types){
	const string type = scope_def.get_object_element("type").get_string();
	const string name = scope_def.get_object_element("name").get_string();
	const string return_type_id = scope_def.get_object_element("return_type").get_string();
	const auto args = scope_def.get_optional_object_element("args", json_t::make_array()).get_array();
	const auto local_variables = scope_def.get_optional_object_element("locals", json_t::make_array()).get_array();
	const auto members = scope_def.get_optional_object_element("members", json_t::make_array()).get_array();
	const auto statements = scope_def.get_object_element("statements").get_array();

	std::vector<member_t> args2 = conv_members(args, types);
	std::vector<member_t> local_variables2 = conv_members(local_variables, types);
	std::vector<member_t> members2 = conv_members(members, types);

	if(type == "function"){
		std::vector<std::shared_ptr<statement_t> > statements2;
		const auto function_type = scope_def.get_optional_object_element("function_type");
		if(function_type && function_type.get_string() == "def-constructor"){
			return scope_def_t::make_builtin_function_def(
				type_identifier_t::make(name),
				scope_def_t::efunc_variant::k_default_constructor,
				resolve_type123(return_type_id, types)
			);
		}
		else{
			return scope_def_t::make_function_def(
				type_identifier_t::make(name),
				args2,
				local_variables2,
				statements2,
				resolve_type123(return_type_id, types)
			);
		}
	}
	else if(type == "struct"){
		return scope_def_t::make_struct(type_identifier_t::make(name), members2);
	}
	else if(type == "global"){
		return scope_def_t::make_global_scope();
	}
	else{
		QUARK_ASSERT(false);
	}
	return {};
}

/*
	Input is a typedef
*/
typeid_t conv_type_def__no_expressions(const json_t& def, const map<string, symbol_t>& types){
	QUARK_ASSERT(def.check_invariant());

	const auto base_type = def.get_object_element("base_type");
	const auto scope_def = def.get_optional_object_element("scope_def");

	if(base_type == "null"){
		return typeid_t::make_null();
	}
	else if(base_type == "bool"){
		return typeid_t::make_bool();
	}
	else if(base_type == "int"){
		return typeid_t::make_int();
	}
	else if(base_type == "float"){
		return typeid_t::make_float();
	}
	else if(base_type == "string"){
		return typeid_t::make_string();
	}

	else if(base_type == "struct"){
		const auto r = conv_scope_def__no_expressions(scope_def, types);
		return typeid_t::make_struct("");
	}
	else if(base_type == "vector"){
		QUARK_ASSERT(false);
	}
	else if(base_type == "function"){
		const auto r = conv_scope_def__no_expressions(scope_def, types);
		vector<typeid_t> args;
		for(const auto e: r->_args){
			args.push_back(e._type);
		}
		return typeid_t::make_function(r->_return_type, args);
	}
	else if(base_type == "subscope"){
		QUARK_ASSERT(false);
	}
	else{
		QUARK_ASSERT(false);
	}
}

