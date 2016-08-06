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
	public: vm_t(const floyd_parser::ast_t& ast);
	public: bool check_invariant() const;

	public: std::shared_ptr<floyd_parser::function_def_t> resolve_function_type(const std::string& s) const;


	////////////////////////		STATE
	public: const floyd_parser::ast_t _ast;

	//	Last scope if the current one. First scope is the root.
	public: std::vector<std::shared_ptr<floyd_parser::scope_instance_t>> _scope_instances;
};




#endif /* floyd_vm_hpp */
