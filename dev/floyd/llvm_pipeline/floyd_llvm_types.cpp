//
//  floyd_llvm_types.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_types.h"

#include <llvm/IR/Module.h>

namespace floyd {

struct builder_t;

static llvm::Type* touch_get_exact_llvm_type(builder_t& builder, const typeid_t& type);



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
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context)
	};

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
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "struct");
	return s;
}

static llvm::StructType* make_wide_return_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.wide_return_type;
}








struct builder_t {
	llvm::LLVMContext& context;
	const type_interner_t interner;
	state_t acc;
};


static llvm::StructType* make_exact_struct_type(builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(type.is_struct());

	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto m2 = touch_get_exact_llvm_type(builder, m._type);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(builder.context, members, false);
	return s;
}

static llvm_function_def_t map_function_arguments_internal(builder_t& builder, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto ret = function_type.get_function_return();

	// Notice: we always resolve the return type in semantic analysis -- no need to use WIDE_RETURN and provide a dynamic type. We use int64 here and cast it when calling.
	llvm::Type* return_type = ret.is_any() ? llvm::Type::getInt64Ty(builder.context) : touch_get_exact_llvm_type(builder, ret);

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
			auto arg_itype = touch_get_exact_llvm_type(builder, arg);
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
static llvm::Type* make_function_type_internal(builder_t& builder, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments_internal(builder, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}








static llvm::Type* make_exact_type_internal(builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		builder_t& builder;
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
			return builder.acc.generic_vec_type->getPointerTo();
		}

		llvm::Type* operator()(const typeid_t::json_type_t& e) const{
			return builder.acc.json_type->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::typeid_type_t& e) const{
			return builder.acc.runtime_type_type;
		}

		llvm::Type* operator()(const typeid_t::struct_t& e) const{
			return make_exact_struct_type(builder, type)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::vector_t& e) const{
			return builder.acc.generic_vec_type->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::dict_t& e) const{
			return builder.acc.generic_dict_type->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::function_t& e) const{
			return make_function_type_internal(builder, type);
		}
		llvm::Type* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
			return llvm::Type::getInt16Ty(builder.context);
		}
	};
	return std::visit(visitor_t{ builder, type }, type._contents);
}

static llvm::Type* make_generic_type_internals(builder_t& builder, const typeid_t& type){
	if(type.is_vector()){
		return builder.acc.generic_vec_type->getPointerTo();
	}
	else if(type.is_string()){
		return builder.acc.generic_vec_type->getPointerTo();
	}
	else if(type.is_dict()){
		return builder.acc.generic_dict_type->getPointerTo();
	}
	else if(type.is_struct()){
		return builder.acc.generic_struct_type->getPointerTo();
	}
	else{
		return nullptr;
	}
}

