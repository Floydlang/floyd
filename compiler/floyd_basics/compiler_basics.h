//
//  compiler_basics.hpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef compiler_basics_hpp
#define compiler_basics_hpp

/*
	Infrastructure primitives under the compiler.
*/
#include "types.h"
#include "ast_value.h"

#include "quark.h"

#include <optional>

struct seq_t;
struct json_t;


namespace floyd {




/*
	int64_max	9223372036854775807		(2 ^ 63 - 1)	0b01111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111	0x7fffffff'ffffffff
	int64_min	-9223372036854775808	(2 ^ 63)		0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000	0x80000000'00000000

	uint64_max	18446744073709551615	(2 ^ 64 - 1)	0b11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111	0xffffffff'ffffffff
*/

const uint64_t k_floyd_int64_max =	0x7fffffff'ffffffff;
const uint64_t k_floyd_int64_min =	0x80000000'00000000;
const uint64_t k_floyd_uint64_max =	0xffffffff'ffffffff;



bool is_floyd_literal(const type_desc_t& type);
bool is_preinitliteral(const type_desc_t& type);


enum class epod_type {
	pod,
	nonpod
};


////////////////////////////////////////		main() init() and message handler


inline type_t get_main_signature_arg_impure(types_t& types){
	return make_function(types, type_t::make_int(), { make_vector(types, type_t::make_string()) }, epure::impure);
}

inline type_t get_main_signature_no_arg_impure(types_t& types){
 	return make_function(types, type_t::make_int(), { }, epure::impure);
}


inline type_t get_main_signature_arg_pure(types_t& types){
	return make_function(types, type_t::make_int(), { make_vector(types, type_t::make_string()) }, epure::pure);
}

inline type_t get_main_signature_no_arg_pure(types_t& types){
	return make_function(types, type_t::make_int(), { }, epure::pure);
}

//	T x_init() impure
type_t make_process_init_type(types_t& types, const type_t& t);

//	T x(T state, json message) impure
type_t make_process_message_handler_type(types_t& types, const type_t& t);





//////////////////////////////////////		benchmark_result_t


struct benchmark_result_t {
	int64_t dur;
	json_t more;
};
inline bool operator==(const benchmark_result_t& lhs, const benchmark_result_t& rhs){
	return lhs.dur == rhs.dur && lhs.more == rhs.more;
}

inline type_t make_benchmark_result_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ type_t::make_int(), "dur" },
			member_t{ type_t::make_json(), "more" }
		})
	);
	return x;
}


//////////////////////////////////////		make_benchmark_def_t


inline type_t make_benchmark_function_t(types_t& types){
//	return make_function(make_vector(make_benchmark_result_t()), {}, epure::pure);
	return make_function(types, make_vector(types, make_symbol_ref(types, "benchmark_result_t")), {}, epure::pure);
}

inline type_t make_benchmark_def_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ type_t::make_string(), "name" },
			member_t{ make_benchmark_function_t(types), "f" }
		})
	);
	return x;
}




//////////////////////////////////////		benchmark_id_t


struct benchmark_id_t {
	std::string module;
	std::string test;
};
inline bool operator==(const benchmark_id_t& lhs, const benchmark_id_t& rhs){
	return lhs.module == rhs.module && lhs.test == rhs.test;
}
inline type_t make_benchmark_id_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ type_t::make_string(), "module" },
			member_t{ type_t::make_string(), "test" }
		})
	);
	return x;
}


//////////////////////////////////////		bench_t

struct bench_t {
	benchmark_id_t benchmark_id;
	link_name_t f;
};
inline bool operator==(const bench_t& lhs, const bench_t& rhs){ return lhs.benchmark_id == rhs.benchmark_id && lhs.f == rhs.f; }



//////////////////////////////////////		benchmark_result2_t


struct benchmark_result2_t {
	benchmark_id_t test_id;
	benchmark_result_t result;
};

inline bool operator==(const benchmark_result2_t& lhs, const benchmark_result2_t& rhs){
	return lhs.test_id == rhs.test_id && lhs.result == rhs.result;
}

