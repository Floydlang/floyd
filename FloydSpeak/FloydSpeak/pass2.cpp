//
//  pass2.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "pass2.h"



using floyd_parser::scope_def_t;
using floyd_parser::base_type;

/*
		public: base_type _base_type;
		public: std::shared_ptr<struct_def_t> _struct_def;
		public: std::shared_ptr<vector_def_t> _vector_def;
		public: std::shared_ptr<function_def_t> _function_def;
*/
/*
		public: std::weak_ptr<scope_def_t> _parent_scope;
		public: executable_t _executable;
		public: types_collector_t _types_collector;
*/


/*

??? Use struct_def or abstraction for *all* data, even globals, function arguments etc. Unifies all lookup.

Remove all symbols, convert struct members to vectors and use index.
Convert all
*/

bool are_symbols_resolvable2(const floyd_parser::scope_ref_t& struct_def){
	return true;
}

bool are_symbols_resolvable(const floyd_parser::scope_ref_t& scope){
	for(const auto t: scope->_types_collector._type_definitions){
		const auto type_def = t.second;

		if(type_def->get_type() == base_type::k_struct){
			are_symbols_resolvable2(type_def->get_struct_def());
		}
		else if(type_def->get_type() == base_type::k_vector){
//			type_def->_struct_def
		}
		else if(type_def->get_type() == base_type::k_function){
			are_symbols_resolvable2(type_def->get_function_def());
		}
	}
	return true;
}

floyd_parser::ast_t run_pass2(const floyd_parser::ast_t& ast1){
	auto ast2 = ast1;
	are_symbols_resolvable(ast2._global_scope);
	return ast2;
}


floyd_parser::ast_t run_pass3(const floyd_parser::ast_t& ast1){
	auto ast2 = ast1;
	return ast2;
}
