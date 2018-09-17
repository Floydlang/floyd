//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statement.h"


namespace floyd {

bool body_t::check_invariant() const {
	for(const auto i: _statements){
		QUARK_ASSERT(i->check_invariant());
	};
	return true;
}


int add_constant_literal(symbol_table_t& symbols, const std::string& name, const floyd::value_t& value){
	const auto s = symbol_t::make_constant(value);
	symbols._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	return static_cast<int>(symbols._symbols.size() - 1);
}


int add_temp(symbol_table_t& symbols, const std::string& name, const floyd::typeid_t& value_type){
	const auto s = symbol_t::make_immutable_local(value_type);
	symbols._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	return static_cast<int>(symbols._symbols.size() - 1);
}


QUARK_UNIT_TEST("", "", "", ""){
	const auto a = statement_t::make__block_statement({});
	const auto b = statement_t::make__block_statement({});
	QUARK_UT_VERIFY(a == b);
}

}
