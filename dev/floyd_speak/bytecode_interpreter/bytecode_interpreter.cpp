//
//  parser_evaluator.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_interpreter.h"

#include "host_functions.h"
#include "text_parser.h"
#include "ast_value.h"
#include "ast_json.h"
#include <sys/time.h>
#include <algorithm>


namespace floyd {


void release_pod_external(bc_pod_value_t& value){
	QUARK_ASSERT(value._external != nullptr);

	value._external->_rc--;
	if(value._external->_rc == 0){
		delete value._external;
		value._external = nullptr;
	}
}

////////////////////////////////////////////			bc_value_t



bc_value_t::bc_value_t() :
	_type(typeid_t::make_undefined())
{
	_pod._external = nullptr;
	QUARK_ASSERT(check_invariant());
}

bc_value_t::~bc_value_t(){
	QUARK_ASSERT(check_invariant());

	if(encode_as_external(_type)){
		release_pod_external(_pod);
	}
}

bc_value_t::bc_value_t(const bc_value_t& other) :
	_type(other._type),
	_pod(other._pod)
{
	QUARK_ASSERT(other.check_invariant());

	if(encode_as_external(_type)){
		_pod._external->_rc++;
	}

	QUARK_ASSERT(check_invariant());
}

bc_value_t& bc_value_t::operator=(const bc_value_t& other){
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());

	bc_value_t temp(other);
	temp.swap(*this);

	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
	return *this;
}

void bc_value_t::swap(bc_value_t& other){
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());

	std::swap(_type, other._type);
	std::swap(_pod, other._pod);

	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}

bc_value_t::bc_value_t(const bc_static_frame_t* frame_ptr) :
	_type(typeid_t::make_void())
{
	_pod._inplace._frame_ptr = frame_ptr;
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		internal-undefined type


bc_value_t bc_value_t::make_undefined(){
	return bc_value_t();
}


//////////////////////////////////////		internal-dynamic type


bc_value_t bc_value_t::make_internal_dynamic(){
	return bc_value_t();
}

//////////////////////////////////////		void


bc_value_t bc_value_t::make_void(){
	return bc_value_t();
}


//////////////////////////////////////		bool


bc_value_t bc_value_t::make_bool(bool v){
	return bc_value_t(v);
}
bool bc_value_t::get_bool_value() const {
	QUARK_ASSERT(check_invariant());

	return _pod._inplace._bool;
}
bc_value_t::bc_value_t(bool value) :
	_type(typeid_t::make_bool())
{
	_pod._inplace._bool = value;
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		int


bc_value_t bc_value_t::make_int(int64_t v){
	return bc_value_t{ v };
}
int64_t bc_value_t::get_int_value() const {
	QUARK_ASSERT(check_invariant());

	return _pod._inplace._int64;
}
bc_value_t::bc_value_t(int64_t value) :
	_type(typeid_t::make_int())
{
	_pod._inplace._int64 = value;
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		double


bc_value_t bc_value_t::make_double(double v){
	return bc_value_t{ v };
}
double bc_value_t::get_double_value() const {
	QUARK_ASSERT(check_invariant());

	return _pod._inplace._double;
}
bc_value_t::bc_value_t(double value) :
	_type(typeid_t::make_double())
{
	_pod._inplace._double = value;
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		string


bc_value_t bc_value_t::make_string(const std::string& v){
	return bc_value_t{ v };
}
std::string bc_value_t::get_string_value() const{
	QUARK_ASSERT(check_invariant());

	return _pod._external->_string;
}
bc_value_t::bc_value_t(const std::string& value) :
	_type(typeid_t::make_string())
{
	_pod._external = new bc_external_value_t{value};
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		json_value


bc_value_t bc_value_t::make_json_value(const json_t& v){
	return bc_value_t{ std::make_shared<json_t>(v) };
}
json_t bc_value_t::get_json_value() const{
	QUARK_ASSERT(check_invariant());

	return *_pod._external->_json_value.get();
}
bc_value_t::bc_value_t(const std::shared_ptr<json_t>& value) :
	_type(typeid_t::make_json_value())
{
	QUARK_ASSERT(value);
	QUARK_ASSERT(value->check_invariant());

	_pod._external = new bc_external_value_t{value};

	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		typeid


bc_value_t bc_value_t::make_typeid_value(const typeid_t& type_id){
	return bc_value_t{ type_id };
}
typeid_t bc_value_t::get_typeid_value() const {
	QUARK_ASSERT(check_invariant());

	return _pod._external->_typeid_value;
}
bc_value_t::bc_value_t(const typeid_t& type_id) :
	_type(typeid_t::make_typeid())
{
	QUARK_ASSERT(type_id.check_invariant());

	_pod._external = new bc_external_value_t{type_id};

	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		struct



bc_value_t bc_value_t::make_struct_value(const typeid_t& struct_type, const std::vector<bc_value_t>& values){
	return bc_value_t{ struct_type, values, true };
}
const std::vector<bc_value_t>& bc_value_t::get_struct_value() const {
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(_type.is_struct());

	return _pod._external->_struct_members;
}
bc_value_t::bc_value_t(const typeid_t& struct_type, const std::vector<bc_value_t>& values, bool struct_tag) :
	_type(struct_type)
{
	QUARK_ASSERT(struct_type.check_invariant());
#if QUARK_ASSERT_ON
	for(const auto& e: values) {
		QUARK_ASSERT(e.check_invariant());
	}
#endif

	_pod._external = new bc_external_value_t{ struct_type, values, true };
	QUARK_ASSERT(check_invariant());
}


//////////////////////////////////////		function


bc_value_t bc_value_t::make_function_value(const typeid_t& function_type, int function_id){
	return bc_value_t{ function_type, function_id, true };
}
int bc_value_t::get_function_value() const{
	QUARK_ASSERT(check_invariant());

	return _pod._inplace._function_id;
}
bc_value_t::bc_value_t(const typeid_t& function_type, int function_id, bool dummy) :
	_type(function_type)
{
	_pod._inplace._function_id = function_id;
	QUARK_ASSERT(check_invariant());
}




bc_value_t::bc_value_t(const typeid_t& type, const bc_pod_value_t& internals) :
	_type(type),
	_pod(internals)
{
	QUARK_ASSERT(type.check_invariant());
#if QUARK_ASSERT_ON
	if(encode_as_external(type)){
		QUARK_ASSERT(check_external_deep(type, internals._external));
	}
#endif

	if(encode_as_external(_type)){
		_pod._external->_rc++;
	}
	QUARK_ASSERT(check_invariant());
}

bc_value_t::bc_value_t(const typeid_t& type, const bc_inplace_value_t& pod64) :
	_type(type),
	_pod{._inplace = pod64}
{
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(encode_as_external(_type) == false);

	QUARK_ASSERT(check_invariant());
}

bc_value_t::bc_value_t(const typeid_t& type, const bc_external_handle_t& handle) :
	_type(type),
	_pod{._external = handle._external}
{
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(handle.check_invariant());

	_pod._external->_rc++;

	QUARK_ASSERT(check_invariant());
}



#if DEBUG
bool bc_value_t::check_invariant() const {
	QUARK_ASSERT(_type.check_invariant());
	if(encode_as_external(_type)){
		QUARK_ASSERT(check_external_deep(_type, _pod._external))
	}

	return true;
}
#endif

bc_value_t::bc_value_t(const typeid_t& type, mode mode) :
	_type(type)
{
	QUARK_ASSERT(type.check_invariant());

	//	Allocate a dummy external value.
	auto temp = new bc_external_value_t{"UNWRITTEN EXT VALUE"};
#if DEBUG
	temp->_debug__is_unwritten_external_value = true;
#endif
	_pod._external = temp;

	QUARK_ASSERT(check_invariant());
}





////////////////////////////////////////////			bc_external_handle_t



bc_external_handle_t::bc_external_handle_t(const bc_external_handle_t& other) :
	_external(other._external)
{
	QUARK_ASSERT(other.check_invariant());

	_external->_rc++;

	QUARK_ASSERT(check_invariant());
}

bc_external_handle_t::bc_external_handle_t(const bc_external_value_t* ext) :
	_external(ext)
{
	QUARK_ASSERT(ext != nullptr);

	_external->_rc++;

	QUARK_ASSERT(check_invariant());
}

bc_external_handle_t::bc_external_handle_t(const bc_value_t& value) :
	_external(value._pod._external)
{
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(encode_as_external(value._type));

	_external->_rc++;

	QUARK_ASSERT(check_invariant());
}

void bc_external_handle_t::swap(bc_external_handle_t& other){
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	std::swap(_external, other._external);

	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());
}

bc_external_handle_t& bc_external_handle_t::operator=(const bc_external_handle_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}

bc_external_handle_t::~bc_external_handle_t(){
	QUARK_ASSERT(check_invariant());

	_external->_rc--;
	if(_external->_rc == 0){
		delete _external;
		_external = nullptr;
	}
}

bool bc_external_handle_t::check_invariant() const {
	QUARK_ASSERT(_external != nullptr);
	QUARK_ASSERT(_external->check_invariant());
	return true;
}


bool encode_as_inplace(const typeid_t& type){
	return type.is_bool() || type.is_int() || type.is_double();
}
bool encode_as_vector_w_inplace_elements(const typeid_t& type){
	return type.is_vector() && encode_as_inplace(type.get_vector_element_type());
}

bool encode_as_dict_w_inplace_values(const typeid_t& type){
	return type.is_dict() && encode_as_inplace(type.get_dict_value_type());
}

value_encoding type_to_encoding(const typeid_t& type){
	const auto basetype = type.get_base_type();
	if(false){
	}
	else if(basetype == base_type::k_internal_undefined){
		return value_encoding::k_none;
	}
	else if(basetype == base_type::k_internal_dynamic){
		return value_encoding::k_none;
	}
	else if(basetype == base_type::k_void){
		return value_encoding::k_none;
	}
	else if(basetype == base_type::k_bool){
		return value_encoding::k_inplace__bool;
	}
	else if(basetype == base_type::k_int){
		return value_encoding::k_inplace__int_as_uint64;
	}
	else if(basetype == base_type::k_double){
		return value_encoding::k_inplace__double;
	}
	else if(basetype == base_type::k_string){
		return value_encoding::k_external__string;
	}
	else if(basetype == base_type::k_json_value){
		return value_encoding::k_external__json_value;
	}
	else if(basetype == base_type::k_typeid){
		return value_encoding::k_external__typeid;
	}
	else if(basetype == base_type::k_struct){
		return value_encoding::k_external__struct;
	}
	else if(basetype == base_type::k_protocol){
		return value_encoding::k_external__protocol;
	}
	else if(basetype == base_type::k_vector){
		const auto& element_type = type.get_vector_element_type().get_base_type();
		if(element_type == base_type::k_bool){
			return value_encoding::k_external__vector_pod64;
		}
		else if(element_type == base_type::k_int){
			return value_encoding::k_external__vector_pod64;
		}
		else if(element_type == base_type::k_double){
			return value_encoding::k_external__vector_pod64;
		}
		else{
			return value_encoding::k_external__vector;
		}
	}
	else if(basetype == base_type::k_dict){
		return value_encoding::k_external__dict;
	}
	else if(basetype == base_type::k_function){
		return value_encoding::k_inplace__function;
	}
	else if(basetype == base_type::k_internal_unresolved_type_identifier){
	}
	else{
	}
	QUARK_ASSERT(false);
	quark::throw_exception();
}

bool encode_as_external(value_encoding encoding){
	return false
		|| encoding == value_encoding::k_external__string
		|| encoding == value_encoding::k_external__json_value
		|| encoding == value_encoding::k_external__typeid
		|| encoding == value_encoding::k_external__struct
		|| encoding == value_encoding::k_external__vector
		|| encoding == value_encoding::k_external__vector_pod64
		|| encoding == value_encoding::k_external__dict
		;
}

bool encode_as_external(const typeid_t& type){
	const auto basetype = type.get_base_type();
	return false
		|| basetype == base_type::k_string
		|| basetype == base_type::k_json_value
		|| basetype == base_type::k_typeid
		|| basetype == base_type::k_struct
		|| basetype == base_type::k_protocol
		|| basetype == base_type::k_vector
		|| basetype == base_type::k_dict
		;
}


#if DEBUG
bool bc_external_value_t::check_invariant() const{
	QUARK_ASSERT(encode_as_external(_debug_type));
	QUARK_ASSERT(_rc > 0);
	QUARK_ASSERT(_debug_type.check_invariant());
	QUARK_ASSERT(_typeid_value.check_invariant());

	QUARK_ASSERT(check_external_deep(_debug_type, this));

	const auto encoding = type_to_encoding(_debug_type);
	if(encoding == value_encoding::k_external__string){
//				QUARK_ASSERT(_string);
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);
	}
	else if(encoding == value_encoding::k_external__json_value){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value != nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);

		QUARK_ASSERT(_json_value->check_invariant());
	}
	else if(encoding == value_encoding::k_external__typeid){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value == nullptr);
//		QUARK_ASSERT(_typeid_value != typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);

		QUARK_ASSERT(_typeid_value.check_invariant());
	}
	else if(encoding == value_encoding::k_external__struct){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
//				QUARK_ASSERT(_struct != nullptr);
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);

//				QUARK_ASSERT(_struct && _struct->check_invariant());
	}
	else if(encoding == value_encoding::k_external__vector){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
//		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);
	}
	else if(encoding == value_encoding::k_external__vector_pod64){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_dict_w_external_values.size() == 0);
		QUARK_ASSERT(_dict_w_inplace_values.size() == 0);
	}
	else if(encoding == value_encoding::k_external__dict){
		QUARK_ASSERT(_string.empty());
		QUARK_ASSERT(_json_value == nullptr);
		QUARK_ASSERT(_typeid_value == typeid_t::make_undefined());
		QUARK_ASSERT(_struct_members.empty());
		QUARK_ASSERT(_vector_w_external_elements.empty());
		QUARK_ASSERT(_vector_w_inplace_elements.empty());
//				QUARK_ASSERT(_dict_w_external_values.size() == 0);
//				QUARK_ASSERT(_dict_w_inplace_values.size() == 0);
	}
	else {
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return true;
}
#endif

bc_external_value_t::bc_external_value_t(const std::string& s) :
	_rc(1),
#if DEBUG
	_debug_type(typeid_t::make_string()),
#endif
	_string(s)
{
	QUARK_ASSERT(check_invariant());
}

bc_external_value_t::bc_external_value_t(const std::shared_ptr<json_t>& s) :
	_rc(1),
#if DEBUG
	_debug_type(typeid_t::make_json_value()),
#endif
	_json_value(s)
{
	QUARK_ASSERT(s->check_invariant());
	QUARK_ASSERT(check_invariant());
}

bc_external_value_t::bc_external_value_t(const typeid_t& s) :
	_rc(1),
#if DEBUG
	_debug_type(typeid_t::make_typeid()),
#endif
	_typeid_value(s)
{
	QUARK_ASSERT(s.check_invariant());
	QUARK_ASSERT(check_invariant());
}






bc_external_value_t::bc_external_value_t(const typeid_t& type, const std::vector<bc_value_t>& s, bool struct_tag) :
		_rc(1),
#if DEBUG
	_debug_type(type),
#endif
	_struct_members(s)
{
	QUARK_ASSERT(type.check_invariant());
	#if QUARK_ASSERT_ON
		for(const auto& e: s){
			QUARK_ASSERT(e.check_invariant());
		}
	#endif
	QUARK_ASSERT(check_invariant());
}
bc_external_value_t::bc_external_value_t(const typeid_t& type, const immer::vector<bc_external_handle_t>& s) :
	_rc(1),
#if DEBUG
	_debug_type(type),
#endif
	_vector_w_external_elements(s)
{
	QUARK_ASSERT(type.check_invariant());
	#if QUARK_ASSERT_ON
		for(const auto& e: s){
			QUARK_ASSERT(e.check_invariant());
		}
	#endif
	QUARK_ASSERT(check_invariant());
}
bc_external_value_t::bc_external_value_t(const typeid_t& type, const immer::vector<bc_inplace_value_t>& s) :
	_rc(1),
#if DEBUG
	_debug_type(type),
#endif
	_vector_w_inplace_elements(s)
{
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(check_invariant());
}
bc_external_value_t::bc_external_value_t(const typeid_t& type, const immer::map<std::string, bc_external_handle_t>& s) :
	_rc(1),
#if DEBUG
	_debug_type(type),
#endif
	_dict_w_external_values(s)
{
	QUARK_ASSERT(type.check_invariant());
	#if QUARK_ASSERT_ON
		for(const auto& e: s){
			QUARK_ASSERT(e.first.size() > 0);
			QUARK_ASSERT(e.second.check_invariant());
		}
	#endif
	QUARK_ASSERT(check_invariant());
}
bc_external_value_t::bc_external_value_t(const typeid_t& type, const immer::map<std::string, bc_inplace_value_t>& s) :
	_rc(1),
#if DEBUG
	_debug_type(type),
#endif
	_dict_w_inplace_values(s)
{
	QUARK_ASSERT(type.check_invariant());
	#if QUARK_ASSERT_ON
		for(const auto& e: s){
			QUARK_ASSERT(e.first.size() > 0);
		}
	#endif
	QUARK_ASSERT(check_invariant());
}




bool check_external_deep(const typeid_t& type, const bc_external_value_t* ext){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(encode_as_external(type));
	QUARK_ASSERT(ext != nullptr);
	QUARK_ASSERT(ext->_rc > 0);

	const auto basetype = type.get_base_type();

	if(basetype == base_type::k_struct){
		for(const auto& e: ext->_struct_members){
			QUARK_ASSERT(e.check_invariant());
		}
	}
	else if(basetype == base_type::k_protocol){
		QUARK_ASSERT(false);
		return false;
	}
	else if(basetype == base_type::k_vector){
		const auto& element_type  = type.get_vector_element_type();
		if(encode_as_external(element_type)){
			for(const auto& e: ext->_vector_w_external_elements){
				QUARK_ASSERT(e.check_invariant());
			}
			return true;
		}
		else{
			return true;
		}
	}
	else if(basetype == base_type::k_dict){
		const auto& element_type  = type.get_dict_value_type();
		if(encode_as_external(element_type)){
			for(const auto& e: ext->_vector_w_external_elements){
				QUARK_ASSERT(e.check_invariant());
			}
			return true;
		}
		else{
			return true;
		}
	}
	else if(basetype == base_type::k_function){
		QUARK_ASSERT(false);
	}
	else if(basetype == base_type::k_json_value){
	}
	else if(basetype == base_type::k_typeid){
	}
	else if(basetype == base_type::k_string){
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
	return true;
}







const immer::vector<bc_value_t> get_vector(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value._type.is_vector());

	const auto element_type = value._type.get_vector_element_type();

	if(encode_as_vector_w_inplace_elements(value._type)){
		immer::vector<bc_value_t> result;
		for(const auto& e: value._pod._external->_vector_w_inplace_elements){
			bc_value_t temp(element_type, e);
			result = result.push_back(temp);
		}
		return result;
	}
	else{
		immer::vector<bc_value_t> result;
		for(const auto& e: value._pod._external->_vector_w_external_elements){
			bc_value_t temp(element_type, e);
			result = result.push_back(temp);
		}
		return result;
	}
}



const immer::vector<bc_external_handle_t>* get_vector_external_elements(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value._type.is_vector());
	QUARK_ASSERT(encode_as_vector_w_inplace_elements(value._type) == false);

	return &value._pod._external->_vector_w_external_elements;
}

const immer::vector<bc_inplace_value_t>* get_vector_inplace_elements(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(value._type.is_vector());
	QUARK_ASSERT(encode_as_vector_w_inplace_elements(value._type) == true);

	return &value._pod._external->_vector_w_inplace_elements;
}

bc_value_t make_vector(const typeid_t& element_type, const immer::vector<bc_value_t>& elements){
	QUARK_ASSERT(element_type.check_invariant());
#if QUARK_ASSERT_ON
	for(const auto& e: elements) {
		QUARK_ASSERT(e.check_invariant());
	}
#endif

	const auto vector_type = typeid_t::make_vector(element_type);
	if(encode_as_vector_w_inplace_elements(vector_type)){
		immer::vector<bc_inplace_value_t> elements2;
		for(const auto& e: elements){
			elements2 = elements2.push_back(e._pod._inplace);
		}

		bc_value_t temp;
		temp._type = vector_type;
		temp._pod._external = new bc_external_value_t{vector_type, elements2};
		QUARK_ASSERT(temp.check_invariant());
		return temp;
	}
	else{
		immer::vector<bc_external_handle_t> elements2;
		for(const auto& e: elements){
			elements2 = elements2.push_back(bc_external_handle_t(e));
		}

		bc_value_t temp;
		temp._type = vector_type;
		temp._pod._external = new bc_external_value_t{vector_type, elements2};
		QUARK_ASSERT(temp.check_invariant());
		return temp;
	}
}

bc_value_t make_vector(const typeid_t& element_type, const immer::vector<bc_external_handle_t>& elements){
	QUARK_ASSERT(element_type.check_invariant());
#if QUARK_ASSERT_ON
	for(const auto& e: elements) {
		QUARK_ASSERT(e.check_invariant());
	}
#endif

	const auto vector_type = typeid_t::make_vector(element_type);
	QUARK_ASSERT(encode_as_vector_w_inplace_elements(vector_type) == false);

	bc_value_t temp;
	temp._type = vector_type;
	temp._pod._external = new bc_external_value_t{vector_type, elements};
	QUARK_ASSERT(temp.check_invariant());
	return temp;
}

bc_value_t make_vector(const typeid_t& element_type, const immer::vector<bc_inplace_value_t>& elements){
	QUARK_ASSERT(element_type.check_invariant());

	const auto vector_type = typeid_t::make_vector(element_type);
	QUARK_ASSERT(encode_as_vector_w_inplace_elements(vector_type) == true);

	bc_value_t temp;
	temp._type = vector_type;
	temp._pod._external = new bc_external_value_t{vector_type, elements};
	QUARK_ASSERT(temp.check_invariant());
	return temp;
}





const immer::map<std::string, bc_external_handle_t>& get_dict_value(const bc_value_t& value){
	QUARK_ASSERT(value.check_invariant());

	return value._pod._external->_dict_w_external_values;
}

bc_value_t make_dict(const typeid_t& value_type, const immer::map<std::string, bc_external_handle_t>& entries){
	QUARK_ASSERT(value_type.check_invariant());
#if QUARK_ASSERT_ON
	for(const auto& e: entries) {
		QUARK_ASSERT(e.first.size() > 0);
		QUARK_ASSERT(e.second.check_invariant());
	}
#endif

	bc_value_t temp;
	temp._type = typeid_t::make_dict(value_type);
	temp._pod._external = new bc_external_value_t{typeid_t::make_dict(value_type), entries};
	QUARK_ASSERT(temp.check_invariant());
	return temp;
}

bc_value_t make_dict(const typeid_t& value_type, const immer::map<std::string, bc_inplace_value_t>& entries){
	QUARK_ASSERT(value_type.check_invariant());

	bc_value_t temp;
	temp._type = typeid_t::make_dict(value_type);
	temp._pod._external = new bc_external_value_t{typeid_t::make_dict(value_type), entries};
	QUARK_ASSERT(temp.check_invariant());
	return temp;
}





const typeid_t& lookup_full_type(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type];
}
const base_type lookup_full_basetype(const interpreter_t& vm, const bc_typeid_t& type){
	return vm._imm->_program._types[type].get_base_type();
}

