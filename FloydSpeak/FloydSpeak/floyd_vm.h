//
//  floyd_vm.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_vm_hpp
#define floyd_vm_hpp

#include "floyd_parser.h"


struct vm_t {
	vm_t(const floyd_parser::ast_t& ast) :
		_ast(ast)
	{
		QUARK_ASSERT(ast.check_invariant());

		QUARK_ASSERT(check_invariant());
	}

	bool check_invariant() const {
		QUARK_ASSERT(_ast.check_invariant());
		return true;
	}


	////////////////////////		STATE
	floyd_parser::ast_t _ast;
};




#endif /* floyd_vm_hpp */
