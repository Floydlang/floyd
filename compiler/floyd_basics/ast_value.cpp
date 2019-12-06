//
//  parser_value.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast_value.h"
#include "text_parser.h"
#include "compiler_basics.h"

#include <cinttypes>

//#define QUARK_TEST QUARK_TEST_VIP

namespace floyd {



//////////////////////////////////////////////////		big-int


/*
	Why: we cannot store a full 64-bit integer in a double without losing information.
	For value_t we detect this and store big integers as a string inside and object instead.

	Example: number 9223372036854775807 is encoded as { "big-int": 64, "value": "9223372036854775807" }
*/



//	Anything less than 2^53 can be stored, with 52 bits explicitly stored in the mantissa, and then the exponent in effect giving you another one.
//	 (9,007,199,254,740,991 or ~9 quadrillion). The reasoning behind that number is that JavaScript uses double-precision floating-point format numbers as specified in IEEE 754 and can only safely represent numbers between -(253 - 1) and 253 - 1.
#define MAX_SAFE_INTEGER 9007199254740991
#define MIN_SAFE_INTEGER -9007199254740991


static std::pair<std::string, int64_t> encode_big_int(int64_t value){
#if 1
	const bool safe_in_double = value <= MAX_SAFE_INTEGER && value >= -MAX_SAFE_INTEGER;
#else
	uint64_t u = value;
	uint64_t high = u & 0xffffffff'00000000;
	const bool safe_in_double = (high == 0xffffffff'00000000 || high == 0x00000000'00000000) ? true : false;
#endif


	if(safe_in_double){
		//	Double-check that this value can be stored non-lossy in a double.
#if DEBUG
		const double d = (double)value;
		const int64_t back = (int64_t)d;
		QUARK_ASSERT(back == value);
#endif
		return { "", value };
	}
	else{
		const auto s = std::to_string(value);
		return { s, -1 };
	}
}

static int64_t decode_big_int(const std::string& s){
	const auto i = std::stol(s);
	return i;
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(0) == (std::pair<std::string, int64_t>("", 0)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(-1) == (std::pair<std::string, int64_t>("", -1)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(k_floyd_int64_max) == (std::pair<std::string, int64_t>("9223372036854775807", -1)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(k_floyd_int64_min) == (std::pair<std::string, int64_t>("-9223372036854775808", -1)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(k_floyd_int64_max) == (std::pair<std::string, int64_t>("9223372036854775807", -1)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(MIN_SAFE_INTEGER) == (std::pair<std::string, int64_t>("", MIN_SAFE_INTEGER)));
}
QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(MAX_SAFE_INTEGER) == (std::pair<std::string, int64_t>("", MAX_SAFE_INTEGER)));
}

QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(MIN_SAFE_INTEGER - 1) == (std::pair<std::string, int64_t>("-9007199254740992", -1)));
}
QUARK_TEST("", "encode_big_int()", "", ""){
	QUARK_VERIFY(encode_big_int(MAX_SAFE_INTEGER + 1) == (std::pair<std::string, int64_t>("9007199254740992", -1)));
}





//////////////////////////////////////////////////		struct_value_t


bool struct_value_t::check_invariant() const{
	QUARK_ASSERT(_def.check_invariant());

	for(const auto& m: _member_values){
		QUARK_ASSERT(m.check_invariant());
	}
	return true;
}

bool struct_value_t::operator==(const struct_value_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _def == other._def && _member_values == other._member_values;
}

static std::string struct_instance_to_compact_string(const types_t& types, const struct_value_t& v){
	std::vector<std::string> members;
	for(int i = 0 ; i < v._def._members.size() ; i++){
		const auto& def = v._def._members[i];
		const auto& value = v._member_values[i];

		const auto m = /*type_to_compact_string(def._type) + " " +*/ def._name + "=" + to_compact_string_quote_strings(types, value);
		members.push_back(m);
	}
//		return "struct {" + concat_strings_with_divider(members, ", ") + "}";
	return "{" + concat_strings_with_divider(members, ", ") + "}";
}







//////////////////////////////////////////////////		more functions



static std::string vector_instance_to_compact_string(const types_t& types, const std::vector<value_t>& elements0){
	std::vector<std::string> elements;
	for(const auto& e: elements0){
		const auto es = to_compact_string_quote_strings(types, e);
		elements.push_back(es);
	}
//		return "[" + type_to_compact_string(instance._element_type) + "]" + "(" + concat_strings_with_divider(elements, ",") + ")";
	return "[" + concat_strings_with_divider(elements, ", ") + "]";
}


std::string dict_instance_to_compact_string(const types_t& types, const std::map<std::string, value_t>& entries){
	std::vector<std::string> elements;
	for(const auto& e: entries){
		const auto& key_str = quote(e.first);
		const auto& value_str = to_compact_string_quote_strings(types, e.second);
		const auto& es = key_str + ": " + value_str;
		elements.push_back(es);
	}
//		return "[string:" + type_to_compact_string(instance._value_type) + "]" + "{" + concat_strings_with_divider(elements, ",") + "}";
	return "{" + concat_strings_with_divider(elements, ", ") + "}";
}





//////////////////////////////////////////////////		value_ext_t



value_ext_t::value_ext_t(const type_t& s) :
	_rc(1),
	_type(type_desc_t::make_typeid()),
	_typeid_value(s)
{
	QUARK_ASSERT(check_invariant());
}


value_ext_t::value_ext_t(const type_t& type, std::shared_ptr<struct_value_t>& s) :
	_rc(1),
	_type(type),
	_struct(s)
{
	QUARK_ASSERT(check_invariant());
}

value_ext_t::value_ext_t(const type_t& type, const std::vector<value_t>& s) :
	_rc(1),
	_type(type),
	_vector_elements(s)
{
	QUARK_ASSERT(check_invariant());
}

value_ext_t::value_ext_t(const type_t& type, const std::map<std::string, value_t>& s) :
	_rc(1),
	_type(type),
	_dict_entries(s)
{
	QUARK_ASSERT(check_invariant());
}

value_ext_t::value_ext_t(const type_t& type, function_id_t function_id) :
	_rc(1),
	_type(type),
	_function_id(function_id)
{
	QUARK_ASSERT(check_invariant());
}


bool value_ext_t::operator==(const value_ext_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

//			_rc(1),

	const auto base_type = _type.get_base_type();
	if(base_type == base_type::k_string){
		return _string == other._string;
	}
	else if(base_type == base_type::k_json){
		return *_json == *other._json;
	}
	else if(base_type == base_type::k_typeid){
		return _typeid_value == other._typeid_value;
	}
	else if(base_type == base_type::k_struct){
		return *_struct == *other._struct;
	}
	else if(base_type == base_type::k_vector){
		return _vector_elements == other._vector_elements;
	}
	else if(base_type == base_type::k_dict){
		return _dict_entries == other._dict_entries;
	}
	else if(base_type == base_type::k_function){
		return _function_id == other._function_id;
	}
	else {
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


int limit_comparison(int64_t value){
	if(value < 0){
		return -1;
	}
	else if(value > 0){
		return 1;
	}
	else{
		return 0;
	}
}

int compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	const auto result = std::strcmp(left.c_str(), right.c_str());
	return limit_comparison(result);
}

QUARK_TESTQ("compare_string()", ""){
	ut_verify_auto(QUARK_POS, compare_string("", ""), 0);
}
QUARK_TESTQ("compare_string()", ""){
	ut_verify_auto(QUARK_POS, compare_string("aaa", "aaa"), 0);
}
QUARK_TEST("", "compare_string()", "", ""){
	ut_verify_auto(QUARK_POS, compare_string("b", "a"), 1);
}


int compare_struct_true_deep(const struct_value_t& left, const struct_value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	std::vector<value_t>::const_iterator a_it = left._member_values.begin();
	std::vector<value_t>::const_iterator b_it = right._member_values.begin();

	while(a_it !=left._member_values.end()){
		int diff = value_t::compare_value_true_deep(*a_it, *b_it);
		if(diff != 0){
			return diff;
		}

		a_it++;
		b_it++;
	}
	return 0;
}


//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int compare_vector_true_deep(const std::vector<value_t>& left, const std::vector<value_t>& right){
//	QUARK_ASSERT(left.check_invariant());
//	QUARK_ASSERT(right.check_invariant());
//	QUARK_ASSERT(left._element_type == right._element_type);

	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = value_t::compare_value_true_deep(left[i], right[i]);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left.size() == right.size()){
		return 0;
	}
	else if(left.size() > right.size()){
		return -1;
	}
	else{
		return +1;
	}
}

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}