int get_global_n_pos(int n){
	return k_frame_overhead + n;
}
int get_local_n_pos(int frame_pos, int n){
	return frame_pos + n;
}


//	Check memory layouts.
QUARK_UNIT_TEST("", "", "", ""){
	const auto int_size = sizeof(int);
	QUARK_ASSERT(int_size == 4);

	const auto pod_size = sizeof(bc_pod_value_t);
	QUARK_ASSERT(pod_size == 8);

	const auto value_object_size = sizeof(bc_external_value_t);
	QUARK_ASSERT(value_object_size >= 8);

	const auto bcvalue_size = sizeof(bc_value_t);
	QUARK_ASSERT(bcvalue_size == 56);

	struct mockup_value_t {
		private: bool _is_ext;
		public: bc_pod_value_t _pod;
	};

	const auto mockup_value_size = sizeof(mockup_value_t);
	QUARK_ASSERT(mockup_value_size == 16);

	const auto instruction2_size = sizeof(bc_instruction_t);
	QUARK_ASSERT(instruction2_size == 8);


	const auto immer_vec_bool_size = sizeof(immer::vector<bool>);
	const auto immer_vec_int_size = sizeof(immer::vector<int>);
	const auto immer_vec_string_size = sizeof(immer::vector<std::string>);

	QUARK_ASSERT(immer_vec_bool_size == 32);
	QUARK_ASSERT(immer_vec_int_size == 32);
	QUARK_ASSERT(immer_vec_string_size == 32);


	const auto immer_stringkey_map_size = sizeof(immer::map<std::string, double>);
	QUARK_ASSERT(immer_stringkey_map_size == 16);

	const auto immer_intkey_map_size = sizeof(immer::map<uint32_t, double>);
	QUARK_ASSERT(immer_intkey_map_size == 16);
}




//////////////////////////////////////////		UPDATE




