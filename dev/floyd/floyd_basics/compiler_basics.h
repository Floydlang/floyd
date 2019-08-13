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
#include "typeid.h"
#include "ast_value.h"

#include "quark.h"

struct seq_t;
struct json_t;


namespace floyd {



bool is_floyd_literal(const typeid_t& type);
bool is_preinitliteral(const typeid_t& type);


////////////////////////////////////////		main() init() and message handler


inline typeid_t get_main_signature_arg_impure(){
	return typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_vector(typeid_t::make_string()) }, epure::impure);
}

inline typeid_t get_main_signature_no_arg_impure(){
 	return typeid_t::make_function(typeid_t::make_int(), { }, epure::impure);
}


inline typeid_t get_main_signature_arg_pure(){
	return typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_vector(typeid_t::make_string()) }, epure::pure);
}

inline typeid_t get_main_signature_no_arg_pure(){
	return typeid_t::make_function(typeid_t::make_int(), { }, epure::pure);
}

//	T x_init() impure
typeid_t make_process_init_type(const typeid_t& t);

//	T x(T state, json message) impure
typeid_t make_process_message_handler_type(const typeid_t& t);



//////////////////////////////////////		BENCHMARK SUPPORT


struct benchmark_result_t {
	int64_t dur;
	json_t more;
};
inline bool operator==(const benchmark_result_t& lhs, const benchmark_result_t& rhs){
	return lhs.dur == rhs.dur && lhs.more == rhs.more;
}

inline typeid_t make_benchmark_result_t(){
	const auto x = typeid_t::make_struct2( {
		member_t{ typeid_t::make_int(), "dur" },
		member_t{ typeid_t::make_json(), "more" }
	} );
	return x;
}

inline typeid_t make_benchmark_function_t(){
	return typeid_t::make_function(typeid_t::make_vector(make_benchmark_result_t()), {}, epure::pure);
}

inline typeid_t make_benchmark_def_t(){
	const auto x = typeid_t::make_struct2( {
		member_t{ typeid_t::make_string(), "name" },
		member_t{ make_benchmark_function_t(), "f" }
	} );
	return x;
}


const std::string k_global_benchmark_registry = "benchmark_registry";




//////////////////////////////////////		CORE CALLS


//??? move to compiler_basics

struct corecall_signature_t {
	std::string name;
	function_id_t _function_id;
	floyd::typeid_t _function_type;
};
std::string get_corecall_opcode(const corecall_signature_t& signature);

corecall_signature_t make_assert_signature();
corecall_signature_t make_to_string_signature();
corecall_signature_t make_to_pretty_string_signature();

corecall_signature_t make_typeof_signature();

corecall_signature_t make_update_signature();
corecall_signature_t make_size_signature();
corecall_signature_t make_find_signature();
corecall_signature_t make_exists_signature();
corecall_signature_t make_erase_signature();
corecall_signature_t make_get_keys_signature();
corecall_signature_t make_push_back_signature();
corecall_signature_t make_subset_signature();
corecall_signature_t make_replace_signature();

corecall_signature_t make_parse_json_script_signature();
corecall_signature_t make_generate_json_script_signature();
corecall_signature_t make_to_json_signature();
corecall_signature_t make_from_json_signature();

corecall_signature_t make_get_json_type_signature();




//////////////////////////////////////		HIGHER-ORDER CORE CALLS



corecall_signature_t make_map_signature();
typeid_t harden_map_func_type(const typeid_t& resolved_call_type);
bool check_map_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context);

corecall_signature_t make_map_string_signature();
typeid_t harden_map_string_func_type(const typeid_t& resolved_call_type);
bool check_map_string_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context);

corecall_signature_t make_map_dag_signature();
typeid_t harden_map_dag_func_type(const typeid_t& resolved_call_type);
bool check_map_dag_func_type(const typeid_t& elements, const typeid_t& depends_on, const typeid_t& f, const typeid_t& context);


corecall_signature_t make_filter_signature();
typeid_t harden_filter_func_type(const typeid_t& resolved_call_type);
bool check_filter_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context);

corecall_signature_t make_reduce_signature();
typeid_t harden_reduce_func_type(const typeid_t& resolved_call_type);
bool check_reduce_func_type(const typeid_t& elements, const typeid_t& accumulator_init, const typeid_t& f, const typeid_t& context);

corecall_signature_t make_stable_sort_signature();
typeid_t harden_stable_sort_func_type(const typeid_t& resolved_call_type);
bool check_stable_sort_func_type(const typeid_t& elements, const typeid_t& less, const typeid_t& context);


//////////////////////////////////////		IMPURE FUNCTIONS



corecall_signature_t make_print_signature();
corecall_signature_t make_send_signature();



//////////////////////////////////////		BITWISE


corecall_signature_t make_bw_not_signature();
corecall_signature_t make_bw_and_signature();
corecall_signature_t make_bw_or_signature();
corecall_signature_t make_bw_xor_signature();
corecall_signature_t make_bw_shift_left_signature();
corecall_signature_t make_bw_shift_right_signature();
corecall_signature_t make_bw_shift_right_arithmetic_signature();

std::vector<corecall_signature_t> get_corecall_signatures();





////////////////////////////////////////		location_t


//	Specifies character offset inside source code.

struct location_t {
	explicit location_t(std::size_t offset) :
		offset(offset)
	{
	}

	location_t(const location_t& other) = default;
	location_t& operator=(const location_t& other) = default;

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
	std::string prefix_source;
	std::string program_text;
	std::string source_file_path;
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


void throw_compiler_error_nopos(const std::string& message) __dead2;
inline void throw_compiler_error_nopos(const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(k_no_location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}



void throw_compiler_error(const location_t& location, const std::string& message) __dead2;
inline void throw_compiler_error(const location_t& location, const std::string& message){
//	location2_t(const std::string& source_file_path, int line_number, int column, std::size_t start, std::size_t end, const std::string& line) :
	throw compiler_error(location, location2_t("", 0, 0, 0, 0, "", k_no_location), message);
}

void throw_compiler_error(const location2_t& location2, const std::string& message) __dead2;
inline void throw_compiler_error(const location2_t& location2, const std::string& message){
	throw compiler_error(location2.loc, location2, message);
}


location2_t find_source_line(const std::string& program, const std::string& file, bool corelib, const location_t& loc);


////////////////////////////////////////		refine_compiler_error_with_loc2()


std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e);




////////////////////////////////	MISSING FEATURES



void NOT_IMPLEMENTED_YET() __dead2;
void UNSUPPORTED() __dead2;



void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest);

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected);




////////////////////////////////	make_ast_node()




//	Creates json values for different AST constructs like expressions and statements.

json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params);



}	// floyd

#endif /* compiler_basics_hpp */
