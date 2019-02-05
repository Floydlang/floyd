//
//  ast_basics.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-12.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_basics_hpp
#define ast_basics_hpp


#include <vector>


namespace floyd {



	//////////////////////////////////////		variable_address_t


	struct variable_address_t {
		public: variable_address_t() :
			_parent_steps(-1),
			_index(-1)
		{
		}

		public: bool is_empty() const {
			return _parent_steps == -1 && _index == -1;
		}
		public: bool check_invariant() const {
			return true;
		}
		public: static variable_address_t make_variable_address(int parent_steps, int index){
			return variable_address_t(parent_steps, index);
		}
		private: variable_address_t(int parent_steps, int index) :
			_parent_steps(parent_steps),
			_index(index)
		{
		}

		/*
			0: current stack frame
			1: previous stack frame
			2: previous-previous stack frame
			-1: global stack frame
		*/
		public: int _parent_steps;
		public: int _index;
	};

	inline bool operator==(const variable_address_t& lhs, const variable_address_t& rhs){
		return lhs._parent_steps == rhs._parent_steps && lhs._index == rhs._index;
	}


}

#endif /* ast_basics_hpp */
