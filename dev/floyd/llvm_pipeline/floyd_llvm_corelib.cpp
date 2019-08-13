//
//  floyd_llvm_runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-04-24.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_corelib.h"

#include "floyd_llvm_runtime.h"
#include "floyd_runtime.h"
#include "floyd_corelib.h"
#include "text_parser.h"
#include "file_handling.h"
#include "os_process.h"

#include <iostream>
#include <fstream>

namespace floyd {




llvm_execution_engine_t& get_floyd_runtime(floyd_runtime_t* frp);



struct native_binary_t {
	VEC_T* ascii40;
};

struct native_sha1_t {
	VEC_T* ascii40;
};



static DICT_T* llvm_corelib__detect_hardware_caps(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	const std::vector<std::pair<std::string, json_t>> caps = corelib_detect_hardware_caps();

	std::map<std::string, value_t> caps_map;
	for(const auto& e: caps){
  		caps_map.insert({ e.first, value_t::make_json(e.second) });
	}

	const auto a = value_t::make_dict_value(typeid_t::make_json(), caps_map);
	auto result = to_runtime_value(r, a);
	return result.dict_ptr;
}




static STRUCT_T* llvm_corelib__calc_string_sha1(floyd_runtime_t* frp, runtime_value_t s0){
	auto& r = get_floyd_runtime(frp);

	const auto& s = from_runtime_string(r, s0);
	const auto ascii40 = corelib_calc_string_sha1(s);

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
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		typeid_t::make_struct2({ member_t{ typeid_t::make_string(), "ascii40" } }),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}



static int64_t llvm_corelib__get_time_of_day(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	return corelib__get_time_of_day();
}



static runtime_value_t llvm_corelib__read_text_file(floyd_runtime_t* frp, runtime_value_t arg){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, arg);
	const auto file_contents = 	corelib_read_text_file(path);
	return to_runtime_string(r, file_contents);
}

static void llvm_corelib__write_text_file(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t data0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);
	const auto file_contents = from_runtime_string(r, data0);
	corelib_write_text_file(path, file_contents);
}




static VEC_T* llvm_corelib__get_fsentries_shallow(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_shallow(path);

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

static VEC_T* llvm_corelib__get_fsentries_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_deep(path);

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
	const auto path = from_runtime_string(r, path0);

	const auto info = corelib_get_fsentry_info(path);

	const auto info2 = pack_fsentry_info(info);

	const auto v = to_runtime_value(r, info2);
	return v.struct_ptr;
}

static runtime_value_t llvm_corelib__get_fs_environment(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	const auto env = corelib_get_fs_environment();

	const auto result = pack_fs_environment_t(env);
	const auto v = to_runtime_value(r, result);
	return v;
}




static uint8_t llvm_corelib__does_fsentry_exist(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	const auto path = from_runtime_string(r, path0);

	bool exists = corelib_does_fsentry_exist(path);

	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif
	return exists ? 0x01 : 0x00;
}

static void llvm_corelib__create_directory_branch(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	const auto path = from_runtime_string(r, path0);

	corelib_create_directory_branch(path);
}

static void llvm_corelib__delete_fsentry_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);

	const auto path = from_runtime_string(r, path0);

	corelib_delete_fsentry_deep(path);
}

static void llvm_corelib__rename_fsentry(floyd_runtime_t* frp, runtime_value_t path0, runtime_value_t name0){
	auto& r = get_floyd_runtime(frp);
	const auto path = from_runtime_string(r, path0);
	const auto n = from_runtime_string(r, name0);

	corelib_rename_fsentry(path, n);
}




std::map<std::string, void*> get_corelib_c_function_ptrs(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {

		{ "detect_hardware_caps", reinterpret_cast<void *>(&llvm_corelib__detect_hardware_caps) },

		{ "calc_string_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_string_sha1) },
		{ "calc_binary_sha1", reinterpret_cast<void *>(&llvm_corelib__calc_binary_sha1) },

		{ "get_time_of_day", reinterpret_cast<void *>(&llvm_corelib__get_time_of_day) },

		{ "read_text_file", reinterpret_cast<void *>(&llvm_corelib__read_text_file) },
		{ "write_text_file", reinterpret_cast<void *>(&llvm_corelib__write_text_file) },

		{ "get_fsentries_shallow", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_shallow) },
		{ "get_fsentries_deep", reinterpret_cast<void *>(&llvm_corelib__get_fsentries_deep) },
		{ "get_fsentry_info", reinterpret_cast<void *>(&llvm_corelib__get_fsentry_info) },
		{ "get_fs_environment", reinterpret_cast<void *>(&llvm_corelib__get_fs_environment) },

		{ "does_fsentry_exist", reinterpret_cast<void *>(&llvm_corelib__does_fsentry_exist) },
		{ "create_directory_branch", reinterpret_cast<void *>(&llvm_corelib__create_directory_branch) },
		{ "delete_fsentry_deep", reinterpret_cast<void *>(&llvm_corelib__delete_fsentry_deep) },
		{ "rename_fsentry", reinterpret_cast<void *>(&llvm_corelib__rename_fsentry) }
	};
	return host_functions_map;
}


}	//	namespace floyd



