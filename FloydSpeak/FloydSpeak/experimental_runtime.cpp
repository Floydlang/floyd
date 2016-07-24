//
//  experimental_runtime.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 25/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "experimental_runtime.h"
#include "runtime_core.h"
#include "runtime_value.h"

#include "quark.h"
#include <vector>
#include <string>
#include <map>


using namespace runtime_core;
using namespace runtime_value;

/*
	Frontend = the perspective of the Floyd program and developer
	Backend = the actual storage of the data type etc. Can chose to organize things in a different way.
*/



namespace experimental_runtime {



	/*
	*/
	struct rt_stack_frame {
		rt_stack_frame* _prev;

		/*
			Fixed maximum size of local variables. ### Make this dynamic to unlimited stack?
		*/
		rt_cache_line_t* _local_variables[16384 / sizeof(rt_cache_line_t)];
	};








////////////////////////////////		STATE




/*
	Stores its data in rt_cache_line_t.
	Leaf node directly stores values if at least 8 values fit in one leaf (requires 4 rt_cache_line_t to hold leaf data).Else the leaf keeps references to the values.
*/
template<typename T> struct rt_pvec {
};
	


struct experimental_floydrt__state {
//	interned_strings* _strings;

	rt_stack_frame _stack_frame;

	uint32_t _thread_id;
	rt_memory_manager _memory_manager;
};


}	//	experimental_runtime







//////////		JSON






/*
	### Define format for source-as-JSON roundtrip. Normalized source code format. Goal for JSON is easy machine transformation.

	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"




	string hello(int x, int y, string z){
		return "test abc";
	}

	int main(string args){
		return 3;
	}


{
	"program_as_json" : [
		{
			"_func" : {
				"_prototype" : "string hello(int x, int y, string z)",
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}

{
	"program_as_json" : [
		{
			"_func" : {
				"return": "string",
				"name": "hello",
				"args": [
					{ "type": "int", "name": "x" },
					{ "type": "int", "name": "y" },
					{ "type": "string", "name": "z" },
				],
				"body": [
					[ "assign", "a",
						[
							[ "return",
								[ "*", "a", "y" ]
							]
						],
					[ "return", "test abc" ]
				]
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}

!!! JSON does not support multi-line strings.
*/



