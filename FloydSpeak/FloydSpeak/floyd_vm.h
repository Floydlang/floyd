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




	struct scope_instance_t {
		public: const floyd_parser::scope_def_t* _def = nullptr;

		//	### idea: Values are indexes same as scope_def_t::_runtime_value_spec.
		//	key string is name of variable.
		public: std::map<std::string, floyd_parser::value_t> _values;
	};


struct vm_t {
	public: vm_t(const floyd_parser::ast_t& ast);
	public: bool check_invariant() const;



	////////////////////////		STATE
	public: const floyd_parser::ast_t _ast;

	//	Last scope if the current one. First scope is the root.
	public: std::vector<std::shared_ptr<scope_instance_t>> _scope_instances;
};



	typedef std::pair<std::size_t, std::size_t> byte_range_t;

	/*
		s: all types must be fully defined, deeply, for this function to work.
		result, item[0] the memory range for the entire struct.
				item[1] the first member in the struct. Members may be mapped in any order in memory!
				item[2] the first member in the struct.
	*/
	std::vector<byte_range_t> calc_struct_default_memory_layout(const floyd_parser::types_collector_t& types, const floyd_parser::type_def_t& s);



#endif /* floyd_vm_hpp */