inline type_t make_benchmark_result2_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ make_benchmark_id_t(types), "test_id" },
			member_t{ make_symbol_ref(types, "benchmark_result_t"), "result" }
			})
	);
	return x;
}

std::vector<benchmark_result2_t> unpack_vec_benchmark_result2_t(types_t& types, const value_t& value);



//////////////////////////////////////		k_global_benchmark_registry



const std::string k_global_benchmark_registry = "benchmark_registry";





//////////////////////////////////////		make_test_def_t



inline type_t make_test_function_t(types_t& types){
	return make_function(types, type_t::make_void(), {}, epure::pure);
}

inline type_t make_test_def_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ type_t::make_string(), "function_name" },
			member_t{ type_t::make_string(), "scenario" },
			member_t{ make_test_function_t(types), "f" }
		})
	);
	return x;
}

//////////////////////////////////////		test_id_t


struct test_id_t {
	std::string module;
	std::string function_name;
	std::string scenario;
};
inline bool operator==(const test_id_t& lhs, const test_id_t& rhs){
	return lhs.module == rhs.module && lhs.function_name == rhs.function_name && lhs.scenario == rhs.scenario;
}
inline type_t make_test_id_t(types_t& types){
	const auto x = make_struct(
		types,
		struct_type_desc_t({
			member_t{ type_t::make_string(), "module" },
			member_t{ type_t::make_string(), "function_name" },
			member_t{ type_t::make_string(), "scenario" }
		})
	);
	return x;
}


//////////////////////////////////////		test_t


struct test_t {
	test_id_t test_id;
	link_name_t f;
};
inline bool operator==(const test_t& lhs, const test_t& rhs){ return lhs.test_id == rhs.test_id && lhs.f == rhs.f; }



const std::string k_global_test_registry = "test_registry";

std::vector<test_t> unpack_test_registry(const std::vector<value_t>& r);

struct test_result_t {
	enum class type { success, fail_with_string, not_run };
	type type;
	std::string fail_string;

	test_id_t test_id;
};

inline bool operator==(const test_result_t& lhs, const test_result_t& rhs){ return lhs.type == rhs.type && lhs.fail_string == rhs.fail_string; }

std::string pack_test_id(const test_id_t& id);
std::optional<test_id_t> unpack_test_id(const std::string& test);
std::vector<test_id_t> unpack_test_ids(const std::vector<std::string>& tests);
std::vector<int> filter_tests(const std::vector<test_t>& b, const std::vector<std::string>& wanted_tests);

std::string make_report(const std::vector<test_result_t>& test_results);

size_t count_fails(const std::vector<test_result_t>& test_results);




//////////////////////////////////////		container_t


struct connection_t {
	std::string _source_key;
	std::string _dest_key;
	std::string _interaction_desc;
	std::string _tech_desc;
};
inline bool operator==(const connection_t& lhs, const connection_t& rhs){
	return
		lhs._source_key == rhs._source_key
		&& lhs._dest_key == rhs._dest_key
		&& lhs._interaction_desc == rhs._interaction_desc
		&& lhs._tech_desc == rhs._tech_desc
	;
}

struct process_def_t {
	std::string name_key;

	type_t state_type;
	type_t msg_type;

	std::string init_func_linkname;
	std::string msg_func_linkname;
};
inline bool operator==(const process_def_t& lhs, const process_def_t& rhs){
	return
		lhs.name_key == rhs.name_key

		&& lhs.state_type == rhs.state_type
		&& lhs.msg_type == rhs.msg_type

		&& lhs.init_func_linkname == rhs.init_func_linkname
		&& lhs.msg_func_linkname == rhs.msg_func_linkname
		;
}

struct clock_bus_t {
	//	k: name of bus, v: struct-name of the process' state, also used to find function names.
	std::map<std::string, process_def_t> _processes;
};
inline bool operator==(const clock_bus_t& lhs, const clock_bus_t& rhs){
	return
		lhs._processes == rhs._processes
	;
}