static type_entry_t touch_type(builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(builder.interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto it = std::find_if(builder.acc.types.begin(), builder.acc.types.end(), [&](const type_entry_t& e){ return e.type == type; });
	if(it != builder.acc.types.end()){
		return *it;
	}
	else{
		const auto itype = lookup_itype(builder.interner, type);
		const auto llvm_type = make_exact_type_internal(builder, type);
		llvm::Type* llvm_generic_type = make_generic_type_internals(builder, type);

		std::shared_ptr<const llvm_function_def_t> optional_function_def;
		if(type.is_function()){
			const auto function_def = map_function_arguments_internal(builder, type);
			optional_function_def = std::make_shared<llvm_function_def_t>(function_def);
		}

		const auto entry = type_entry_t{
			itype,
			type,
			llvm_type,
			llvm_generic_type,
			optional_function_def
		};

		builder.acc.types.push_back(entry);
		return entry;
	}
}

static llvm::Type* touch_get_exact_llvm_type(builder_t& builder, const typeid_t& type){
	QUARK_ASSERT(builder.interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

//??? Use types-lookup.

	const type_entry_t a = touch_type(builder, type);
	if(a.llvm_type_generic != nullptr){
		return a.llvm_type_generic;
	}
	else{
		return a.llvm_type_specific;
	}
}




//??? Doesn't work if a type references a type later in the type_lookup vector.
llvm_type_lookup::llvm_type_lookup(llvm::LLVMContext& context, const type_interner_t& i){
	QUARK_ASSERT(i.check_invariant());

	state_t acc;
	acc.generic_vec_type = make_generic_vec_type_internal(context);
	acc.generic_dict_type = make_generic_dict_type_internal(context);
	acc.json_type = make_json_type_internal(context);
	acc.generic_struct_type = make_generic_struct_type_internal(context);
	acc.wide_return_type = make_wide_return_type_internal(context);
	acc.runtime_ptr_type = make_generic_runtime_type_internal(context)->getPointerTo();

	//??? 32!!!
	acc.runtime_type_type = make_runtime_type_type_internal(context);
	acc.runtime_value_type = make_runtime_value_type_internal(context);


	builder_t builder { context, i, acc };

	for(const auto& e: i.interned){
		touch_type(builder, e.second);
	}

	state = builder.acc;

	QUARK_ASSERT(check_invariant());
}

bool llvm_type_lookup::check_invariant() const {
	QUARK_ASSERT(state.generic_vec_type != nullptr);
	QUARK_ASSERT(state.generic_dict_type != nullptr);
	QUARK_ASSERT(state.json_type != nullptr);
	QUARK_ASSERT(state.generic_struct_type != nullptr);

	QUARK_ASSERT(state.wide_return_type != nullptr);
	return true;
}

const type_entry_t& llvm_type_lookup::find_from_type(const typeid_t& type) const {
	QUARK_ASSERT(check_invariant());

	const auto it = std::find_if(state.types.begin(), state.types.end(), [&](const type_entry_t& e){ return e.type == type; });
	if(it == state.types.end()){
		throw std::exception();
	}
	return *it;
}

const type_entry_t& llvm_type_lookup::find_from_itype(const itype_t& itype) const {
	QUARK_ASSERT(check_invariant());

	const auto it = std::find_if(state.types.begin(), state.types.end(), [&](const type_entry_t& e){ return e.itype.itype == itype.itype; });
	if(it == state.types.end()){
		throw std::exception();
	}
	return *it;
}


llvm::Type* make_runtime_type_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_type_type;
}

llvm::Type* make_runtime_value_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_value_type;
}


typeid_t lookup_type(const llvm_type_lookup& type_lookup, const itype_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.find_from_itype(type).type;
}

itype_t lookup_itype(const llvm_type_lookup& type_lookup, const typeid_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.find_from_type(type).itype;
}



//??? Make get_exact_llvm_type() return vector, struct etc. directly, not getPointerTo().
llvm::StructType* get_exact_struct_type(const llvm_type_lookup& i, const typeid_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.is_struct());

	const auto& entry = i.find_from_type(type);
	auto result = entry.llvm_type_specific;
	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
}

llvm::Type* get_exact_llvm_type(const llvm_type_lookup& i, const typeid_t& type){
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

llvm::Type* make_generic_vec_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_vec_type;
}

llvm::Type* make_generic_dict_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_dict_type;
}

llvm::Type* make_json_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.json_type;
}

llvm::Type* get_generic_struct_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_struct_type;
}

llvm::Type* make_frp_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_ptr_type;
}


llvm_function_def_t map_function_arguments(const llvm_type_lookup& type_lookup, const floyd::typeid_t& function_type){
	QUARK_ASSERT(function_type.is_function());

	const auto& entry = type_lookup.find_from_type(function_type);
	QUARK_ASSERT(entry.optional_function_def);
	return *entry.optional_function_def;
}


////////////////////////////////		TESTS




static llvm_type_lookup make_basic_interner(llvm::LLVMContext& context){
	type_interner_t temp;
	intern_type(temp, typeid_t::make_void());
	intern_type(temp, typeid_t::make_int());
	intern_type(temp, typeid_t::make_bool());
	intern_type(temp, typeid_t::make_string());
	return llvm_type_lookup(context, temp);
}


static llvm_type_lookup make_basic_interner(llvm::LLVMContext& context);

#if 0
QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
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

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
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

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
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

QUARK_UNIT_TEST
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



////////////////////////////////	type_interner_t helpers



runtime_type_t lookup_runtime_type(const llvm_type_lookup& type_lookup, const typeid_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto a = lookup_itype(type_lookup, type);
	return make_runtime_type(a.itype);
}


}	// floyd
