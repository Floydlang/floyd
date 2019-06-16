//
//  floyd_filelib.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-06-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_filelib_hpp
#define floyd_filelib_hpp

#include <string>
#include <vector>

class TDirEntry;



namespace floyd {

struct value_t;
struct typeid_t;


extern const std::string k_filelib_builtin_types_and_constants;


typeid_t make__fsentry_t__type();
typeid_t make__fsentry_info_t__type();
typeid_t make__fs_environment_t__type();

bool is_valid_absolute_dir_path(const std::string& s);
std::vector<value_t> directory_entries_to_values(const std::vector<TDirEntry>& v);

value_t impl__get_fsentry_info(const std::string& path);




/*
	struct sha1_t {
		string ascii40
	}
*/
typeid_t make__sha1_t__type();

/*
	struct binary_t {
		string bytes
	}
*/
typeid_t make__binary_t__type();


}	//	floyd



#endif /* floyd_filelib_hpp */