int compare_dict_true_deep(const std::map<std::string, value_t>& left, const std::map<std::string, value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it && *left_it == *right_it){
		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	else if(left_it != left_end_it && right_it != right_end_it){
		int key_diff = compare_string(left_it->first, right_it->first);
		if(key_diff != 0){
			return key_diff;
		}
		else {
			return value_t::compare_value_true_deep(left_it->second, right_it->second);
		}
	}
	else{
		QUARK_ASSERT(false)
		quark::throw_exception();
	}
}


int compare_jsons(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		// ??? implement compare.
		return 1;
	}
}

int value_t::compare_value_true_deep(const value_t& left, const value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left.get_type() == right.get_type());

	if(left.is_undefined()){
		return 0;
	}
	else if(left.is_any()){
		return 0;
	}
	else if(left.is_void()){
		return 0;
	}
	else if(left.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(left.is_int()){
		return limit_comparison(left.get_int_value() - right.get_int_value());
	}
	else if(left.is_double()){
		const auto a = left.get_double_value();
		const auto b = right.get_double_value();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(left.is_string()){
		return compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(left.is_json()){
		return compare_jsons(left.get_json(), right.get_json());
	}
	else if(left.is_typeid()){
	//???
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(left.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		if(left.get_type() != right.get_type()){
			quark::throw_runtime_error("Cannot compare structs of different type.");
		}
		else{
			//	Shortcut: same object == we know values are same without having to check them.
			if(left.get_struct_value() == right.get_struct_value()){
				return 0;
			}
			else{
				return compare_struct_true_deep(*left.get_struct_value(), *right.get_struct_value());
			}
		}
	}
	else if(left.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
		if(left.get_type() != right.get_type()){
			quark::throw_runtime_error("Cannot compare vectors of different type.");
		}
		else{
			const auto& left_vec = left.get_vector_value();
			const auto& right_vec = right.get_vector_value();
			return compare_vector_true_deep(left_vec, right_vec);
		}
	}
	else if(left.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
		if(left.get_type() != right.get_type()){
			quark::throw_runtime_error("Cannot compare dicts of different type.");
		}
		else{
			const auto& left2 = left.get_dict_value();
			const auto& right2 = right.get_dict_value();
			return compare_dict_true_deep(left2, right2);
		}
	}
	else if(left.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


bool value_t::check_invariant() const{
	const auto basetype = _type.get_base_type();
	if(basetype == base_type::k_undefined){
	}
	else if(basetype == base_type::k_any){
	}
	else if(basetype == base_type::k_void){
	}
	else if(basetype == base_type::k_bool){
	}
	else if(basetype == base_type::k_int){
	}
	else if(basetype == base_type::k_double){
	}
	else if(basetype == base_type::k_string){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_json){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_typeid){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_struct){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_vector){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_dict){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else if(basetype == base_type::k_function){
		QUARK_ASSERT(_value_internals._ext && _value_internals._ext->check_invariant());
	}
	else {
		QUARK_ASSERT(false);
	}
	return true;
}


std::string to_compact_string2(const types_t& types, const value_t& value) {
	QUARK_ASSERT(value.check_invariant());

	const auto base_type = value.get_type().get_base_type();
	if(base_type == base_type::k_undefined){
		return base_type_to_opcode(base_type);
	}
	else if(base_type == base_type::k_any){
		return base_type_to_opcode(base_type);
	}
	else if(base_type == base_type::k_void){
		return base_type_to_opcode(base_type);
	}
	else if(base_type == base_type::k_bool){
		return value.get_bool_value() ? "true" : "false";
	}
	else if(base_type == base_type::k_int){
		char temp[200 + 1];//### Use C++ function instead.
		sprintf(temp, "%" PRId64, value.get_int_value());
		return std::string(temp);
	}
	else if(base_type == base_type::k_double){
		return double_to_string_always_decimals(value.get_double_value());
	}
	else if(base_type == base_type::k_string){
		return value.get_string_value();
	}
	else if(base_type == base_type::k_json){
		return json_to_compact_string(value.get_json());
	}
	else if(base_type == base_type::k_typeid){
		return type_to_compact_string(types, value.get_typeid_value());
	}
	else if(base_type == base_type::k_struct){
		return struct_instance_to_compact_string(types, *value.get_struct_value());
	}
	else if(base_type == base_type::k_vector){
		return vector_instance_to_compact_string(types, value.get_vector_value());
	}
	else if(base_type == base_type::k_dict){
		return dict_instance_to_compact_string(types, value.get_dict_value());
	}
	else if(base_type == base_type::k_function){
		//??? include link name.
		return type_to_compact_string(types, value.get_type());
	}

	else{
		return "??";
	}
}

std::string to_compact_string_quote_strings(const types_t& types, const value_t& value) {
	QUARK_ASSERT(types.check_invariant());

	const auto s = to_compact_string2(types, value);
	if(value.is_string()){
		return "\"" + s + "\"";
	}
	else{
		return s;
	}
}

std::string value_and_type_to_string(const types_t& types, const value_t& value) {
	QUARK_ASSERT(types.check_invariant());

	std::string type_string = type_to_compact_string(types, value.get_type());
	if(value.is_undefined()){
		return type_string;
	}
	else if(value.is_any()){
		return type_string;
	}
	else if(value.is_void()){
		return type_string;
	}
	else{
		return type_string + ": " + to_compact_string_quote_strings(types, value);
	}
}




std::string value_t::get_string_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_string()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_string;
}


json_t value_t::get_json() const{
	QUARK_ASSERT(check_invariant());
	if(!is_json()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return *_value_internals._ext->_json;
}


type_t value_t::get_typeid_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_typeid()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_typeid_value;
}


std::shared_ptr<struct_value_t> value_t::get_struct_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_struct()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_struct;
}



const std::vector<value_t>& value_t::get_vector_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_vector()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_vector_elements;
}


const std::map<std::string, value_t>& value_t::get_dict_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_dict()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_dict_entries;
}

function_id_t value_t::get_function_value() const{
	QUARK_ASSERT(check_invariant());
	if(!is_function()){
		quark::throw_runtime_error("Type mismatch!");
	}

	return _value_internals._ext->_function_id;
}


#if 0
value_t::value_t(types_t& types, const type_t& struct_type, std::shared_ptr<struct_value_t>& instance) :
	_basetype(base_type::k_struct)
{
	QUARK_ASSERT(peek2(types, struct_type).is_struct());
	QUARK_ASSERT(instance && instance->check_invariant());

	_value_internals._ext = new value_ext_t{struct_type, instance};
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}
value_t::value_t(const types_t& types, const type_t& struct_type, std::shared_ptr<struct_value_t>& instance) :
	_basetype(base_type::k_struct)
{
	QUARK_ASSERT(peek2(types, struct_type).is_struct());
	QUARK_ASSERT(instance && instance->check_invariant());

	_value_internals._ext = new value_ext_t{struct_type, instance};
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}




value_t::value_t(const types_t& types, const type_t& element_type, const std::vector<value_t>& elements) :
	_basetype(base_type::k_vector)
{
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(element_type.check_invariant());

	_value_internals._ext = new value_ext_t{ make_vector(types, element_type), elements };
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}
value_t::value_t(types_t& types, const type_t& element_type, const std::vector<value_t>& elements) :
	_basetype(base_type::k_vector)
{
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(element_type.check_invariant());

	_value_internals._ext = new value_ext_t{ make_vector(types, element_type), elements };
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}



value_t::value_t(const types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries) :
	_basetype(base_type::k_dict)
{
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(value_type.check_invariant());

	_value_internals._ext = new value_ext_t{make_dict(types, value_type), entries};
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}
value_t::value_t(types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries) :
	_basetype(base_type::k_dict)
{
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(value_type.check_invariant());

	_value_internals._ext = new value_ext_t{make_dict(types, value_type), entries};
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}



value_t::value_t(const type_t& type, function_id_t function_id) :
	_basetype(base_type::k_function)
{
	_value_internals._ext = new value_ext_t{type, function_id};
	QUARK_ASSERT(_value_internals._ext->_rc == 1);

#if DEBUG_DEEP
	DEBUG_STR = make_value_debug_str(*this);
#endif

	QUARK_ASSERT(check_invariant());
}
#endif


//??? swap(), operator=, copy-constructor.

QUARK_TESTQ("value_t::make_undefined()", "undef"){
	types_t types;
	const auto a = value_t::make_undefined();
	QUARK_VERIFY(a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_undefined());
	QUARK_VERIFY(a != value_t::make_string("test"));
	QUARK_VERIFY(to_compact_string2(types, a) == "undef");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "undef");
}


QUARK_TEST("", "value_t::make_any()", "**dynamic**", ""){
	types_t types;
	const auto a = value_t::make_any();
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_any());
	QUARK_VERIFY(a != value_t::make_string("test"));
	QUARK_VERIFY(to_compact_string2(types, a) == "any");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "any");
}


