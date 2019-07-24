//
//  floyd_filelib.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-06-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_filelib.h"


#include "ast_typeid.h"
#include "floyd_runtime.h"

namespace floyd {





/*
std::string GetTemporaryFile(const std::string& name){
	//	char *tmpnam(char *str)
	//	tempnam()

	char* temp_buffer = std::tempnam(const char *dir, const char *pfx);
	std::free(temp_buffer);
	temp_buffer = nullptr;


}


## make\_temporary\_path()

Returns a path where you can write a file or a directory and it goes into the file system. The name will be randomized. The operating system will delete this data at some point.

	string make_temporary_path() impure

Example:

	"/var/folders/kw/7178zwsx7p3_g10y7rp2mt5w0000gn/T/g10y/7178zwsxp3_g10y7rp2mt

There is never a file extension. You could add one if you want too.
*/






extern const std::string k_filelib_builtin_types_and_constants = R"(
	let double cmath_pi = 3.14159265358979323846

	struct cpu_address_t {
		int address
	}

	struct size_t {
		int address
	}

	struct file_pos_t {
		int pos
	}

	struct time_ms_t {
		int pos
	}

	struct uuid_t {
		int high64
		int low64
	}


	struct ip_address_t {
		int high64
		int low_64_bits
	}


	struct url_t {
		string absolute_url
	}

	struct url_parts_t {
		string protocol
		string domain
		string path
		[string:string] query_strings
		int port
	}

	struct quick_hash_t {
		int hash
	}

	struct key_t {
		quick_hash_t hash
	}

	struct date_t {
		string utc_date
	}

	struct sha1_t {
		string ascii40
	}


	struct relative_path_t {
		string relative_path
	}

	struct absolute_path_t {
		string absolute_path
	}

	struct binary_t {
		string bytes
	}

	struct text_location_t {
		absolute_path_t source_file
		int line_number
		int pos_in_line
	}

	struct seq_t {
		string str
		size_t pos
	}

	struct text_t {
		binary_t data
	}

	struct text_resource_id {
		quick_hash_t id
	}

	struct image_id_t {
		int id
	}

	struct color_t {
		double red
		double green
		double blue
		double alpha
	}

	struct vector2_t {
		double x
		double y
	}



	let color__black = color_t(0.0, 0.0, 0.0, 1.0)
	let color__white = color_t(1.0, 1.0, 1.0, 1.0)


	func color_t add_colors(color_t a, color_t b){
		return color_t(
			a.red + b.red,
			a.green + b.green,
			a.blue + b.blue,
			a.alpha + b.alpha
		)
	}


	////////////////////////////		FILE SYSTEM TYPES


	struct fsentry_t {
		string type	//	"dir" or "file"
		string abs_parent_path
		string name
	}

	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		string abs_parent_path

		string creation_date
		string modification_date
		int file_size
	}

	struct fs_environment_t {
		string home_dir
		string documents_dir
		string desktop_dir

		string hidden_persistence_dir
		string preferences_dir
		string cache_dir
		string temp_dir

		string executable_dir
	}
)";





	//??? check path is valid dir
bool is_valid_absolute_dir_path(const std::string& s){
	if(s.empty()){
		return false;
	}
	else{
/*
		if(s.back() != '/'){
			return false;
		}
*/

	}
	return true;
}



typeid_t make__fsentry_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "name" },
		{ typeid_t::make_string(), "parent_path" }
	});
	return temp;
}

/*
	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		absolute_path_t parent_path

		date_t creation_date
		date_t modification_date
		file_pos_t file_size
	}
*/
typeid_t make__fsentry_info_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "name" },
		{ typeid_t::make_string(), "parent_path" },

		{ typeid_t::make_string(), "creation_date" },
		{ typeid_t::make_string(), "modification_date" },
		{ typeid_t::make_int(), "file_size" }
	});
	return temp;
}




