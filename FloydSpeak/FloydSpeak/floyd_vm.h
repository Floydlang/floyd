//
//  floyd_vm.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_vm_hpp
#define floyd_vm_hpp

#include "floyd_parser.h"


struct vm_t {
	vm_t(const floyd_parser::ast_t& ast);
	bool check_invariant() const;


	////////////////////////		STATE
	floyd_parser::ast_t _ast;
};




#endif /* floyd_vm_hpp */
