//
//  expressions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "expressions.h"

#include "text_parser.h"
#include "parser_statement.h"

#include <cmath>


namespace floyd_parser {


using std::pair;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;
















//////////////////////////////////////////////////		expression_t


bool expression_t::operator==(const expression_t& other) const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	if(_constant){
		return other._constant && *_constant == *other._constant;
	}
	else if(_math_operation1_expr){
		return other._math_operation1_expr && *_math_operation1_expr == *other._math_operation1_expr;
	}
	else if(_math_operation2_expr){
		return other._math_operation2_expr && *_math_operation2_expr == *other._math_operation2_expr;
	}
	else if(_call_function_expr){
		return other._call_function_expr && *_call_function_expr == *other._call_function_expr;
	}
	else if(_variable_read_expr){
		return other._variable_read_expr && *_variable_read_expr == *other._variable_read_expr;
	}
	else if(_resolve_member_expr){
		return other._resolve_member_expr && *_resolve_member_expr == *other._resolve_member_expr;
	}
	else if(_lookup_element_expr){
		return other._lookup_element_expr && *_lookup_element_expr == *other._lookup_element_expr;
	}
	else{
		QUARK_ASSERT(false);
		return false;
	}
}




expression_t make_constant(const value_t& value){
	expression_t result;
	result._constant = std::make_shared<value_t>(value);
	return result;
}

expression_t make_constant(const std::string& s){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(s));
	return result;
}

expression_t make_constant(const int i){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(i));
	return result;
}

expression_t make_constant(const float f){
	expression_t result;
	result._constant = std::make_shared<value_t>(value_t(f));
	return result;
}



expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input){
	expression_t result;
	auto input2 = make_shared<expression_t>(input);

	math_operation1_expr_t r = math_operation1_expr_t{ op, input2 };
	result._math_operation1_expr = std::make_shared<math_operation1_expr_t>(r);
	return result;
}

expression_t make_math_operation2_expr(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right){
	expression_t result;
	auto left2 = make_shared<expression_t>(left);
	auto right2 = make_shared<expression_t>(right);

	math_operation2_expr_t r = math_operation2_expr_t{ op, left2, right2 };
	result._math_operation2_expr = std::make_shared<math_operation2_expr_t>(r);
	return result;
}



expression_t make_function_call(){
	expression_t result;
/*
	auto input2 = make_shared<expression_t>(input);



		const std::string _function_name;
		const std::vector<std::shared_ptr<expression_t>> _inputs;



	math_operation1_expr_t r = math_operation1_expr_t{ op, input2 };
	result._math_operation1_expr = std::make_shared<math_operation1_expr_t>(r);
*/
	return result;
}


expression_t make_variable_read(const expression_t& address_expression){
	expression_t result;
	auto address = make_shared<expression_t>(address_expression);
	variable_read_expr_t r = variable_read_expr_t{address};
	result._variable_read_expr = std::make_shared<variable_read_expr_t>(r);
	return result;
}

expression_t make_variable_read_variable(const std::string& name){
	const expression_t resolve(resolve_member_expr_t { name });
	return make_variable_read(resolve);
}






string operation_to_string(const math_operation2_expr_t::operation& op){
	if(op == math_operation2_expr_t::add){
		return "add";
	}
	else if(op == math_operation2_expr_t::subtract){
		return "subtract";
	}
	else if(op == math_operation2_expr_t::multiply){
		return "multiply";
	}
	else if(op == math_operation2_expr_t::divide){
		return "divide";
	}
	else{
		QUARK_ASSERT(false);
	}
}

string operation_to_string(const math_operation1_expr_t::operation& op){
	if(op == math_operation1_expr_t::negate){
		return "negate";
	}
	else{
		QUARK_ASSERT(false);
	}
}





void trace(const math_operation2_expr_t& e){
	string s = "math_operation2_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._left);
	trace(*e._right);
}

void trace(const math_operation1_expr_t& e){
	string s = "math_operation1_expr_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._input);
}

void trace(const function_call_expr_t& e){
	string s = "function_call_expr_t: " + e._function_name;
	QUARK_SCOPED_TRACE(s);
	for(const auto i: e._inputs){
		trace(*i);
	}
}

void trace(const variable_read_expr_t& e){
	string s = "variable_read_expr_t: address:";
	QUARK_SCOPED_TRACE(s);
	trace(*e._address);
}

void trace(const resolve_member_expr_t& e){
	QUARK_TRACE_SS("resolve_member_expr_t: " << e._member_name);
}

void trace(const lookup_element_expr_t& e){
	string s = "lookup_element_expr_t: ";
	QUARK_SCOPED_TRACE(s);
	trace(*e._lookup_key);
}



void trace(const expression_t& e){
	if(e._constant){
		trace(*e._constant);
	}
	else if(e._math_operation2_expr){
		trace(*e._math_operation2_expr);
	}
	else if(e._math_operation1_expr){
		trace(*e._math_operation1_expr);
	}
	else if(e._call_function_expr){
		trace(*e._call_function_expr);
	}
	else if(e._variable_read_expr){
		trace(*e._variable_read_expr);
	}
	else if(e._resolve_member_expr){
		trace(*e._resolve_member_expr);
	}
	else if(e._lookup_element_expr){
		trace(*e._lookup_element_expr);
	}
	else{
		QUARK_ASSERT(false);
	}
}



}