QUARK_TESTQ("value_t::make_void()", "void"){
	types_t types;
	const auto a = value_t::make_void();
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_void());
	QUARK_VERIFY(a != value_t::make_string("test"));
	QUARK_VERIFY(to_compact_string2(types, a) == "void");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "void");
}


QUARK_TESTQ("value_t()", "bool - true"){
	types_t types;
	const auto a = value_t::make_bool(true);
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_bool(true));
	QUARK_VERIFY(a != value_t::make_bool(false));
	QUARK_VERIFY(to_compact_string2(types, a) == "true");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "bool: true");
}

QUARK_TESTQ("value_t()", "bool - false"){
	types_t types;
	const auto a = value_t::make_bool(false);
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_bool(false));
	QUARK_VERIFY(a != value_t::make_bool(true));
	QUARK_VERIFY(to_compact_string2(types, a) == "false");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "bool: false");
}

//??? test full range of int64
QUARK_TESTQ("value_t()", "int"){
	types_t types;
	const auto a = value_t::make_int(13);
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_int(13));
	QUARK_VERIFY(a != value_t::make_int(14));
	QUARK_VERIFY(to_compact_string2(types, a) == "13");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "int: 13");
}

QUARK_TESTQ("value_t()", "double"){
	types_t types;
	const auto a = value_t::make_double(13.5f);
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(a.is_double());
	QUARK_VERIFY(!a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_double(13.5f));
	QUARK_VERIFY(a != value_t::make_double(14.0f));
	QUARK_VERIFY(to_compact_string2(types, a) == "13.5");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "double: 13.5");
}