struct container_t {
	std::string _name;
	std::string _desc;
	std::string _tech;
	std::map<std::string, clock_bus_t> _clock_busses;
};

inline bool operator==(const container_t& lhs, const container_t& rhs){
	return
		lhs._name == rhs._name
		&& lhs._desc == rhs._desc
		&& lhs._tech == rhs._tech

		&& lhs._clock_busses == rhs._clock_busses
	;
}

container_t parse_container_def_json(types_t& types, const json_t& value);
json_t container_to_json(const types_t& types, const container_t& v);


//////////////////////////////////////		software_system_t


struct person_t {
	std::string _name_key;
	std::string _desc;
};
inline bool operator==(const person_t& lhs, const person_t& rhs){
	return
		lhs._name_key == rhs._name_key
		&& lhs._desc == rhs._desc
	;
}

struct software_system_t {
	std::string _name;
	std::string _desc;
	std::vector<person_t> _people;
	std::vector<std::string> _containers;
};

inline bool operator==(const software_system_t& lhs, const software_system_t& rhs){
	return
		lhs._name == rhs._name
		&& lhs._desc == rhs._desc

		&& lhs._people == rhs._people
		&& lhs._containers == rhs._containers
	;
}

software_system_t parse_software_system_json(const json_t& value);
json_t software_system_to_json(const software_system_t& v);



//////////////////////////////////////		INTRINSICS




struct intrinsic_signature_t {
	std::string name;
	type_t _function_type;
};
std::string get_intrinsic_opcode(const intrinsic_signature_t& signature);

intrinsic_signature_t make_assert_signature(types_t& types);
intrinsic_signature_t make_to_string_signature(types_t& types);
intrinsic_signature_t make_to_pretty_string_signature(types_t& types);

intrinsic_signature_t make_typeof_signature(types_t& types);

intrinsic_signature_t make_update_signature(types_t& types);
intrinsic_signature_t make_size_signature(types_t& types);
intrinsic_signature_t make_find_signature(types_t& types);
intrinsic_signature_t make_exists_signature(types_t& types);
intrinsic_signature_t make_erase_signature(types_t& types);
intrinsic_signature_t make_get_keys_signature(types_t& types);
intrinsic_signature_t make_push_back_signature(types_t& types);
intrinsic_signature_t make_subset_signature(types_t& types);
intrinsic_signature_t make_replace_signature(types_t& types);

intrinsic_signature_t make_parse_json_script_signature(types_t& types);
intrinsic_signature_t make_generate_json_script_signature(types_t& types);
intrinsic_signature_t make_to_json_signature(types_t& types);
intrinsic_signature_t make_from_json_signature(types_t& types);

intrinsic_signature_t make_get_json_type_signature(types_t& types);




//////////////////////////////////////		HIGHER-ORDER INTRINSICS



intrinsic_signature_t make_map_signature(types_t& types);
type_t harden_map_func_type(types_t& types, const type_t& resolved_call_type);
bool check_map_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context);

intrinsic_signature_t make_map_string_signature(types_t& types);
type_t harden_map_string_func_type(types_t& types, const type_t& resolved_call_type);
bool check_map_string_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context);

intrinsic_signature_t make_map_dag_signature(types_t& types);
type_t harden_map_dag_func_type(types_t& types, const type_t& resolved_call_type);
bool check_map_dag_func_type(types_t& types, const type_t& elements, const type_t& depends_on, const type_t& f, const type_t& context);


intrinsic_signature_t make_filter_signature(types_t& types);
type_t harden_filter_func_type(types_t& types, const type_t& resolved_call_type);
bool check_filter_func_type(types_t& types, const type_t& elements, const type_t& f, const type_t& context);

intrinsic_signature_t make_reduce_signature(types_t& types);
type_t harden_reduce_func_type(types_t& types, const type_t& resolved_call_type);
bool check_reduce_func_type(types_t& types, const type_t& elements, const type_t& accumulator_init, const type_t& f, const type_t& context);

intrinsic_signature_t make_stable_sort_signature(types_t& types);
type_t harden_stable_sort_func_type(types_t& types, const type_t& resolved_call_type);
bool check_stable_sort_func_type(types_t& types, const type_t& elements, const type_t& less, const type_t& context);


