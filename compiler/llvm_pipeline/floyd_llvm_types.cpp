//
//  floyd_llvm_types.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_types.h"

#include "floyd_llvm_helpers.h"
#include "format_table.h"

#include <llvm/IR/Module.h>

namespace floyd {

struct builder_t;


bool pass_as_ptr(const typeid_t& type){
	if(type.is_string() || type.is_json() || type.is_struct() || type.is_vector() || type.is_dict() || type.is_function()){
		return true;
	}
	else {
		return false;
	}
}


llvm::Type* deref_ptr(llvm::Type* type){
	QUARK_ASSERT(type != nullptr);

	if(type->isPointerTy()){
		llvm::PointerType* type2 = llvm::cast<llvm::PointerType>(type);
  		llvm::Type* element_type = type2->getElementType();
  		return element_type;
	}
	else{
		QUARK_ASSERT(false);
		return type;
	}
}



////////////////////////////////		llvm_function_def_t



llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args){
	QUARK_ASSERT(def.check_invariant());

	if(args.empty()){
		QUARK_ASSERT(def.args.size() == 1);
		return def;
	}
	else{
		//	Skip arg #0, which is "floyd_runtime_ptr".
		const auto floyd_arg_count = def.args.back().floyd_arg_index + 1;
		QUARK_ASSERT(floyd_arg_count == args.size());

		std::vector<llvm_arg_mapping_t> arg_results;

		for(int out_index = 0 ; out_index < def.args.size() ; out_index++){
			auto arg_copy = def.args[out_index];
			if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name;
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name + "-dynval";
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];

				arg_copy.floyd_name = floyd_arg._name + "-dyntype";
			}
			else{
				QUARK_ASSERT(false);
			}
			arg_results.push_back(arg_copy);
		}

		return llvm_function_def_t { def.return_type, arg_results, def.llvm_args };
	}
}




////////////////////////////////		llvm_type_lookup



static llvm::Type* make_runtime_value_type_internal(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
}

static llvm::Type* make_runtime_type_type_internal(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
}

static llvm::StructType* make_wide_return_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	a
		llvm::Type::getInt64Ty(context),

		//	b
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

static llvm::StructType* make_generic_vec_type_internal(llvm::LLVMContext& context){
	const auto n = sizeof(heap_alloc_64_t);
	QUARK_ASSERT((n % 8) == 0);

	const std::vector<llvm::Type*> members(n / 8, llvm::Type::getInt64Ty(context));
	llvm::StructType* s = llvm::StructType::create(context, members, "vec");
	return s;
}

static llvm::StructType* make_generic_dict_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "dict");
	return s;
}

static llvm::StructType* make_json_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "json");
	return s;
}

static llvm::StructType* make_generic_runtime_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt16Ty(context)
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "frp");
	return s;
}

static llvm::StructType* make_generic_struct_type_internal(llvm::LLVMContext& context){
	const auto n = sizeof(heap_alloc_64_t);
	QUARK_ASSERT((n % 8) == 0);

	const std::vector<llvm::Type*> members(n / 8, llvm::Type::getInt64Ty(context));
	llvm::StructType* s = llvm::StructType::create(context, members, "struct");
	return s;
}






struct builder_t {
	llvm::LLVMContext& context;
	state_t acc;
};