QUARK_TESTQ("value_t()", "string"){
	types_t types;
	const auto a = value_t::make_string("xyz");
	QUARK_VERIFY(!a.is_undefined());
	QUARK_VERIFY(!a.is_any());
	QUARK_VERIFY(!a.is_void());
	QUARK_VERIFY(!a.is_bool());
	QUARK_VERIFY(!a.is_int());
	QUARK_VERIFY(!a.is_double());
	QUARK_VERIFY(a.is_string());
	QUARK_VERIFY(!a.is_struct());
	QUARK_VERIFY(!a.is_vector());
	QUARK_VERIFY(!a.is_dict());
	QUARK_VERIFY(!a.is_function());

	QUARK_VERIFY(a == value_t::make_string("xyz"));
	QUARK_VERIFY(a != value_t::make_string("xyza"));
	QUARK_VERIFY(to_compact_string2(types, a) == "xyz");
	QUARK_VERIFY(value_and_type_to_string(types, a) == "string: \"xyz\"");
}


json_t value_and_type_to_ast_json(const types_t& types, const value_t& v){
	return
		json_t::make_array({
			type_to_json(types, v.get_type()),
			value_to_ast_json(types, v)
		});
}

value_t ast_json_to_value_and_type(types_t& types, const json_t& v){
	const auto type0 = v.get_array_n(0);
	const auto value0 = v.get_array_n(1);

	const auto type1 = type_from_json(types, type0);
	const auto value1 = ast_json_to_value(types, type1, value0);
	return value1;
}