//////////////////////////////////////		IMPURE FUNCTIONS



intrinsic_signature_t make_print_signature(types_t& types);
intrinsic_signature_t make_send_signature(types_t& types);
intrinsic_signature_t make_exit_signature(types_t& types);



//////////////////////////////////////		BITWISE


intrinsic_signature_t make_bw_not_signature(types_t& types);
intrinsic_signature_t make_bw_and_signature(types_t& types);
intrinsic_signature_t make_bw_or_signature(types_t& types);
intrinsic_signature_t make_bw_xor_signature(types_t& types);
intrinsic_signature_t make_bw_shift_left_signature(types_t& types);
intrinsic_signature_t make_bw_shift_right_signature(types_t& types);
intrinsic_signature_t make_bw_shift_right_arithmetic_signature(types_t& types);


struct intrinsic_signatures_t {
	std::vector<intrinsic_signature_t> vec;

	intrinsic_signature_t assert;
	intrinsic_signature_t to_string;
	intrinsic_signature_t to_pretty_string;

	intrinsic_signature_t typeof_sign;

	intrinsic_signature_t update;
	intrinsic_signature_t size;
	intrinsic_signature_t find;
	intrinsic_signature_t exists;
	intrinsic_signature_t erase;
	intrinsic_signature_t get_keys;
	intrinsic_signature_t push_back;
	intrinsic_signature_t subset;
	intrinsic_signature_t replace;

	intrinsic_signature_t parse_json_script;
	intrinsic_signature_t generate_json_script;
	intrinsic_signature_t to_json;
	intrinsic_signature_t from_json;

	intrinsic_signature_t get_json_type;



	intrinsic_signature_t map;
	intrinsic_signature_t filter;
	intrinsic_signature_t reduce;
	intrinsic_signature_t map_dag;

	intrinsic_signature_t stable_sort;

	intrinsic_signature_t print;
	intrinsic_signature_t send;
	intrinsic_signature_t exit;

	intrinsic_signature_t bw_not;
	intrinsic_signature_t bw_and;
	intrinsic_signature_t bw_or;
	intrinsic_signature_t bw_xor;
	intrinsic_signature_t bw_shift_left;
	intrinsic_signature_t bw_shift_right;
	intrinsic_signature_t bw_shift_right_arithmetic;
};

intrinsic_signatures_t make_intrinsic_signatures(types_t& types);





////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	location_t(const location_t& other) = default;
	location_t& operator=(const location_t& other) = default;
	bool check_invariant() const {
		return true;
	}


	std::size_t offset;
};

inline bool operator==(const location_t& lhs, const location_t& rhs){
	return lhs.offset == rhs.offset;
}
extern const location_t k_no_location;



////////////////////////////////////////		location2_t


struct location2_t {
	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line, const location_t& loc) :
		source_file_path(source_file_path),
		line_number(line_number),
		column(column),
		start(start),
		end(end),
		line(line),
		loc(loc)
	{
	}

	std::string source_file_path;
	int line_number;
	int column;
	std::size_t start;
	std::size_t end;
	std::string line;
	location_t loc;
};


//	INPUT: [2, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"]]
std::pair<json_t, location_t> unpack_loc(const json_t& s);

//	Reads a location_t from a statement, if one exists. Else it returns k_no_location.
//	INPUT: [2, "bind", "^double", "cmath_pi", ["k", 3.14159, "^double"]]
location_t unpack_loc2(const json_t& s);



////////////////////////////////////////		compilation_unit_t


struct compilation_unit_t {
	bool check_invariant() const {
		return true;
	}


	std::string prefix_source;
	std::string program_text;
	std::string source_file_path;
};




////////////////////////////////////////		config_t



enum class vector_backend {
	carray,
	hamt
};
enum class dict_backend {
	cppmap,
	hamt
};

struct config_t {
	bool check_invariant() const {
		return true;
	}