//??? The update mechanism uses strings == slow.
bc_value_t update_struct_member_shallow(interpreter_t& vm, const bc_value_t& obj, const std::string& member_name, const bc_value_t& new_value){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(obj._type.is_struct());
	QUARK_ASSERT(member_name.empty() == false);
	QUARK_ASSERT(new_value.check_invariant());

	const auto& values = obj.get_struct_value();
	const auto& struct_def = obj._type.get_struct();

	int member_index = find_struct_member_index(struct_def, member_name);
	if(member_index == -1){
		quark::throw_runtime_error("Unknown member.");
	}

#if DEBUG
	QUARK_TRACE(typeid_to_compact_string(new_value._type));
	QUARK_TRACE(typeid_to_compact_string(struct_def._members[member_index]._type));

	const auto dest_member_entry = struct_def._members[member_index];
#endif

	auto values2 = values;
	values2[member_index] = new_value;

	auto s2 = bc_value_t::make_struct_value(obj._type, values2);
	return s2;
}

bc_value_t update_struct_member_deep(interpreter_t& vm, const bc_value_t& obj, const std::vector<std::string>& path, const bc_value_t& new_value){
	QUARK_ASSERT(obj.check_invariant());
	QUARK_ASSERT(path.empty() == false);
	QUARK_ASSERT(new_value.check_invariant());

	if(path.size() == 1){
		return update_struct_member_shallow(vm, obj, path[0], new_value);
	}
	else{
		std::vector<std::string> subpath = path;
		subpath.erase(subpath.begin());

		const auto& values = obj.get_struct_value();
		const auto& struct_def = obj._type.get_struct();
		int member_index = find_struct_member_index(struct_def, path[0]);
		if(member_index == -1){
			quark::throw_runtime_error("Unknown member.");
		}

		const auto& child_value = values[member_index];
		const auto& child_type = struct_def._members[member_index]._type;
		if(child_type.is_struct() == false){
			quark::throw_runtime_error("Value type not matching struct member type.");
		}

		const auto child2 = update_struct_member_deep(vm, child_value, subpath, new_value);
		const auto obj2 = update_struct_member_shallow(vm, obj, path[0], child2);
		return obj2;
	}
}

bc_value_t update_string_char(interpreter_t& vm, const bc_value_t s, int64_t lookup_index, int64_t ch){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s._type.is_string());
	QUARK_ASSERT(lookup_index >= 0 && lookup_index < s.get_string_value().size());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	std::string s2 = s.get_string_value();
	if(lookup_index < 0 || lookup_index >= s2.size()){
		quark::throw_runtime_error("String lookup out of bounds.");
	}
	else{
		s2[lookup_index] = static_cast<char>(ch);
//		const auto s3 = value_t::make_string(s2);
//		return value_to_bc(s3);
		return bc_value_t::make_string(s2);
	}
}

bc_value_t update_vector_element(interpreter_t& vm, const bc_value_t vec, int64_t lookup_index, const bc_value_t& value){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(vec.check_invariant());
	QUARK_ASSERT(vec._type.is_vector());
	QUARK_ASSERT(lookup_index >= 0);
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(vec._type.get_vector_element_type() == value._type);

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto element_type = vec._type.get_vector_element_type();
	if(encode_as_vector_w_inplace_elements(vec._type)){
		auto v2 = vec._pod._external->_vector_w_inplace_elements;

		if(lookup_index < 0 || lookup_index >= v2.size()){
			quark::throw_runtime_error("Vector lookup out of bounds.");
		}
		else{
			v2 = v2.set(lookup_index, value._pod._inplace);
			const auto s2 = make_vector(element_type, v2);
			return s2;
		}
	}
	else{
		const auto obj = vec;
		auto v2 = *get_vector_external_elements(obj);
		if(lookup_index < 0 || lookup_index >= v2.size()){
			quark::throw_runtime_error("Vector lookup out of bounds.");
		}
		else{
//			QUARK_TRACE_SS("bc1:  " << json_to_pretty_string(bcvalue_to_json(obj)));

			QUARK_ASSERT(encode_as_external(value._type));
			const auto e = bc_external_handle_t(value);
			v2 = v2.set(lookup_index, e);
			const auto s2 = make_vector(element_type, v2);

//			QUARK_TRACE_SS("bc2:  " << json_to_pretty_string(bcvalue_to_json(s2)));
			return s2;
		}
	}
}

bc_value_t update_dict_entry(interpreter_t& vm, const bc_value_t dict, const std::string& key, const bc_value_t& value){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(dict.check_invariant());
	QUARK_ASSERT(dict._type.is_dict());
	QUARK_ASSERT(key.empty() == false);
	QUARK_ASSERT(value.check_invariant());
	QUARK_ASSERT(dict._type.get_dict_value_type() == value._type);

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto value_type = dict._type.get_dict_value_type();

	if(encode_as_dict_w_inplace_values(dict._type)){
		auto entries2 = dict._pod._external->_dict_w_inplace_values.set(key, value._pod._inplace);
		const auto value2 = make_dict(value_type, entries2);
		return value2;
	}
	else{
		const auto entries = get_dict_value(dict);
		auto entries2 = entries.set(key, bc_external_handle_t(value));
		const auto value2 = make_dict(value_type, entries2);
		return value2;
	}
}

bc_value_t update_struct_member(interpreter_t& vm, const bc_value_t str, const std::vector<std::string>& path, const bc_value_t& value){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(str.check_invariant());
	QUARK_ASSERT(str._type.is_struct());
	QUARK_ASSERT(path.empty() == false);
	QUARK_ASSERT(value.check_invariant());
//	QUARK_ASSERT(str._type.get_struct_ref()->_members[member_index]._type == value._type);

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	return update_struct_member_deep(vm, str, path, value);
}

bc_value_t update_element(interpreter_t& vm, const bc_value_t& obj1, const bc_value_t& lookup_key, const bc_value_t& new_value){
	QUARK_ASSERT(vm.check_invariant());

//	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	if(obj1._type.is_string()){
		if(lookup_key._type.is_int() == false){
			quark::throw_runtime_error("String lookup using integer index only.");
		}
		else{
			const auto v = obj1.get_string_value();
			if(new_value._type.is_int() == false){
				quark::throw_runtime_error("Update element must be a character in an int.");
			}
			else{
				const auto lookup_index = lookup_key.get_int_value();
				return update_string_char(vm, obj1, lookup_index, new_value.get_int_value());
			}
		}
	}
	else if(obj1._type.is_json_value()){
		const auto json_value0 = obj1.get_json_value();
		if(json_value0.is_array()){
			QUARK_ASSERT(false);
		}
		else if(json_value0.is_object()){
			QUARK_ASSERT(false);
		}
		else{
			quark::throw_runtime_error("Can only update string, vector, dict or struct.");
		}
	}
	else if(obj1._type.is_vector()){
		const auto element_type = obj1._type.get_vector_element_type();
		if(lookup_key._type.is_int() == false){
			quark::throw_runtime_error("Vector lookup using integer index only.");
		}
		else if(element_type != new_value._type){
			quark::throw_runtime_error("Update element must match vector type.");
		}
		else{
			const auto lookup_index = lookup_key.get_int_value();
			return update_vector_element(vm, obj1, lookup_index, new_value);
		}
	}
	else if(obj1._type.is_dict()){
		if(lookup_key._type.is_string() == false){
			quark::throw_runtime_error("Dict lookup using string key only.");
		}
		else{
			const auto obj = obj1;
			const auto value_type = obj._type.get_dict_value_type();
			if(value_type != new_value._type){
				quark::throw_runtime_error("Update element must match dict value type.");
			}
			else{
				const std::string key = lookup_key.get_string_value();
				return update_dict_entry(vm, obj1, key, new_value);
			}
		}
	}
	else if(obj1._type.is_struct()){
		if(lookup_key._type.is_string() == false){
			quark::throw_runtime_error("You must specify structure member using string.");
		}
		else{
			const auto nodes = split_on_chars(seq_t(lookup_key.get_string_value()), ".");
			if(nodes.empty()){
				quark::throw_runtime_error("You must specify structure member using string.");
			}
			return update_struct_member(vm, obj1, nodes, new_value);
		}
	}
	else {
		quark::throw_runtime_error("Can only update string, vector, dict or struct.");
	}
}



//////////////////////////////////////////		COMPARE



int compare(int64_t value){
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

int bc_compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	return compare(std::strcmp(left.c_str(), right.c_str()));
}

QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("", ""), 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("aaa", "aaa"), 0);
}
QUARK_UNIT_TESTQ("bc_compare_string()", ""){
	ut_verify_auto(QUARK_POS, bc_compare_string("b", "a"), 1);
}


int bc_compare_struct_true_deep(const std::vector<bc_value_t>& left, const std::vector<bc_value_t>& right, const typeid_t& type){
	const auto& struct_def = type.get_struct();

	for(int i = 0 ; i < struct_def._members.size() ; i++){
		const auto& member_type = struct_def._members[i]._type;
		int diff = bc_compare_value_true_deep(left[i], right[i], member_type);
		if(diff != 0){
			return diff;
		}
	}
	return 0;
}

