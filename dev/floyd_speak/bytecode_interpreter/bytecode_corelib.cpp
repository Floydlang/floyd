//
//  host_functions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_corelib.h"

#include "json_support.h"
#include "ast_typeid_helpers.h"
#include "floyd_interpreter.h"

#include "text_parser.h"
#include "file_handling.h"
#include "floyd_runtime.h"
#include "floyd_corelib.h"
#include "ast_value.h"
#include "ast_json.h"


#include <algorithm>
#include <iostream>
#include <fstream>



namespace floyd {


//??? remove using
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::make_shared;





//######################################################################################################################
//	CORE LIBRARY
//######################################################################################################################








/////////////////////////////////////////		PURE -- SHA1



bc_value_t host__calc_string_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const auto& s = args[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto result = value_t::make_struct_value(
		make__sha1_t__type(),
		{
			value_t::make_string(ascii40)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}



bc_value_t host__calc_binary_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type == make__binary_t__type());

	const auto& sha1_struct = args[0].get_struct_value();
	QUARK_ASSERT(sha1_struct.size() == make__binary_t__type().get_struct()._members.size());
	QUARK_ASSERT(sha1_struct[0]._type.is_string());

	const auto& sha1_string = sha1_struct[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(sha1_string);

	const auto result = value_t::make_struct_value(
		make__sha1_t__type(),
		{
			value_t::make_string(ascii40)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}



bc_value_t host__get_time_of_day(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto result = corelib__get_time_of_day();
	const auto result2 = bc_value_t::make_int(result);
	return result2;
}




/////////////////////////////////////////		IMPURE -- FILE SYSTEM




bc_value_t host__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string source_path = args[0].get_string_value();
	std::string file_contents = corelib_read_text_file(source_path);
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}

bc_value_t host__write_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_string());

	const string path = args[0].get_string_value();
	const string file_contents = args[1].get_string_value();

	corelib_write_text_file(path, file_contents);

	return bc_value_t();
}






bc_value_t host__get_fsentries_shallow(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItems(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(vec2);

	return v;
}

bc_value_t host__get_fsentries_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_deep() illegal input path.");
	}

	const auto a = GetDirItemsDeep(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 0
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug._value));
#endif

	const auto v = value_to_bc(vec2);

	return v;
}


//??? implement
std::string posix_timespec__to__utc(const time_t& t){
	return std::to_string(t);
}


value_t impl__get_fsentry_info(const std::string& path){
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentry_info() illegal input path.");
	}

	TFileInfo info;
	bool ok = GetFileInfo(path, info);
	QUARK_ASSERT(ok);
	if(ok == false){
		quark::throw_exception();
	}

	const auto parts = SplitPath(path);
	const auto parent = UpDir2(path);

	const auto type_string = info.fDirFlag ? "dir" : "string";
	const auto name = info.fDirFlag ? parent.second : parts.fName;
	const auto parent_path = info.fDirFlag ? parent.first : parts.fPath;

	const auto creation_date = posix_timespec__to__utc(info.fCreationDate);
	const auto modification_date = posix_timespec__to__utc(info.fModificationDate);
	const auto file_size = info.fFileSize;

	const auto result = value_t::make_struct_value(
		make__fsentry_info_t__type(),
		{
			value_t::make_string(type_string),
			value_t::make_string(name),
			value_t::make_string(parent_path),

			value_t::make_string(creation_date),
			value_t::make_string(modification_date),

			value_t::make_int(file_size)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}


bc_value_t host__get_fsentry_info(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	const auto result = impl__get_fsentry_info(path);
	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__get_fs_environment(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto dirs = GetDirectories();

	const auto result = value_t::make_struct_value(
		make__fs_environment_t__type(),
		{
			value_t::make_string(dirs.home_dir),
			value_t::make_string(dirs.documents_dir),
			value_t::make_string(dirs.desktop_dir),

			value_t::make_string(dirs.application_support),
			value_t::make_string(dirs.preferences_dir),
			value_t::make_string(dirs.cache_dir),
			value_t::make_string(dirs.temp_dir),

			value_t::make_string(dirs.process_dir)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}

//??? refactor common code.

bc_value_t host__does_fsentry_exist(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(path);
	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = value_to_bc(result);
	return v;
}


bc_value_t host__create_directory_branch(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(path);
	return bc_value_t::make_void();
}

bc_value_t host__delete_fsentry_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	QUARK_ASSERT(args[0]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(path);

	return bc_value_t::make_void();
}


bc_value_t host__rename_fsentry(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	QUARK_ASSERT(args[0]._type.is_string());
	QUARK_ASSERT(args[1]._type.is_string());

	const string path = args[0].get_string_value();
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("rename_fsentry() illegal input path.");
	}
	const string n = args[1].get_string_value();
	if(n.empty()){
		quark::throw_runtime_error("rename_fsentry() illegal input name.");
	}

	RenameEntry(path, n);

	return bc_value_t::make_void();
}





/////////////////////////////////////////		REGISTRY



static std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_corelib_internal(){
	std::vector<std::pair<libfunc_signature_t, BC_HOST_FUNCTION_PTR>> log;

	log.push_back({ make_calc_string_sha1_signature(), host__calc_string_sha1 });
	log.push_back({ make_calc_binary_sha1_signature(), host__calc_binary_sha1 });

	log.push_back({ make_get_time_of_day_signature(), host__get_time_of_day });

	log.push_back({ make_read_text_file_signature(), host__read_text_file });
	log.push_back({ make_write_text_file_signature(), host__write_text_file });

	log.push_back({ make_get_fsentries_shallow_signature(), host__get_fsentries_shallow });
	log.push_back({ make_get_fsentries_deep_signature(), host__get_fsentries_deep });
	log.push_back({ make_get_fsentry_info_signature(), host__get_fsentry_info });
	log.push_back({ make_get_fs_environment_signature(), host__get_fs_environment });
	log.push_back({ make_does_fsentry_exist_signature(), host__does_fsentry_exist });
	log.push_back({ make_create_directory_branch_signature(), host__create_directory_branch });
	log.push_back({ make_delete_fsentry_deep_signature(), host__delete_fsentry_deep });
	log.push_back({ make_rename_fsentry_signature(), host__rename_fsentry });


	std::map<function_id_t, BC_HOST_FUNCTION_PTR> result;
	for(const auto& e: log){
		result.insert({ function_id_t { e.first.name }, e.second });
	}

	return result;
}

std::map<function_id_t, BC_HOST_FUNCTION_PTR> bc_get_corelib_calls(){
	static const auto f = bc_get_corelib_internal();
	return f;
}


}	// floyd