#if DEBUG
std::string make_value_debug_str(const value_t& value){
	types_t types;
	return value_and_type_to_string(types, value);

//	std::string type_string = type_to_compact_string(value.get_type());
//	return type_string;
}
#endif


value_t ast_json_to_value(types_t& types, const type_t& type, const json_t& v){
	const auto type_peek = peek2(types, type);

	if(type_peek.is_undefined()){
		return value_t();
	}
	else if(type_peek.is_any()){
		return make_def(type);
	}
	else if(type_peek.is_void()){
		return make_def(type);
	}
	else if(type_peek.is_bool()){
		if(v.is_true() || v.is_false()){
			return value_t::make_bool(v.is_true() ? true : false);
		}
		else{
			throw std::exception();
		}
	}
	else if(type_peek.is_int()){
		if(v.is_object()){
			const auto tag = v.get_object_element("big-int");
			const auto value = v.get_object_element("value").get_string();
  			const auto i = decode_big_int(value);
  			return value_t::make_int(i);
		}
		else if(v.is_number()){
			return value_t::make_int(static_cast<int64_t>(v.get_number()));
		}
		else{
			quark::throw_exception();
		}
	}
	else if(type_peek.is_double()){
		return value_t::make_double(v.get_number());
	}
	else if(type_peek.is_string()){
		return value_t::make_string(v.get_string());
	}
	else if(type_peek.is_json()){
		return value_t::make_json(v);
	}
	else if(type_peek.is_typeid()){
		const auto t = type_from_json(types, v);
		return value_t::make_typeid_value(t);
	}
	else if(type_peek.is_struct()){
		QUARK_ASSERT(false);
		return make_def(type);

/*
		const auto& members = v.get_array();
		if(members.get_array_count != )
		const auto& struct_value = v.get_struct_value();
		std::map<string, json_t> obj2;
		for(int i = 0 ; i < struct_value->_def->_members.size() ; i++){
			const auto& member = struct_value->_def->_members[i];
			const auto& key = member._name;
			const auto& value = struct_value->_member_values[i];
			const auto& value2 = value_to_ast_json(value, tags);
			obj2[key] = value2;
		}
		return json_t::make_object(obj2);
*/

	}
	else if(type_peek.is_vector()){
		QUARK_ASSERT(false);
		return make_def(type);
//		const auto& vec = v.get_vector_value();
//		return values_to_json_array(vec);
	}
	else if(type_peek.is_dict()){
		QUARK_ASSERT(false);
		return make_def(type);
/*
		const auto entries = v.get_dict_value();
		std::map<string, json_t> result;
		for(const auto& e: entries){
			result[e.first] = value_to_ast_json(e.second, tags);
		}
		return result;
*/

	}
	else if(type_peek.is_function()){
		const auto function_id0 = v.get_object_element("function_id").get_string();
		const auto function_id = function_id_t { function_id0 };
		return value_t::make_function_value(type, function_id);
	}
	else{
		quark::throw_exception();
	}
}




