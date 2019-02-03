//
//  core_api.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef core_api_hpp
#define core_api_hpp

#include <string>
#include <vector>

/*
	This is the IMPLEMENTATION of the Floyd runtime's core API. The VM calls these functions.
*/

struct core_api_runtime_t {
	core_api_runtime_t(int64_t os_process_id) :
		os_process_id(os_process_id)
	{
	}

	int64_t os_process_id;
};

struct green_process_runtime_t {
	green_process_runtime_t() :
		random_seed(234782348723)
	{
	}


	//////////////////////		STATE
	int64_t random_seed;
};

struct runtime_t {
	std::vector<green_process_runtime_t> process_runtimes;
};





//////////////////////////		MORE COLLECTION OPERATIONS


#if 0

////////////////////////////			CORE



void on_assert_hook(runtime_i* runtime, source_code_location location, char expression[])


protocol trace_i {
	void trace_i__trace(char s[])
	void trace_i__open_scope(char s[])
	void trace_i__close_scope(char s[])
}


//	??? logging logs values, not strings. Strings are created on-demand.

//	??? Simple app that receives debug messages and prints them. Uses sockets.



#define QUARK_TRACE(s)
#define QUARK_TRACE_SS(s)
#define QUARK_SCOPED_TRACE(s)


struct unit_test_def {
	source_code_location loc

	string _class_under_test
	string _function_under_test
	string _scenario
	string _expected_result

	unit_test_function _test_f
	bool _vip
}

inline void run_tests(unit_test_registry registry, [string] source_file_order){



/////////////////////////////////////////			sha1




///////////////////////////////		String utils




std::vector<string> split_on_chars(seq_t s, string match_chars)
/*
	Concatunates vector of strings. Adds div between every pair of strings. There is never a div at end.
*/
string concat_strings_with_divider([string] v, string div)


float parse_float(string pos)
double parse_double(string pos)










#endif

#endif /* core_api_hpp */