static const type_entry_t& find_type(const builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(builder.acc.type_interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto itype = lookup_itype_from_typeid(builder.acc.type_interner, type);
	const auto index = itype.get_lookup_index();
	return builder.acc.types[index];
}

static llvm::Type* get_llvm_type_prefer_generic(const type_entry_t& entry){
	if(entry.llvm_type_generic != nullptr){
		return entry.llvm_type_generic;
	}
	else{
		return entry.llvm_type_specific;
	}
}

static llvm_function_def_t map_function_arguments_internal(const builder_t& builder, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto ret = function_type.get_function_return();

	// Notice: we always resolve the return type in semantic analysis -- no need to use WIDE_RETURN and provide a dynamic type. We use int64 here and cast it when calling.
	llvm::Type* return_type = nullptr;
	if(ret.is_any()){
	 	return_type = llvm::Type::getInt64Ty(builder.context);
	}
	else {
		const auto& a = find_type(builder, ret);
		return_type = get_llvm_type_prefer_generic(a);
	}

	const auto args = function_type.get_function_args();
	std::vector<llvm_arg_mapping_t> arg_results;

	auto frp_type = builder.acc.runtime_ptr_type;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation in Floyd function type.
	arg_results.push_back({ frp_type, "floyd_runtime_ptr", floyd::typeid_t::make_undefined(), -1, llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr });

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(arg.is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(arg.is_any()){
			arg_results.push_back({ builder.acc.runtime_value_type, std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_dyn_value });
			arg_results.push_back({ builder.acc.runtime_type_type, std::to_string(index), typeid_t::make_undefined(), index, llvm_arg_mapping_t::map_type::k_dyn_type });
		}
		else {
			const auto& a = find_type(builder, arg);
			auto arg_itype = get_llvm_type_prefer_generic(a);
			arg_results.push_back({ arg_itype, std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_known_value_type });
		}
	}

	std::vector<llvm::Type*> llvm_args;
	for(const auto& e: arg_results){
		llvm_args.push_back(e.llvm_type);
	}

	return llvm_function_def_t { return_type, arg_results, llvm_args };
}


//	Function-types are always returned as pointer-to-function types.
static llvm::Type* make_function_type(const builder_t& builder, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments_internal(builder, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}


static llvm::StructType* make_exact_struct_type(const builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto& a = find_type(builder, m._type);
		const auto m2 = get_llvm_type_prefer_generic(a);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(builder.context, members, false);
//	QUARK_TRACE(print_type(s));
	return s;
}

static llvm::Type* make_llvm_type(const builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		const builder_t& builder;
		const typeid_t& type;

		llvm::Type* operator()(const typeid_t::undefined_t& e) const{
			return llvm::Type::getInt16Ty(builder.context);
		}
		llvm::Type* operator()(const typeid_t::any_t& e) const{
			return builder.acc.runtime_value_type;
		}

		llvm::Type* operator()(const typeid_t::void_t& e) const{
			return llvm::Type::getVoidTy(builder.context);
		}
		llvm::Type* operator()(const typeid_t::bool_t& e) const{
			return llvm::Type::getInt1Ty(builder.context);
		}
		llvm::Type* operator()(const typeid_t::int_t& e) const{
			return llvm::Type::getInt64Ty(builder.context);
		}
		llvm::Type* operator()(const typeid_t::double_t& e) const{
			return llvm::Type::getDoubleTy(builder.context);
		}
		llvm::Type* operator()(const typeid_t::string_t& e) const{
			return builder.acc.generic_vec_type;
		}

		llvm::Type* operator()(const typeid_t::json_type_t& e) const{
			return builder.acc.json_type;
		}
		llvm::Type* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.acc.runtime_type_type;
		}

		llvm::Type* operator()(const typeid_t::struct_t& e) const{
			return make_exact_struct_type(builder, type);
		}
		llvm::Type* operator()(const typeid_t::vector_t& e) const{
			return builder.acc.generic_vec_type;
		}
		llvm::Type* operator()(const typeid_t::dict_t& e) const{
			return builder.acc.generic_dict_type;
		}
		llvm::Type* operator()(const typeid_t::function_t& e) const{
			return deref_ptr(make_function_type(builder, type));
		}
		llvm::Type* operator()(const typeid_t::identifier_t& e) const {
			return llvm::Type::getInt8Ty(builder.context);
//			QUARK_ASSERT(false); throw std::exception();
		}
	};
	return std::visit(visitor_t{ builder, type }, type._contents);
}

