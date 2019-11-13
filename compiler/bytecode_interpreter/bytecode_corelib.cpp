//
//  bytecode_corelib.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_corelib.h"

#include "json_support.h"
#include "ast_value.h"

#include "floyd_runtime.h"
#include "floyd_corelib.h"
#include "bytecode_helpers.h"


namespace floyd {


static const bool k_trace = false;


bc_value_t bc_corelib__make_benchmark_report(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;

	const auto& symbols = vm._imm->_program._globals._symbols;
	const auto it = std::find_if(symbols.begin(), symbols.end(), [&](const auto& s){ return s.first == "benchmark_result2_t"; } );
	QUARK_ASSERT(it != symbols.end());

	const auto benchmark_result2_vec_type = make_vector(temp_types, it->second._value_type);

//	QUARK_ASSERT(args[0]._type == benchmark_result2_t__type);

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto test_results = unpack_vec_benchmark_result2_t(temp_types, b2);
	const auto report = make_benchmark_report(test_results);
	return value_to_bc(temp_types, value_t::make_string(report));
}




bc_value_t bc_corelib__detect_hardware_caps(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto& types = vm._imm->_program._types;
	const std::vector<std::pair<std::string, json_t>> caps = corelib_detect_hardware_caps();

	std::map<std::string, value_t> caps_map;
	for(const auto& e: caps){
  		caps_map.insert({ e.first, value_t::make_json(e.second) });
	}

	const auto a = value_t::make_dict_value(types, type_t::make_json(), caps_map);
	return value_to_bc(types, a);
}



bc_value_t bc_corelib__make_hardware_caps_report(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;
	QUARK_ASSERT(args[0]._type == make_dict(temp_types, type_t::make_json()));

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report(caps);
	return value_to_bc(temp_types, value_t::make_string(s));
}
bc_value_t bc_corelib__make_hardware_caps_report_brief(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	auto temp_types = vm._imm->_program._types;
	QUARK_ASSERT(args[0]._type == make_dict(temp_types, type_t::make_json()));

	const auto b2 = bc_to_value(temp_types, args[0]);
	const auto m = b2.get_dict_value();
	std::vector<std::pair<std::string, json_t>> caps;
	for(const auto& e: m){
		caps.push_back({ e.first, e.second.get_json() });
	}
	const auto s = corelib_make_hardware_caps_report_brief(caps);
	return value_to_bc(temp_types, value_t::make_string(s));
}
bc_value_t bc_corelib__get_current_date_and_time_string(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	auto temp_types = vm._imm->_program._types;
	const auto s = get_current_date_and_time_string();
	return value_to_bc(temp_types, value_t::make_string(s));
}







/////////////////////////////////////////		PURE -- SHA1



bc_value_t bc_corelib__calc_string_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = types;

	const auto& s = args[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(s);

	const auto result = value_t::make_struct_value(
		temp_types,
		make__sha1_t__type(temp_types),
		{
			value_t::make_string(ascii40)
		}
	);

	if(k_trace && false){
		const auto debug = value_and_type_to_ast_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}



bc_value_t bc_corelib__calc_binary_sha1(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
//	QUARK_ASSERT(args[0]._type == make__binary_t__type());

	const auto& types = vm._imm->_program._types;

	auto temp_types = types;
	const auto& sha1_struct = args[0].get_struct_value();
	QUARK_ASSERT(sha1_struct.size() == peek2(temp_types, make__binary_t__type(temp_types)).get_struct(temp_types)._members.size());
	QUARK_ASSERT(peek2(types, sha1_struct[0]._type).is_string());

	const auto& sha1_string = sha1_struct[0].get_string_value();
	const auto ascii40 = corelib_calc_string_sha1(sha1_string);

	const auto result = value_t::make_struct_value(
		temp_types,
		make__sha1_t__type(temp_types),
		{
			value_t::make_string(ascii40)
		}
	);

	if(k_trace && false){
		const auto debug = value_and_type_to_ast_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}



bc_value_t bc_corelib__get_time_of_day(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	const auto result = corelib__get_time_of_day();
	const auto result2 = bc_value_t::make_int(result);
	return result2;
}




/////////////////////////////////////////		IMPURE -- FILE SYSTEM




bc_value_t bc_corelib__read_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);

	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string source_path = args[0].get_string_value();
	std::string file_contents = corelib_read_text_file(source_path);
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}

bc_value_t bc_corelib__write_text_file(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const std::string path = args[0].get_string_value();
	const std::string file_contents = args[1].get_string_value();

	corelib_write_text_file(path, file_contents);

	return bc_value_t();
}

bc_value_t bc_corelib__read_line_stdin(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	std::string file_contents = corelib_read_line_stdin();
	const auto v = bc_value_t::make_string(file_contents);
	return v;
}




bc_value_t bc_corelib__get_fsentries_shallow(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto a = corelib_get_fsentries_shallow(path);

	const auto elements = directory_entries_to_values(temp_types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(temp_types);
	const auto vec2 = value_t::make_vector_value(temp_types, k_fsentry_t__type, elements);

	if(k_trace && false){
		const auto debug = value_and_type_to_ast_json(temp_types, vec2);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, vec2);

	return v;
}

bc_value_t bc_corelib__get_fsentries_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto a = corelib_get_fsentries_deep(path);

	const auto elements = directory_entries_to_values(temp_types, a);
	const auto k_fsentry_t__type = make__fsentry_t__type(temp_types);
	const auto vec2 = value_t::make_vector_value(temp_types, k_fsentry_t__type, elements);

	if(k_trace && false){
		const auto debug = value_and_type_to_ast_json(temp_types, vec2);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, vec2);

	return v;
}

bc_value_t bc_corelib__get_fsentry_info(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	const auto info = corelib_get_fsentry_info(path);

	const auto info2 = pack_fsentry_info(temp_types, info);
	const auto v = value_to_bc(vm._imm->_program._types, info2);
	return v;
}


bc_value_t bc_corelib__get_fs_environment(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 0);

