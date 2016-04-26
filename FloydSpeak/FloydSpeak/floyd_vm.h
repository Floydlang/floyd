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
	vm_t(const ast_t& ast) :
		_ast(ast)
	{
	}

	ast_t _ast;
};




#endif /* floyd_vm_hpp */