json_t value_to_ast_json(const types_t& types, const value_t& v){
	if(v.is_undefined()){
		return json_t();
	}
	else if(v.is_any()){
		return json_t();
	}
	else if(v.is_void()){
		return json_t();
	}
	else if(v.is_bool()){
		return json_t(v.get_bool_value());
	}
	else if(v.is_int()){
		const auto i = v.get_int_value();
		const std::pair<std::string, int64_t> big_int = encode_big_int(i);
		if(big_int.first.empty()){
			return json_t(static_cast<double>(big_int.second));
		}
		else{
			std::map<std::string, json_t> result = { { "big-int", json_t(64) }, { "value", big_int.first } };
			return result;
		}
	}
	else if(v.is_double()){
		return json_t(static_cast<double>(v.get_double_value()));
	}
	else if(v.is_string()){
		return json_t(v.get_string_value());
	}
	else if(v.is_json()){
		return v.get_json();
	}
	else if(v.is_typeid()){
		return type_to_json(types, v.get_typeid_value());
	}
	else if(v.is_struct()){
		const auto& struct_value = v.get_struct_value();
		std::map<std::string, json_t> obj2;
		for(int i = 0 ; i < struct_value->_def._members.size() ; i++){
			const auto& member = struct_value->_def._members[i];
			const auto& key = member._name;
			const auto& value = struct_value->_member_values[i];
			const auto& value2 = value_to_ast_json(types, value);
			obj2[key] = value2;
		}
		return json_t::make_object(obj2);
	}
	else if(v.is_vector()){
		const auto& vec = v.get_vector_value();
		return values_to_json_array(types, vec);
	}
	else if(v.is_dict()){
		const auto entries = v.get_dict_value();
		std::map<std::string, json_t> result;
		for(const auto& e: entries){
			result[e.first] = value_to_ast_json(types, e.second);
		}
		return result;
	}
	else if(v.is_function()){
		return json_t::make_object(
			{
				{ "function_id", v.get_function_value().name }
			}
		);
	}
	else{
		quark::throw_exception();
	}
}