	auto temp_types = vm._imm->_program._types;
	const auto env = corelib_get_fs_environment();

	const auto result = pack_fs_environment_t(temp_types, env);
	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}


bc_value_t bc_corelib__does_fsentry_exist(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	auto temp_types = vm._imm->_program._types;
	const std::string path = args[0].get_string_value();

	bool exists = corelib_does_fsentry_exist(path);

	const auto result = value_t::make_bool(exists);
	if(k_trace && false){
		const auto debug = value_and_type_to_ast_json(vm._imm->_program._types, result);
		QUARK_TRACE(json_to_pretty_string(debug));
	}

	const auto v = value_to_bc(vm._imm->_program._types, result);
	return v;
}


bc_value_t bc_corelib__create_directory_branch(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string path = args[0].get_string_value();

	corelib_create_directory_branch(path);

	return bc_value_t::make_void();
}

bc_value_t bc_corelib__delete_fsentry_deep(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 1);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());

	const std::string path = args[0].get_string_value();

	corelib_delete_fsentry_deep(path);

	return bc_value_t::make_void();
}


bc_value_t bc_corelib__rename_fsentry(interpreter_t& vm, const bc_value_t args[], int arg_count){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(arg_count == 2);
	const auto& types = vm._imm->_program._types;
	QUARK_ASSERT(peek2(types, args[0]._type).is_string());
	QUARK_ASSERT(peek2(types, args[1]._type).is_string());

	const std::string path = args[0].get_string_value();
	const std::string n = args[1].get_string_value();

	corelib_rename_fsentry(path, n);

	return bc_value_t::make_void();
}



/////////////////////////////////////////		REGISTRY



std::map<function_id_t, BC_NATIVE_FUNCTION_PTR> bc_get_corelib_calls(){

	const auto result = std::map<function_id_t, BC_NATIVE_FUNCTION_PTR>{
		{ { "make_benchmark_report" }, bc_corelib__make_benchmark_report },

		{ { "detect_hardware_caps" }, bc_corelib__detect_hardware_caps },
		{ { "make_hardware_caps_report" }, bc_corelib__make_hardware_caps_report },
		{ { "make_hardware_caps_report_brief" }, bc_corelib__make_hardware_caps_report_brief },
		{ { "get_current_date_and_time_string" }, bc_corelib__get_current_date_and_time_string },

		{ { "calc_string_sha1" }, bc_corelib__calc_string_sha1 },
		{ { "calc_binary_sha1" }, bc_corelib__calc_binary_sha1 },

		{ { "get_time_of_day" }, bc_corelib__get_time_of_day },

		{ { "read_text_file" }, bc_corelib__read_text_file },
		{ { "write_text_file" }, bc_corelib__write_text_file },
		{ { "read_line_stdin" }, bc_corelib__read_line_stdin },

		{ { "get_fsentries_shallow" }, bc_corelib__get_fsentries_shallow },
		{ { "get_fsentries_deep" }, bc_corelib__get_fsentries_deep },
		{ { "get_fsentry_info" }, bc_corelib__get_fsentry_info },
		{ { "get_fs_environment" }, bc_corelib__get_fs_environment },
		{ { "does_fsentry_exist" }, bc_corelib__does_fsentry_exist },
		{ { "create_directory_branch" }, bc_corelib__create_directory_branch },
		{ { "delete_fsentry_deep" }, bc_corelib__delete_fsentry_deep },
		{ { "rename_fsentry" }, bc_corelib__rename_fsentry }
	};
	return result;
}


}	// floyd