	vector_backend vector_backend_mode;
	dict_backend dict_backend_mode;
	bool trace_allocs;
};

inline bool operator==(const config_t& lhs, const config_t& rhs){
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());
	return lhs.vector_backend_mode == rhs.vector_backend_mode && lhs.dict_backend_mode == rhs.dict_backend_mode && lhs.trace_allocs == rhs.trace_allocs;
}


config_t make_default_config();



////////////////////////////////////////		compiler_settings_t


/*
Optimization level
Flags controlling how much optimization should be performed.

-O<arg>, -O (equivalent to -O2), --optimize, --optimize=<arg>
-Ofast<arg>
Debug information generation
Flags controlling how much and what kind of debug information should be generated.

Kind and level of debug information
-g, --debug, --debug=<arg>

OPTIONS:
  Choose optimization level:
    -g          - No optimizations, enable debugging
    -O1         - Enable trivial optimizations
    -O2         - Enable default optimizations
    -O3         - Enable expensive optimizations
  -f            - Enable binary output on terminals
  -help         - display available options (-help-hidden for more)
  -o <filename> - Specify output filename
  -quiet        - Don't print informational messages
*/

enum class eoptimization_level {
	g_no_optimizations_enable_debugging,
	O1_enable_trivial_optimizations,
	O2_enable_default_optimizations,
	O3_enable_expensive_optimizations
//	Ofast
};

struct compiler_settings_t {
	bool check_invariant() const {
		return true;
	}


	config_t config;
	eoptimization_level optimization_level;
};

compiler_settings_t make_default_compiler_settings();

inline bool operator==(const compiler_settings_t& lhs, const compiler_settings_t& rhs){
	QUARK_ASSERT(lhs.check_invariant());
	QUARK_ASSERT(rhs.check_invariant());
	return lhs.config == rhs.config && lhs.optimization_level == rhs.optimization_level;
}



////////////////////////////////////////		compilation_task_t

//	All inputs requires to compile one translation unit.


struct compilation_task_t {
	compilation_unit_t cu;

	compiler_settings_t compiler_settings;
};





////////////////////////////////////////		compiler_error


class compiler_error : public std::runtime_error {
	public: compiler_error(const location_t& location, const location2_t& location2, const std::string& message) :
		std::runtime_error(message),
		location(location),
		location2(location2)
	{
	}

	public: location_t location;
	public: location2_t location2;
};



////////////////////////////////////////		throw_compiler_error()


void throw_compiler_error_nopos(const std::string& message) QUARK_NO_RETURN;
inline void throw_compiler_error_nopos(const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(k_no_location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}



void throw_compiler_error(const location_t& location, const std::string& message) QUARK_NO_RETURN;
inline void throw_compiler_error(const location_t& location, const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}

void throw_compiler_error(const location2_t& location2, const std::string& message) QUARK_NO_RETURN;
inline void throw_compiler_error(const location2_t& location2, const std::string& message){
	throw compiler_error(location2.loc, location2, message);
}


location2_t find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc);


////////////////////////////////////////		refine_compiler_error_with_loc2()


std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e);




////////////////////////////////	MISSING FEATURES



void NOT_IMPLEMENTED_YET() QUARK_NO_RETURN;
void UNSUPPORTED() QUARK_NO_RETURN;



void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);

void ut_verify_string_seq(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);




////////////////////////////////	make_ast_node()




//	Creates json values for different AST constructs like expressions and statements.

json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);




////////////////////////////////		ENCODE / DECODE LINK NAMES



//	"hello" => "floydf_hello"
link_name_t encode_floyd_func_link_name(const std::string& name);
std::string decode_floyd_func_link_name(const link_name_t& name);


//	"hello" => "floyd_runtime_hello"
link_name_t encode_runtime_func_link_name(const std::string& name);
std::string decode_runtime_func_link_name(const link_name_t& name);


//	"hello" => "floyd_intrinsic_hello"
link_name_t encode_intrinsic_link_name(const std::string& name);
std::string decode_intrinsic_link_name(const link_name_t& name);





}	// floyd

#endif /* compiler_basics_hpp */
