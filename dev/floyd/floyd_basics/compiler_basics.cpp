//
//  compiler_basics.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "compiler_basics.h"

#include "text_parser.h"
#include "parser_primitives.h"

namespace floyd {


const location_t k_no_location(std::numeric_limits<std::size_t>::max());







std::vector<benchmark_result2_t> unpack_vec_benchmark_result2_t(const value_t& value){
	QUARK_ASSERT(value.get_type() == typeid_t::make_vector(make_benchmark_result2_t()));

	std::vector<benchmark_result2_t> test_results;
	for(const auto& e: value.get_vector_value()){
		const auto& f = e.get_struct_value();

		const auto& test_id = f->_member_values[0].get_struct_value();
		const auto& result = f->_member_values[1].get_struct_value();

		const auto module = test_id->_member_values[0].get_string_value();
		const auto test = test_id->_member_values[1].get_string_value();

		const auto dur = result->_member_values[0].get_int_value();
		const auto more = result->_member_values[1].get_json();

		const auto r = benchmark_result2_t { benchmark_id_t { module, test }, benchmark_result_t { dur, more } };
		test_results.push_back(r);
	}
	return test_results;
}



bool is_floyd_literal(const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(type.is_void() || type.is_int() || type.is_double() || type.is_string() || type.is_bool() || type.is_typeid() || type.is_any() || type.is_json() || type.is_function()){
		return true;
	}
	else{
		return false;
	}
}

bool is_preinitliteral(const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	//??? json is allowed but only for json::null. We should have a null-type instead.
	if(type.is_int() || type.is_double() || type.is_bool() || type.is_typeid() || type.is_any() || type.is_function()){
		return true;
	}
	else{
		return false;
	}
}


////////////////////////////////////////		main() init() and message handler




typeid_t make_process_init_type(const typeid_t& t){
	return typeid_t::make_function(t, {}, epure::impure);
}

typeid_t make_process_message_handler_type(const typeid_t& t){
	return typeid_t::make_function(t, { t, typeid_t::make_json() }, epure::impure);
}



//////////////////////////////////////		INTRINSICS




#define ANY_TYPE typeid_t::make_any()

static intrinsic_signature_t make_intrinsic(
	const std::string& name,
	floyd::typeid_t function_type
){
	return { name, function_type };
}


intrinsic_signature_t make_assert_signature(){
	return make_intrinsic("assert", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_bool() }, epure::pure) );
}
intrinsic_signature_t make_to_string_signature(){
	return make_intrinsic("to_string", typeid_t::make_function(typeid_t::make_string(), { ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_to_pretty_string_signature(){
	return make_intrinsic("to_pretty_string", typeid_t::make_function(typeid_t::make_string(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_typeof_signature(){
	return make_intrinsic("typeof", typeid_t::make_function(typeid_t::make_typeid(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_update_signature(){
	return make_intrinsic("update", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}

intrinsic_signature_t make_size_signature(){
	return make_intrinsic("size", typeid_t::make_function(typeid_t::make_int(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_find_signature(){
	return make_intrinsic("find", typeid_t::make_function(typeid_t::make_int(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_exists_signature(){
	return make_intrinsic("exists", typeid_t::make_function(typeid_t::make_bool(), { ANY_TYPE, ANY_TYPE }, epure::pure) );
}

intrinsic_signature_t make_erase_signature(){
	return make_intrinsic("erase", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}
intrinsic_signature_t make_get_keys_signature(){
	return make_intrinsic("get_keys", typeid_t::make_function(typeid_t::make_vector(typeid_t::make_string()), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_push_back_signature(){
	return make_intrinsic("push_back", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}

intrinsic_signature_t make_subset_signature(){
	return make_intrinsic("subset", typeid_t::make_function_dyn_return({ ANY_TYPE, typeid_t::make_int(), typeid_t::make_int() }, epure::pure, typeid_t::return_dyn_type::arg0) );
}
intrinsic_signature_t make_replace_signature(){
	return make_intrinsic("replace", typeid_t::make_function_dyn_return({ ANY_TYPE, typeid_t::make_int(), typeid_t::make_int(), ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}

intrinsic_signature_t make_parse_json_script_signature(){
	return make_intrinsic("parse_json_script", typeid_t::make_function(typeid_t::make_json(), { typeid_t::make_string() }, epure::pure) );
}

intrinsic_signature_t make_generate_json_script_signature(){
	return make_intrinsic("generate_json_script", typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_json() }, epure::pure) );
}


intrinsic_signature_t make_to_json_signature(){
	return make_intrinsic("to_json", typeid_t::make_function(typeid_t::make_json(), { ANY_TYPE }, epure::pure) );
}


intrinsic_signature_t make_from_json_signature(){
	//	??? Tricky. How to we compute the return type from the input arguments?
	return make_intrinsic("from_json", typeid_t::make_function_dyn_return({ typeid_t::make_json(), typeid_t::make_typeid() }, epure::pure, typeid_t::return_dyn_type::arg1_typeid_constant_type) );
}


intrinsic_signature_t make_get_json_type_signature(){
	return make_intrinsic("get_json_type", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_json() }, epure::pure) );
}




//////////////////////////////////////		HIGHER-ORDER INTRINSICS



intrinsic_signature_t make_map_signature(){
	return make_intrinsic("map", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::vector_of_arg1func_return) );
}
typeid_t harden_map_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto arg2_type = resolved_call_type.get_function_args()[1];
	if(arg2_type.is_function() == false){
		quark::throw_runtime_error("map() arg 2 must be a function.");
	}

	const auto r_type = arg2_type.get_function_return();

	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(r_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_function(r_type, { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());

	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}



//	string map_string(string s, func int(int e, C context) f, C context)
intrinsic_signature_t make_map_string_signature(){
	return make_intrinsic(
		"map_string",
		typeid_t::make_function(
			typeid_t::make_string(),
			{
				typeid_t::make_string(),
				ANY_TYPE,
				ANY_TYPE
			},
			epure::pure
		)
	);
}

typeid_t harden_map_string_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_string_signature();
	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_string(),
		{
			typeid_t::make_string(),
			typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_string_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_string());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == typeid_t::make_int());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}




//	[R] map_dag([E] elements, [int] depends_on, func R (E, [R], C context) f, C context)
intrinsic_signature_t make_map_dag_signature(){
	return make_intrinsic("map_dag", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::vector_of_arg2func_return) );
}

typeid_t harden_map_dag_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_map_dag_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map_dag() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto arg3_type = resolved_call_type.get_function_args()[2];
	if(arg3_type.is_function() == false){
		quark::throw_runtime_error("map_dag() arg 3 must be a function.");
	}

	const auto r_type = arg3_type.get_function_return();

	const auto context_type = resolved_call_type.get_function_args()[3];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(r_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_vector(typeid_t::make_int()),
			typeid_t::make_function(r_type, { e_type, typeid_t::make_vector(r_type), context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_map_dag_func_type(const typeid_t& elements, const typeid_t& depends_on, const typeid_t& f, const typeid_t& context){
	//	Check topology.
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(depends_on == typeid_t::make_vector(typeid_t::make_int()));
	QUARK_ASSERT(f.is_function() && f.get_function_args().size () == 3);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == typeid_t::make_vector(f.get_function_return()));
	QUARK_ASSERT(f.get_function_args()[2] == context);
	return true;
}





//	[E] filter([E] elements, func bool (E e, C context) f, C context)
intrinsic_signature_t make_filter_signature(){
	return make_intrinsic("filter", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}
typeid_t harden_filter_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_filter_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		typeid_t::make_vector(e_type),
		{
			typeid_t::make_vector(e_type),
			typeid_t::make_function(typeid_t::make_bool(), { e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_filter_func_type(const typeid_t& elements, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args().size() == 2);
	QUARK_ASSERT(f.get_function_args()[0] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[1] == context);
	return true;
}




intrinsic_signature_t make_reduce_signature(){
	return make_intrinsic("reduce", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg1) );
}

typeid_t harden_reduce_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto call_function_arg_type = resolved_call_type.get_function_args();

	const auto elements_arg_type = call_function_arg_type[0];
	if(elements_arg_type.is_vector() == false){
		quark::throw_runtime_error("map() arg 1 must be a vector.");
	}
	const auto e_type = elements_arg_type.get_vector_element_type();

	const auto r_type = call_function_arg_type[1];

	const auto context_type = call_function_arg_type[3];

	const auto expected = typeid_t::make_function(
		r_type,
		{
			typeid_t::make_vector(e_type),
			r_type,
			typeid_t::make_function(r_type, { r_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_reduce_func_type(const typeid_t& elements, const typeid_t& accumulator_init, const typeid_t& f, const typeid_t& context){
	QUARK_ASSERT(elements.check_invariant());
	QUARK_ASSERT(accumulator_init.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	QUARK_ASSERT(context.check_invariant());

	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(f.is_function());
	QUARK_ASSERT(f.get_function_args().size () == 3);

	QUARK_ASSERT(f.get_function_args()[1] == elements.get_vector_element_type());
	QUARK_ASSERT(f.get_function_args()[0] == accumulator_init);

	return true;
}




//	[T] stable_sort([T] elements, func bool (T left, T right, C context) less, C context)
intrinsic_signature_t make_stable_sort_signature(){
	return make_intrinsic("stable_sort", typeid_t::make_function_dyn_return({ ANY_TYPE, ANY_TYPE, ANY_TYPE }, epure::pure, typeid_t::return_dyn_type::arg0) );
}

typeid_t harden_stable_sort_func_type(const typeid_t& resolved_call_type){
	QUARK_ASSERT(resolved_call_type.check_invariant());

	const auto sign = make_stable_sort_signature();

	const auto arg1_type = resolved_call_type.get_function_args()[0];
	if(arg1_type.is_vector() == false){
		quark::throw_runtime_error("stable_sort() arg 1 must be a vector.");
	}
	const auto e_type = arg1_type.get_vector_element_type();

	const auto arg2_type = resolved_call_type.get_function_args()[1];
	if(arg2_type.is_function() == false){
		quark::throw_runtime_error("stable_sort() arg 2 must be a function.");
	}

	const auto context_type = resolved_call_type.get_function_args()[2];

	const auto expected = typeid_t::make_function(
		arg1_type,
		{
			arg1_type,
			typeid_t::make_function(typeid_t::make_bool(), { e_type, e_type, context_type }, epure::pure),
			context_type
		},
		epure::pure
	);
	return expected;
}

bool check_stable_sort_func_type(const typeid_t& elements, const typeid_t& less, const typeid_t& context){
	QUARK_ASSERT(elements.is_vector());
	QUARK_ASSERT(less.is_function());
	QUARK_ASSERT(less.get_function_args().size() == 3);

	const auto& e_type = elements.get_vector_element_type();
	QUARK_ASSERT(less.get_function_return() == typeid_t::make_bool());
	QUARK_ASSERT(less.get_function_args()[0] == e_type);
	QUARK_ASSERT(less.get_function_args()[1] == e_type);
	QUARK_ASSERT(less.get_function_args()[2] == context);

	return true;
}





//////////////////////////////////////		IMPURE FUNCTIONS



intrinsic_signature_t make_print_signature(){
	return make_intrinsic("print", typeid_t::make_function(typeid_t::make_void(), { ANY_TYPE }, epure::pure) );
}
intrinsic_signature_t make_send_signature(){
	return make_intrinsic("send", typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_string(), typeid_t::make_json() }, epure::impure) );
}



//////////////////////////////////////		BITWISE



intrinsic_signature_t make_bw_not_signature(){
	return make_intrinsic("bw_not", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_and_signature(){
	return make_intrinsic("bw_and", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_or_signature(){
	return make_intrinsic("bw_or", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_xor_signature(){
	return make_intrinsic("bw_xor", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_left_signature(){
	return make_intrinsic("bw_shift_left", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_signature(){
	return make_intrinsic("bw_shift_right", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}
intrinsic_signature_t make_bw_shift_right_arithmetic_signature(){
	return make_intrinsic("bw_shift_right_arithmetic", typeid_t::make_function(typeid_t::make_int(), { typeid_t::make_int(), typeid_t::make_int() }, epure::pure) );
}







std::string get_intrinsic_opcode(const intrinsic_signature_t& signature){
	return std::string() + "$" + signature.name;
}

std::vector<intrinsic_signature_t> get_intrinsic_signatures(){
	const std::vector<intrinsic_signature_t> result = {
		make_assert_signature(),
		make_to_string_signature(),
		make_to_pretty_string_signature(),
		make_typeof_signature(),

		make_update_signature(),
		make_size_signature(),
		make_find_signature(),
		make_exists_signature(),
		make_erase_signature(),
		make_get_keys_signature(),
		make_push_back_signature(),
		make_subset_signature(),
		make_replace_signature(),

		make_parse_json_script_signature(),
		make_generate_json_script_signature(),
		make_to_json_signature(),
		make_from_json_signature(),

		make_get_json_type_signature(),

		make_map_signature(),
//		make_map_string_signature(),
		make_filter_signature(),
		make_reduce_signature(),
		make_map_dag_signature(),

		make_stable_sort_signature(),

		make_print_signature(),
		make_send_signature(),

		make_bw_not_signature(),
		make_bw_and_signature(),
		make_bw_or_signature(),
		make_bw_xor_signature(),
		make_bw_shift_left_signature(),
		make_bw_shift_right_signature(),
		make_bw_shift_right_arithmetic_signature()
	};
	return result;
}





//////////////////////////////////////////////////		location_t





std::pair<json_t, location_t> unpack_loc(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;

		const auto elements = s.get_array();
		const std::vector<json_t> elements2 = { elements.begin() + 1, elements.end() };
		const auto statement = json_t::make_array(elements2);

		return { statement, source_offset };
	}
	else{
		return { s, k_no_location };
	}
}

location_t unpack_loc2(const json_t& s){
	QUARK_ASSERT(s.is_array());

	const bool has_location = s.get_array_n(0).is_number();
	if(has_location){
		const location_t source_offset = has_location ? location_t(static_cast<std::size_t>(s.get_array_n(0).get_number())) : k_no_location;
		return source_offset;
	}
	else{
		return k_no_location;
	}
}



//////////////////////////////////////////////////		MACROS



void NOT_IMPLEMENTED_YET() {
	throw std::exception();
}

void UNSUPPORTED() {
	QUARK_ASSERT(false);
	throw std::exception();
}


//	Return one entry per source line PLUS one extra end-marker. int tells byte offset of files that maps to this line-start.
//	Never returns empty vector, at least 2 elements.
std::vector<int> make_location_lookup(const std::string& source){
	if(source.empty()){
		return { 0 };
	}
	else{
		std::vector<int> result;
		int char_index = 0;
		result.push_back(char_index);
		for(const auto& ch: source){
			char_index++;

			if(ch == '\n' || ch == '\r'){
				result.push_back(char_index);
			}
		}
		const auto back_char = source.back();
		if(back_char == '\r' || back_char == '\n'){
			return result;
		}
		else{
			int end_index = static_cast<int>(source.size());
			result.push_back(end_index);
			return result;
		}
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 3 }));
}
QUARK_TEST("", "make_location_lookup()", "", ""){
	const auto r = make_location_lookup("aaa\nbbb\n");
	QUARK_UT_VERIFY((r == std::vector<int>{ 0, 4, 8 }));
}

const std::string cleanup_line_snippet(const std::string& s){
	const auto line1 = skip(seq_t(s), "\t ");
	const auto split = split_on_chars(line1, "\n\r");
	const auto line = split.size() > 0 ? split.front() : "";
	return line;
}
QUARK_TEST("", "cleanup_line_snippet()", "", ""){
	ut_verify(QUARK_POS, cleanup_line_snippet(" \tabc\n\a"), "abc");
}

location2_t make_loc2(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc, int line_index){
	const auto start = lookup[line_index];
	const auto end = lookup[line_index + 1];
	const auto line = cleanup_line_snippet(program.substr(start, end - start));
	const auto column = loc.offset - start;
	return location2_t(file, line_index, static_cast<int>(column), start, end, line, loc);
}


location2_t find_loc_info(const std::string& program, const std::vector<int>& lookup, const std::string& file, const location_t& loc){
	QUARK_ASSERT(lookup.size() >= 2);
	QUARK_ASSERT(loc.offset <= lookup.back());

	int last_line_offset = lookup.back();
	QUARK_ASSERT(last_line_offset >= loc.offset);

	//	 EOF? Use program's last non-blank line.
	if(last_line_offset == loc.offset){
		auto line_index = static_cast<int>(lookup.size()) - 2;
		auto loc2 = make_loc2(program, lookup, file, loc, line_index);
		while(line_index >= 0 && loc2.line ==""){
			line_index--;
			loc2 = make_loc2(program, lookup, file, loc, line_index);
		}
		return loc2;
	}
	else{
		int line_index = 0;
		while(lookup[line_index + 1] <= loc.offset){
			line_index++;
			QUARK_ASSERT(line_index < lookup.size());
		}
		return make_loc2(program, lookup, file, loc, line_index);
	}
}

QUARK_TEST("", "make_location_lookup()", "", ""){
	const std::vector<int> lookup = { 0, 1, 2, 18, 19, 21 };
	const std::string program = R"(

			{ let a = 10

		)";
	const auto r = find_loc_info(program, lookup, "path.txt", location_t(21));
}

location2_t find_source_line(const compilation_unit_t& cu, const location_t& loc){
	const auto program_lookup = make_location_lookup(cu.program_text);

	if(cu.prefix_source != ""){
		const auto corelib_lookup = make_location_lookup(cu.prefix_source);
		const auto corelib_end_offset = corelib_lookup.back();
		if(loc.offset < corelib_end_offset){
			return find_loc_info(cu.prefix_source, corelib_lookup, "corelib", loc);
		}
		else{
			const auto program_loc = location_t(loc.offset - corelib_end_offset);
			const auto loc2 = find_loc_info(cu.program_text, program_lookup, cu.source_file_path, program_loc);
			const auto result = location2_t(
				loc2.source_file_path,
				loc2.line_number,
				loc2.column,
				loc2.start,
				loc2.end,
				loc2.line,
				location_t(loc2.loc.offset + corelib_end_offset)
			);
			return result;
		}
	}
	else{
		return find_loc_info(cu.program_text, program_lookup, cu.source_file_path, loc);
	}
}



/*	const auto line_numbers2 = mapf<int>(
		statements_pos.line_numbers,
		[&](const auto& e){
			return e - pre_line_count;
		}
	);
*/



std::pair<location2_t, std::string> refine_compiler_error_with_loc2(const compilation_unit_t& cu, const compiler_error& e){
	const auto loc2 = find_source_line(cu, e.location);
	const auto what1 = std::string(e.what());

	std::stringstream what2;
	const auto line_snippet = parser::reverse(parser::skip_whitespace(parser::reverse(loc2.line)));
	what2 << what1 << " Line: " << std::to_string(loc2.line_number + 1) << " \"" << line_snippet << "\"";
	if(loc2.source_file_path.empty() == false){
		what2 << " file: " << loc2.source_file_path;
	}

	return { loc2, what2.str() };
}




void ut_verify_json_and_rest(const quark::call_context_t& context, const std::pair<json_t, seq_t>& result_pair, const std::string& expected_json, const std::string& expected_rest){
	ut_verify(
		context,
		result_pair.first,
		parse_json(seq_t(expected_json)).first
	);

	ut_verify(context, result_pair.second.str(), expected_rest);
}

void ut_verify(const quark::call_context_t& context, const std::pair<std::string, seq_t>& result, const std::pair<std::string, seq_t>& expected){
	if(result == expected){
	}
	else{
		ut_verify(context, result.first, expected.first);
		ut_verify(context, result.second.str(), expected.second.str());
	}
}




////////////////////////////////	make_ast_node()



json_t make_ast_node(const location_t& location, const std::string& opcode, const std::vector<json_t>& params){
	if(location == k_no_location){
		std::vector<json_t> e = { json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
	else{
		const auto offset = static_cast<double>(location.offset);
		std::vector<json_t> e = { json_t(offset), json_t(opcode) };
		e.insert(e.end(), params.begin(), params.end());
		return json_t(e);
	}
}

QUARK_TEST("", "make_ast_node()", "", ""){
	const auto r = make_ast_node(location_t(1234), "def-struct", std::vector<json_t>{});

	ut_verify(QUARK_POS, r, json_t::make_array({ 1234.0, "def-struct" }));
}




////////////////////////////////		ENCODE / DECODE LINK NAMES



static const std::string k_floyd_func_link_prefix = "floydf_";


link_name_t encode_floyd_func_link_name(const std::string& name){
	return link_name_t { k_floyd_func_link_prefix + name };
}
std::string decode_floyd_func_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_floyd_func_link_prefix.size());
	const auto right = name.s.substr(k_floyd_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_floyd_func_link_prefix);
	return right;
}



static const std::string k_runtime_func_link_prefix = "floyd_runtime_";

link_name_t encode_runtime_func_link_name(const std::string& name){
	return link_name_t { k_runtime_func_link_prefix + name };
}

std::string decode_runtime_func_link_name(const link_name_t& name){
	const auto left = name.s.substr(0, k_runtime_func_link_prefix.size());
	const auto right = name.s.substr(k_runtime_func_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_runtime_func_link_prefix);
	return right;
}



static const std::string k_intrinsic_link_prefix = "floyd_intrinsic_";

link_name_t encode_intrinsic_link_name(const std::string& name){
	return link_name_t { k_intrinsic_link_prefix + name };
}
std::string decode_intrinsic_link_name(const link_name_t& name){
	const auto left = name.s. substr(0, k_intrinsic_link_prefix.size());
	const auto right = name.s.substr(k_intrinsic_link_prefix.size(), std::string::npos);
	QUARK_ASSERT(left == k_intrinsic_link_prefix);
	return right;
}


}	// floyd

