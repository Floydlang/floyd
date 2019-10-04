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





struct native_binary_t {
	VECTOR_CARRAY_T* ascii40;
};

struct native_sha1_t {
	VECTOR_CARRAY_T* ascii40;
};


//??? Should find the symbol for "benchmark_result2_t".
static runtime_value_t llvm_corelib__make_benchmark_report(floyd_runtime_t* frp, const runtime_value_t b){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto benchmark_result2_t__itype = make_benchmark_result2_t(interner);
	const auto b2 = from_runtime_value(r, b, make_vector(interner, benchmark_result2_t__itype));
	const auto test_results = unpack_vec_benchmark_result2_t(interner, b2);
	const auto report = make_benchmark_report(test_results);
	auto result = to_runtime_string(r, report);
	return result;
}




static DICT_CPPMAP_T* llvm_corelib__detect_hardware_caps(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	const auto& interner = r.backend.type_interner;
	const std::vector<std::pair<std::string, json_t>> caps = corelib_detect_hardware_caps();

	std::map<std::string, value_t> caps_map;
	for(const auto& e: caps){
  		caps_map.insert({ e.first, value_t::make_json(e.second) });
	}

	const auto a = value_t::make_dict_value(interner, type_t::make_json(), caps_map);
	auto result = to_runtime_value(r, a);
	return result.dict_cppmap_ptr;
}

runtime_value_t llvm_corelib__make_hardware_caps_report(floyd_runtime_t* frp, runtime_value_t caps0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto type = make_dict(interner, type_t::make_json());
	const auto b2 = from_runtime_value(r, caps0, type);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report(caps);
	return to_runtime_string(r, s);
}
runtime_value_t llvm_corelib__make_hardware_caps_report_brief(floyd_runtime_t* frp, runtime_value_t caps0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto b2 = from_runtime_value(r, caps0, make_dict(interner, type_t::make_json()));
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report_brief(caps);
	return to_runtime_string(r, s);
}
runtime_value_t llvm_corelib__get_current_date_and_time_string(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);

	const auto s = get_current_date_and_time_string();
	return to_runtime_string(r, s);
}








static STRUCT_T* llvm_corelib__calc_string_sha1(floyd_runtime_t* frp, runtime_value_t s0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto& s = from_runtime_string(r, s0);
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		interner,
		make_struct(interner, struct_type_desc_t({ member_t{ type_t::make_string(), "ascii40" } }) ),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}

static STRUCT_T* llvm_corelib__calc_binary_sha1(floyd_runtime_t* frp, STRUCT_T* binary_ptr){
	auto& r = get_floyd_runtime(frp);
	QUARK_ASSERT(binary_ptr != nullptr);
	auto& interner = r.backend.type_interner;

	const auto& binary = *reinterpret_cast<const native_binary_t*>(binary_ptr->get_data_ptr());

	const auto& s = from_runtime_string(r, make_runtime_vector_carray(binary.ascii40));
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto a = value_t::make_struct_value(
		interner,
		make_struct(interner, struct_type_desc_t({ member_t{ type_t::make_string(), "ascii40" } })),
		{ value_t::make_string(ascii40) }
	);

	auto result = to_runtime_value(r, a);
	return result.struct_ptr;
}



static int64_t llvm_corelib__get_time_of_day(floyd_runtime_t* frp){
	get_floyd_runtime(frp);

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




static VECTOR_CARRAY_T* llvm_corelib__get_fsentries_shallow(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_shallow(path);

	const auto elements = directory_entries_to_values(interner, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(interner);
	const auto vec2 = value_t::make_vector_value(interner, k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(interner, vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_carray_ptr;
}

static VECTOR_CARRAY_T* llvm_corelib__get_fsentries_deep(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto path = from_runtime_string(r, path0);

	const auto a = corelib_get_fsentries_deep(path);

	const auto elements = directory_entries_to_values(interner, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(interner);
	const auto vec2 = value_t::make_vector_value(interner, k_fsentry_t__type, elements);

#if 1
	const auto debug = value_and_type_to_ast_json(interner, vec2);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	const auto v = to_runtime_value(r, vec2);
	return v.vector_carray_ptr;
}

static STRUCT_T* llvm_corelib__get_fsentry_info(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;
	const auto path = from_runtime_string(r, path0);

	const auto info = corelib_get_fsentry_info(path);

	const auto info2 = pack_fsentry_info(interner, info);

	const auto v = to_runtime_value(r, info2);
	return v.struct_ptr;
}

static runtime_value_t llvm_corelib__get_fs_environment(floyd_runtime_t* frp){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;

	const auto env = corelib_get_fs_environment();

	const auto result = pack_fs_environment_t(interner, env);
	const auto v = to_runtime_value(r, result);
	return v;
}




static uint8_t llvm_corelib__does_fsentry_exist(floyd_runtime_t* frp, runtime_value_t path0){
	auto& r = get_floyd_runtime(frp);
	auto& interner = r.backend.type_interner;
	const auto path = from_runtime_string(r, path0);

	bool exists = corelib_does_fsentry_exist(path);

	const auto result = value_t::make_bool(exists);
#if 1
	const auto debug = value_and_type_to_ast_json(interner, result);
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




std::map<std::string, void*> get_corelib_binds(){

	////////////////////////////////		CORE FUNCTIONS AND HOST FUNCTIONS
	const std::map<std::string, void*> host_functions_map = {
		{ "make_benchmark_report", reinterpret_cast<void *>(&llvm_corelib__make_benchmark_report) },

		{ "detect_hardware_caps", reinterpret_cast<void *>(&llvm_corelib__detect_hardware_caps) },
		{ "make_hardware_caps_report", reinterpret_cast<void *>(&llvm_corelib__make_hardware_caps_report) },
		{ "make_hardware_caps_report_brief", reinterpret_cast<void *>(&llvm_corelib__make_hardware_caps_report_brief) },
		{ "get_current_date_and_time_string", reinterpret_cast<void *>(&llvm_corelib__get_current_date_and_time_string) },

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



