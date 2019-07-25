//
//  floyd_llvm_runtime.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_corelib.h"

#include "floyd_llvm_runtime.h"

#include "floyd_runtime.h"
#include "floyd_filelib.h"

#include "text_parser.h"
#include "file_handling.h"
#include "os_process.h"
#include "floyd_filelib.h"

#include <iostream>
#include <fstream>

namespace floyd {


static const bool k_trace_messaging = false;



llvm_execution_engine_t& get_floyd_runtime(floyd_runtime_t* frp);



struct native_binary_t {
	VEC_T* ascii40;
};

struct native_sha1_t {
	VEC_T* ascii40;
};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	FILELIB FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





static STRUCT_T* llvm_corelib__calc_string_sha1(floyd_runtime_t* frp, runtime_value_t s0){
	auto& r = get_floyd_runtime(frp);

	const auto& s = from_runtime_string(r, s0);
	const auto ascii40 = filelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		typeid_t::make_struct2({ member_t{ typeid_t::make_string(), "ascii40" } }),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}

static STRUCT_T* llvm_corelib__calc_binary_sha1(floyd_runtime_t* frp, STRUCT_T* binary_ptr){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(binary_ptr != nullptr);

	const auto& binary = *reinterpret_cast<const native_binary_t*>(binary_ptr->get_data_ptr());

	const auto& s = from_runtime_string(r, runtime_value_t { .vector_ptr = binary.ascii40 });
	const auto ascii40 = filelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		typeid_t::make_struct2({ member_t{ typeid_t::make_string(), "ascii40" } }),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}



static int64_t llvm_corelib__get_time_of_day(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	return filelib__get_time_of_day();
}



static runtime_value_t llvm_corelib__read_text_file(floyd_runtime_t* frp, runtime_value_t arg){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, arg);
	const auto file_contents = 	filelib_read_text_file(path);
	return to_runtime_string(r, file_contents);
}

static void llvm_corelib__write_text_file(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t data0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	const auto file_contents = from_runtime_string(r, data0);
	filelib_write_text_file(path, file_contents);
}





static void llvm_corelib__create_directory_branch(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(path);
}

static void llvm_corelib__delete_fsentry_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(path);
}

static uint8_t llvm_corelib__does_fsentry_exist(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(path);
	const auto result = value_t::make_bool(exists);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif
	return exists ? 0x01 : 0x00;
}

static runtime_value_t llvm_corelib__get_fs_environment(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

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

	const auto v = to_runtime_value(r, result);
	return v;
}

static VEC_T* llvm_corelib__get_fsentries_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("get_fsentries_deep() illegal input path.");
	}

	const auto a = GetDirItemsDeep(path);
	const auto elements = directory_entries_to_values(a);
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto vec2 = value_t::make_vector_value(k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_ptr;
}


static VEC_T* llvm_corelib__get_fsentries_shallow(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
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

	const auto v = to_runtime_value(r, vec2);
	return v.vector_ptr;
}

static STRUCT_T* llvm_corelib__get_fsentry_info(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto result = impl__get_fsentry_info(from_runtime_string(r, path0));
	const auto v = to_runtime_value(r, result);
	return v.struct_ptr;
}

static void llvm_corelib__rename_fsentry(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t name0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);

	if(is_valid_absolute_dir_path(path) == false){
		quark::throw_runtime_error("rename_fsentry() illegal input path.");
	}
	const auto n = from_runtime_string(r, name0);
	if(n.empty()){
		quark::throw_runtime_error("rename_fsentry() illegal input name.");
	}

	RenameEntry(path, n);
}




std::map<std::string, void*> get_corelib_c_function_ptrs(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {

		{ "floyd_funcdef__calc_string_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_string_sha1) },
		{ "floyd_funcdef__calc_binary_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_binary_sha1) },

		{ "floyd_funcdef__get_time_of_day", reinterpret_cast<void *>(&llvm_corelib__get_time_of_day) },

		{ "floyd_funcdef__read_text_file", reinterpret_cast<void *>(&llvm_corelib__read_text_file) },
		{ "floyd_funcdef__write_text_file", reinterpret_cast<void *>(&llvm_corelib__write_text_file) },

		{ "floyd_funcdef__rename_fsentry", reinterpret_cast<void *>(&llvm_corelib__rename_fsentry) },
		{ "floyd_funcdef__create_directory_branch", reinterpret_cast<void *>(&llvm_corelib__create_directory_branch) },
		{ "floyd_funcdef__delete_fsentry_deep", reinterpret_cast<void *>(&llvm_corelib__delete_fsentry_deep) },
		{ "floyd_funcdef__does_fsentry_exist", reinterpret_cast<void *>(&llvm_corelib__does_fsentry_exist) },

		{ "floyd_funcdef__get_fs_environment", reinterpret_cast<void *>(&llvm_corelib__get_fs_environment) },
		{ "floyd_funcdef__get_fsentries_deep", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_deep) },
		{ "floyd_funcdef__get_fsentries_shallow", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_shallow) },
		{ "floyd_funcdef__get_fsentry_info", reinterpret_cast<void *>(&llvm_corelib__get_fsentry_info) },
	};
	return host_functions_map;
}


}	//	namespace floyd