QUARK_TESTQ("value_to_ast_json()", ""){
	types_t types;
	ut_verify_json(QUARK_POS, value_to_ast_json(types, value_t::make_string("hello")), json_t("hello"));
}

QUARK_TESTQ("value_to_ast_json()", ""){
	types_t types;
	ut_verify_json(QUARK_POS, value_to_ast_json(types, value_t::make_int(123)), json_t(123.0));
}

QUARK_TESTQ("value_to_ast_json()", ""){
	types_t types;
	ut_verify_json(QUARK_POS, value_to_ast_json(types, value_t::make_bool(true)), json_t(true));
}

QUARK_TESTQ("value_to_ast_json()", ""){
	types_t types;
	ut_verify_json(QUARK_POS, value_to_ast_json(types, value_t::make_bool(false)), json_t(false));
}

QUARK_TESTQ("value_to_ast_json()", ""){
	types_t types;
	ut_verify_json(QUARK_POS, value_to_ast_json(types, value_t::make_undefined()), json_t());
}


//	Used internally in check_invariant() -- don't call check_invariant().
type_t value_t::get_type() const{
//			QUARK_ASSERT(check_invariant());

	const auto basetype = _type.get_base_type();
	if(basetype == base_type::k_undefined){
		return floyd::make_undefined();
	}
	else if(basetype == base_type::k_any){
		return type_t::make_any();
	}
	else if(basetype == base_type::k_void){
		return type_t::make_void();
	}
	else if(basetype == base_type::k_bool){
		return type_t::make_bool();
	}
	else if(basetype == base_type::k_int){
		return type_t::make_int();
	}
	else if(basetype == base_type::k_double){
		return type_t::make_double();
	}
	else{
		QUARK_ASSERT(_value_internals._ext);
		return _value_internals._ext->_type;
	}
}



/*
value_t value_t::make_json(const json_t& v){
	auto f = std::make_shared<json_t>(v);
	return value_t(f);
}
*/