static llvm::Type* make_generic_type(const builder_t& builder, const typeid_t& type){
	if(type.is_vector()){
		return builder.acc.generic_vec_type;
	}
	else if(type.is_string()){
		return builder.acc.generic_vec_type;
	}
	else if(type.is_dict()){
		return builder.acc.generic_dict_type;
	}
	else if(type.is_struct()){
		return builder.acc.generic_struct_type;
	}
	else{
		return nullptr;
	}
}

static type_entry_t make_type(const builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(builder.acc.type_interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto llvm_type0 = make_llvm_type(builder, type);
	const auto llvm_type = pass_as_ptr(type) ? llvm_type0->getPointerTo() : llvm_type0;

	llvm::Type* llvm_generic_type0 = make_generic_type(builder, type);
	llvm::Type* llvm_generic_type = llvm_generic_type0 ? llvm_generic_type0->getPointerTo() : nullptr;

	std::shared_ptr<const llvm_function_def_t> optional_function_def;
	if(type.is_function()){
		const auto function_def = map_function_arguments_internal(builder, type);
		optional_function_def = std::make_shared<llvm_function_def_t>(function_def);
	}

	const auto entry = type_entry_t{
		llvm_type,
		llvm_generic_type,
		optional_function_def
	};

	return entry;
}

//	Notice: the entries in the type_interner may reference eachother = we need to process recursively.
llvm_type_lookup::llvm_type_lookup(llvm::LLVMContext& context, const type_interner_t& type_interner){
	QUARK_ASSERT(type_interner.check_invariant());

	state_t acc;
	acc.type_interner = type_interner;

	//	Make an entry for each entry in type_interner
	acc.types = std::vector<type_entry_t>(type_interner.interned2.size(), type_entry_t());


	acc.generic_vec_type = make_generic_vec_type_internal(context);
	acc.generic_dict_type = make_generic_dict_type_internal(context);
	acc.json_type = make_json_type_internal(context);
	acc.generic_struct_type = make_generic_struct_type_internal(context);
	acc.wide_return_type = make_wide_return_type_internal(context);
	acc.runtime_ptr_type = make_generic_runtime_type_internal(context)->getPointerTo();

	acc.runtime_type_type = make_runtime_type_type_internal(context);
	acc.runtime_value_type = make_runtime_value_type_internal(context);


	builder_t builder { context, acc };

	for(const auto& e: acc.type_interner.interned2){
		QUARK_ASSERT(builder.acc.type_interner.check_invariant());
		QUARK_ASSERT(e.second.check_invariant());

		const auto itype = lookup_itype_from_typeid(builder.acc.type_interner, e.second);
		const auto index = itype.get_lookup_index();
		const auto entry = make_type(builder, e.second);
		builder.acc.types[index] = entry;
	}

	state = builder.acc;

	QUARK_ASSERT(check_invariant());

//	trace_llvm_type_lookup(*this);
}




bool llvm_type_lookup::check_invariant() const {
	QUARK_ASSERT(state.generic_vec_type != nullptr);
	QUARK_ASSERT(state.generic_dict_type != nullptr);
	QUARK_ASSERT(state.json_type != nullptr);
	QUARK_ASSERT(state.generic_struct_type != nullptr);

	QUARK_ASSERT(state.wide_return_type != nullptr);

	QUARK_ASSERT(state.type_interner.check_invariant());
	QUARK_ASSERT(state.type_interner.interned2.size() == state.types.size());
	return true;
}

const type_entry_t& llvm_type_lookup::find_from_type(const typeid_t& type) const {
	QUARK_ASSERT(check_invariant());

	const auto itype = lookup_itype_from_typeid(state.type_interner, type);
	const auto index = itype.get_lookup_index();
	return state.types[index];
}

const type_entry_t& llvm_type_lookup::find_from_itype(const itype_t& itype) const {
	QUARK_ASSERT(check_invariant());

	const auto index = itype.get_lookup_index();
	return state.types[index];
}

void trace_llvm_type_lookup(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	std::vector<line_t> table = {
		line_t( { "#", "ITYPE INDEX", "typeid_t", "llvm_type_specific", "llvm_type_generic", "optional_function_def" }, ' ', '|'),
		line_t( { "", "", "", "", "", "" }, '-', '|'),
	};

	for(int i = 0 ; i < type_lookup.state.types.size() ; i++){
		const auto& e = type_lookup.state.types[i];
		const auto type = type_lookup.state.type_interner.interned2[i].second;
		const auto l = line_t {
			{
				std::to_string(i),
				std::to_string(i),
				typeid_to_compact_string(type),
				print_type(e.llvm_type_specific),
				print_type(e.llvm_type_generic),
				e.optional_function_def != nullptr ? "YES" : ""
			},
			' ',
			'|'
		};
		table.push_back(l);
	}

	table.push_back(line_t( { "", "", "", "", "", "" }, '-', '|'));

	const auto default_column = column_t{ 0, -1, 0 };
	const auto columns0 = std::vector<column_t>{ default_column, default_column, default_column, default_column, default_column, default_column };
	const auto columns = fit_columns(columns0, table);
	const auto r = generate_table(table, columns);

	std::stringstream ss;
	ss << std::endl;
	for(const auto& e: r){
		ss << e << std::endl;
	}
	QUARK_TRACE(ss.str());
}




llvm::Type* make_runtime_type_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_type_type;
}