typeid_t make__fs_environment_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "home_dir" },
		{ typeid_t::make_string(), "documents_dir" },
		{ typeid_t::make_string(), "desktop_dir" },

		{ typeid_t::make_string(), "hidden_persistence_dir" },
		{ typeid_t::make_string(), "preferences_dir" },
		{ typeid_t::make_string(), "cache_dir" },
		{ typeid_t::make_string(), "temp_dir" },

		{ typeid_t::make_string(), "executable_dir" }
	});
	return temp;
}






/*
	struct date_t {
		string utd_date
	}
*/
typeid_t make__date_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "utd_date" }
	});
	return temp;
}

typeid_t make__sha1_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "ascii40" }
	});
	return temp;
}

typeid_t make__binary_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "bytes" }
	});
	return temp;
}

/*
	struct absolute_path_t {
		string absolute_path
	}
*/
typeid_t make__absolute_path_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "absolute_path" }
	});
	return temp;
}

/*
	struct file_pos_t {
		int pos
	}
*/
typeid_t make__file_pos_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_int(), "pos" }
	});
	return temp;
}






//////////////////////////////////////		FILELIB





libfunc_signature_t make_get_time_of_day_signature(){
	return { "get_time_of_day", "1005", typeid_t::make_function(typeid_t::make_int(), {}, epure::impure) };
}


libfunc_signature_t make_read_text_file_signature(){
	return { "read_text_file", "1015", typeid_t::make_function(typeid_t::make_string(), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_write_text_file_signature(){
	return { "write_text_file", "1016", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string(), typeid_t::make_string() }, epure::impure) };
}


libfunc_signature_t make_get_fsentries_shallow_signature(){
	const auto k_fsentry_t__type = make__fsentry_t__type();
	return { "get_fsentries_shallow", "1023", typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_get_fsentries_deep_signature(){
	const auto k_fsentry_t__type = make__fsentry_t__type();
	return { "get_fsentries_deep", "1024", typeid_t::make_function(typeid_t::make_vector(k_fsentry_t__type), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_get_fsentry_info_signature(){
	return { "get_fsentry_info", "1025", typeid_t::make_function(make__fsentry_info_t__type(), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_get_fs_environment_signature(){
	return { "get_fs_environment", "1026", typeid_t::make_function(make__fs_environment_t__type(), {}, epure::impure) };
}
libfunc_signature_t make_does_fsentry_exist_signature(){
	return { "does_fsentry_exist", "1027", typeid_t::make_function(typeid_t::make_bool(), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_create_directory_branch_signature(){
	return { "create_directory_branch", "1028", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string() }, epure::impure) };
}
libfunc_signature_t make_delete_fsentry_deep_signature(){
	return { "delete_fsentry_deep", "1029", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string() }, epure::impure)};
}
libfunc_signature_t make_rename_fsentry_signature(){
	return { "rename_fsentry", "1030", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string(), typeid_t::make_string() }, epure::impure)};
}


libfunc_signature_t make_calc_string_sha1_signature(){
	return { "calc_string_sha1", "1031", typeid_t::make_function(make__sha1_t__type(), { typeid_t::make_string() }, epure::pure) };
}


libfunc_signature_t make_calc_binary_sha1_signature(){
	return { "calc_binary_sha1", "1032", typeid_t::make_function(make__sha1_t__type(), { make__binary_t__type() }, epure::pure) };
}





static std::vector<libfunc_signature_t> get_host_function_records(){
	const std::vector<libfunc_signature_t> result = {
		make_get_time_of_day_signature(),

		make_read_text_file_signature(),
		make_write_text_file_signature(),

		make_get_fsentries_shallow_signature(),
		make_get_fsentries_deep_signature(),
		make_get_fsentry_info_signature(),
		make_get_fs_environment_signature(),
		make_does_fsentry_exist_signature(),
		make_create_directory_branch_signature(),
		make_delete_fsentry_deep_signature(),
		make_rename_fsentry_signature(),

		make_calc_string_sha1_signature(),
		make_calc_binary_sha1_signature()
	};
	return result;
}


std::vector<libfunc_signature_t> get_filelib_signatures(){
	static const auto a = get_host_function_records();

	return a;
}





}	// floyd

