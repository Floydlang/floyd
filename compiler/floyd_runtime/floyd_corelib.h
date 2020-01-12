//
//  floyd_corelib.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-06-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_corelib_hpp
#define floyd_corelib_hpp


#include <string>
#include <map>
#include <vector>

#include "file_handling.h"
#include "value_backend.h"

namespace floyd {

struct value_t;


//////////////////////////////////////////		FUNCTIONALITY


bool is_valid_absolute_dir_path(const std::string& s);
std::vector<value_t> directory_entries_to_values(types_t& types, const std::vector<TDirEntry>& v);


//////////////////////////////////////////		CORELIB GLUE


type_t make__ip_address_t__type(types_t& types);
type_t make__ip_address_t__type(const types_t& types);

type_t make__fsentry_t__type(types_t& types);
type_t make__fsentry_info_t__type(types_t& types);
type_t make__fs_environment_t__type(types_t& types);

/*
	struct sha1_t {
		string ascii40
	}
*/
type_t make__sha1_t__type(types_t& types);

//	Part of corelib / standard lib
struct native_sha1_t {
	rt_pod_t ascii40_string;
};


//	Part of language
struct native_binary_t {
	rt_pod_t bytes_string;
};

/*
	struct binary_t {
		string bytes
	}
*/
type_t make__binary_t__type(types_t& types);


std::string make_benchmark_report(const std::vector<benchmark_result2_t>& test_results);

std::vector<std::pair<std::string, json_t>> corelib_detect_hardware_caps();
std::string corelib_make_hardware_caps_report(const std::vector<std::pair<std::string, json_t>>& caps);
std::string corelib_make_hardware_caps_report_brief(const std::vector<std::pair<std::string, json_t>>& caps);


std::string get_current_date_and_time_string();

std::string corelib_calc_string_sha1(const std::string& s);

std::string corelib_read_text_file(const std::string& abs_path);
void corelib_write_text_file(const std::string& abs_path, const std::string& file_contents);

std::string corelib_read_line_stdin();

int64_t corelib__get_time_ns();

std::vector<TDirEntry> corelib_get_fsentries_shallow(const std::string& abs_path);
std::vector<TDirEntry> corelib_get_fsentries_deep(const std::string& abs_path);


struct fsentry_info_t {
	std::string type;
	std::string name;
	std::string abs_parent_path;

	std::string creation_date;
	std::string modification_date;
	int64_t file_size;
};

fsentry_info_t corelib_get_fsentry_info(const std::string& abs_path);

value_t pack_fsentry_info(types_t& types, const fsentry_info_t& info);


struct fs_environment_t {
	std::string home_dir;
	std::string documents_dir;
	std::string desktop_dir;

	std::string hidden_persistence_dir;
	std::string preferences_dir;
	std::string cache_dir;
	std::string temp_dir;

	std::string executable_dir;
};

fs_environment_t corelib_get_fs_environment();
value_t pack_fs_environment_t(types_t& types, const fs_environment_t& env);

bool corelib_does_fsentry_exist(const std::string& abs_path);
void corelib_create_directory_branch(const std::string& abs_path);
void corelib_delete_fsentry_deep(const std::string& abs_path);
void corelib_rename_fsentry(const std::string& abs_path, const std::string& n);


std::string get_corelib_header();
std::map<std::string, void*> get_unified_corelib_binds();

}	//	floyd

#endif /* floyd_corelib_hpp */