value_t value_t::make_struct_value(const types_t& types, const type_t& struct_type, const std::vector<value_t>& values){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(struct_type.check_invariant());
	QUARK_ASSERT(peek2(types, struct_type).get_struct(types)._members.size() == values.size());

	auto instance = std::make_shared<struct_value_t>(struct_value_t{ peek2(types, struct_type).get_struct(types), values });
	auto ext = new value_ext_t{ struct_type, instance };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, struct_type);



}
value_t value_t::make_struct_value(types_t& types, const type_t& struct_type, const std::vector<value_t>& values){
	QUARK_ASSERT(types.check_invariant());
	QUARK_ASSERT(struct_type.check_invariant());
	QUARK_ASSERT(peek2(types, struct_type).get_struct(types)._members.size() == values.size());

	auto instance = std::make_shared<struct_value_t>(struct_value_t{ peek2(types, struct_type).get_struct(types), values });
	auto ext = new value_ext_t{ struct_type, instance };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, struct_type);
}

value_t value_t::make_vector_value(const types_t& types, const type_t& element_type, const std::vector<value_t>& elements){
	const auto type = make_vector(types, element_type);
	auto ext = new value_ext_t{ type, elements };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, type);


}
value_t value_t::make_vector_value(types_t& types, const type_t& element_type, const std::vector<value_t>& elements){
	const auto type = make_vector(types, element_type);
	auto ext = new value_ext_t{ type, elements };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, type);
}

value_t value_t::make_dict_value(const types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries){
	const auto type = make_dict(types, value_type);
	auto ext = new value_ext_t{ type, entries };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, type);
}
value_t value_t::make_dict_value(types_t& types, const type_t& value_type, const std::map<std::string, value_t>& entries){
	const auto type = make_dict(types, value_type);
	auto ext = new value_ext_t{ type, entries };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, type);
}

value_t value_t::make_function_value(const type_t& type, const function_id_t& function_id){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(type.is_function());

	auto ext = new value_ext_t{ type, function_id };
	QUARK_ASSERT(ext->_rc == 1);
	return value_t(value_internals_t { ._ext = ext }, type);
}


json_t values_to_json_array(const types_t& types, const std::vector<value_t>& values){
	std::vector<json_t> r;
	for(const auto& i: values){
		const auto& j = value_to_ast_json(types, i);
		r.push_back(j);
	}
	return json_t::make_array(r);
}

value_t make_def(const type_t& type){
	const base_type bt = type.get_base_type();
	if(false){
	}
	else if(bt == base_type::k_void){
		return value_t::make_void();
	}
	else if(bt == base_type::k_bool){
		return value_t::make_bool(false);
	}
	else if(bt == base_type::k_int){
		return value_t::make_int(0);
	}
	else if(bt == base_type::k_double){
		return value_t::make_double(0.0f);
	}
	else if(bt == base_type::k_string){
		return value_t::make_string("");
	}
	else if(bt == base_type::k_json){
		return value_t::make_json(json_t());
	}
	else if(bt == base_type::k_typeid){
		return value_t::make_typeid_value(type_t::make_void());
	}
	else if(bt == base_type::k_struct){
//		return value_t::make_struct_value(typid_t::make_struct(), {});
	}
	else if(bt == base_type::k_struct){
	}
	else if(bt == base_type::k_function){
		return value_t::make_function_value(type, function_id_t {});
	}
	else if(bt == base_type::k_undefined){
		return value_t::make_undefined();
	}
	else if(bt == base_type::k_any){
		return value_t::make_any();
	}
	else{
	}

	QUARK_ASSERT(false);
	quark::throw_exception();
}


void ut_verify_values(const quark::call_context_t& context, const value_t& result, const value_t& expected){
	types_t types;
	ut_verify_json(
		context,
		value_and_type_to_ast_json(types, result),
		value_and_type_to_ast_json(types, expected)
	);
}


}	//	floyd