llvm::Type* make_runtime_value_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_value_type;
}


typeid_t lookup_type(const llvm_type_lookup& type_lookup, const itype_t& itype){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto type = lookup_type_from_itype(type_lookup.state.type_interner, itype);
	return type;
}

itype_t lookup_itype(const llvm_type_lookup& type_lookup, const typeid_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());

	const auto itype = lookup_itype_from_typeid(type_lookup.state.type_interner, type);
	return itype;
}



llvm::StructType* get_exact_struct_type_byvalue(const llvm_type_lookup& i, const typeid_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.is_struct());

	const auto& entry = i.find_from_type(type);
	auto result = entry.llvm_type_specific;
	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
}

llvm::Type* get_llvm_type_as_arg(const llvm_type_lookup& i, const typeid_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& entry = i.find_from_type(type);
	if(entry.llvm_type_generic != nullptr){
		return entry.llvm_type_generic;
	}
	else{
		return entry.llvm_type_specific;
	}
}


llvm::FunctionType* get_llvm_function_type(const llvm_type_lookup& type_lookup, const typeid_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto t = get_llvm_type_as_arg(type_lookup, type);
	return llvm::cast<llvm::FunctionType>(t);
}



llvm::Type* make_generic_vec_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_vec_type;
}

llvm::Type* make_generic_dict_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_dict_type;
}

llvm::Type* make_json_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.json_type;
}

llvm::Type* get_generic_struct_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_struct_type;
}

llvm::Type* make_frp_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_ptr_type;
}



////////////////////////////////		TESTS




static llvm_type_lookup make_basic_interner(llvm::LLVMContext& context){
	type_interner_t temp;
	return llvm_type_lookup(context, temp);
}


static llvm_type_lookup make_basic_interner(llvm::LLVMContext& context);

#if 0
QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(
		interner,
		typeid_t::make_function(typeid_t::make_void(), {}, epure::pure)
	);

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
	llvm::LLVMContext context;
	auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(interner, typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isIntegerTy(64));

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 2);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}

QUARK_TEST
("LLVM Codegen", "map_function_arguments()", "func void(int, DYN, bool)", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int(), typeid_t::make_any(), typeid_t::make_bool() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 5);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);

	QUARK_UT_VERIFY(r.args[2].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[2].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[2].floyd_type.is_any());
	QUARK_UT_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_UT_VERIFY(r.args[3].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[3].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[3].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_UT_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_UT_VERIFY(r.args[4].floyd_name == "2");
	QUARK_UT_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_UT_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_UT_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}
#endif


}	// floyd