int bc_compare_vectors_obj(const immer::vector<bc_external_handle_t>& left, const immer::vector<bc_external_handle_t>& right, const typeid_t& type){
	QUARK_ASSERT(type.is_vector());

	const auto& shared_count = std::min(left.size(), right.size());
	const auto& element_type = typeid_t(type.get_vector_element_type());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = bc_compare_value_true_deep(bc_value_t(element_type, left[i]), bc_value_t(element_type, right[i]), element_type);
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

int compare_bools(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	auto left2 = left._bool ? 1 : 0;
	auto right2 = right._bool ? 1 : 0;
	if(left2 < right2){
		return -1;
	}
	else if(left2 > right2){
		return 1;
	}
	else{
		return 0;
	}
}

int compare_ints(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	if(left._int64 < right._int64){
		return -1;
	}
	else if(left._int64 > right._int64){
		return 1;
	}
	else{
		return 0;
	}
}
int compare_doubles(const bc_inplace_value_t& left, const bc_inplace_value_t& right){
	if(left._double < right._double){
		return -1;
	}
	else if(left._double > right._double){
		return 1;
	}
	else{
		return 0;
	}
}

int bc_compare_vectors_bool(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_bools(left[i], right[i]);
		if(result != 0){
			return result;
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
int bc_compare_vectors_int(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_ints(left[i], right[i]);
		if(result != 0){
			return result;
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
int bc_compare_vectors_double(const immer::vector<bc_inplace_value_t>& left, const immer::vector<bc_inplace_value_t>& right){
	const auto& shared_count = std::min(left.size(), right.size());
	for(int i = 0 ; i < shared_count ; i++){
		int result = compare_doubles(left[i], right[i]);
		if(result != 0){
			return result;
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
bool bc_map_compare (Map const &lhs, Map const &rhs) {
	// No predicate needed because there is operator== for pairs already.
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

int bc_compare_dicts_obj(const immer::map<std::string, bc_external_handle_t>& left, const immer::map<std::string, bc_external_handle_t>& right, const typeid_t& type){
	const auto& element_type = typeid_t(type.get_dict_value_type());

	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		const auto element_result = bc_compare_value_true_deep(bc_value_t(element_type, (*left_it).second), bc_value_t(element_type, (*right_it).second), element_type);
		if(element_result != 0){
			return element_result;
		}

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
	QUARK_ASSERT(false)
	quark::throw_exception();
}

//??? make template.
int bc_compare_dicts_bool(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_bools((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

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
	QUARK_ASSERT(false)
	quark::throw_exception();
}
int bc_compare_dicts_int(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_ints((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

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
	QUARK_ASSERT(false)
	quark::throw_exception();
}
int bc_compare_dicts_double(const immer::map<std::string, bc_inplace_value_t>& left, const immer::map<std::string, bc_inplace_value_t>& right){
	auto left_it = left.begin();
	auto left_end_it = left.end();

	auto right_it = right.begin();
	auto right_end_it = right.end();

	while(left_it != left_end_it && right_it != right_end_it){
		auto left_key = (*left_it).first;
		auto right_key = (*right_it).first;

		const auto key_result = bc_compare_string(left_key, right_key);
		if(key_result != 0){
			return key_result;
		}

		int result = compare_doubles((*left_it).second, (*right_it).second);
		if(result != 0){
			return result;
		}

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
	QUARK_ASSERT(false)
	quark::throw_exception();
}



int json_value_type_to_int(const json_t& value){
	if(value.is_object()){
		return 0;
	}
	else if(value.is_array()){
		return 1;
	}
	else if(value.is_string()){
		return 2;
	}
	else if(value.is_number()){
		return 4;
	}
	else if(value.is_true()){
		return 5;
	}
	else if(value.is_false()){
		return 6;
	}
	else if(value.is_null()){
		return 7;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


int bc_compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		const auto lhs_type = json_value_type_to_int(lhs);
		const auto rhs_type = json_value_type_to_int(rhs);
		int type_diff = rhs_type - lhs_type;
		if(type_diff != 0){
			return type_diff;
		}
		else{
			if(lhs.is_object()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_array()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_string()){
				const auto diff = std::strcmp(lhs.get_string().c_str(), rhs.get_string().c_str());
				return diff < 0 ? -1 : 1;
			}
			else if(lhs.is_number()){
				const auto lhs_number = lhs.get_number();
				const auto rhs_number = rhs.get_number();
				return lhs_number < rhs_number ? -1 : 1;
			}
			else if(lhs.is_true()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_false()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else if(lhs.is_null()){
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
			else{
				QUARK_ASSERT(false);
				quark::throw_exception();
			}
		}
	}
}

int bc_compare_value_exts(const bc_external_handle_t& left, const bc_external_handle_t& right, const typeid_t& type){
	return bc_compare_value_true_deep(bc_value_t(type, left), bc_value_t(type, right), type);
}

int bc_compare_value_true_deep(const bc_value_t& left, const bc_value_t& right, const typeid_t& type0){
	QUARK_ASSERT(left._type == right._type);
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	const auto type = type0;
	if(type.is_undefined()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(type.is_int()){
		return compare(left.get_int_value() - right.get_int_value());
	}
	else if(type.is_double()){
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
	else if(type.is_string()){
		return bc_compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(type.is_json_value()){
		return bc_compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
		if(left.get_typeid_value() == right.get_typeid_value()){
			return 0;
		}
		else{
			return -1;//??? Hack -- should return +1 depending on values.
		}
	}
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		return bc_compare_struct_true_deep(left.get_struct_value(), right.get_struct_value(), type0);
	}
	else if(type.is_vector()){
		if(false){
		}
		else if(type.get_vector_element_type().is_bool()){
			return bc_compare_vectors_bool(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else if(type.get_vector_element_type().is_int()){
			return bc_compare_vectors_int(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else if(type.get_vector_element_type().is_double()){
			return bc_compare_vectors_double(left._pod._external->_vector_w_inplace_elements, right._pod._external->_vector_w_inplace_elements);
		}
		else{
			const auto& left_vec = get_vector_external_elements(left);
			const auto& right_vec = get_vector_external_elements(right);
			return bc_compare_vectors_obj(*left_vec, *right_vec, type0);
		}
	}
	else if(type.is_dict()){
		if(false){
		}
		else if(type.get_dict_value_type().is_bool()){
			return bc_compare_dicts_bool(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(type.get_dict_value_type().is_int()){
			return bc_compare_dicts_int(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else if(type.get_dict_value_type().is_double()){
			return bc_compare_dicts_double(left._pod._external->_dict_w_inplace_values, right._pod._external->_dict_w_inplace_values);
		}
		else  {
			const auto& left2 = get_dict_value(left);
			const auto& right2 = get_dict_value(right);
			return bc_compare_dicts_obj(left2, right2, type0);
		}
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}

extern const std::map<bc_opcode, opcode_info_t> k_opcode_info = {
	{ bc_opcode::k_nop, { "nop", opcode_info_t::encoding::k_e_0000 }},

	{ bc_opcode::k_load_global_obj, { "load_global_obj", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_load_global_intern, { "load_global_intern", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_store_global_obj, { "store_global_obj", opcode_info_t::encoding::k_r_0ir0 } },
	{ bc_opcode::k_store_global_intern, { "store_global_intern", opcode_info_t::encoding::k_r_0ir0 } },

	{ bc_opcode::k_copy_reg_intern, { "copy_reg_intern", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_copy_reg_obj, { "copy_reg_obj", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_get_struct_member, { "get_struct_member", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_lookup_element_string, { "lookup_element_string", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_json_value, { "lookup_element_jsonvalue", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_vector_obj, { "lookup_element_vector_obj", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_vector_pod64, { "lookup_element_vector_pod64", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_dict_obj, { "lookup_element_dict_obj", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_lookup_element_dict_pod64, { "lookup_element_dict_pod64", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_get_size_vector_obj, { "get_size_vector_obj", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_vector_pod64, { "get_size_vector_pod64", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_dict_obj, { "get_size_dict_obj", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_dict_pod64, { "get_size_dict_pod64", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_string, { "get_size_string", opcode_info_t::encoding::k_q_0rr0 } },
	{ bc_opcode::k_get_size_jsonvalue, { "get_size_jsonvalue", opcode_info_t::encoding::k_q_0rr0 } },

	{ bc_opcode::k_pushback_vector_obj, { "pushback_vector_obj", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_pushback_vector_pod64, { "pushback_vector_pod64", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_pushback_string, { "pushback_string", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_call, { "call", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_add_bool, { "add_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_int, { "add_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_add_double, { "add_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_concat_strings, { "concat_strings", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_concat_vectors_obj, { "concat_vectors_obj", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_concat_vectors_pod64, { "concat_vectors_pod64", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_double, { "subtract_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_subtract_int, { "subtract_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_double, { "multiply_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_multiply_int, { "multiply_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_double, { "divide_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_divide_int, { "divide_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_remainder_int, { "remainder_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_and_bool, { "logical_and_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_int, { "logical_and_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_and_double, { "logical_and_double", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_bool, { "logical_or_bool", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_int, { "logical_or_int", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_or_double, { "logical_or_double", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_comparison_smaller_or_equal, { "comparison_smaller_or_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_or_equal_int, { "comparison_smaller_or_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_comparison_smaller, { "comparison_smaller", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_comparison_smaller_int, { "comparison_smaller_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_equal, { "logical_equal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_equal_int, { "logical_equal_int", opcode_info_t::encoding::k_o_0rrr } },

	{ bc_opcode::k_logical_nonequal, { "logical_nonequal", opcode_info_t::encoding::k_o_0rrr } },
	{ bc_opcode::k_logical_nonequal_int, { "logical_nonequal_int", opcode_info_t::encoding::k_o_0rrr } },


	{ bc_opcode::k_new_1, { "new_1", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_vector_obj, { "new_vector_obj", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_vector_pod64, { "new_vector_pod64", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_dict_obj, { "new_dict_obj", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_dict_pod64, { "new_dict_pod64", opcode_info_t::encoding::k_t_0rii } },
	{ bc_opcode::k_new_struct, { "new_struct", opcode_info_t::encoding::k_t_0rii } },

	{ bc_opcode::k_return, { "return", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_stop, { "stop", opcode_info_t::encoding::k_e_0000 } },

	{ bc_opcode::k_push_frame_ptr, { "push_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_pop_frame_ptr, { "pop_frame_ptr", opcode_info_t::encoding::k_e_0000 } },
	{ bc_opcode::k_push_intern, { "push_intern", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_push_obj, { "push_obj", opcode_info_t::encoding::k_p_0r00 } },
	{ bc_opcode::k_popn, { "popn", opcode_info_t::encoding::k_n_0ii0 } },

	{ bc_opcode::k_branch_false_bool, { "branch_false_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_true_bool, { "branch_true_bool", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_zero_int, { "branch_zero_int", opcode_info_t::encoding::k_k_0ri0 } },
	{ bc_opcode::k_branch_notzero_int, { "branch_notzero_int", opcode_info_t::encoding::k_k_0ri0 } },

	{ bc_opcode::k_branch_smaller_int, { "branch_smaller_int", opcode_info_t::encoding::k_s_0rri } },
	{ bc_opcode::k_branch_smaller_or_equal_int, { "branch_smaller_or_equal_int", opcode_info_t::encoding::k_s_0rri } },

	{ bc_opcode::k_branch_always, { "branch_always", opcode_info_t::encoding::k_l_00i0 } }


};


//??? Use enum with register / immediate / unused, current info is not enough.

reg_flags_t encoding_to_reg_flags(opcode_info_t::encoding e){
	if(e == opcode_info_t::encoding::k_e_0000){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_k_0ri0){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_l_00i0){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_n_0ii0){
		return { false, false, false };
	}
	else if(e == opcode_info_t::encoding::k_o_0rrr){
		return { true, true, true };
	}
	else if(e == opcode_info_t::encoding::k_p_0r00){
		return { true, false, false };
	}
	else if(e == opcode_info_t::encoding::k_q_0rr0){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_r_0ir0){
		return { false, true, false };
	}
	else if(e == opcode_info_t::encoding::k_s_0rri){
		return { true, true, false };
	}
	else if(e == opcode_info_t::encoding::k_t_0rii){
		return { true, false, false };
	}

	else{
		QUARK_ASSERT(false);
		quark::throw_exception();
	}
}


//////////////////////////////////////////		bc_instruction_t


bc_instruction_t::bc_instruction_t(bc_opcode opcode, int16_t a, int16_t b, int16_t c) :
	_opcode(opcode),
	_a(a),
	_b(b),
	_c(c)
{
	QUARK_ASSERT(check_invariant());
}

#if DEBUG
bool bc_instruction_t::check_invariant() const {
//	const auto encoding = k_opcode_info.at(_opcode)._encoding;
//	const auto reg_flags = encoding_to_reg_flags(encoding);

/*
	QUARK_ASSERT(check_register(_reg_a, reg_flags._a));
	QUARK_ASSERT(check_register(_reg_b, reg_flags._b));
	QUARK_ASSERT(check_register(_reg_c, reg_flags._c));
*/
	return true;
}
#endif


//////////////////////////////////////////		bc_static_frame_t

//?? STATIC frame definition -- not a runtime thing!

bc_static_frame_t::bc_static_frame_t(const std::vector<bc_instruction_t>& instrs2, const std::vector<std::pair<std::string, bc_symbol_t>>& symbols, const std::vector<typeid_t>& args) :
	_instrs2(instrs2),
	_symbols(symbols),
	_args(args)
{
	const auto parameter_count = static_cast<int>(_args.size());

	for(int i = 0 ; i < _symbols.size() ; i++){
		const auto type = _symbols[i].second._value_type;
		const bool ext = encode_as_external(type);
		_exts.push_back(ext);
	}

	//	Process the locals & temps. They go after any parameters, which already sits on stack.
	for(std::vector<bc_value_t>::size_type i = parameter_count ; i < _symbols.size() ; i++){
		const auto& symbol = _symbols[i];
		bool is_ext = _exts[i];

		_locals_exts.push_back(is_ext);

		//	Variable slot.
		//	This is just a variable slot without constant. We need to put something there, but that don't confuse RC.
		//	Problem is that IF this is an RC_object, it WILL be decremented when written to.
		//	Use a placeholder object of correct type.
		if(symbol.second._const_value._type.get_base_type() == base_type::k_internal_undefined){
			if(is_ext){
				const auto value = bc_value_t(symbol.second._value_type, bc_value_t::mode::k_unwritten_ext_value);
				_locals.push_back(value);
			}
			else{
				const auto value = make_def(symbol.second._value_type);
				const auto bc = value_to_bc(value);
				_locals.push_back(bc);
			}
		}

		//	Constant.
		else{
			_locals.push_back(symbol.second._const_value);
		}
	}

	QUARK_ASSERT(check_invariant());
}

bool bc_static_frame_t::check_invariant() const {
//	QUARK_ASSERT(_body.check_invariant());
	QUARK_ASSERT(_symbols.size() == _exts.size());

/*
	for(const auto& e: _instrs2){
//		const auto encoding = k_opcode_info.at(e._opcode)._encoding;
//		const auto reg_flags = encoding_to_reg_flags(encoding);

		QUARK_ASSERT(check_register__local(e._reg_a, reg_flags._a));
		QUARK_ASSERT(check_register__local(e._reg_b, reg_flags._b));
		QUARK_ASSERT(check_register__local(e._reg_c, reg_flags._c));

	}
*/
	return true;
}


//////////////////////////////////////////		bc_function_definition_t


bc_function_definition_t::bc_function_definition_t(
	const typeid_t& function_type,
	const std::vector<member_t>& args,
	const std::shared_ptr<bc_static_frame_t>& frame,
	int host_function_id
) :
	_function_type(function_type),
	_args(args),
	_frame_ptr(frame),
	_host_function_id(host_function_id),
	_dyn_arg_count(-1),
	_return_is_ext(encode_as_external(_function_type.get_function_return()))
{
	_dyn_arg_count = count_function_dynamic_args(function_type);
}

#if DEBUG
bool bc_function_definition_t::check_invariant() const {
	return true;
}
#endif



//////////////////////////////////////////		interpreter_stack_t



frame_pos_t interpreter_stack_t::read_prev_frame(int frame_pos) const{
//	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(frame_pos >= k_frame_overhead);

	const auto v = load_intq(frame_pos - k_frame_overhead + 0);
	QUARK_ASSERT(v < frame_pos);
	QUARK_ASSERT(v >= 0);

	const auto ptr = _entries[frame_pos - k_frame_overhead + 1]._inplace._frame_ptr;
	QUARK_ASSERT(ptr != nullptr);
	QUARK_ASSERT(ptr->check_invariant());

	return frame_pos_t{ static_cast<int>(v), ptr };
}

#if DEBUG
bool interpreter_stack_t::check_stack_frame(const frame_pos_t& in_frame) const{
	QUARK_ASSERT(in_frame._frame_ptr != nullptr && in_frame._frame_ptr->check_invariant());

	if(in_frame._frame_pos == 0){
		return true;
	}
	else{
		for(int i = 0 ; i < in_frame._frame_ptr->_symbols.size() ; i++){
			const auto& symbol = in_frame._frame_ptr->_symbols[i];

			bool symbol_ext = encode_as_external(symbol.second._value_type);
			int local_pos = get_local_n_pos(in_frame._frame_pos, i);

			bool stack_ext = is_ext(local_pos);
			QUARK_ASSERT(symbol_ext == stack_ext);
		}

		const auto prev_frame = read_prev_frame(in_frame._frame_pos);
		QUARK_ASSERT(prev_frame._frame_pos >= 0 && prev_frame._frame_pos < in_frame._frame_pos);
		if(prev_frame._frame_pos > k_frame_overhead){
			return check_stack_frame(prev_frame);
		}
		else{
			return true;
		}
	}
}
#endif

std::vector<std::pair<int, int>> interpreter_stack_t::get_stack_frames(int frame_pos) const{
	QUARK_ASSERT(check_invariant());

	//	We use the entire current stack to calc top frame's size. This can be wrong, if
	//	someone pushed more stuff there. Same goes with the previous stack frames too..
	std::vector<std::pair<int, int>> result{ { frame_pos, static_cast<int>(size()) - frame_pos }};

	while(frame_pos > k_frame_overhead){
		const auto prev_frame = read_prev_frame(frame_pos);
		const auto prev_size = (frame_pos - k_frame_overhead) - prev_frame._frame_pos;
		result.push_back(std::pair<int, int>{ frame_pos, prev_size });

		frame_pos = prev_frame._frame_pos;
	}
	return result;
}

json_t interpreter_stack_t::stack_to_json() const{
	const int size = static_cast<int>(_stack_size);

	const auto stack_frames = get_stack_frames(get_current_frame_start());

	std::vector<json_t> frames;
	for(int64_t i = 0 ; i < stack_frames.size() ; i++){
		auto a = json_t::make_array({
			json_t(i),
			json_t(stack_frames[i].first),
			json_t(stack_frames[i].second)
		});
		frames.push_back(a);
	}

	std::vector<json_t> elements;
	for(int64_t i = 0 ; i < size ; i++){
		const auto& frame_it = std::find_if(
			stack_frames.begin(),
			stack_frames.end(),
			[&i](const std::pair<int, int>& e) { return e.first == i; }
		);

		bool frame_start_flag = frame_it != stack_frames.end();

		if(frame_start_flag){
			elements.push_back(json_t(""));
		}

#if DEBUG
		const auto debug_type = _debug_types[i];
		const auto ext = encode_as_external(debug_type);
		const auto bc_pod = _entries[i];
		const auto bc = bc_value_t(debug_type, bc_pod);

		bool unwritten = ext && bc._pod._external->_debug__is_unwritten_external_value;

		auto a = json_t::make_array({
			json_t(i),
			typeid_to_ast_json(debug_type, json_tags::k_plain)._value,
			unwritten ? json_t("UNWRITTEN") : bcvalue_to_json(bc_value_t{bc})
		});
		elements.push_back(a);
#endif
	}

	return json_t::make_object({
		{ "size", json_t(size) },
		{ "frames", json_t::make_array(frames) },
		{ "elements", json_t::make_array(elements) }
	});
}


//////////////////////////////////////////		GLOBAL FUNCTIONS


const bc_function_definition_t& get_function_def(const interpreter_t& vm, int function_id){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

	const auto& function_def = vm._imm->_program._function_defs[function_id];
	return function_def;
}


//??? Use bc_value_t:s instead of bc_value_t -- types are known via function-signature.
bc_value_t call_function_bc(interpreter_t& vm, const bc_value_t& f, const bc_value_t args[], int arg_count){
#if DEBUG
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(f.check_invariant());
	for(int i = 0 ; i < arg_count ; i++){ QUARK_ASSERT(args[i].check_invariant()); };
	QUARK_ASSERT(f._type.is_function());
#endif

	const auto& function_def = get_function_def(vm, f.get_function_value());
	if(function_def._host_function_id != 0){
		const auto host_function_id = function_def._host_function_id;
		QUARK_ASSERT(host_function_id >= 0);

		const auto& host_function = vm._imm->_host_functions.at(host_function_id);

		//	arity
	//	QUARK_ASSERT(args.size() == host_function._function_type.get_function_args().size());

		const auto& result = (host_function)(vm, &args[0], arg_count);
		return result;
	}
	else{
#if DEBUG
		const auto& arg_types = f._type.get_function_args();

		//	arity
		QUARK_ASSERT(arg_count == arg_types.size());

		for(int i = 0 ; i < arg_count; i++){
			if(args[i]._type != arg_types[i]){
				QUARK_ASSERT(false);
			}
		}
#endif

		vm._stack.save_frame();

		//??? use exts-info inside function_def.
		//	We push the values to the stack = the stack will take RC ownership of the values.
		std::vector<bool> exts;
		for(int i = 0 ; i < arg_count ; i++){
			const auto& bc = args[i];
			bool is_ext = encode_as_external(args[i]._type);
			exts.push_back(is_ext);
			if(is_ext){
				vm._stack.push_obj(bc);
			}
			else{
				vm._stack.push_intern(bc);
			}
		}

		vm._stack.open_frame(*function_def._frame_ptr, arg_count);
		const auto& result = execute_instructions(vm, function_def._frame_ptr->_instrs2);
		vm._stack.close_frame(*function_def._frame_ptr);
		vm._stack.pop_batch(exts);
		vm._stack.restore_frame();

		if(vm._imm->_program._types[result.first].is_void() == false){
			return result.second;
		}
		else{
			return bc_value_t::make_undefined();
		}
	}
}

json_t bcvalue_to_json(const bc_value_t& v){
	if(v._type.is_undefined()){
		return json_t();
	}
	else if(v._type.is_internal_dynamic()){
		return json_t();
	}
	else if(v._type.is_void()){
		return json_t();
	}
	else if(v._type.is_bool()){
		return json_t(v.get_bool_value());
	}
	else if(v._type.is_int()){
		return json_t(static_cast<double>(v.get_int_value()));
	}
	else if(v._type.is_double()){
		return json_t(static_cast<double>(v.get_double_value()));
	}
	else if(v._type.is_string()){
		return json_t(v.get_string_value());
	}
	else if(v._type.is_json_value()){
		return v.get_json_value();
	}
	else if(v._type.is_typeid()){
		return typeid_to_ast_json(v.get_typeid_value(), json_tags::k_plain)._value;
	}
	else if(v._type.is_struct()){
		const auto& struct_value = v.get_struct_value();
		std::map<std::string, json_t> obj2;
		const auto& struct_def = v._type.get_struct();
		for(int i = 0 ; i < struct_def._members.size() ; i++){
			const auto& member = struct_def._members[i];
			const auto& key = member._name;
//			const auto& type = member._type;
			const auto& value = struct_value[i];
			const auto& value2 = bcvalue_to_json(value);
			obj2[key] = value2;
		}
		return json_t::make_object(obj2);
	}
	else if(v._type.is_vector()){
		const auto element_type = v._type.get_vector_element_type();

		std::vector<json_t> result;
		if(element_type.is_bool()){
			for(int i = 0 ; i < v._pod._external->_vector_w_inplace_elements.size() ; i++){
				const auto element_value2 = v._pod._external->_vector_w_inplace_elements[i]._bool;
				result.push_back(json_t(element_value2));
			}
		}
		else if(element_type.is_int()){
			for(int i = 0 ; i < v._pod._external->_vector_w_inplace_elements.size() ; i++){
				const auto element_value2 = v._pod._external->_vector_w_inplace_elements[i]._int64;
				result.push_back(json_t(element_value2));
			}
		}
		else if(element_type.is_double()){
			for(int i = 0 ; i < v._pod._external->_vector_w_inplace_elements.size() ; i++){
				const auto element_value2 = v._pod._external->_vector_w_inplace_elements[i]._double;
				result.push_back(json_t(element_value2));
			}
		}
		else{
			const auto vec = get_vector_external_elements(v);
			for(int i = 0 ; i < vec->size() ; i++){
				const auto element_value2 = vec->operator[](i);
				result.push_back(bcvalue_to_json(bc_value_t(element_type, element_value2)));
			}
		}
		return result;
	}
	else if(v._type.is_dict()){
		const auto value_type = v._type.get_dict_value_type();
		const auto entries = get_dict_value(v);
		std::map<std::string, json_t> result;
		for(const auto& e: entries){
			const auto value2 = e.second;
			//??? works for all types? Use that technique in all thunking! Slower but less code.
			result[e.first] = bcvalue_to_json(bc_value_t(value_type, value2));
		}
		return result;
	}
	else if(v._type.is_function()){
		return json_t::make_object(
			{
				{ "funtyp", typeid_to_ast_json(v._type, json_tags::k_plain)._value }
			}
		);
	}
	else{
		quark::throw_exception();
	}
}


json_t bcvalue_and_type_to_json(const bc_value_t& v){
	return json_t::make_array({
		typeid_to_ast_json(v._type, json_tags::k_plain)._value,
		bcvalue_to_json(v)
	});
}



//////////////////////////////////////////		interpreter_t



interpreter_t::interpreter_t(const bc_program_t& program, interpreter_handler_i* handler) :
	_stack(nullptr),
	_handler(handler)
{
	QUARK_ASSERT(program.check_invariant());

	//	Make lookup table from host-function ID to an implementation of that host function in the interpreter.
	const auto& host_functions = get_host_functions();
	std::map<int, HOST_FUNCTION_PTR> host_functions2;
	for(auto& hf_kv: host_functions){
		const auto& function_id = hf_kv.second._signature._function_id;
		const auto& function_ptr = hf_kv.second._f;
		host_functions2.insert({ function_id, function_ptr });
	}

	const auto start_time = std::chrono::high_resolution_clock::now();
	_imm = std::make_shared<interpreter_imm_t>(interpreter_imm_t{start_time, program, host_functions2});

	interpreter_stack_t temp(&_imm->_program._globals);
	temp.swap(_stack);
	_stack.save_frame();
	_stack.open_frame(_imm->_program._globals, 0);

	//	Run static intialization (basically run global instructions before calling main()).
	/*const auto& r =*/ execute_instructions(*this, _imm->_program._globals._instrs2);
	QUARK_ASSERT(check_invariant());
}
interpreter_t::interpreter_t(const bc_program_t& program) : interpreter_t(program, nullptr) {}

/*
interpreter_t::interpreter_t(const interpreter_t& other) :
	_imm(other._imm),
	_stack(other._stack),
	_current_stack_frame(other._current_stack_frame),
	_print_output(other._print_output)
{
	QUARK_ASSERT(other.check_invariant());
	QUARK_ASSERT(check_invariant());
}
*/

void interpreter_t::swap(interpreter_t& other) throw(){
	other._imm.swap(this->_imm);
	std::swap(other._handler, this->_handler);
	other._stack.swap(this->_stack);
	other._print_output.swap(this->_print_output);
}

/*
const interpreter_t& interpreter_t::operator=(const interpreter_t& other){
	auto temp = other;
	temp.swap(*this);
	return *this;
}
*/

#if DEBUG
bool interpreter_t::check_invariant() const {
	QUARK_ASSERT(_imm->_program.check_invariant());
	QUARK_ASSERT(_stack.check_invariant());
	return true;
}
#endif


//////////////////////////////////////////		INSTRUCTIONS


QUARK_UNIT_TEST("", "", "", ""){
	const auto value_size = sizeof(bc_value_t);
	QUARK_UT_VERIFY(value_size >= 8);
/*
	QUARK_UT_VERIFY(value_size == 16);
	QUARK_UT_VERIFY(expression_size == 40);
	QUARK_UT_VERIFY(e_count_offset == 4);
	QUARK_UT_VERIFY(e_offset == 8);
	QUARK_UT_VERIFY(value_offset == 16);
*/


//	QUARK_UT_VERIFY(sizeof(temp) == 56);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(variable_address_t);
	QUARK_UT_VERIFY(s == 8);
}
QUARK_UNIT_TEST("", "", "", ""){
	const auto s = sizeof(bc_value_t);
	QUARK_UT_VERIFY(s >= 8);
//	QUARK_UT_VERIFY(s == 16);
}


//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_1(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t source_itype){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector() == false && target_type.is_dict() == false && target_type.is_struct() == false);

	const int arg0_stack_pos = vm._stack.size() - 1;
	const auto input_value_type = lookup_full_type(vm, source_itype);
	const auto input_value = vm._stack.load_value(arg0_stack_pos + 0, input_value_type);

	const bc_value_t result = [&]{
		if(target_type.is_bool() || target_type.is_int() || target_type.is_double() || target_type.is_typeid()){
			return input_value;
		}
		else if(target_type.is_string()){
			if(input_value_type.is_json_value() && input_value.get_json_value().is_string()){
				return bc_value_t::make_string(input_value.get_json_value().get_string());
			}
			else{
				return input_value;
			}
		}
		else if(target_type.is_json_value()){
			const auto arg = bcvalue_to_json(input_value);
			return bc_value_t::make_json_value(arg);
		}
		else{
			return input_value;
		}
	}();

	vm._stack.write_register(dest_reg, result);
}


//	IMPORTANT: NO arguments are passed as DYN arguments.
void execute_new_vector_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_vector());
	QUARK_ASSERT(encode_as_vector_w_inplace_elements(target_type) == false);

	const auto& element_type = target_type.get_vector_element_type();
	QUARK_ASSERT(element_type.is_undefined() == false);
	QUARK_ASSERT(target_type.is_undefined() == false);

	const int arg0_stack_pos = vm._stack.size() - arg_count;
//	bool is_element_ext = encode_as_external(element_type);

	immer::vector<bc_external_handle_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto pos = arg0_stack_pos + i;
		QUARK_ASSERT(vm._stack._debug_types[pos] == element_type);
		const auto e = bc_external_handle_t(vm._stack._entries[pos]._external);
		elements2 = elements2.push_back(e);
	}

	const auto result = make_vector(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}

void execute_new_dict_obj(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_type.get_dict_value_type();
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = typeid_t::make_string();

	immer::map<std::string, bc_external_handle_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2 = elements2.insert({ key2, bc_external_handle_t(value) });
	}

	const auto result = make_dict(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}
void execute_new_dict_pod64(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_dict());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& element_type = target_type.get_dict_value_type();
	QUARK_ASSERT(target_type.is_undefined() == false);
	QUARK_ASSERT(element_type.is_undefined() == false);

	const auto string_type = typeid_t::make_string();

	immer::map<std::string, bc_inplace_value_t> elements2;
	int dict_element_count = arg_count / 2;
	for(auto i = 0 ; i < dict_element_count ; i++){
		const auto key = vm._stack.load_value(arg0_stack_pos + i * 2 + 0, string_type);
		const auto value = vm._stack.load_value(arg0_stack_pos + i * 2 + 1, element_type);
		const auto key2 = key.get_string_value();
		elements2 = elements2.insert({ key2, value._pod._inplace });
	}

	const auto result = make_dict(element_type, elements2);
	vm._stack.write_register_obj(dest_reg, result);
}

void execute_new_struct(interpreter_t& vm, int16_t dest_reg, int16_t target_itype, int16_t arg_count){
	QUARK_ASSERT(vm.check_invariant());

	const auto& target_type = lookup_full_type(vm, target_itype);
	QUARK_ASSERT(target_type.is_struct());

	const int arg0_stack_pos = vm._stack.size() - arg_count;

	const auto& struct_def = target_type.get_struct();
	std::vector<bc_value_t> elements2;
	for(int i = 0 ; i < arg_count ; i++){
		const auto member_type = struct_def._members[i]._type;
		const auto value = vm._stack.load_value(arg0_stack_pos + i, member_type);
		elements2.push_back(value);
	}

	const auto result = bc_value_t::make_struct_value(target_type, elements2);
//	QUARK_TRACE(to_compact_string2(instance));

	vm._stack.write_register_obj(dest_reg, result);
}




#if 0
//	Computed goto-dispatch of expressions: -- not faster than switch when running max optimizations. C++ Optimizer makes compute goto?
//	May be better bet when doing a SEQUENCE of opcode dispatches in a loop.
//??? use C++ local variables as our VM locals1?
//https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
bc_value_t execute_expression__computed_goto(interpreter_t& vm, const bc_expression_t& e){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	const auto& op = static_cast<int>(e._opcode);
	//	Not thread safe -- avoid static locals!
	static void* dispatch_table[] = {
		&&bc_expression_opcode___k_expression_literal,
		&&bc_expression_opcode___k_expression_logical_or
	};
//	#define DISPATCH() goto *dispatch_table[op]

//	DISPATCH();
	goto *dispatch_table[op];
	while (1) {
        bc_expression_opcode___k_expression_literal:
			return e._value;

		bc_expression_opcode___k_expression_resolve_member:
			return execute_resolve_member_expression(vm, e);

		bc_expression_opcode___k_expression_lookup_element:
			return execute_lookup_element_expression(vm, e);

	}
}
#endif



//??? pass returns value(s) via parameters instead.
//???	Future: support dynamic Floyd functions too.

std::pair<bc_typeid_t, bc_value_t> execute_instructions(interpreter_t& vm, const std::vector<bc_instruction_t>& instructions){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(instructions.empty() == true || (instructions.back()._opcode == bc_opcode::k_return || instructions.back()._opcode == bc_opcode::k_stop));

	interpreter_stack_t& stack = vm._stack;
	const bc_static_frame_t* frame_ptr = stack._current_frame_ptr;
	bc_pod_value_t* regs = stack._current_frame_entry_ptr;
	bc_pod_value_t* globals = &stack._entries[k_frame_overhead];

//	const typeid_t* type_lookup = &vm._imm->_program._types[0];
//	const auto type_count = vm._imm->_program._types.size();

//	QUARK_TRACE_SS("STACK:  " << json_to_pretty_string(stack.stack_to_json()));

	int pc = 0;
	while(true){
		QUARK_ASSERT(pc >= 0);
		QUARK_ASSERT(pc < instructions.size());
		const auto i = instructions[pc];

		QUARK_ASSERT(vm.check_invariant());
		QUARK_ASSERT(i.check_invariant());

		QUARK_ASSERT(frame_ptr == stack._current_frame_ptr);
		QUARK_ASSERT(regs == stack._current_frame_entry_ptr);


		const auto opcode = i._opcode;
		switch(opcode){

		case bc_opcode::k_nop:
			break;


		//////////////////////////////////////////		ACCESS GLOBALS


		case bc_opcode::k_load_global_obj: {
			QUARK_ASSERT(stack.check_reg_obj(i._a));
			QUARK_ASSERT(stack.check_global_access_obj(i._b));

			release_pod_external(regs[i._a]);
			const auto& new_value_pod = globals[i._b];
			regs[i._a] = new_value_pod;
			new_value_pod._external->_rc++;
			break;
		}
		case bc_opcode::k_load_global_intern: {
			QUARK_ASSERT(stack.check_reg_intern(i._a));

			regs[i._a] = globals[i._b];
			break;
		}


		case bc_opcode::k_store_global_obj: {
			QUARK_ASSERT(stack.check_global_access_obj(i._a));
			QUARK_ASSERT(stack.check_reg_obj(i._b));

			release_pod_external(globals[i._a]);
			const auto& new_value_pod = regs[i._b];
			globals[i._a] = new_value_pod;
			new_value_pod._external->_rc++;
			break;
		}
		case bc_opcode::k_store_global_intern: {
			QUARK_ASSERT(stack.check_global_access_intern(i._a));
			QUARK_ASSERT(stack.check_reg_intern(i._b));

			globals[i._a] = regs[i._b];
			break;
		}


		//////////////////////////////////////////		ACCESS LOCALS


		case bc_opcode::k_copy_reg_intern: {
			QUARK_ASSERT(stack.check_reg_intern(i._a));
			QUARK_ASSERT(stack.check_reg_intern(i._b));

			regs[i._a] = regs[i._b];
			break;
		}
		case bc_opcode::k_copy_reg_obj: {
			QUARK_ASSERT(stack.check_reg_obj(i._a));
			QUARK_ASSERT(stack.check_reg_obj(i._b));

			release_pod_external(regs[i._a]);
			const auto& new_value_pod = regs[i._b];
			regs[i._a] = new_value_pod;
			new_value_pod._external->_rc++;
			break;
		}


		//////////////////////////////////////////		STACK


		case bc_opcode::k_return: {
			bool is_ext = frame_ptr->_exts[i._a];
			QUARK_ASSERT(
				(is_ext && stack.check_reg_obj(i._a))
				|| (!is_ext && stack.check_reg_intern(i._a))
			);

			return { true, bc_value_t(frame_ptr->_symbols[i._a].second._value_type, regs[i._a]) };
		}

		case bc_opcode::k_stop: {
			return { false, bc_value_t::make_undefined() };
		}

		case bc_opcode::k_push_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT((stack._stack_size + k_frame_overhead) < stack._allocated_count)

			stack._entries[stack._stack_size + 0]._inplace._int64 = static_cast<int64_t>(stack._current_frame_entry_ptr - &stack._entries[0]);
			stack._entries[stack._stack_size + 1]._inplace._frame_ptr = frame_ptr;
			stack._stack_size += k_frame_overhead;
#if DEBUG
			stack._debug_types.push_back(typeid_t::make_int());
			stack._debug_types.push_back(typeid_t::make_void());
#endif
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pop_frame_ptr: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack._stack_size >= k_frame_overhead);

			const auto frame_pos = stack._entries[stack._stack_size - k_frame_overhead + 0]._inplace._int64;
			frame_ptr = stack._entries[stack._stack_size - k_frame_overhead + 1]._inplace._frame_ptr;
			stack._stack_size -= k_frame_overhead;

#if DEBUG
			stack._debug_types.pop_back();
			stack._debug_types.pop_back();
#endif

			regs = &stack._entries[frame_pos];

			//??? do we need stack._current_frame_ptr, stack._current_frame_pos? Only use local variables to track these?
			stack._current_frame_ptr = frame_ptr;
			stack._current_frame_entry_ptr = regs;

			QUARK_ASSERT(frame_ptr == stack._current_frame_ptr);
			QUARK_ASSERT(regs == stack._current_frame_entry_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_push_intern: {
			QUARK_ASSERT(stack.check_reg_intern(i._a));
#if DEBUG
			const auto debug_type = stack._debug_types[stack.get_current_frame_start() + i._a];
#endif

			stack._entries[stack._stack_size] = regs[i._a];
			stack._stack_size++;
#if DEBUG
			stack._debug_types.push_back(debug_type);
#endif
			QUARK_ASSERT(stack.check_invariant());
			break;
		}
		case bc_opcode::k_push_obj: {
			QUARK_ASSERT(stack.check_reg_obj(i._a));

#if DEBUG
			const auto debug_type = stack._debug_types[stack.get_current_frame_start() + i._a];
#endif

			const auto& new_value_pod = regs[i._a];
			new_value_pod._external->_rc++;
			stack._entries[stack._stack_size] = new_value_pod;
			stack._stack_size++;
#if DEBUG
			stack._debug_types.push_back(debug_type);
#endif
			break;
		}

		case bc_opcode::k_popn: {
			QUARK_ASSERT(vm.check_invariant());

			const uint32_t n = i._a;
			const uint32_t extbits = i._b;

			QUARK_ASSERT(stack._stack_size >= n);
			QUARK_ASSERT(n >= 0);
			QUARK_ASSERT(n <= 32);

			uint32_t bits = extbits;
			int pos = static_cast<int>(stack._stack_size) - 1;
			for(int m = 0 ; m < n ; m++){
				bool ext = (bits & 1) ? true : false;

				QUARK_ASSERT(encode_as_external(stack._debug_types.back()) == ext);
	#if DEBUG
				stack._debug_types.pop_back();
	#endif
				if(ext){
					release_pod_external(stack._entries[pos]);
				}
				pos--;
				bits = bits >> 1;
			}
			stack._stack_size -= n;

			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		//////////////////////////////////////////		BRANCHING


		case bc_opcode::k_branch_false_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._bool ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_true_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._bool ? pc + i._b - 1: pc;
			break;
		}
		case bc_opcode::k_branch_zero_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._int64 == 0 ? pc + i._b - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_notzero_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._int64 == 0 ? pc : pc + i._b - 1;
			break;
		}
		case bc_opcode::k_branch_smaller_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._int64 < regs[i._b]._inplace._int64 ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_smaller_or_equal_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));

			//	Notice that pc will be incremented too, hence the - 1.
			pc = regs[i._a]._inplace._int64 <= regs[i._b]._inplace._int64 ? pc + i._c - 1 : pc;
			break;
		}
		case bc_opcode::k_branch_always: {
			//	Notice that pc will be incremented too, hence the - 1.
			pc = pc + i._a - 1;
			break;
		}


		//////////////////////////////////////////		COMPLEX


		//??? Make obj/intern version.
		case bc_opcode::k_get_struct_member: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_any(i._a));
			QUARK_ASSERT(stack.check_reg_struct(i._b));

			const auto& value_pod = regs[i._b]._external->_struct_members[i._c]._pod;
			bool ext = frame_ptr->_exts[i._a];
			if(ext){
				release_pod_external(regs[i._a]);
				value_pod._external->_rc++;
			}
			regs[i._a] = value_pod;
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto& s = regs[i._b]._external->_string;
			const auto lookup_index = regs[i._c]._inplace._int64;
			if(lookup_index < 0 || lookup_index >= s.size()){
				quark::throw_runtime_error("Lookup in string: out of bounds.");
			}
			else{
				regs[i._a]._inplace._int64 = s[lookup_index];
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		//	??? Simple JSON-values should not require ext. null, int, bool, empty object, empty array.
		case bc_opcode::k_lookup_element_json_value: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_json(i._a));
			QUARK_ASSERT(stack.check_reg_json(i._b));

			// reg c points to different types depending on the runtime-type of the json_value.
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& parent_json_value = regs[i._b]._external->_json_value;

			if(parent_json_value->is_object()){
				QUARK_ASSERT(stack.check_reg_string(i._c));

				const auto& lookup_key = regs[i._c]._external->_string;

				//	get_object_element() throws if key can't be found.
				const auto& value = parent_json_value->get_object_element(lookup_key);

				//??? no need to create full bc_value_t here! We only need pod.
				const auto value2 = bc_value_t::make_json_value(value);

				value2._pod._external->_rc++;
				release_pod_external(regs[i._a]);
				regs[i._a] = value2._pod;
			}
			else if(parent_json_value->is_array()){
				QUARK_ASSERT(stack.check_reg_int(i._c));

				const auto lookup_index = regs[i._c]._inplace._int64;
				if(lookup_index < 0 || lookup_index >= parent_json_value->get_array_size()){
					quark::throw_runtime_error("Lookup in json_value array: out of bounds.");
				}
				else{
					const auto& value = parent_json_value->get_array_n(lookup_index);

					//??? value2 will soon go out of scope - avoid creating bc_value_t all together.
					//??? no need to create full bc_value_t here! We only need pod.
					const auto value2 = bc_value_t::make_json_value(value);

					value2._pod._external->_rc++;
					release_pod_external(regs[i._a]);
					regs[i._a] = value2._pod;
				}
			}
			else{
				quark::throw_runtime_error("Lookup using [] on json_value only works on objects and arrays.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_vector_obj: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_obj(i._a));
			QUARK_ASSERT(stack.check_reg_vector_obj(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto& vec = regs[i._b]._external->_vector_w_external_elements;
			const auto lookup_index = regs[i._c]._inplace._int64;
			if(lookup_index < 0 || lookup_index >= vec.size()){
				quark::throw_runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				auto handle = vec[lookup_index];
				handle._external->_rc++;
				release_pod_external(regs[i._a]);
				regs[i._a]._external = handle._external;
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_lookup_element_vector_pod64: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto& vec = regs[i._b]._external->_vector_w_inplace_elements;
			const auto lookup_index = regs[i._c]._inplace._int64;
			if(lookup_index < 0 || lookup_index >= vec.size()){
				quark::throw_runtime_error("Lookup in vector: out of bounds.");
			}
			else{
				regs[i._a]._inplace = vec[lookup_index];
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_lookup_element_dict_obj: {
			QUARK_ASSERT(stack.check_reg_obj(i._a));
			QUARK_ASSERT(stack.check_reg_dict_obj(i._b));
			QUARK_ASSERT(stack.check_reg_string(i._c));

			const auto& entries = regs[i._b]._external->_dict_w_external_values;
			const auto& lookup_key = regs[i._c]._external->_string;
			const auto found_ptr = entries.find(lookup_key);
			if(found_ptr == nullptr){
				quark::throw_runtime_error("Lookup in dict: key not found.");
			}
			else{
				const auto& handle = *found_ptr;
				handle._external->_rc++;
				release_pod_external(regs[i._a]);
				regs[i._a]._external = handle._external;
			}
			break;
		}
		case bc_opcode::k_lookup_element_dict_pod64: {
			QUARK_ASSERT(stack.check_reg_any(i._a));
			QUARK_ASSERT(stack.check_reg_dict_pod64(i._b));
			QUARK_ASSERT(stack.check_reg_string(i._c));

			const auto& entries = regs[i._b]._external->_dict_w_inplace_values;
			const auto& lookup_key = regs[i._c]._external->_string;
			const auto found_ptr = entries.find(lookup_key);
			if(found_ptr == nullptr){
				quark::throw_runtime_error("Lookup in dict: key not found.");
			}
			else{
				regs[i._a]._inplace = *found_ptr;
			}
			break;
		}


		case bc_opcode::k_get_size_vector_obj: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_vector_obj(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a]._inplace._int64 = regs[i._b]._external->_vector_w_external_elements.size();
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_vector_pod64: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a]._inplace._int64 = regs[i._b]._external->_vector_w_inplace_elements.size();
			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		case bc_opcode::k_get_size_dict_obj: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_dict_obj(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a]._inplace._int64 = regs[i._b]._external->_dict_w_external_values.size();
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_dict_pod64: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_dict_pod64(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a]._inplace._int64 = regs[i._b]._external->_dict_w_inplace_values.size();
			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		case bc_opcode::k_get_size_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(i._c == 0);

			regs[i._a]._inplace._int64 = regs[i._b]._external->_string.size();
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_get_size_jsonvalue: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_json(i._b));
			QUARK_ASSERT(i._c == 0);

			const auto& json_value = *regs[i._b]._external->_json_value;
			if(json_value.is_object()){
				regs[i._a]._inplace._int64 = json_value.get_object_size();
			}
			else if(json_value.is_array()){
				regs[i._a]._inplace._int64 = json_value.get_array_size();
			}
			else if(json_value.is_string()){
				regs[i._a]._inplace._int64 = json_value.get_string().size();
			}
			else{
				quark::throw_runtime_error("Calling size() on unsupported type of value.");
			}
			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		case bc_opcode::k_pushback_vector_obj: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_obj(i._a));
			QUARK_ASSERT(stack.check_reg_vector_obj(i._b));
			QUARK_ASSERT(stack.check_reg_obj(i._c));

			const auto& type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = type.get_vector_element_type();

			auto elements2 = regs[i._b]._external->_vector_w_external_elements.push_back(bc_external_handle_t(regs[i._c]._external));
			//??? always allocates a new bc_external_value_t!
			const auto vec2 = make_vector(element_type, elements2);
			vm._stack.write_register_obj(i._a, vec2);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}
		case bc_opcode::k_pushback_vector_pod64: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._a));
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._b));
			QUARK_ASSERT(stack.check_reg(i._c));

			const auto& type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = type.get_vector_element_type();

			//??? optimize - bypass bc_value_t
			//??? always allocates a new bc_external_value_t!
			auto elements2 = regs[i._b]._external->_vector_w_inplace_elements.push_back(regs[i._c]._inplace);
			const auto vec = make_vector(element_type, elements2);
			vm._stack.write_register_obj(i._a, vec);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_pushback_string: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_string(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			std::string str2 = regs[i._b]._external->_string;
			const auto ch = regs[i._c]._inplace._int64;
			str2.push_back(static_cast<char>(ch));

			//??? optimize - bypass bc_value_t
			//??? always allocates a new bc_external_value_t!
			const auto str3 = bc_value_t::make_string(str2);
			vm._stack.write_register_obj(i._a, str3);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}


		/*
			??? Make stub bc_static_frame_t for each host function to make call conventions same as Floyd functions.
		*/

		//	Notice: host calls and floyd calls have the same type -- we cannot detect host calls until we have a callee value.
		case bc_opcode::k_call: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_function(i._b));

			const int function_id = regs[i._b]._inplace._function_id;
			const int callee_arg_count = i._c;
			QUARK_ASSERT(function_id >= 0 && function_id < vm._imm->_program._function_defs.size())

			const auto& function_def = vm._imm->_program._function_defs[function_id];
			const int function_def_dynamic_arg_count = function_def._dyn_arg_count;
			const auto& function_return_type = function_def._function_type.get_function_return();

			//	_e[...] contains first callee, then each argument.
			//	We need to examine the callee, since we support magic argument lists of varying size.

			QUARK_ASSERT(function_def._args.size() == callee_arg_count);

			if(function_def._host_function_id != 0){
				const auto& host_function = vm._imm->_host_functions.at(function_def._host_function_id);

				const int arg0_stack_pos = stack.size() - (function_def_dynamic_arg_count + callee_arg_count);
				int stack_pos = arg0_stack_pos;

				//	Notice that dynamic functions will have each DYN argument with a leading itype as an extra argument.
				const auto function_def_arg_count = function_def._args.size();
				std::vector<bc_value_t> arg_values;
				for(int a = 0 ; a < function_def_arg_count ; a++){
					const auto& func_arg_type = function_def._args[a]._type;
					if(func_arg_type.is_internal_dynamic()){
						const auto arg_itype = stack.load_intq(stack_pos);
						const auto& arg_type = lookup_full_type(vm, static_cast<int16_t>(arg_itype));
						const auto arg_value = stack.load_value(stack_pos + 1, arg_type);
						arg_values.push_back(arg_value);
						stack_pos += k_frame_overhead;
					}
					else{
						const auto arg_value = stack.load_value(stack_pos + 0, func_arg_type);
						arg_values.push_back(arg_value);
						stack_pos++;
					}
				}

				const auto& result = (host_function)(vm, &arg_values[0], static_cast<int>(arg_values.size()));
				const auto bc_result = result;

				if(function_return_type.is_void() == true){
				}
				else if(function_return_type.is_internal_dynamic()){
					stack.write_register(i._a, bc_result);
				}
				else{
					stack.write_register(i._a, bc_result);
				}
			}
			else{
				QUARK_ASSERT(function_def_dynamic_arg_count == 0);

				//	We need to remember the global pos where to store return value, since we're switching frame to call function.
				int result_reg_pos = static_cast<int>(stack._current_frame_entry_ptr - &stack._entries[0]) + i._a;

				stack.open_frame(*function_def._frame_ptr, callee_arg_count);
				const auto& result = execute_instructions(vm, function_def._frame_ptr->_instrs2);
				stack.close_frame(*function_def._frame_ptr);

				//	Update our cached pointers.
				frame_ptr = stack._current_frame_ptr;
				regs = stack._current_frame_entry_ptr;

//				QUARK_ASSERT(result.first);
				if(function_return_type.is_void() == false){

					//	Cannot store via register, we have not yet executed k_pop_frame_ptr that restores our frame.
					if(function_def._return_is_ext){
						stack.replace_obj(result_reg_pos, result.second);
					}
					else{
						stack.replace_intern(result_reg_pos, result.second);
					}
				}
			}

			QUARK_ASSERT(frame_ptr == stack._current_frame_ptr);
			QUARK_ASSERT(regs == stack._current_frame_entry_ptr);
			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_1: {
			QUARK_ASSERT(stack.check_reg(i._a));

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto source_itype = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_vector() == false && target_type.is_dict() == false && target_type.is_struct() == false);
			execute_new_1(vm, dest_reg, target_itype, source_itype);
			break;
		}

		case bc_opcode::k_new_vector_obj: {
			QUARK_ASSERT(stack.check_reg_vector_obj(i._a));
			QUARK_ASSERT(i._b >= 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& vector_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(vector_type.is_vector());
			QUARK_ASSERT(encode_as_vector_w_inplace_elements(vector_type) == false);

			execute_new_vector_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}

		case bc_opcode::k_new_vector_pod64: {
			QUARK_ASSERT(vm.check_invariant());
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._a));
			QUARK_ASSERT(i._b == 0);
			QUARK_ASSERT(i._c >= 0);

			const auto dest_reg = i._a;
			const auto arg_count = i._c;

//???works for all pod64
			const int arg0_stack_pos = vm._stack.size() - arg_count;
			immer::vector<bc_inplace_value_t> elements2;
			for(int a = 0 ; a < arg_count ; a++){
				const auto pos = arg0_stack_pos + a;
				elements2 = elements2.push_back(stack._entries[pos]._inplace);
			}

			const auto& type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = type.get_vector_element_type();

			const auto result = make_vector(element_type, elements2);
			vm._stack.write_register_obj(dest_reg, result);

			QUARK_ASSERT(vm.check_invariant());
			break;
		}

		case bc_opcode::k_new_dict_obj: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_dict());
			execute_new_dict_obj(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_dict_pod64: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_dict());
			execute_new_dict_pod64(vm, dest_reg, target_itype, arg_count);
			break;
		}
		case bc_opcode::k_new_struct: {
			const auto dest_reg = i._a;
			const auto target_itype = i._b;
			const auto arg_count = i._c;
			const auto& target_type = lookup_full_type(vm, target_itype);
			QUARK_ASSERT(target_type.is_struct());
			execute_new_struct(vm, dest_reg, target_itype, arg_count);
			break;
		}


		//////////////////////////////		COMPARISON

		//??? compare_value_true_deep().

		case bc_opcode::k_comparison_smaller_or_equal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			QUARK_ASSERT(type.is_int() == false);

			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._inplace._bool = diff <= 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_or_equal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._int64 <= regs[i._c]._inplace._int64;
			break;
		}

		case bc_opcode::k_comparison_smaller: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._inplace._bool = diff < 0;
			break;
		}
		case bc_opcode::k_comparison_smaller_int:
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._int64 < regs[i._c]._inplace._int64;
			break;

		case bc_opcode::k_logical_equal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._inplace._bool = diff == 0;
			break;
		}
		case bc_opcode::k_logical_equal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._int64 == regs[i._c]._inplace._int64;
			break;
		}

		case bc_opcode::k_logical_nonequal: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_any(i._b));
			QUARK_ASSERT(stack.check_reg_any(i._c));

			const auto& type = frame_ptr->_symbols[i._b].second._value_type;
			QUARK_ASSERT(type.is_int() == false);
			const auto left = stack.read_register(i._b);
			const auto right = stack.read_register(i._c);
			long diff = bc_compare_value_true_deep(left, right, type);

			regs[i._a]._inplace._bool = diff != 0;
			break;
		}
		case bc_opcode::k_logical_nonequal_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._int64 != regs[i._c]._inplace._int64;
			break;
		}


		//////////////////////////////		ARITHMETICS


		//??? Replace by a | b opcode.
		case bc_opcode::k_add_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._bool + regs[i._c]._inplace._bool;
			break;
		}
		case bc_opcode::k_add_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._int64 = regs[i._b]._inplace._int64 + regs[i._c]._inplace._int64;
			break;
		}
		case bc_opcode::k_add_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a]._inplace._double = regs[i._b]._inplace._double + regs[i._c]._inplace._double;
			break;
		}
		case bc_opcode::k_concat_strings: {
			QUARK_ASSERT(stack.check_reg_string(i._a));
			QUARK_ASSERT(stack.check_reg_string(i._b));
			QUARK_ASSERT(stack.check_reg_string(i._c));

			//	??? No need to create bc_value_t here.
			const auto s = regs[i._b]._external->_string + regs[i._c]._external->_string;
			const auto value = bc_value_t::make_string(s);
			auto prev_copy = regs[i._a];
			value._pod._external->_rc++;
			regs[i._a] = value._pod;
			release_pod_external(prev_copy);
			break;
		}

		case bc_opcode::k_concat_vectors_obj: {
			QUARK_ASSERT(stack.check_reg_vector_obj(i._a));
			QUARK_ASSERT(stack.check_reg_vector_obj(i._b));
			QUARK_ASSERT(stack.check_reg_vector_obj(i._c));

			const auto& vector_type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = vector_type.get_vector_element_type();
			QUARK_ASSERT(encode_as_vector_w_inplace_elements(vector_type) == false);

			//	Copy left into new vector.
			immer::vector<bc_external_handle_t> elements2 = regs[i._b]._external->_vector_w_external_elements;

			const auto& right_elements = regs[i._c]._external->_vector_w_external_elements;
			for(const auto& e: right_elements){
				elements2 = elements2.push_back(e);
			}
			const auto& value2 = make_vector(element_type, elements2);
			stack.write_register_obj(i._a, value2);
			break;
		}
		case bc_opcode::k_concat_vectors_pod64: {
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._a));
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._b));
			QUARK_ASSERT(stack.check_reg_vector_pod64(i._c));

			const auto& vector_type = frame_ptr->_symbols[i._a].second._value_type;
			const auto& element_type = vector_type.get_vector_element_type();
			QUARK_ASSERT(encode_as_vector_w_inplace_elements(vector_type) == true);

			//	Copy left into new vector.
			auto elements2 = regs[i._b]._external->_vector_w_inplace_elements;

			const auto& right_elements = regs[i._c]._external->_vector_w_inplace_elements;
			for(const auto& e: right_elements){
				elements2 = elements2.push_back(e);
			}
			const auto& value2 = make_vector(element_type, elements2);
			stack.write_register_obj(i._a, value2);
			break;
		}

		case bc_opcode::k_subtract_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a]._inplace._double = regs[i._b]._inplace._double - regs[i._c]._inplace._double;
			break;
		}
		case bc_opcode::k_subtract_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._int64 = regs[i._b]._inplace._int64 - regs[i._c]._inplace._int64;
			break;
		}
		case bc_opcode::k_multiply_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._c));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a]._inplace._double = regs[i._b]._inplace._double * regs[i._c]._inplace._double;
			break;
		}
		case bc_opcode::k_multiply_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._c));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._int64 = regs[i._b]._inplace._int64 * regs[i._c]._inplace._int64;
			break;
		}
		case bc_opcode::k_divide_double: {
			QUARK_ASSERT(stack.check_reg_double(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			const auto right = regs[i._c]._inplace._double;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._inplace._double = regs[i._b]._inplace._double / right;
			break;
		}
		case bc_opcode::k_divide_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c]._inplace._int64;
			if(right == 0.0f){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._inplace._int64 = regs[i._b]._inplace._int64 / right;
			break;
		}
		case bc_opcode::k_remainder_int: {
			QUARK_ASSERT(stack.check_reg_int(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			const auto right = regs[i._c]._inplace._int64;
			if(right == 0){
				quark::throw_runtime_error("EEE_DIVIDE_BY_ZERO");
			}
			regs[i._a]._inplace._int64 = regs[i._b]._inplace._int64 % right;
			break;
		}


		//### Could be replaced by feature to convert any value to bool -- they use a generic comparison for && and ||
		case bc_opcode::k_logical_and_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._bool  && regs[i._c]._inplace._bool;
			break;
		}
		case bc_opcode::k_logical_and_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = (regs[i._b]._inplace._int64 != 0) && (regs[i._c]._inplace._int64 != 0);
			break;
		}
		case bc_opcode::k_logical_and_double: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a]._inplace._bool = (regs[i._b]._inplace._double != 0) && (regs[i._c]._inplace._double != 0);
			break;
		}

		case bc_opcode::k_logical_or_bool: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_bool(i._b));
			QUARK_ASSERT(stack.check_reg_bool(i._c));

			regs[i._a]._inplace._bool = regs[i._b]._inplace._bool || regs[i._c]._inplace._bool;
			break;
		}
		case bc_opcode::k_logical_or_int: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_int(i._b));
			QUARK_ASSERT(stack.check_reg_int(i._c));

			regs[i._a]._inplace._bool = (regs[i._b]._inplace._int64 != 0) || (regs[i._c]._inplace._int64 != 0);
			break;
		}
		case bc_opcode::k_logical_or_double: {
			QUARK_ASSERT(stack.check_reg_bool(i._a));
			QUARK_ASSERT(stack.check_reg_double(i._b));
			QUARK_ASSERT(stack.check_reg_double(i._c));

			regs[i._a]._inplace._bool = (regs[i._b]._inplace._double != 0.0f) || (regs[i._c]._inplace._double != 0.0f);
			break;
		}


		//////////////////////////////		NONE


		default:
			QUARK_ASSERT(false);
			quark::throw_exception();
		}
		pc++;
	}
	return { false, bc_value_t::make_undefined() };
}


//////////////////////////////////////////		FUNCTIONS



std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s){
	QUARK_ASSERT(vm.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	const auto& symbols = vm._imm->_program._globals._symbols;
    const auto& it = std::find_if(
    	symbols.begin(),
    	symbols.end(),
    	[&s](const std::pair<std::string, bc_symbol_t>& e) { return e.first == s; }
	);
	if(it != symbols.end()){
		const auto index = static_cast<int>(it - symbols.begin());
		const auto pos = get_global_n_pos(index);
		QUARK_ASSERT(pos >= 0 && pos < vm._stack.size());

		const auto value_entry = value_entry_t{
			vm._stack.load_value(pos, it->second._value_type),
			it->first,
			it->second,
			static_cast<int>(index)
		};
		return std::make_shared<value_entry_t>(value_entry);
	}
	else{
		return nullptr;
	}
}


std::string opcode_to_string(bc_opcode opcode){
	return k_opcode_info.at(opcode)._as_text;
}

json_t interpreter_to_json(const interpreter_t& vm){
	std::vector<json_t> callstack;
	QUARK_ASSERT(vm.check_invariant());

	const auto stack = vm._stack.stack_to_json();

	return json_t::make_object({
		{ "ast", bcprogram_to_json(vm._imm->_program) },
		{ "callstack", stack }
	});
}

std::vector<json_t> bc_symbols_to_json(const std::vector<std::pair<std::string, bc_symbol_t>>& symbols){
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbols){
		const auto& symbol = e.second;
		const auto symbol_type_str = symbol._symbol_type == bc_symbol_t::immutable_local ? "immutable_local" : "mutable_local";

		if(symbol._const_value._type.is_undefined() == false){
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"CONST",
				bcvalue_and_type_to_json(symbol._const_value)
			});
			r.push_back(e2);
		}
		else{
			const auto e2 = json_t::make_array({
				symbol_index,
				e.first,
				"LOCAL",
				json_t::make_object({
					{ "value_type", typeid_to_ast_json(symbol._value_type, json_tags::k_tag_resolve_state)._value },
					{ "type", symbol_type_str }
				})
			});
			r.push_back(e2);
		}

		symbol_index++;
	}
	return r;
}

json_t frame_to_json(const bc_static_frame_t& frame){
	std::vector<json_t> exts;
	for(int i = 0 ; i < frame._exts.size() ; i++){
		const auto& e = frame._exts[i];
		exts.push_back(json_t::make_array({
			json_t(i),
			json_t(e)
		}));
	}

	std::vector<json_t> instructions;
	int pc = 0;
	for(const auto& e: frame._instrs2){
		const auto i = json_t::make_array({
			pc,
			opcode_to_string(e._opcode),
			json_t(e._a),
			json_t(e._b),
			json_t(e._c)
		});
		instructions.push_back(i);
		pc++;
	}

	return json_t::make_object({
		{ "symbols", json_t::make_array(bc_symbols_to_json(frame._symbols)) },
		{ "instructions", json_t::make_array(instructions) },
		{ "exts", json_t::make_array(exts) }
	});
}

json_t types_to_json(const std::vector<typeid_t>& types){
	std::vector<json_t> r;
	int id = 0;
	for(const auto& e: types){
		const auto i = json_t::make_array({
			id,
			typeid_to_ast_json(e, json_tags::k_plain)._value
		});
		r.push_back(i);
		id++;
	}
	return json_t::make_array(r);
}

json_t functiondef_to_json(const bc_function_definition_t& def){
	return json_t::make_array({
		json_t(typeid_to_compact_string(def._function_type)),
		members_to_json(def._args),
		def._frame_ptr ? frame_to_json(*def._frame_ptr) : json_t(),
		json_t(def._host_function_id)
	});
}

json_t bcprogram_to_json(const bc_program_t& program){
	std::vector<json_t> callstack;
/*
	for(int env_index = 0 ; env_index < vm._call_stack.size() ; env_index++){
		const auto e = &vm._call_stack[vm._call_stack.size() - 1 - env_index];

		const auto local_end = (env_index == (vm._call_stack.size() - 1)) ? vm._value_stack.size() : vm._call_stack[vm._call_stack.size() - 1 - env_index + 1]._values_offset;
		const auto local_count = local_end - e->_values_offset;
		std::vector<json_t> values;
		for(int local_index = 0 ; local_index < local_count ; local_index++){
			const auto& v = vm._value_stack[e->_values_offset + local_index];
			const auto& a = value_and_type_to_ast_json(v);
			values.push_back(a._value);
		}

		const auto& env = json_t::make_object({
			{ "values", values }
		});
		callstack.push_back(env);
	}
*/
	std::vector<json_t> function_defs;
	for(int i = 0 ; i < program._function_defs.size() ; i++){
		const auto& function_def = program._function_defs[i];
		function_defs.push_back(json_t::make_array({
			json_t(i),
			functiondef_to_json(function_def)
		}));
	}

	return json_t::make_object({
		{ "globals", frame_to_json(program._globals) },
		{ "types", types_to_json(program._types) },
		{ "function_defs", json_t::make_array(function_defs) }
//		{ "callstack", json_t::make_array(callstack) }
	});
}

}	//	floyd
